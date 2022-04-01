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

#ifndef FUSION_ENGINE_OPTIMIZER_COMMON_DUMP_UTIL_H_
#define FUSION_ENGINE_OPTIMIZER_COMMON_DUMP_UTIL_H_

#include <string>
#include <vector>
#include "graph/node.h"
#include "tensor_engine/fusion_api.h"

namespace fe {
/*
 *  @ingroup fe
 *  @brief   get input and output's name
 *  @param   [in]  op                 op desc
 *  @param   [in]  info_store          ops info store pointer
 *  @param   [in/out] op_kernel_info_ptr infos of op
 *  @param   [in/out] input_map        map to recorde inputs's name
 *  @param   [in/out] output_map       map to recorde outputs's name
 *  @return  SUCCESS or FAILED
 */
void DumpOpInfoTensor(std::vector<te::TbeOpParam> &puts, std::string &debug_str);

void DumpOpInfo(te::TbeOpInfo &op_info);

void DumpL1Attr(const ge::Node *node);

void DumpL2Attr(const ge::Node *node);
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_COMMON_DUMP_UTIL_H_
