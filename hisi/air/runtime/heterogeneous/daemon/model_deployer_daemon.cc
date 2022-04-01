/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thread>
#include <chrono>
#include <csignal>
#include "framework/common/debug/log.h"
#include "daemon/grpc_server.h"
#include "common/mem_grp/memory_group_manager.h"
#include "deploy/flowrm/datagw_manager.h"
#include "common/config/configurations.h"
#include "common/utils/bind_cpu_utils.h"
#include "common/utils/rts_api_utils.h"
#include "mmpa/mmpa_api.h"

namespace ge {
namespace {
constexpr int32_t kForkWaitTimeInSec = 5;
}
class ModelDeployerDaemon {
 public:
  Status Start() {
    (void)std::signal(SIGTERM, static_cast<sighandler_t>(&SignalHandler));
    (void)std::signal(SIGCHLD, static_cast<sighandler_t>(&SignalChldHandler));
    GE_CHK_STATUS_RET_NOLOG(BindCpuUtils::BindCore(0));
    GE_CHK_STATUS_RET(MemoryGroupManager::GetInstance().Initialize(),
                      "[Initialize][MemoryGroup] failed");
    GE_CHK_STATUS_RET(RtsApiUtils::MbufInit(), "[Initialize][MBuf] failed");
    GE_CHK_STATUS_RET(StartTsDaemon());
    GELOGD("[Start][TSD] success, pid = %d", tsd_pid_);
    GE_CHK_STATUS_RET(StartQueueSchedule());
    GELOGD("[Start][QS] success, pid = %d", qs_pid_);
    GE_CHK_STATUS_RET(RunGrpcService());
    GELOGD("[Start][GrpcServer] success");
    GELOGD("[Start][MdlDeployDaemon] success");
    return SUCCESS;
  }

  static void SignalHandler(int32_t sig_num) {
    GELOGD("Interrupt signal[%d] received.", sig_num);
    grpc_server_.Finalize();
  }

  static void SignalChldHandler(int32_t sig_num) {
    GELOGD("Interrupt signal %d received.", sig_num);
    int32_t wait_status = 0;
    const auto ret = mmWaitPid(-1, &wait_status, M_WAIT_NOHANG);
    GELOGD("mmWaitPid ret is %d, wait_status is %d", ret, wait_status);
  }

 private:
  static Status RunGrpcService() {
    const char *kConfigFilePathEnv = "HELPER_RES_FILE_PATH";
    std::string file_path(std::getenv(kConfigFilePathEnv));
    if (file_path.empty()) {
      REPORT_INNER_ERROR("E19999", "Check device env path failed.");
      GELOGE(FAILED, "[Check][EnvPath] Check device env path failed.");
      return FAILED;
    }

    GE_CHK_STATUS_RET(grpc_server_.Run(), "Failed to run grpc server");
    return SUCCESS;
  }

  Status StartQueueSchedule() {
    int32_t dev_count = 1;
    GE_CHK_STATUS_RET(RtsApiUtils::GetDeviceCount(dev_count), "[Start][QS] get device count failed.");
    GELOGI("[Start][QS] get device count = %d", dev_count);
    GE_CHK_RT_RET(rtSetAicpuAttr("RunMode", "PROCESS_MODE"));
    for (int32_t device_id = 0; device_id < dev_count; ++device_id) {
      GE_CHK_STATUS_RET(RtsApiUtils::EschedAttachDevice(device_id), "[Start][QS] attach device[%d] failed", device_id);
      Status res = ge::DataGwManager::GetInstance().InitDeviceDataGwServer(device_id, mem_group_name_, qs_pid_);
      if (res != ge::SUCCESS) {
        return FAILED;
      }

      res = DataGwManager::GetInstance().InitDataGwClient(qs_pid_, device_id);
      if (res != SUCCESS) {
        return FAILED;
      }
    }
    return SUCCESS;
  }

  Status StartTsDaemon() {
    pid_t pid = fork();
    if (pid < 0) {
      GELOGE(FAILED, "[Fork] process for tsd failed.");
      return FAILED;
    }

    if (pid == 0) {  // in forked process
      char bin_path[] = "/var/tsdaemon";
      char *argv[] = {nullptr};
      mmSetEnv("AICPU_ADD_BUFFERGROUP", "1", 1);
      mmSetEnv("AICPU_MAX_NUM", "64", 1);
      mmSetEnv("AICPU_MEMORY_GRPID", "1", 1);
      mmSetEnv("AICPU_MODEL_TIMEOUT", "10", 1);
      mmSetEnv("REGISTER_TO_ASCENDMONITOR", "0", 1);
      ::prctl(PR_SET_PDEATHSIG, SIGKILL);
      auto res = ::execv(bin_path, argv);
      if (res < 0) {
        GELOGE(FAILED, "Execute ts-daemon failed.");
        return FAILED;
      }
      return SUCCESS;
    }

    // in parent process
    tsd_pid_ = pid;
    mem_group_name_ = MemoryGroupManager::GetInstance().GetQsMemGroupName();
    GE_CHK_STATUS_RET(MemoryGroupManager::GetInstance().MemGrpAddProc(mem_group_name_, tsd_pid_, true, false),
                      "[Init][TsDaemon] failed to add group, tsd pid = %d", tsd_pid_);
    std::this_thread::sleep_for(std::chrono::seconds(kForkWaitTimeInSec));  // wait for tsd fully initialized
    return SUCCESS;
  }

  pid_t tsd_pid_ = -1;
  pid_t qs_pid_ = -1;
  std::string mem_group_name_;
  static ge::GrpcServer grpc_server_;
};
ge::GrpcServer ModelDeployerDaemon::grpc_server_{};
}  // namespace ge

int main() {
  constexpr char *kConfigFilePathEnv = "HELPER_RES_FILE_PATH";
  char *path = std::getenv(kConfigFilePathEnv);
  if (path == nullptr) {
    REPORT_CALL_ERROR("E19999", "Check host env path failed.");
    return 1;
  }
  if (ge::Configurations::GetInstance().InitDeviceInformation(path) != ge::SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Parse device configuration failed.");
    return 1;
  }

  ge::ModelDeployerDaemon daemon;
  auto ret = daemon.Start();
  if (ret != ge::SUCCESS) {
    return 1;  // failure
  }

  return 0;
}
