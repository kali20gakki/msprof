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

//
// Created by l00506234 on 2020/7/22.
//

#ifndef LLT_FUSION_ENGINE_UT_TESTCASE_FUSION_ENGINE_UB_FUSION_BUILTIN_BUFFER_FUSION_PASS_TEST_H_
#define LLT_FUSION_ENGINE_UT_TESTCASE_FUSION_ENGINE_UB_FUSION_BUILTIN_BUFFER_FUSION_PASS_TEST_H_
#include "register/graph_optimizer/buffer_fusion/buffer_fusion_pass_base.h"
namespace fe {
    class BuiltBufferFusionPassTest : public BufferFusionPassBase {
    public:
        std::vector<BufferFusionPattern *> DefinePatterns() override {
            return vector<BufferFusionPattern *>{};
        }
    };
}
#endif  // LLT_FUSION_ENGINE_UT_TESTCASE_FUSION_ENGINE_UB_FUSION_BUILTIN_BUFFER_FUSION_PASS_TEST_H_
