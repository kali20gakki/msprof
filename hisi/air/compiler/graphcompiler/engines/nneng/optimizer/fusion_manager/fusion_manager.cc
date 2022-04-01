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

#include "fusion_manager/fusion_manager.h"

#include <string>
#include <utility>
#include "common/configuration.h"
#include "common/fe_error_code.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/string_utils.h"
#include "common/util/op_info_util.h"
#include "common/sgt_slice_type.h"
#include "common/cmo_id_gen_strategy.h"
#include "common/scope_allocator.h"
#include "ge/ge_api_types.h"

using std::map;
using std::string;
std::mutex kFmLock;

namespace fe {
static const string FE_OPTIMIZER_NAME = "FEOptimizer";
static const string SINGLE_OP_FLAG_DEFAULT = "0";

FusionManager::FusionManager()
    : ops_kernel_info_store_(nullptr), graph_opt_(nullptr), op_store_adapter_manager_(nullptr), inited_(false) {}

FusionManager::~FusionManager() {}

FusionManager &FusionManager::Instance(const std::string &engine_name) {
  static FusionManager ai_core_fm;
  static FusionManager vector_core_fm;
  static FusionManager dsa_core_fm;

  if (engine_name == VECTOR_CORE_NAME) {
    return vector_core_fm;
  } if (engine_name == kDsaCoreName) {
    return dsa_core_fm;
  } else {
    return ai_core_fm;
  }
}

Status FusionManager::CheckOptiCompilationOfAiCoreNum (const map<string, string> &options,
                                                       const PlatformInfo &platform_info,
                                                       OptionalInfo &opti_compilation_info) const {
  auto ai_core_num = options.find(ge::AICORE_NUM);
  if (ai_core_num != options.end() && !ai_core_num->second.empty()) {
    FE_LOGI("option[ge.aicore_num] is %s.", ai_core_num->second.c_str());
    std::string ai_core_num_temp = ai_core_num->second;

    if (!StringUtils::IsInteger(ai_core_num_temp)) {
      REPORT_FE_ERROR("[FusionMngr][ChkOptiCmpt] The ai_core_num should be non-negative integer.");
      return FAILED;
    }
    int32_t ai_core_cnt = 0;
    try {
      ai_core_cnt = static_cast<int32_t>(stoi(ai_core_num_temp));
    } catch (...) {
      REPORT_FE_ERROR("[FusionMngr][ChkOptiCmpt] The ai_core_num:%s is out of range.", ai_core_num_temp.c_str());
      return FAILED;
    }

    if (static_cast<uint32_t>(ai_core_cnt) > platform_info.soc_info.ai_core_cnt || ai_core_cnt <= 0) {
      std::map<std::string, std::string> error_key_map;
      error_key_map[EM_VALUE] = ai_core_num_temp;
      error_key_map[EM_AICORE_NUM] = std::to_string(platform_info.soc_info.ai_core_cnt);
      LogErrorMessage(EM_AICORENUM_OUT_OF_RANGE, error_key_map);

      REPORT_FE_ERROR(
          "[FusionMngr][ChkOptiCmpt] Optional compilation of ai_core_num[%d] is out range of "
          "platformInfo ai_core_num (0, %d].",
          ai_core_cnt, platform_info.soc_info.ai_core_cnt);
      return FAILED;
    }
    opti_compilation_info.ai_core_num = ai_core_cnt;
  } else {
    FE_LOGD("There is no aicore_num in options.");
    opti_compilation_info.ai_core_num = platform_info.soc_info.ai_core_cnt;
  }

  return SUCCESS;
}

Status FusionManager::CheckOptiCompilationInfo(const map<string, string> &options, const PlatformInfo &platform_info,
                                               OptionalInfo &opti_compilation_info) const {
  bool result_status;
  /* ge::CORE_TYPE is actually engine type. */
  auto eng_type = options.find(ge::CORE_TYPE);
  result_status = (eng_type != options.end() && !eng_type->second.empty());
  if (result_status) {
    FE_LOGI("ge.engine_type is %s.", eng_type->second.c_str());
    if (eng_type->second != AI_CORE_TYPE && eng_type->second != VECTOR_CORE_TYPE) {
      std::map<std::string, std::string> error_key_map;
      error_key_map["engine_type"] = eng_type->second;
      LogErrorMessage(EM_ENGINE_TYPE_INVALID, error_key_map);

      REPORT_FE_ERROR("[FusionMngr][ChkOptiCmptInfo] Optional compilation of eng_type is invalid, eng_type is %s.",
                      eng_type->second.c_str());
      return FAILED;
    }
    opti_compilation_info.core_type = eng_type->second;
  } else {
    FE_LOGD("There is no engine_type in options.");
    opti_compilation_info.core_type = AI_CORE_TYPE;
  }

  if (CheckOptiCompilationOfAiCoreNum(options, platform_info, opti_compilation_info) != SUCCESS) {
    REPORT_FE_ERROR("[FusionMngr][ChkOptiCmptInfo] CheckOptiCompilationOfAiCoreNum failed.");
    return FAILED;
  }

  auto l1_fusion_flag = options.find(ge::BUFFER_OPTIMIZE);
  result_status = (l1_fusion_flag != options.end() && l1_fusion_flag->second == L1_OPTIMIZE);
  if (result_status) {
    opti_compilation_info.l1_fusion_flag = kStrTrue;
  } else {
    FE_LOGD("There is no l1_fusion_flag in options.");
    opti_compilation_info.l1_fusion_flag = kStrFalse;
  }

  return SUCCESS;
}

Status FusionManager::CheckOptiCompilationOfAiCoreNum(const map<string, string> &options, PlatFormInfos &platform_info,
                                                      OptionalInfos &opti_compilation_info) const {
  auto ai_core_num = options.find(ge::AICORE_NUM);
  string str_label = "SoCInfo";
  string str_key = "ai_core_cnt";
  string str_val = "1";
  (void)platform_info.GetPlatformRes(str_label, str_key, str_val);
  uint32_t ai_core_cnt_inf;
  try {
    ai_core_cnt_inf = static_cast<uint32_t>(stoi(str_val));
  } catch (...) {
    REPORT_FE_ERROR("[FusionMngr][ChkOptiCmpt] Fail to load ai_core_cnt[%s].", str_val.c_str());
    return FAILED;
  }

  if (ai_core_num != options.end() && !ai_core_num->second.empty()) {
    FE_LOGI("option[ge.aicore_num] is %s.", ai_core_num->second.c_str());
    std::string ai_core_num_temp = ai_core_num->second;
    if (!StringUtils::IsInteger(ai_core_num_temp)) {
      REPORT_FE_ERROR("[FusionMngr][ChkOptiCmpt] The ai_core_num should be non-negative integer.");
      return FAILED;
    }
    int32_t ai_core_cnt;
    try {
      ai_core_cnt = static_cast<int32_t>(stoi(ai_core_num_temp));
    } catch (...) {
      REPORT_FE_ERROR("[FusionMngr][ChkOptiCmpt] The ai_core_num:%s is out of range.", ai_core_num_temp.c_str());
      return FAILED;
    }

    if (static_cast<uint32_t>(ai_core_cnt) > ai_core_cnt_inf || ai_core_cnt <= 0) {
      std::map<std::string, std::string> error_key_map;
      error_key_map[EM_VALUE] = ai_core_num_temp;
      error_key_map[EM_AICORE_NUM] = std::to_string(ai_core_cnt_inf);
      LogErrorMessage(EM_AICORENUM_OUT_OF_RANGE, error_key_map);

      REPORT_FE_ERROR(
          "[FusionMngr][ChkOptiCmpt] Optional compilation of ai_core_num[%d] is out range of "
          "platformInfo ai_core_num (0, %d].",
          ai_core_cnt, ai_core_cnt_inf);
      return FAILED;
    }
    opti_compilation_info.SetAICoreNum(ai_core_cnt);
  } else {
    FE_LOGD("There is no aicore_num in options.");
    opti_compilation_info.SetAICoreNum(ai_core_cnt_inf);
  }

  return SUCCESS;
}

Status FusionManager::CheckOptiCompilationInfo(const map<string, string> &options, PlatFormInfos &platform_info,
                                               OptionalInfos &opti_compilation_info) const {
  bool ret_status;
  /* ge::CORE_TYPE is actually engine type. */
  auto eng_type = options.find(ge::CORE_TYPE);
  ret_status = (eng_type != options.end() && !eng_type->second.empty());

  if (ret_status) {
    FE_LOGI("ge.engine_type is %s.", eng_type->second.c_str());
    if (eng_type->second != AI_CORE_TYPE && eng_type->second != VECTOR_CORE_TYPE) {
      std::map<std::string, std::string> error_key_map;
      error_key_map["engine_type"] = eng_type->second;
      LogErrorMessage(EM_ENGINE_TYPE_INVALID, error_key_map);

      REPORT_FE_ERROR("[FusionMngr][ChkOptiCmptInfo] Optional compilation of eng_type is invalid, eng_type is %s.",
                      eng_type->second.c_str());
      return FAILED;
    }
    opti_compilation_info.SetCoreType(eng_type->second);
  } else {
    FE_LOGD("There is no engine_type in options.");
    opti_compilation_info.SetCoreType(AI_CORE_TYPE);
  }

  if (CheckOptiCompilationOfAiCoreNum(options, platform_info, opti_compilation_info) != SUCCESS) {
    REPORT_FE_ERROR("[FusionMngr][ChkOptiCmptInfo] CheckOptiCompilationOfAiCoreNum failed.");
    return FAILED;
  }

  auto l1_fusion_flag = options.find(ge::BUFFER_OPTIMIZE);
  ret_status = (l1_fusion_flag != options.end() && l1_fusion_flag->second == L1_OPTIMIZE);
  if (ret_status) {
    opti_compilation_info.SetL1FusionFlag(kStrTrue);
  } else {
    FE_LOGD("There is no l1_fusion_flag in options.");
    opti_compilation_info.SetL1FusionFlag(kStrFalse);
  }

  return SUCCESS;
}

Status FusionManager::InitPlatformConfig(const std::string &soc_version, const map<string, string> &options) const {
  PlatformInfo platform_info;
  OptionalInfo opti_compilation_info;

  PlatformInfoManager &platform_inst = PlatformInfoManager::Instance();
  if (platform_inst.InitializePlatformInfo() != SUCCESS) {
    REPORT_FE_ERROR("[FusionMngr][InitPlatCfg] Initialize platform_info failed.");
    return FAILED;
  }

  // verify soc version.
  // if the platform file is not found by soc version,
  // the soc version is invalid.
  if (platform_inst.GetPlatformInfo(soc_version, platform_info, opti_compilation_info) != SUCCESS) {
    REPORT_FE_ERROR("[FusionMngr][InitPlatCfg] Cannot find platform config file by soc version[%s].",
                    soc_version.c_str());
    std::map<std::string, std::string> error_key_map;
    error_key_map[fe::EM_VALUE] = soc_version;
    error_key_map[fe::EM_OPTION] = ge::SOC_VERSION;
    fe::LogErrorMessage(fe::EM_INPUT_OPTION_INVALID, error_key_map);
    return FAILED;
  }

  opti_compilation_info.soc_version = soc_version;

  if (CheckOptiCompilationInfo(options, platform_info, opti_compilation_info) != SUCCESS) {
    REPORT_FE_ERROR("[FusionMngr][InitPlatCfg] CheckOptiCompilationInfo failed.");
    return FAILED;
  }

  platform_inst.SetOptionalCompilationInfo(opti_compilation_info);

  PlatFormInfos platform_infos;
  OptionalInfos opti_compilation_infos;

  if (platform_inst.GetPlatformInfos(soc_version, platform_infos, opti_compilation_infos) != SUCCESS) {
    REPORT_FE_ERROR("[FusionMngr][InitPlatCfg] Cannot find platform config file by soc version[%s].",
                    soc_version.c_str());
    std::map<std::string, std::string> error_key_map;
    error_key_map[fe::EM_VALUE] = soc_version;
    error_key_map[fe::EM_OPTION] = ge::SOC_VERSION;
    fe::LogErrorMessage(fe::EM_INPUT_OPTION_INVALID, error_key_map);
    return FAILED;
  }

  opti_compilation_infos.SetSocVersion(soc_version);

  if (CheckOptiCompilationInfo(options, platform_infos, opti_compilation_infos) != SUCCESS) {
    REPORT_FE_ERROR("[FusionMngr][InitPlatCfg] CheckOptiCompilationInfo failed.");
    return FAILED;
  }

  platform_inst.SetOptionalCompilationInfo(opti_compilation_infos);

  FE_LOGD("Initialize platform_info success.");
  return SUCCESS;
}
/*
 * to initialize the subparts of fusion manager
 * param[in] the options of init
 * return Status(SUCCESS/FAILED)
 */
Status FusionManager::Initialize(const map<string, string> &options, const std::string &engine_name,
                                 const std::string &soc_version) {
  // add func lock
  std::lock_guard<std::mutex> lock_guard(kFmLock);

  FE_LOGD("Initialize start, engine_name:[%s]", engine_name.c_str());
  FE_TIMECOST_START(FusionMgrInit);
  if (inited_) {
    FE_LOGW("FusionManager has been inited, directly return!");
    return SUCCESS;
  }

  inited_ = true;

  if (InitPlatformConfig(soc_version, options) != SUCCESS) {
    REPORT_FE_ERROR("[FusionMngr][Init] Fail to initialize platform configuration.");
    return FAILED;
  }

  Configuration &config_inst = Configuration::Instance(engine_name);
  Status config_status = config_inst.Initialize(options, soc_version);
  if (config_status != SUCCESS) {
    REPORT_FE_ERROR("[FusionMngr][Init] Failed to initialize configuration, return status is [%u]", config_status);
    return CONFIGURATION_INIT_FAILED;
  }

  int64_t scope_id = ScopeAllocator::Instance().GetCurrentScopeId();
  FE_LOGD("Current scope id is [%ld]", scope_id);

  FE_MAKE_SHARED(op_store_adapter_manager_ = make_shared<OpStoreAdapterManager>(), return FAILED);
  Status ret = op_store_adapter_manager_->Initialize(options, engine_name);
  if (ret != fe::SUCCESS) {
    REPORT_FE_ERROR("[FusionMngr][Init] Op store adapter manager init failed.");
    return OP_STORE_ADAPTER_MANAGER_INIT_FAILED;
  }

  FE_MAKE_SHARED(ops_kernel_info_store_ = make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_, engine_name),
                 return FAILED);
  ret = ops_kernel_info_store_->Initialize(options);
  if (ret != fe::SUCCESS) {
    REPORT_FE_ERROR("[FusionMngr][Init] OpInfoKernelStores init failed! Ret [%u]", ret);
    return OPINFO_STORES_INIT_FAILED;
  }
  if (engine_name == kDsaCoreName) {
    FE_MAKE_SHARED(
        dsa_graph_opt_ = make_shared<DSAGraphOptimizer>(ops_kernel_info_store_, op_store_adapter_manager_, engine_name),
        return FAILED);
    ret = dsa_graph_opt_->Initialize(options, nullptr);
    if (ret != fe::SUCCESS) {
      REPORT_FE_ERROR("[FusionMngr][Init] DSAGraphOptimizer init failed ");
      return GRAPH_OPTIMIZER_INIT_FAILED;
    }
  } else {
    FE_MAKE_SHARED(
        graph_opt_ = make_shared<FEGraphOptimizer>(ops_kernel_info_store_, op_store_adapter_manager_, engine_name),
        return FAILED);
  }

  FE_LOGI("Initialize successfully.");
  FE_TIMECOST_END(FusionMgrInit, "FusionManager::Initialize");
  return SUCCESS;
}

/*
 * to release the source of fusion manager
 * return Status(SUCCESS/FAILED)
 */
Status FusionManager::Finalize() {
  // add func lock
  std::lock_guard<std::mutex> lock_guard(kFmLock);

  if (inited_) {
    FE_TIMECOST_START(FusionMgrFinal);

    FE_LOGD("Finalize begin.");

    Status ret1 = SUCCESS;
    if (graph_opt_ != nullptr) {
      ret1 = graph_opt_->Finalize();
      FE_LOGE_IF(ret1 != SUCCESS, "Finalize GraphOptimizer failed!");
    }

    Status ret2 = SUCCESS;
    std::string eng_type;
    if (ops_kernel_info_store_ != nullptr) {
      eng_type = ops_kernel_info_store_->GetFEOpsKernelInfoStoreName();
      ret2 = ops_kernel_info_store_->Finalize();
      FE_LOGE_IF(ret2 != SUCCESS, "Finalize Ops Kernel Info Store failed!");
    }

    Status ret3 = SUCCESS;
    if (op_store_adapter_manager_ != nullptr) {
      ret3 = op_store_adapter_manager_->Finalize();
      FE_LOGE_IF(ret3 != SUCCESS, "Finalize Ops Store Adapter Manage failed!");
    }

    Configuration &config_inst = Configuration::Instance(eng_type);
    Status ret4 = config_inst.Finalize();
    FE_LOGE_IF(ret4 != SUCCESS, "Finalize configuration failed!");

    PlatformInfoManager &config_inst_platform = PlatformInfoManager::Instance();
    Status ret5 = config_inst_platform.Finalize();
    FE_LOGE_IF(ret5 != SUCCESS, "Finalize PlatformInfoManager failed!");

    Status ret6 = CMOIdGenStrategy::Instance().Finalize();
    FE_LOGE_IF(ret6 != SUCCESS, "Finalize CMOIdGenStrategy failed!");

    Status ret_status = ((ret1 != SUCCESS) || (ret2 != SUCCESS) || (ret3 != SUCCESS) || (ret4 != SUCCESS) ||
                         (ret5 != SUCCESS) || (ret6 != SUCCESS));
    if (ret_status) {
      FE_LOGW("FusionManager finalize not success!");
      return FAILED;
    }

    inited_ = false;
    FE_LOGD("Finalize successfully.");
    FE_TIMECOST_END(FusionMgrFinal, "FusionManager::Finalize");
    return SUCCESS;
  }

  FE_LOGW("already Finalized,directly return.");
  return SUCCESS;
}

/*
 * to get the information of OpsKernel InfoStores
 * param[out] the map of OpsKernel InfoStores
 */
void FusionManager::GetOpsKernelInfoStores(map<string, OpsKernelInfoStorePtr> &op_kern_infos,
                                           const std::string &engine_name) {
  FE_LOGD("Get OpsKernelInfoStores start.");
  if (ops_kernel_info_store_ != nullptr) {
    op_kern_infos.emplace(
        std::make_pair(ops_kernel_info_store_->GetFEOpsKernelInfoStoreName(), ops_kernel_info_store_));
  } else {
    FE_LOGW("opsKernelInfoStore_ of engine named %s is nullptr! inited_ = %d", engine_name.c_str(), inited_);
  }

  FE_LOGD("Get OpsKernelInfoStores finished.");
}

/*
 * to get the information of Graph Optimizer
 * param[out] the map of Graph Optimizer
 */
void FusionManager::GetGraphOptimizerObjs(map<string, GraphOptimizerPtr> &graph_optimizers,
                                          const std::string &engine_name) {
  FE_LOGD("Get GraphOptimizer start.");
  ge::GraphOptimizerAttribute attrs;
  if (engine_name != kDsaCoreName && graph_opt_ == nullptr) {
    FE_LOGD("Engine named %s is not initialized.", engine_name.c_str());
    return;
  }
  if (engine_name == kDsaCoreName && dsa_graph_opt_ == nullptr) {
    FE_LOGD("Engine named %s is not initialzed.", engine_name.c_str());
    return;
  }
  if (engine_name == kDsaCoreName) {
    (void)dsa_graph_opt_->GetAttributes(attrs);
    graph_optimizers[attrs.engineName] = dsa_graph_opt_;
  } else {
    (void) graph_opt_->GetAttributes(attrs);
    graph_optimizers[attrs.engineName] = graph_opt_;
  }
  FE_LOGD("Get GraphOptimizer success.");
}

}  // namespace fe
