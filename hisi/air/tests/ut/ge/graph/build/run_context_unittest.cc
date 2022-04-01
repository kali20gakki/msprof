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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>

#define private public
#define protected public
#include "graph/build/run_context.h"

#include "framework/common/util.h"
#include "framework/common/debug/ge_log.h"
#include "graph/debug/ge_attr_define.h"
#include "common/omg_util.h"

#undef private
#undef protected

using namespace std;
using namespace testing;

namespace ge {

class UtestRunContext : public testing::Test {
 protected:
  void SetUp() {
  }
  void TearDown() {
  }
};

TEST_F(UtestRunContext, InitMemInfo) {
    RunContextUtil run_util;
    uint8_t *data_mem_base = nullptr;
    uint64_t data_mem_size = 1024;
    std::map<int64_t, uint8_t *> mem_type_to_data_mem_base;
    std::map<int64_t, uint64_t> mem_type_to_data_mem_size;
    uint8_t *weight_mem_base = nullptr;
    uint64_t weight_mem_size = 256;
    EXPECT_EQ(run_util.InitMemInfo(data_mem_base, data_mem_size, mem_type_to_data_mem_base, mem_type_to_data_mem_size, weight_mem_base, weight_mem_size), PARAM_INVALID);
    uint8_t buf1[1024] = {0};
    data_mem_base = buf1;
    EXPECT_EQ(run_util.InitMemInfo(data_mem_base, data_mem_size, mem_type_to_data_mem_base, mem_type_to_data_mem_size, weight_mem_base, weight_mem_size), PARAM_INVALID);
    uint8_t buf2[1024] = {0};
    weight_mem_base = buf2;
    EXPECT_EQ(run_util.InitMemInfo(data_mem_base, data_mem_size, mem_type_to_data_mem_base, mem_type_to_data_mem_size, weight_mem_base, weight_mem_size), PARAM_INVALID);
}

TEST_F(UtestRunContext, CreateRunContext) {
    RunContextUtil run_util;
    Model model("name", "1.1");
    ComputeGraphPtr graph = nullptr;
    Buffer buffer(1024);
    uint64_t session_id = 0;
    AttrUtils::SetInt(&model, ATTR_MODEL_STREAM_NUM, 10);
    EXPECT_EQ(run_util.CreateRunContext(model, graph, buffer, session_id), PARAM_INVALID);
}


} // namespace ge