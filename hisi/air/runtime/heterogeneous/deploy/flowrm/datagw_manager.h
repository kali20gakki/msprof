/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#ifndef AIR_RUNTIME_HETEROGENEOUS_FLOWRM_DATAGW_MANAGER_H_
#define AIR_RUNTIME_HETEROGENEOUS_FLOWRM_DATAGW_MANAGER_H_

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <signal.h>
#include <string>
#include <utility>
#include <mutex>
#include "framework/common/debug/log.h"
#include "ge/ge_api_error_codes.h"
#include "aicpu/queue_schedule/dgw_client.h"
#include "aicpu/queue_schedule/qs_client.h"
#include "runtime/rt_mem_queue.h"
#include "common/mem_grp/memory_group_manager.h"
#include "common/string_util.h"
#include "common/config/json_parser.h"

namespace ge {
enum EntityType {
  ENTITY = 0,
  ENTITY_QUEUE,
  ENTITY_TAG,
  ENTITY_INVALID
};

struct QueueInfo {
  EntityType type;
  uint32_t id;
  uint64_t handle;
};

enum GrantType {
  READ_ONLY = 0,
  WRITE_ONLY,
  READ_AND_WRITE,
  GRANT_INVALID
};

class DataGwManager {
 public:
  static DataGwManager &GetInstance() {
    static DataGwManager instance;
    return instance;
  }

  /*
   *  @ingroup ge
   *  @brief   Init host datagw server
   *  @param   [in]  const uint32_t
   *  @param   [in]  const uint32_t
   *  @param   [in]  const std::string &
   *  @param   [in]  pid_t &
   *  @return  SUCCESS or FAILED
   */
  Status InitHostDataGwServer(uint32_t device_id, uint32_t vf_id, const std::string &group_name,
                              pid_t &dgw_pid);

  /*
   *  @ingroup ge
   *  @brief   Init device datagw server
   *  @param   [in]  const uint32_t
   *  @param   [in]  uint32_t
   *  @param   [in]  const std::string &
   *  @param   [in]  pid_t &
   *  @return  SUCCESS or FAILED
   */
  Status InitDeviceDataGwServer(uint32_t device_id, const std::string &group_name, pid_t &dgw_pid);

  /*
   *  @ingroup ge
   *  @brief   Init datagw client
   *  @param   [in]  const std::string &
   *  @param   [in]  pid_t &
   *  @param   [in]  uint32_t
   *  @return: SUCCESS or FAILED
   */
  Status InitDataGwClient(uint32_t dgw_pid, uint32_t device_id);

  /*
   *  @ingroup ge
   *  @brief   Create datagw handle
   *  @param   [in]  bqs::CreateHcomInfo &
   *  @param   [in]  const uint32_t
   *  @return: SUCCESS or FAILED
   */
  Status CreateDataGwHandle(uint32_t device_id, bqs::CreateHcomInfo &hcom_info);

  /*
   *  @ingroup ge
   *  @brief   Destroy datagw handle
   *  @param   [in]  bqs::CreateHcomInfo &
   *  @param   [in]  const uint32_t
   *  @return: SUCCESS or FAILED
   */
  Status DestroyDataGwHandle(uint32_t device_id, uint64_t hcom_handle);

  /*
   *  @ingroup ge
   *  @brief   Create datagw tag
   *  @param   [in]  const uint64_t
   *  @param   [in]  const std::string &
   *  @param   [in]  const uint32_t
   *  @param   [in]  int32_t
   *  @return: SUCCESS or FAILED
   */
  Status CreateDataGwTag(uint64_t hcom_handle, const std::string &tag_name, uint32_t device_id,
                         int32_t &hcom_tag);

  /*
   *  @ingroup ge
   *  @brief   Create datagw tag
   *  @param   [in]  const uint32_t
   *  @param   [in]  const uint64_t
   *  @param   [in]  const int32_t
   *  @return: SUCCESS or FAILED
   */
  Status DestroyDataGwTag(uint32_t device_id, uint64_t hcom_handle, int32_t hcom_tag);

  /*
   *  @ingroup ge
   *  @brief   Create datagw group
   *  @param   [in]   const uint32_t
   *  @param   [in]   const std::vector<bqs::Endpoint> &
   *  @param   [out]  int32_t &
   *  @return: SUCCESS or FAILED
   */
  Status CreateDataGwGroup(uint32_t device_id, const std::vector<bqs::Endpoint> &endpoint_list, int32_t &group_id);

  /*
   *  @ingroup ge
   *  @brief   Destroy datagw group
   *  @param   [in]   const uint32_t
   *  @param   [in]   const int32_t
   *  @return: SUCCESS or FAILED
   */
  Status DestroyDataGwGroup(uint32_t device_id, int32_t group_id);

  /*
   *  @ingroup ge
   *  @brief   Init queue
   *  @param   [in]  const uint32_t
   *  @return: SUCCESS or FAILED
   */
  Status InitQueue(uint32_t device_id);

  /*
   *  @ingroup ge
   *  @brief   bind queues
   *  @param   [in]  const uint32_t
   *  @param   [in]  const bqs::QueueRoute &
   *  @return: SUCCESS or FAILED
   */
  Status BindQueues(uint32_t device_id, const std::vector<bqs::Route> &queue_routes);

  /*
   *  @ingroup ge
   *  @brief   unbind queues
   *  @param   [in]  const uint32_t
   *  @param   [in]  const bqs::QueueRoute &
   *  @return: SUCCESS or FAILED
   */
  Status UnbindQueues(uint32_t device_id, const bqs::Route &queue_route);

  /*
   *  @ingroup ge
   *  @brief   grant queues
   *  @param   [in]  const uint32_t
   *  @param   [in]  const uint32_t
   *  @param   [in]  const pid_t
   *  @param   [in]  GrantType
   *  @return: SUCCESS or FAILED
   */
  Status GrantQueue(uint32_t device_id, uint32_t qid, pid_t pid, GrantType grant_type);

  /*
   *  @ingroup ge
   *  @brief   Shut down
   *  @return: SUCCESS or FAILED
   */
  Status Shutdown();

  /*
   *  @ingroup ge
   *  @brief   grant queue for aicpu schedule
   *  @param   [in]  const uint32_t
   *  @param   [in]  const uint32_t
   *  @param   [in]  const bqs::QueueRoute &
   *  @return: SUCCESS or FAILED
   */
  Status GrantQueueForCpuSchedule(uint32_t device_id, int32_t pid,
                                  const std::vector<uint32_t> &input_queues,
                                  const std::vector<uint32_t> &output_queues);

 private:
  DataGwManager() = default;
  virtual ~DataGwManager() = default;

  /*
   *  @ingroup ge
   *  @brief   grant queue for datagw
   *  @param   [in]  const uint32_t
   *  @param   [in]  const bqs::QueueRoute &
   *  @return: SUCCESS or FAILED
   */
  Status GrantQueueForDataGw(uint32_t device_id, const bqs::Route &queue_route);

  void SetDataGwPid(uint32_t device_id, int32_t dwg_pid);

  int32_t GetDataGwPid(uint32_t device_id);

  std::mutex mutex_;
  std::map<uint32_t, int32_t> dgw_pid_map_;
};
}
#endif // AIR_RUNTIME_HETEROGENEOUS_FLOWRM_DATAGW_MANAGER_H_
