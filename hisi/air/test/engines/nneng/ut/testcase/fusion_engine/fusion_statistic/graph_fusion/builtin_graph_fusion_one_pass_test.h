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

#ifndef LLT_FUSION_ENGINE_UT_TESTCASE_FUSION_ENGINE_FUSION_STATISTIC_GRAPH_FUSION_BUILTIN_GRAPH_FUSION_ONE_PASS_TEST_H_
#define LLT_FUSION_ENGINE_UT_TESTCASE_FUSION_ENGINE_FUSION_STATISTIC_GRAPH_FUSION_BUILTIN_GRAPH_FUSION_ONE_PASS_TEST_H_
#include "register/graph_optimizer/fusion_common/pattern_fusion_base_pass.h"
#include "common/fe_log.h"

namespace fe {
    class BuiltGraphFusionOnePassTest : public PatternFusionBasePass {
    public:
        vector<FusionPattern *> DefinePatterns() override {
            vector<FusionPattern *> patterns;
            FusionPattern *pattern = new (std::nothrow) FusionPattern("MyBuiltPattern");
            FE_CHECK(pattern == nullptr, FE_LOGE("New a pattern object failed."),  return patterns);
            /* conv2_d_back_prop_filter(Fragz)
         *          |
         *        a.m.(NCHW)  */
            pattern->AddOpDesc("am1", {"am1"})
                    .AddOpDesc("conv2DBackPropFilter", {"conv2DBackPropFilter"})
                    .SetInputs("am1", {"conv2DBackPropFilter"})
                    .SetOutput("am1");
            patterns.push_back(pattern);
            return patterns;
        }

        Status Fusion(ge::ComputeGraph &graph,
                      Mapping &mapping,
                      vector<ge::NodePtr> &fusion_nodes) override {
            return SUCCESS;
        }
    };
}
#endif  // LLT_FUSION_ENGINE_UT_TESTCASE_FUSION_ENGINE_FUSION_STATISTIC_GRAPH_FUSION_BUILTIN_GRAPH_FUSION_ONE_PASS_TEST_H_
