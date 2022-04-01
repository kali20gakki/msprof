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

#include <thread>
#include <gtest/gtest.h>

#define protected public
#define private public
#include "executor/dynamic_model_executor.h"
#include "executor/cpu_sched_event_dispatcher.h"
#undef private
#undef protected

#include "graph/build/graph_builder.h"
#include "runtime/deploy/stub_models.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "generator/ge_generator.h"
#include "ge/ge_api.h"

using namespace std;

namespace ge {
class MockDynamicModelExecutor : public DynamicModelExecutor {
 public:
  MockDynamicModelExecutor(bool is_host) : DynamicModelExecutor(is_host) {}
  Status DoLoadModel(const shared_ptr<GeRootModel> &root_model) override {
    model_id_ = 0;
    return SUCCESS;
  }

  Status DoExecuteModel(const RunModelData &inputs, RunModelData &outputs) override {
    DataBuffer &data_buffer = outputs.blobs[0];
    if (data_buffer.data == nullptr) {
      data_buffer.data = output_buffer_;
    }
    data_buffer.length = 8;
    auto output_values = reinterpret_cast<int32_t *>(data_buffer.data);
    output_values[0] = 222;
    output_values[1] = 666;
    GeTensorDesc tensor_desc;
    tensor_desc.SetShape(GeShape({2}));
    output_tensor_descs_ = {tensor_desc};
    return SUCCESS;
  }

 private:
  uint8_t output_buffer_[8];
};

class DynamicModelExecutorTest : public testing::Test {
 protected:
  void SetUp() override {
  }
  void TearDown() override {
  }

  GeRootModelPtr BuildRootModel(const std::vector<int64_t> &shape) {
    //  dlog_setlevel(GE_MODULE_NAME, DLOG_DEBUG, 1);
    vector<std::string> engine_list = {"AIcoreEngine"};
    auto data1 = OP_CFG(DATA)
        .InCnt(1)
        .OutCnt(1)
        .TensorDesc(FORMAT_ND, DT_INT32, shape)
        .Attr(ATTR_NAME_INDEX, 0);
    auto neg = OP_CFG(NEG).InCnt(1).OutCnt(1).TensorDesc(FORMAT_ND, DT_INT32, shape);
    auto netoutput = OP_CFG(NETOUTPUT).InCnt(1).OutCnt(1).TensorDesc(FORMAT_NCHW, DT_INT32, shape);
    DEF_GRAPH(g1) {
      CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("neg", neg)->NODE("Node_Output", netoutput));
    };

    auto root_model = std::make_shared<GeRootModel>();
    auto root_graph = ToComputeGraph(g1);
    root_model->SetRootGraph(root_graph);
    return root_model;
  }
};


TEST_F(DynamicModelExecutorTest, TestDynamicModel) {
  auto &dispatcher = CpuSchedEventDispatcher::GetInstance();
  EXPECT_EQ(dispatcher.Initialize(0), SUCCESS);

  rtMbufPtr_t input_mbuf = nullptr;
  RuntimeTensorDesc input_runtime_tensor_desc{};
  input_runtime_tensor_desc.shape[0] = 1;
  input_runtime_tensor_desc.shape[1] = 2;
  input_runtime_tensor_desc.original_shape[0] = 1;
  input_runtime_tensor_desc.original_shape[1] = 2;
  input_runtime_tensor_desc.dtype = DT_FLOAT;
  input_runtime_tensor_desc.format = FORMAT_ND;
  rtMbufAlloc(&input_mbuf, sizeof(input_runtime_tensor_desc) + 8);
  void *input_buffer = nullptr;
  rtMbufGetBuffAddr(input_mbuf, &input_buffer);
  memcpy(input_buffer, &input_runtime_tensor_desc, sizeof(input_runtime_tensor_desc));
  {
    auto root_model = BuildRootModel({-1});
    MockDynamicModelExecutor executor(false);
    EXPECT_EQ(executor.LoadModelWithQueue(root_model, {1}, {2}), SUCCESS);

    AICPUSubEventInfo event_info{};
    event_info.modelId = 0;
    rtEschedEventSummary_t event_summary;
    event_summary.msg = (char *)&event_info;
    event_summary.msgLen = sizeof(event_info);
    executor.input_mbuf_addresses_[0] = input_mbuf;
    dispatcher.OnInputsReady(event_summary);
    executor.UnloadModel();

    uint64_t output_buffer_size = 0;
    rtMbufGetBuffSize(executor.output_mbuf_addresses_[0], &output_buffer_size);
    EXPECT_EQ(output_buffer_size, sizeof(RuntimeTensorDesc) + 8);
    void *output_buffer = nullptr;
    rtMbufGetBuffAddr(executor.output_mbuf_addresses_[0], &output_buffer);
    auto output_desc = reinterpret_cast<RuntimeTensorDesc *>(output_buffer);

    EXPECT_EQ(output_desc->shape[0], 1);
    EXPECT_EQ(output_desc->shape[1], 2);
    auto output_values = reinterpret_cast<int32_t *>(PtrToValue(output_buffer) + sizeof(RuntimeTensorDesc));
    EXPECT_EQ(output_values[0], 222);
    EXPECT_EQ(output_values[1], 666);
    rtMbufFree(executor.output_mbuf_addresses_[0]);
  }

  dispatcher.Finalize();
  rtMbufFree(input_mbuf);
}

TEST_F(DynamicModelExecutorTest, TestStaticModel) {
  auto &dispatcher = CpuSchedEventDispatcher::GetInstance();
  EXPECT_EQ(dispatcher.Initialize(0), SUCCESS);

  rtMbufPtr_t input_mbuf = nullptr;
  rtMbufAlloc(&input_mbuf, 8);
  {
    auto root_model = BuildRootModel({2});
    MockDynamicModelExecutor executor(false);
    EXPECT_EQ(executor.LoadModelWithQueue(root_model, {1}, {2}), SUCCESS);

    AICPUSubEventInfo event_info{};
    event_info.modelId = 0;
    rtEschedEventSummary_t event_summary;
    event_summary.msg = (char *)&event_info;
    event_summary.msgLen = sizeof(event_info);
    executor.input_mbuf_addresses_[0] = input_mbuf;
    dispatcher.OnInputsReady(event_summary);
    executor.UnloadModel();

    uint64_t output_buffer_size = 0;
    rtMbufGetBuffSize(executor.output_mbuf_addresses_[0], &output_buffer_size);
    EXPECT_EQ(output_buffer_size, 8);
    int32_t *output_buffer = nullptr;
    rtMbufGetBuffAddr(executor.output_mbuf_addresses_[0], (void **)&output_buffer);
    EXPECT_EQ(output_buffer[0], 222);
    EXPECT_EQ(output_buffer[1], 666);
    rtMbufFree(executor.output_mbuf_addresses_[0]);
  }

  dispatcher.Finalize();
  rtMbufFree(input_mbuf);
}
}  // namespace ge