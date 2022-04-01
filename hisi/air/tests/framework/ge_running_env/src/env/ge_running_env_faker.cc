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

#include <map>
#include "external/ge/ge_api.h"
#include "opskernel_manager/ops_kernel_builder_manager.h"
#include "init/gelib.h"
#include "plugin/engine/engine_manage.h"
#include "ge_running_env/ge_running_env_faker.h"
#include "ge_default_running_env.h"
#include "op/fake_op_repo.h"

FAKE_NS_BEGIN

namespace {
struct InitEnv {
  static InitEnv& GetInstance() {
    static InitEnv instance;
    return instance;
  }

  void reset(std::map<string, OpsKernelInfoStorePtr> &ops_kernel_info_stores,
             std::map<string, OpsKernelBuilderPtr> &builders,
             std::map<string, GraphOptimizerPtr> &ops_kernel_optimizers,
             std::vector<std::pair<std::string, GraphOptimizerPtr>> &optimizers_by_priority,
             std::map<string, std::set<std::string>> &composite_engines,
             std::map<string, std::string> &composite_engine_kernel_lib_names,
             std::map<string, DNNEnginePtr> &engines) {
    std::set<string> remove_info_names;
    for (auto iter : builders) {
      if (kernel_info_names.find(iter.first) == kernel_info_names.end()) {
        remove_info_names.insert(iter.first);
      }
    }
    for (auto info_name : remove_info_names) {
      ops_kernel_info_stores.erase(info_name);
      builders.erase(info_name);
      ops_kernel_optimizers.erase(info_name);
      composite_engines.erase(info_name);
      composite_engine_kernel_lib_names.erase(info_name);
      engines.erase(info_name);
    }

    for (auto iter = ops_kernel_optimizers.begin(); iter != ops_kernel_optimizers.end(); ) {
      if (optimizer_names.count(iter->first) == 0) {
        iter = ops_kernel_optimizers.erase(iter);
      } else {
        ++iter;
      }
    }

    for (auto iter = optimizers_by_priority.begin(); iter != optimizers_by_priority.end(); ) {
      if (optimizer_names.count(iter->first) == 0) {
        iter = optimizers_by_priority.erase(iter);
      } else {
        ++iter;
      }
    }
  }

 private:
  InitEnv() {
    for (auto iter : OpsKernelManager::GetInstance().GetAllOpsKernelInfoStores()) {
      kernel_info_names.insert(iter.first);
    }
    for (const auto &iter : OpsKernelManager::GetInstance().GetAllGraphOptimizerObjs()) {
      optimizer_names.insert(iter.first);
    }
  }

 private:
  std::set<std::string> kernel_info_names;
  std::set<std::string> optimizer_names;
};
}  // namespace

GeRunningEnvFaker::GeRunningEnvFaker()
    : op_kernel_info_(const_cast<std::map<std::string, vector<OpInfo>>&>(
          OpsKernelManager::GetInstance().GetAllOpsKernelInfo())),
      ops_kernel_info_stores_(const_cast<std::map<std::string, OpsKernelInfoStorePtr>&>(
          OpsKernelManager::GetInstance().GetAllOpsKernelInfoStores())),
      ops_kernel_optimizers_(const_cast<std::map<std::string, GraphOptimizerPtr>&>(
          OpsKernelManager::GetInstance().GetAllGraphOptimizerObjs())),
      optimizers_by_priority_(const_cast<std::vector<std::pair<std::string, GraphOptimizerPtr>>&>(
          OpsKernelManager::GetInstance().GetAllGraphOptimizerObjsByPriority())),
      ops_kernel_builders_(const_cast<std::map<std::string, OpsKernelBuilderPtr>&>(
        OpsKernelBuilderManager::Instance().GetAllOpsKernelBuilders())),
      composite_engines_(const_cast<std::map<std::string, std::set<std::string>>&>(
          OpsKernelManager::GetInstance().GetCompositeEngines())),
      composite_engine_kernel_lib_names_(const_cast<std::map<std::string, std::string>&>(
          OpsKernelManager::GetInstance().GetCompositeEngineKernelLibNames())),
      engine_map_(const_cast<std::map<std::string, DNNEnginePtr>&>(DNNEngineManager::GetInstance().GetAllEngines())) {
  Reset();
}

GeRunningEnvFaker& GeRunningEnvFaker::Reset() {
  InitEnv& init_env = InitEnv::GetInstance();
  FakeOpRepo::Reset();
  init_env.reset(ops_kernel_info_stores_, ops_kernel_builders_,
                 ops_kernel_optimizers_, optimizers_by_priority_,
                 composite_engines_, composite_engine_kernel_lib_names_, engine_map_);
  flush();
  return *this;
}

void GeRunningEnvFaker::BackupEnv() { InitEnv::GetInstance(); }

GeRunningEnvFaker& GeRunningEnvFaker::Install(const EnvInstaller& installer) {
  installer.Install();
  installer.InstallTo(ops_kernel_info_stores_);
  installer.InstallTo(ops_kernel_optimizers_, optimizers_by_priority_);
  installer.InstallTo(ops_kernel_builders_);
  installer.InstallTo(composite_engines_);
  installer.InstallTo(composite_engine_kernel_lib_names_);
  installer.InstallTo(engine_map_);

  flush();
  return *this;
}

void GeRunningEnvFaker::flush() {
  op_kernel_info_.clear();
  OpsKernelManager::GetInstance().GetOpsKernelInfo("");
}

GeRunningEnvFaker& GeRunningEnvFaker::InstallDefault() {
  Reset();
  GeDefaultRunningEnv::InstallTo(*this);
  return *this;
}

FAKE_NS_END
