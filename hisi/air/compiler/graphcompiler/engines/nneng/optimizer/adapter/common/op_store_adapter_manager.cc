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

#include "adapter/common/op_store_adapter_manager.h"
#include <utility>
#include <vector>
#include "common/fe_type_utils.h"
#include "common/configuration.h"
#include "common/fe_inner_error_codes.h"
#include "graph/ge_tensor.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "adapter/dsa_adapter/dsa_op_store_adapter.h"

namespace fe {
static const std::string kStrTbeOpAdapter = "tbe_op_adapter";
static const std::string kStrDsaOpAdapter = "dsa_op_adapter";
static const std::string kStrReserve = "reserve";
static const std::map<OpImplType, std::string> ADAPTER_TYPE_MAP {
        {EN_IMPL_CUSTOM_TIK, kStrReserve},
        {EN_IMPL_CUSTOM_TBE, kStrTbeOpAdapter},
        {EN_IMPL_HW_TIK, kStrReserve},
        {EN_IMPL_HW_TBE, kStrTbeOpAdapter},
        {EN_IMPL_RL, kStrReserve},
        {EN_IMPL_VECTOR_CORE_HW_TBE, kStrTbeOpAdapter},
        {EN_IMPL_VECTOR_CORE_CUSTOM_TBE, kStrTbeOpAdapter},
        {EN_IMPL_NON_PERSISTENT_CUSTOM_TBE, kStrTbeOpAdapter},
        {EN_IMPL_HW_DSA, kStrDsaOpAdapter}};

OpStoreAdapterManager::OpStoreAdapterManager() : init_flag_(false), map_all_op_store_adapter_() {}

OpStoreAdapterManager::~OpStoreAdapterManager() {}

Status OpStoreAdapterManager::InitializeAdapter(const std::string adapter_type,
                                                const std::map<std::string, std::string> &options,
                                                const std::string &engine_name) {
  Status result = SUCCESS;
  FE_LOGD("The InitializeAdapter is adapter[%s].", adapter_type.c_str());

  if (adapter_type == kStrTbeOpAdapter) {
    std::map<std::string, OpStoreAdapterPtr>::const_iterator adapter_ptr_iter =
            map_all_op_store_adapter_.find(kStrTbeOpAdapter);
    if (adapter_ptr_iter != map_all_op_store_adapter_.end()) {
      FE_LOGD("The tbe op store adapter has already been initialized.");
      return SUCCESS;
    }
    TbeOpStoreAdapterPtr tbe_adapter_ptr = nullptr;
    FE_MAKE_SHARED(tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>(), return OP_STORE_ADAPTER_MAKE_SHARED_FAILED);
    FE_CHECK(tbe_adapter_ptr == nullptr, FE_LOGE("tbeAdapterPtr is nullptr."), return PARAM_INVALID);
    result = tbe_adapter_ptr->Initialize(options, engine_name);
    if (result == SUCCESS) {
      map_all_op_store_adapter_.emplace(std::make_pair(kStrTbeOpAdapter, tbe_adapter_ptr));
    } else {
      REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][InitAdapter] InitializeAdapter adapter [%s] failed! Ret [%u]",
                      kStrTbeOpAdapter.c_str(), result);
      return result;
    }
  } else if (adapter_type == kStrDsaOpAdapter) {
    std::map<std::string, OpStoreAdapterPtr>::const_iterator adapter_ptr_iter =
            map_all_op_store_adapter_.find(kStrDsaOpAdapter);
    if (adapter_ptr_iter != map_all_op_store_adapter_.end()) {
      FE_LOGD("The dsa op store adapter has already been initialized.");
      return SUCCESS;
    }
    DSAOpStoreAdapterPtr dsa_adapter_ptr = nullptr;
    FE_MAKE_SHARED(dsa_adapter_ptr = std::make_shared<DSAOpStoreAdapter>(), return OP_STORE_ADAPTER_MAKE_SHARED_FAILED);
    FE_CHECK(dsa_adapter_ptr == nullptr, FE_LOGE("dsaAdapterPtr is nullptr."), return PARAM_INVALID);
    result = dsa_adapter_ptr->Initialize(options, engine_name);
    if (result == SUCCESS) {
      map_all_op_store_adapter_.emplace(std::make_pair(kStrDsaOpAdapter, dsa_adapter_ptr));
    } else {
      REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][InitAdapter] InitializeAdapter adapter [%s] failed! Ret [%u]",
                      kStrDsaOpAdapter.c_str(), result);
      return result;
    }
  }
  return result;
}

Status OpStoreAdapterManager::Initialize(const std::map<std::string, std::string> &options,
                                         const std::string &engine_name) {
  if (init_flag_) {
    FE_LOGD("The OpStoreAdapterManager has already been initialized.");
    return SUCCESS;
  }
  /* Before OpStoreAdapterManager is initialized, Configuration class has
    already loaded ops store info vector */
  init_flag_ = true;
  std::vector<FEOpsStoreInfo> fe_ops_store_info_vec = Configuration::Instance(engine_name).GetOpsStoreInfo();

  for (auto &ops_sub_store_info : fe_ops_store_info_vec) {
    auto adapter_str_iter = ADAPTER_TYPE_MAP.find(ops_sub_store_info.op_impl_type);
    if (adapter_str_iter == ADAPTER_TYPE_MAP.end()) {
      REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][Init] Adapter type is not found, OpsStoreName[%s].",
                      ops_sub_store_info.fe_ops_store_name.c_str());
      return OP_ADAPTER_CHECK_FAILED;
    }
    std::string adapter_type_str = adapter_str_iter->second;
    Status result = InitializeAdapter(adapter_type_str, options, engine_name);
    if (result != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][Init] Initialize op store adapter failed, OpsStoreName[%s].",
                      ops_sub_store_info.fe_ops_store_name.c_str());
      return result;
    }
  }

  return SUCCESS;
}

Status OpStoreAdapterManager::Finalize() {
  FE_LOGD("Finalizing the OpStoreAdapterManager.");
  if (!init_flag_) {
    FE_LOGD(
        "OpStoreAdapterManager has not been initialized, Finalize is not "
        "allowed.");
    return SUCCESS;
  }

  for (auto &elem : map_all_op_store_adapter_) {
    if (elem.second == nullptr) {
      FE_LOGW(
          "OpStoreAdapterManager::Finalize: pointer in "
          "mapAllOpStoreAdapter_ [%s] should not be nullptr!",
          elem.first.c_str());
      continue;
    }
    elem.second->Finalize();
  }
  map_all_op_store_adapter_.clear();
  init_flag_ = false;
  FE_LOGI("OpStoreAdapterManager finalize success.");
  return SUCCESS;
}

Status OpStoreAdapterManager::GetOpStoreAdapter(const OpImplType &op_impl_type,
                                                OpStoreAdapterPtr &adapter_ptr) const {
  auto adapter_str_iter = ADAPTER_TYPE_MAP.find(op_impl_type);
  if (adapter_str_iter == ADAPTER_TYPE_MAP.end()) {
    REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][GetOpStoreAdapter] Op store adapter type is not found, \
                    op_impl_type[%s].", GetImplTypeString(op_impl_type).c_str());
    return OP_ADAPTER_TYPE_CHECK_FAILED;
  }

  auto adapter_ptr_iter = map_all_op_store_adapter_.find(adapter_str_iter->second);
  if (adapter_ptr_iter == map_all_op_store_adapter_.end()) {
    REPORT_FE_ERROR("[SubGraphOpt][PreCompileOp][GetOpStoreAdapter] op store adapter is not found, adapter name [%s].",
                    adapter_str_iter->second.c_str());
    return OP_ADAPTER_CHECK_FAILED;
  }

  adapter_ptr = adapter_ptr_iter->second;
  return SUCCESS;
}

}  // namespace fe
