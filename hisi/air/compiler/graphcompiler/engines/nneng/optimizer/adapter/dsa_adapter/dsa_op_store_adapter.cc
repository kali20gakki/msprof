/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#include "adapter/dsa_adapter/dsa_op_store_adapter.h"
#include "common/configuration.h"
#include "common/fe_error_code.h"
#include "common/fe_log.h"
#include "common/fe_type_utils.h"
#include "common/op_info_common.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/tuning_utils.h"
#include "graph/utils/tensor_utils.h"

namespace fe {
/*
 *  @ingroup fe
 *  @brief   initial resources needed by DSAOpStoreAdapter, such as dlopen so
 *  files and load function symbols etc.
 *  @return  SUCCESS or FAILED
 */
Status DSAOpStoreAdapter::Initialize(const std::map<std::string, std::string> &options,
                                     const std::string &engine_name) {
  // return SUCCESS if graph optimizer has been initialized.
  if (init_flag) {
    FE_LOGW("DSAOpStoreAdapter has been initialized.");
    return SUCCESS;
  }
  /* set the engine name first */
  engine_name_ = engine_name;
  init_flag = true;
  FE_LOGI("Initialize dsa op store adapter success.");
  return SUCCESS;
}

/*
 *  @ingroup fe
 *  @brief   finalize resources initialized in Initialize function,
 *           such as dclose so files etc.
 *  @return  SUCCESS or FAILED
 */
Status DSAOpStoreAdapter::Finalize() {
  // return SUCCESS if graph optimizer has been initialized.
  if (!init_flag) {
    REPORT_FE_ERROR("[GraphOpt][Finalize] DSAOpStoreAdapter not allowed to finalize before initialized.");
    return FAILED;
  }

  init_flag = false;
  FE_LOGI("Finalize dsa op store adapter success.");
  return SUCCESS;
}
}  // namespace fe
