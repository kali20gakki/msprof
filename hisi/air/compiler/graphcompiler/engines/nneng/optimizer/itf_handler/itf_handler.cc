/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#include "itf_handler/itf_handler.h"
#include <map>
#include <string>
#include "common/util/constants.h"
#include "common/configuration.h"
#include "common/fe_utils.h"
#include "common/fe_error_code.h"
#include "fusion_manager/fusion_manager.h"
#include "ge/ge_api_types.h"
using std::map;

/* List of all engine names */
static std::vector<std::string> g_engine_name_list;
/*
 * to invoke the Initialize function of FusionManager
 * param[in] the options of init
 * return Status(SUCCESS/FAILED)
 */
fe::Status Initialize(const map<string, string> &options) {
  g_engine_name_list = {fe::AI_CORE_NAME};
  std::string soc_version = "";
  auto iter_soc = options.find(ge::SOC_VERSION);
  if (iter_soc != options.end() && !iter_soc->second.empty()) {
    FE_LOGI("The value of ge.soc_version is %s.", iter_soc->second.c_str());
    soc_version = iter_soc->second;
    if (soc_version == fe::SOC_VERSION_ASCEND910) {
      FE_LOGI("The soc_version is Ascend910, it will be replaced by Ascend910A.");
      soc_version =  fe::SOC_VERSION_ASCEND910A;
    }
  } else {
    std::map<std::string, std::string> error_key_map;
    error_key_map[fe::EM_PARAM] = fe::STR_SOC_VERSION;
    fe::LogErrorMessage(fe::EM_INPUT_PARAM_EMPTY, error_key_map);
    FE_LOGE("The parameter ge.soc_version is not found in options or the value of ge.soc_version is empty.");
    return fe::FAILED;
  }

  // init VectorCore engine only at MDC/DC
  auto iter = std::find(fe::SOC_VERSION_MDCOrDC_LIST.begin(), fe::SOC_VERSION_MDCOrDC_LIST.end(), soc_version);
  if (iter != fe::SOC_VERSION_MDCOrDC_LIST.end()) {
    g_engine_name_list.push_back(fe::VECTOR_CORE_NAME);
  }

  // init DSACore engine at Cloud-II
  if (soc_version == fe::SOC_VERSION_ASCEND920A) {
    g_engine_name_list.push_back(fe::kDsaCoreName);
  }
  for (auto &engine_name : g_engine_name_list) {
    FE_LOGD("Start to initialize in engine [%s]", engine_name.c_str());
    fe::FusionManager &fm = fe::FusionManager::Instance(engine_name);
    fe::Status ret = fm.Initialize(options, engine_name, soc_version);
    if (ret != fe::SUCCESS) {
      fm.Finalize();
      return ret;
    }
  }

  return fe::SUCCESS;
}

/*
 * to invoke the Finalize function to release the source of fusion manager
 * return Status(SUCCESS/FAILED)
 */
fe::Status Finalize() {
  bool is_any_engine_failed_to_finalize = false;
  for (auto &engine_name : g_engine_name_list) {
    FE_LOGD("Start to finalize in engine [%s]", engine_name.c_str());
    fe::Status ret = fe::FusionManager::Instance(engine_name).Finalize();
    if (ret != fe::SUCCESS) {
      /* If any of the engine fail to finalize, we will return FAILED.
       * Set this flag to true and keep finalizing the next engine. */
      is_any_engine_failed_to_finalize = true;
    }
  }
  if (is_any_engine_failed_to_finalize) {
    return fe::FAILED;
  } else {
    return fe::SUCCESS;
  }
}

/*
 * to invoke the same-name function of FusionManager to get the information of OpsKernel InfoStores
 * param[out] the map of OpsKernel InfoStores
 */
void GetOpsKernelInfoStores(map<string, OpsKernelInfoStorePtr> &op_kern_infos) {
  for (auto &engine_name : g_engine_name_list) {
    fe::FusionManager &fm = fe::FusionManager::Instance(engine_name);
    fm.GetOpsKernelInfoStores(op_kern_infos, engine_name);
  }
}

/*
 * to invoke the same-name function of FusionManager to get the information of Graph Optimizer
 * param[out] the map of Graph Optimizer
 */
void GetGraphOptimizerObjs(map<string, GraphOptimizerPtr> &graph_optimizers) {
  for (auto &engine_name : g_engine_name_list) {
    fe::FusionManager &fm = fe::FusionManager::Instance(engine_name);
    fm.GetGraphOptimizerObjs(graph_optimizers, engine_name);
  }
}
