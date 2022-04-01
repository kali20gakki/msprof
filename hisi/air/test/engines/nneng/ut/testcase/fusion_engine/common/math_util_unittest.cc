/**
 * Copyright 2021-2022 Huawei Technologies Co., Ltd
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

#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <vector>

#define protected public
#define private public
#include "common/math_util.h"
#include "common/fe_utils.h"
#undef protected
#undef private

using namespace std;
using namespace fe;

class UTEST_math_util_unittest : public testing::Test
{
 protected:
  void SetUp()
  {
  }

  void TearDown()
  {
  }
};

TEST_F(UTEST_math_util_unittest, Uint16ToFloat)
{
    uint16_t number = 1;
    float res = Uint16ToFloat(number);
}

TEST_F(UTEST_math_util_unittest, fe_util_get_bool_str)
{
    bool value = true;
    std::string res = GetBoolString(value);
    EXPECT_EQ(res, "true");
}

TEST_F(UTEST_math_util_unittest, fe_util_get_relu_type)
{
    RuleType ruleType = fe::RuleType::BUILT_IN_GRAPH_RULE;
    std::string res = GetRuleTypeString(ruleType);
    EXPECT_EQ(res, "built-in-graph-rule");
}

TEST_F(UTEST_math_util_unittest, fe_util_get_relu_type_unknown)
{
    RuleType ruleType = fe::RuleType::RULE_TYPE_RESERVED;
    std::string res = GetRuleTypeString(ruleType);
    EXPECT_EQ(res, "unknown-rule-type 2");
}
