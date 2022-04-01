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

#include "tensor_engine/fusion_api.h"

namespace te {

bool PreBuildTbeOp(TbeOpInfo &info) {
    info.SetPattern("mocker");
    return true;
}

bool TeFusion(std::vector<ge::NodePtr> teGraphNode, ge::NodePtr output_node)
{
    const std::string key_to_attr_json_file_path = "json_file_path";
    std::string json_file_path = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.json";
    ge::AttrUtils::SetStr(output_node->GetOpDesc(), key_to_attr_json_file_path, json_file_path);
    return true;
}

bool TeFusionEnd()
{
    return true;
}

}
