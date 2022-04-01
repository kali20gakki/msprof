/**
 * Copyright 2019-2021 Huawei Technologies Co., Ltd
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
#include <memory>

#define protected public
#define private public
#include "hybrid/hybrid_davinci_model.h"
#include "common/dump/dump_manager.h"
#include "../base/graph/manager/graph_manager_utils.h"
#undef private
#undef protected

using namespace std;
using namespace testing;
using namespace ge;
using namespace hybrid;

class HybridDavinciModelTest : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

namespace {

void ListenerCallback(Status status, std::vector<ge::Tensor> &tensors){
  return;
}

TEST_F(HybridDavinciModelTest, HybridDavinciModel_SetGet) {
  auto mod = HybridDavinciModel::Create(std::make_shared<GeRootModel>());
  mod->SetModelId(100);
  mod->SetDeviceId(1001);
  EXPECT_EQ(mod->GetDeviceId(), 1001);
  mod->SetOmName("om_name");
  mod->SetListener(std::make_shared<GraphModelListener>());
  EXPECT_EQ(mod->GetSessionId(), 0);
  std::vector<std::vector<int64_t>> batch_info;
  int32_t dynamic_type = 0;
  EXPECT_EQ(mod->GetDynamicBatchInfo(batch_info, dynamic_type), SUCCESS);
  std::vector<std::string> user_input_shape_order;
  mod->GetUserDesignateShapeOrder(user_input_shape_order);
  EXPECT_EQ(user_input_shape_order.size(), 0);
  std::vector<std::string> dynamic_output_shape_info;
  mod->GetOutputShapeInfo(dynamic_output_shape_info);
  EXPECT_EQ(dynamic_output_shape_info.size(), 0);
  mod->SetModelDescVersion(true);
  EXPECT_EQ(mod->GetRunningFlag(), false);
  EXPECT_EQ(mod->SetRunAsyncListenerCallback(ListenerCallback), PARAM_INVALID);
}


TEST_F(HybridDavinciModelTest, HybridDavinciModel_Null) {
  auto mod = HybridDavinciModel::Create(std::make_shared<GeRootModel>());
  mod->impl_ = nullptr;
  EXPECT_EQ(mod->GetDataInputerSize(), PARAM_INVALID);
  EXPECT_EQ(mod->ModelRunStart(), PARAM_INVALID);
  EXPECT_EQ(mod->ModelRunStop(), PARAM_INVALID);
  EXPECT_EQ(mod->GetDataInputerSize(), PARAM_INVALID);
  std::vector<GeTensor> inputs;
  std::vector<GeTensor> outputs;
  EXPECT_EQ(mod->Execute(inputs, outputs), PARAM_INVALID);
  std::vector<InputOutputDescInfo> input_desc;
  std::vector<InputOutputDescInfo> output_desc;
  std::vector<uint32_t> input_formats;
  std::vector<uint32_t> output_formats;
  EXPECT_EQ(mod->GetInputOutputDescInfo(input_desc, output_desc, input_formats, output_formats), PARAM_INVALID);
  uint32_t stream_id = 0;
  uint32_t task_id = 0;
  OpDescInfo op_desc_info;
  EXPECT_EQ(mod->GetOpDescInfo(stream_id, task_id, op_desc_info), false);
}

}
