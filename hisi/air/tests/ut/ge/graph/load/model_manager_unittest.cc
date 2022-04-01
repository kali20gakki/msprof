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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string.h>

#define private public
#define protected public

#include "common/profiling/profiling_manager.h"
#include "common/profiling/profiling_properties.h"
#include "common/helper/om_file_helper.h"
#include "common/op/ge_op_utils.h"
#include "hybrid/model/hybrid_model_builder.h"
#include "graph/load/graph_loader.h"
#include "graph/load/model_manager/model_manager.h"
#include "graph/load/model_manager/davinci_model.h"
#include "graph/load/model_manager/model_utils.h"
#include "graph/load/model_manager/data_inputer.h"
#include "graph/ops_stub.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "exec_runtime/execution_runtime.h"
#include "ut/ge/ffts_plus_proto_tools.h"

using namespace std;
using namespace testing;

namespace ge {
const static std::string ENC_KEY = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

class UtestModelManagerModelManager : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {
    EXPECT_TRUE(ModelManager::GetInstance().model_map_.empty());
    EXPECT_TRUE(ModelManager::GetInstance().hybrid_model_map_.empty());
  }

  void CreateGraph(Graph &graph) {
    TensorDesc desc(ge::Shape({1, 3, 224, 224}));
    uint32_t size = desc.GetShape().GetShapeSize();
    desc.SetSize(size);
    auto data = op::Data("Data").set_attr_index(0);
    data.update_input_desc_data(desc);
    data.update_output_desc_out(desc);

    auto flatten = op::Flatten("Flatten").set_input_x(data, data.name_out_out());

    std::vector<Operator> inputs{data};
    std::vector<Operator> outputs{flatten};
    std::vector<Operator> targets{flatten};
    // Graph graph("test_graph");
    graph.SetInputs(inputs).SetOutputs(outputs).SetTargets(targets);
  }

  void GenUnencryptModelData(ModelData &data) {
    const int model_len = 10;
    data.model_len = sizeof(ModelFileHeader) + model_len;
    data.model_data = new uint8_t[data.model_len];
    memset(data.model_data, 0, data.model_len);

    ModelFileHeader *header = (ModelFileHeader *) data.model_data;
    header->magic = MODEL_FILE_MAGIC_NUM;
    header->version = MODEL_VERSION;
    header->is_encrypt = ModelEncryptType::UNENCRYPTED;
    header->length = model_len;
    header->is_checksum = ModelCheckType::CHECK;
  }

  void LoadStandardModelData(ModelData &data, const uint32_t model_len = 512, const uint32_t model_num = 1U) {
    data.model_len = model_len;
    data.model_data = new uint8_t[data.model_len];
    uint8_t *model_data = reinterpret_cast<uint8_t *>(data.model_data);

    uint32_t mem_offset = sizeof(ModelFileHeader);
    for (uint32_t i = 0; i < model_num; ++i) {
      ModelPartitionTable *partition_table = reinterpret_cast<ModelPartitionTable *>(model_data + mem_offset);
      partition_table->num = PARTITION_SIZE;

      mem_offset += sizeof(ModelPartitionTable) + sizeof(ModelPartitionMemInfo) * 5;
      {
        Model model;
        ComputeGraphPtr graph = make_shared<ComputeGraph>("default");
        model.SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
        model.SetVersion(123);

        Buffer buffer;
        model.Save(buffer);
        EXPECT_TRUE(mem_offset + buffer.GetSize() < model_len);
        memcpy(model_data + mem_offset, buffer.GetData(), buffer.GetSize());

        ModelPartitionMemInfo &partition_info = partition_table->partition[0];
        partition_info.type = ModelPartitionType::MODEL_DEF;
        partition_info.mem_size = buffer.GetSize();
        mem_offset += buffer.GetSize();
      }

      {
        ModelPartitionMemInfo &partition_info = partition_table->partition[1];
        partition_info.type = ModelPartitionType::WEIGHTS_DATA;
        partition_info.mem_offset = mem_offset;
        partition_info.mem_size = 0;
      }

      {
        ModelPartitionMemInfo &partition_info = partition_table->partition[2];
        partition_info.type = ModelPartitionType::TASK_INFO;
        partition_info.mem_offset = mem_offset;
        partition_info.mem_size = 0;
      }

      {
        ModelPartitionMemInfo &partition_info = partition_table->partition[3];
        partition_info.type = ModelPartitionType::TBE_KERNELS;
        partition_info.mem_offset = mem_offset;
        partition_info.mem_size = 0;
      }

      {
        ModelPartitionMemInfo &partition_info = partition_table->partition[4];
        partition_info.type = ModelPartitionType::CUST_AICPU_KERNELS;
        partition_info.mem_offset = mem_offset;
        partition_info.mem_size = 0;
      }
    }

    EXPECT_TRUE(mem_offset < model_len);
    ModelFileHeader *header = new(data.model_data) ModelFileHeader;
    header->length = mem_offset - sizeof(ModelFileHeader);
    header->model_num = model_num;
    data.model_len = mem_offset;
  }
};

  class MockModelDeployer : public ModelDeployer {
   public:
    Status DeployModel(const FlowModelPtr &flow_model,
                       const vector<uint32_t> &input_queue_ids,
                       const vector<uint32_t> &output_queue_ids,
                       DeployResult &deploy_result) override {
      deploy_result.model_id = 1;
      return SUCCESS;
    }

    MOCK_METHOD1(Undeploy, Status(uint32_t));
  };

  class MockExchangeService : public ExchangeService {
   public:
    Status CreateQueue(int32_t device_id,
                       const string &name,
                       uint32_t depth,
                       uint32_t work_mode,
                       uint32_t &queue_id) override {
      return SUCCESS;
    }
    Status DestroyQueue(int32_t device_id, uint32_t queue_id) override {
      return SUCCESS;
    }
    Status Enqueue(int32_t device_id, uint32_t queue_id, const void *data, size_t size) override {
      return SUCCESS;
    }
    Status Enqueue(int32_t device_id, uint32_t queue_id, size_t size, const FillFunc &fill_func) override {
      return SUCCESS;
    }
    Status Peek(int32_t device_id, uint32_t queue_id, size_t &size) override {
      return SUCCESS;
    }
    Status Dequeue(int32_t device_id, uint32_t queue_id, void *data, size_t size, ControlInfo &control_Info) override {
      return SUCCESS;
    }
    Status DequeueTensor(const int32_t device_id, const uint32_t queue_id, GeTensor &tensor,
                         ControlInfo &control_Info) override {
      return 0;
    }
  };

  class MockExecutionRuntime : public ExecutionRuntime {
   public:
    Status Initialize(const map<std::string, std::string> &options) override {
      return SUCCESS;
    }
    Status Finalize() override {
      return SUCCESS;
    }
    ModelDeployer &GetModelDeployer() override {
      return model_deployer_;
    }
    ExchangeService &GetExchangeService() override {
      return exchange_service_;
    }

   private:
    MockModelDeployer model_deployer_;
    MockExchangeService exchange_service_;
  };


class DModelListener : public ModelListener {
 public:
  DModelListener() {};
  uint32_t OnComputeDone(uint32_t model_id, uint32_t data_index,
                         uint32_t resultCode, std::vector<ge::Tensor> &outputs) { return 0; }
};

TEST_F(UtestModelManagerModelManager, case_is_need_hybrid_load) {
  ModelManager mm;
  ComputeGraphPtr root_graph = std::make_shared<ComputeGraph>("graph");
  ge::GeRootModel model;
  EXPECT_EQ(mm.IsNeedHybridLoad(model), false);
  model.SetRootGraph(root_graph);
  EXPECT_EQ(mm.IsNeedHybridLoad(model), false);
}

TEST_F(UtestModelManagerModelManager, case_load_incorrect_param) {
  ModelManager mm;
  uint32_t model_id = 0;
  ModelData data;
  // Load allow listener is null
  EXPECT_EQ(mm.LoadModelOffline(model_id, data), ACL_ERROR_GE_EXEC_MODEL_DATA_SIZE_INVALID);
}

TEST_F(UtestModelManagerModelManager, case_load_model_len_too_short) {
  ModelManager mm;
  ModelData data;
  data.model_len = 10;
  data.model_data = (void *) &data;
  uint32_t model_id = 1;
  EXPECT_EQ(mm.LoadModelOffline(model_id, data), ACL_ERROR_GE_PARAM_INVALID);
  data.model_data = nullptr;
}

TEST_F(UtestModelManagerModelManager, case_load_model_len_not_match) {
  ModelManager mm;
  ModelData data;
  GenUnencryptModelData(data);
  data.model_len = sizeof(ModelFileHeader) + 1;
  uint32_t model_id = 1;
  EXPECT_EQ(mm.LoadModelOffline(model_id, data), ACL_ERROR_GE_PARAM_INVALID);
  delete[](uint8_t *) data.model_data;
}

TEST_F(UtestModelManagerModelManager, case_load_model_encypt_not_match) {
  ModelManager mm;
  ModelData data;
  GenUnencryptModelData(data);
  data.key = ENC_KEY;
  uint32_t model_id = 1;
  EXPECT_EQ(mm.LoadModelOffline(model_id, data), ACL_ERROR_GE_PARAM_INVALID);
  delete[](uint8_t *) data.model_data;
}

TEST_F(UtestModelManagerModelManager, case_load_model_encypt_type_unsupported) {
  ModelManager mm;
  ModelData data;
  GenUnencryptModelData(data);
  ModelFileHeader *header = (ModelFileHeader *) data.model_data;
  header->is_encrypt = 255;
  uint32_t model_id = 1;
  // Error for: LoadModelPartitionTable: Invalid partition_table->num:0
  EXPECT_EQ(mm.LoadModelOffline(model_id, data), ACL_ERROR_GE_PARAM_INVALID);
  delete[](uint8_t *) data.model_data;
}

TEST_F(UtestModelManagerModelManager, case_load_model_data_success) {
  ModelData data;
  LoadStandardModelData(data);
  ModelFileHeader &header = *static_cast<ModelFileHeader *>(data.model_data);
  EXPECT_EQ(header.model_num, 1U);

  {
    ModelManager mm;
    uint32_t model_id = std::numeric_limits<uint32_t>::max();
    EXPECT_EQ(mm.LoadModelOffline(model_id, data), SUCCESS);
  }

  delete [] (uint8_t *)data.model_data;
}

TEST_F(UtestModelManagerModelManager, load_unknown_shape_model_data_success) {
  ModelData data;
  LoadStandardModelData(data, 1024U, 2U);
  ModelFileHeader &header = *static_cast<ModelFileHeader *>(data.model_data);
  EXPECT_EQ(header.model_num, 2U);

  {
    ModelManager mm;
    uint32_t model_id = std::numeric_limits<uint32_t>::max();
    EXPECT_EQ(mm.LoadModelOffline(model_id, data), SUCCESS);
  }

  delete [] (uint8_t *)data.model_data;
}

// test DataInputTensor
TEST_F(UtestModelManagerModelManager, test_data_input_tensor) {
  shared_ptr<ModelListener> g_label_call_back(nullptr);
  auto model = std::make_shared<DavinciModel>(0, g_label_call_back);
  ModelManager mm;
  uint32_t model_id = 1;
  mm.model_map_[1] = model;
  mm.hybrid_model_map_[1] = std::make_shared<hybrid::HybridDavinciModel>();

  ge::Tensor input_tensor;
  vector<ge::Tensor> inputs;
  inputs.emplace_back(input_tensor);
  auto ret = mm.DataInputTensor(model_id, inputs);
  EXPECT_EQ(PARAM_INVALID, ret);    // HybridDavinciModel::impl_ is null.
}

TEST_F(UtestModelManagerModelManager, test_launch_kernel_cust_aicpu) {
  ModelManager mm;

  // cust_aicpu_so_ is empty.
  EXPECT_EQ(mm.LaunchKernelCustAicpuSo("empty_cust_aicpu"), SUCCESS);

  // deleteCustOp after Launch will deleted.
  uintptr_t resource_id = 1;    // for rtCtxGetCurrent stub
  std::vector<char> kernel_bin(256);
  auto &cust_resource_001 = mm.cust_aicpu_so_[resource_id];
  auto tbe_kernel = std::shared_ptr<OpKernelBin>(new OpKernelBin("deleteCustOp", std::move(kernel_bin)));
  cust_resource_001["deleteCustOp"] = tbe_kernel;

  EXPECT_FALSE(mm.cust_aicpu_so_.empty());
  EXPECT_EQ(mm.LaunchKernelCustAicpuSo("deleteCustOp"), SUCCESS);
  EXPECT_TRUE(mm.cust_aicpu_so_.empty());
}

shared_ptr<ModelListener> listerner(new DModelListener());
TEST_F(UtestModelManagerModelManager, test_load_model_online) {
  ModelManager mm;
  uint32_t model_id = 1;
  uint32_t device_id = 0;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeRootModelPtr ge_root_model = make_shared<GeRootModel>(graph);
  GraphNodePtr graph_node = MakeShared<GraphNode>(0);

  ge::ProfilingManager::Instance().SetSubscribeInfo(0, model_id, true);
  EXPECT_EQ(mm.LoadModelOnline(model_id, ge_root_model, graph_node, device_id, 0), PARAM_INVALID);  // GeModel is null
  ge::ProfilingManager::Instance().CleanSubscribeInfo();
}

TEST_F(UtestModelManagerModelManager, command_profiling) {
  ModelManager manager;
  uint32_t model_id = 1;
  Command cmd;
  auto model = std::make_shared<DavinciModel>(1, listerner);
  model->SetId(model_id);
  cmd.cmd_params.push_back("modelId");
  cmd.cmd_params.push_back(to_string(model_id));

  ge::ProfilingManager::Instance().SetSubscribeInfo(0, model_id, true);
  EXPECT_EQ(manager.HandleProfModelUnsubscribeCommand(cmd), FAILED);
  ge::ProfilingManager::Instance().CleanSubscribeInfo();
}

TEST_F(UtestModelManagerModelManager, command_profiling_get_hybrid_model) {
  uint32_t model_id = 999;
  Command cmd;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeRootModelPtr ge_root_model = make_shared<GeRootModel>(graph);
  auto hybrid_model_ptr = ge::hybrid::HybridDavinciModel::Create(ge_root_model);
  auto shared_model = std::shared_ptr<hybrid::HybridDavinciModel>(hybrid_model_ptr.release());
  shared_model->SetDeviceId(0);
  cmd.cmd_params.push_back("modelId");
  cmd.cmd_params.push_back(to_string(model_id));
  EXPECT_EQ(ModelManager::GetInstance().HandleProfModelSubscribeCommand(cmd), FAILED);
  ModelManager::GetInstance().InsertModel(model_id, shared_model);
  EXPECT_EQ(ModelManager::GetInstance().HandleProfModelSubscribeCommand(cmd), SUCCESS);
  EXPECT_EQ(ModelManager::GetInstance().HandleProfModelUnsubscribeCommand(cmd), SUCCESS);
  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(model_id), SUCCESS);
}

TEST_F(UtestModelManagerModelManager, test_get_aipp_info_and_type) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeRootModelPtr ge_root_model = make_shared<GeRootModel>(graph);
  auto hybrid_model_ptr = ge::hybrid::HybridDavinciModel::Create(ge_root_model);
  auto shared_model = std::shared_ptr<hybrid::HybridDavinciModel>(hybrid_model_ptr.release());
  ModelManager model_manager;
  const uint32_t model_id = 1;
  model_manager.hybrid_model_map_[model_id] = shared_model;

  uint32_t index = 0;
  AippConfigInfo aipp_info;
  auto ret = model_manager.GetAippInfo(model_id, index, aipp_info);
  EXPECT_EQ(ret, ACL_ERROR_GE_AIPP_NOT_EXIST);

  InputAippType aipp_type;
  size_t aipp_index;
  ret = model_manager.GetAippType(model_id, index, aipp_type, aipp_index);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestModelManagerModelManager, test_update_task_param) {
  ZeroCopyTask zero_copy_task("task", 0, 0);

  uintptr_t addr = 0;
  set<size_t> offset_set{0, 1, 2};
  map<uintptr_t, set<size_t>> task_addr_offset;
  task_addr_offset[0] = offset_set;
  uintptr_t buffer_addr = 0;
  vector<uint8_t> args_info = {0, 1, 2};

  zero_copy_task.task_addr_offset_ = task_addr_offset;
  zero_copy_task.args_info_ = args_info;
  EXPECT_EQ(zero_copy_task.UpdateTaskParam(addr, buffer_addr), SUCCESS);
}

TEST_F(UtestModelManagerModelManager, test_execute_model) {
  auto davinci_model = std::make_shared<DavinciModel>(0, nullptr);
  ModelManager mm;
  const uint32_t model_id = 1;
  bool async_mode = true;
  rtStream_t stream = nullptr;
  vector<GeTensor> input_tensor;
  vector<GeTensor> output_tensor;
  davinci_model->need_destroy_aicpu_kernel_ = true;
  davinci_model->session_id_ = 1;
  davinci_model->model_id_ = 1;
  davinci_model->sub_model_id_ = 1;

  mm.model_map_[model_id] = davinci_model;
  EXPECT_TRUE(mm.GetModel(model_id) != nullptr);

  mm.ExecuteModel(model_id, stream, async_mode, input_tensor, output_tensor);
}

TEST_F(UtestModelManagerModelManager, test_execute_model1) {
  auto hybrid_model = std::make_shared<hybrid::HybridDavinciModel>();
  ModelManager mm;
  const uint32_t model_id = 1;
  bool async_mode = true;
  rtStream_t stream = nullptr;
  vector<GeTensor> input_tensor;
  vector<GeTensor> output_tensor;

  mm.hybrid_model_map_[model_id] = hybrid_model;
  EXPECT_TRUE(mm.GetHybridModel(model_id) != nullptr);

  mm.ExecuteModel(model_id, stream, async_mode, input_tensor, output_tensor);
}

TEST_F(UtestModelManagerModelManager, TestLoadHeterogeneousModel) {
  ExecutionRuntime::SetExecutionRuntime(make_shared<MockExecutionRuntime>());
  auto root_model = std::make_shared<GeRootModel>();
  root_model->model_relation_.reset(new ModelRelation());
  root_model->submodels_.emplace("subgraph-1", make_shared<GeRootModel>());
  auto flow_root_model = ge::MakeShared<ge::FlowModel>();
  EXPECT_NE(flow_root_model, nullptr);
  flow_root_model->root_graph_ = std::make_shared<ComputeGraph>("root-graph");
  flow_root_model->SetModelRelation(root_model->model_relation_);
  (void)flow_root_model->AddSubModel(root_model);

  ModelManager model_manager;
  GraphNodePtr graph_node = MakeShared<GraphNode>(0);

  uint32_t model_id = 666;
  uint32_t device_id = 0;
  ASSERT_FALSE(model_manager.UnloadHeterogeneousModel(model_id));
  ASSERT_EQ(model_manager.LoadFlowModelOnline(model_id, flow_root_model, graph_node), SUCCESS);
  ASSERT_EQ(model_id, model_id);
  ASSERT_TRUE(model_manager.IsHeterogeneous(model_id));
  ASSERT_TRUE(model_manager.GetHeterogeneousModelExecutor(model_id) != nullptr);
  ASSERT_EQ(model_manager.Start(model_id), SUCCESS);
  ASSERT_EQ(model_manager.Stop(model_id), SUCCESS);

  ASSERT_EQ(model_manager.Unload(model_id), SUCCESS);
  ASSERT_FALSE(model_manager.IsHeterogeneous(model_id));
  ASSERT_TRUE(model_manager.GetHeterogeneousModelExecutor(model_id) == nullptr);
  ExecutionRuntime::SetExecutionRuntime(nullptr);
}

TEST_F(UtestModelManagerModelManager, TestUnloadHeterogeneousModel) {
  ExecutionRuntime::SetExecutionRuntime(nullptr);
  ModelManager model_manager;
  model_manager.heterogeneous_model_map_[666] =
      make_shared<HeterogeneousModelExecutor>(std::make_shared<GeRootModel>(), DeployResult{});
  uint32_t model_id = 666;
  ASSERT_FALSE(model_manager.UnloadHeterogeneousModel(model_id));  // execution runtime not set
  ASSERT_TRUE(model_manager.IsHeterogeneous(model_id));

  ExecutionRuntime::SetExecutionRuntime(make_shared<MockExecutionRuntime>());
  auto &model_deploy = (MockModelDeployer &) ExecutionRuntime::GetInstance()->GetModelDeployer();
  EXPECT_CALL(model_deploy, Undeploy).Times(1).WillOnce(testing::Return(SUCCESS));
  ASSERT_TRUE(model_manager.UnloadHeterogeneousModel(model_id));
  ASSERT_FALSE(model_manager.IsHeterogeneous(model_id));
  ASSERT_TRUE(model_manager.GetHeterogeneousModelExecutor(model_id) == nullptr);
  ExecutionRuntime::SetExecutionRuntime(nullptr);
}

TEST_F(UtestModelManagerModelManager, TestExecuteHeterogeneousModel_InvalidModelId) {
  ModelManager model_manager;
  std::vector<GeTensor> output_tensors;
  ASSERT_EQ(model_manager.ExecuteHeterogeneousModel(555, {}, output_tensors), PARAM_INVALID);
}

TEST_F(UtestModelManagerModelManager, TestLoadModelWithQueueFromDataFailed) {
  uint32_t model_id = 1;
  ModelManager model_manager;
  ModelData data;
  LoadStandardModelData(data);
  std::unique_ptr<uint8_t[]> auto_delete(reinterpret_cast<uint8_t *>(data.model_data));

  // failed: input & output queue are all empty
  ASSERT_EQ(model_manager.LoadModelWithQ(model_id, data, {}, {}), ACL_ERROR_GE_EXEC_MODEL_QUEUE_ID_INVALID);
  ASSERT_EQ(model_manager.model_map_.count(model_id), 0);
}

TEST_F(UtestModelManagerModelManager, TestLoadModelWithQueueFromGeRootModelFailed) {
  auto root_graph = std::make_shared<ComputeGraph>("root-graph");
  auto root_model = std::make_shared<GeRootModel>(root_graph);
  uint32_t model_id = 1;
  ModelManager model_manager;
  // Failed to get GeModel
  ASSERT_EQ(model_manager.LoadModelWithQ(model_id, root_model, {}, {}), INTERNAL_ERROR);

  // GeModel is null
  root_model->SetSubgraphInstanceNameToModel(root_graph->GetName(), nullptr);
  ASSERT_EQ(model_manager.LoadModelWithQ(model_id, root_model, {}, {}), PARAM_INVALID);  // GeModel is null

  root_model->subgraph_instance_name_to_model_[root_graph->GetName()] = std::make_shared<GeModel>();
  // incorrect queue size
  ASSERT_EQ(model_manager.LoadModelWithQ(model_id, root_model, {}, {}), ACL_ERROR_GE_EXEC_MODEL_QUEUE_ID_INVALID);
  ASSERT_EQ(model_manager.model_map_.count(model_id), 0);

  // init davinci model failed, model's graph is nullptr
  ASSERT_EQ(model_manager.LoadModelWithQ(model_id, root_model, {}, {1}), INTERNAL_ERROR);
}

TEST_F(UtestModelManagerModelManager, Cal_follow_stream_sum) {
  std::multimap<int64_t, uint64_t> hccl_stream_map = {{1, 10}, {1, 20}, {2, 10}, {2, 5}};
  uint64_t result = ModelUtils::CalFollowStreamSum(hccl_stream_map);
  EXPECT_EQ(result, 30);
}

TEST_F(UtestModelManagerModelManager, get_max_stream_and_event) {
  ModelManager mm;
  auto model1 = std::make_shared<DavinciModel>(1, nullptr);
  auto model2 = std::make_shared<DavinciModel>(2, nullptr);
  rtStream_t stream = nullptr;
  rtStream_t stream2 = nullptr;
  rtStream_t stream3 = nullptr;
  rtStream_t stream4 = nullptr;
  rtEvent_t event = nullptr;
  rtEvent_t event2 = nullptr;
  rtEvent_t event3 = nullptr;
  model1->stream_list_ = {stream, stream2, stream3, stream4};
  model1->event_list_ = {event, event2};
  model2->stream_list_ = {stream, stream2};
  model2->event_list_ = {event, event2, event3};

  mm.InsertModel(1, model1);
  mm.InsertModel(2, model2);
  uint32_t max_stream_model;
  uint32_t max_event_model;
  mm.GetMaxStreamAndEventModel(max_stream_model, max_event_model);
  EXPECT_EQ(max_stream_model, 1);
  EXPECT_EQ(max_event_model, 2);

  uint64_t free_stream;
  Status ret = mm.GetFreeStream(free_stream);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestModelManagerModelManager, release_resource_stream) {
  ModelManager mm;
  auto model1 = std::make_shared<DavinciModel>(1, nullptr);
  auto model2 = std::make_shared<DavinciModel>(2, nullptr);
  rtStream_t stream = nullptr;
  rtStream_t stream2 = nullptr;
  rtStream_t stream3 = nullptr;
  rtStream_t stream4 = nullptr;
  rtEvent_t event = nullptr;
  rtEvent_t event2 = nullptr;
  rtEvent_t event3 = nullptr;
  model1->stream_list_ = {stream, stream2, stream3, stream4};
  model1->event_list_ = {event, event2};
  model2->stream_list_ = {stream, stream2};
  model2->event_list_ = {event, event2, event3};

  mm.InsertModel(1, model1);
  mm.InsertModel(2, model2);
  string kind = "stream";
  Status ret = mm.ReleaseResource(110, 109, kind);
  EXPECT_EQ(ret, SUCCESS);

  string kind2 = "event";
  Status ret2 = mm.ReleaseResource(110, 109, kind2);
  EXPECT_EQ(ret2, SUCCESS);
}

TEST_F(UtestModelManagerModelManager, check_stream_and_event_resource) {
  ModelManager mm;
  auto model1 = std::make_shared<DavinciModel>(1, nullptr);
  auto model2 = std::make_shared<DavinciModel>(2, nullptr);
  rtStream_t stream = nullptr;
  rtStream_t stream2 = nullptr;
  rtStream_t stream3 = nullptr;
  rtStream_t stream4 = nullptr;
  rtEvent_t event = nullptr;
  rtEvent_t event2 = nullptr;
  rtEvent_t event3 = nullptr;
  model1->stream_list_ = {stream, stream2, stream3, stream4};
  model1->event_list_ = {event, event2};
  model2->stream_list_ = {stream, stream2};
  model2->event_list_ = {event, event2, event3};

  mm.InsertModel(1, model1);
  mm.InsertModel(2, model2);
  // stream used: 6, event used: 5
  // free stream: 1024-6=1018, free event: 1024-5=1019

  auto ge_model = make_shared<GeModel>();
  Status ret = mm.CheckAndReleaseStreamEventResource(ge_model, 1);
  EXPECT_EQ(ret, FAILED);

  ComputeGraphPtr graph = make_shared<ComputeGraph>("default");
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1009);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_EVENT_NUM, 1023);

  shared_ptr<domi::ModelTaskDef> model_task_def = make_shared<domi::ModelTaskDef>();
  ge_model->SetModelTaskDef(model_task_def);

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  {
    OpDescPtr op_desc = CreateOpDesc("data", DATA);
    op_desc->AddInputDesc(tensor);
    op_desc->AddOutputDesc(tensor);
    op_desc->SetInputOffset({1024});
    op_desc->SetOutputOffset({1024});
    NodePtr node = graph->AddNode(op_desc);    // op_index = 0
  }

  {
    OpDescPtr op_desc = CreateOpDesc("square", "Square");
    op_desc->AddInputDesc(tensor);
    op_desc->AddOutputDesc(tensor);
    op_desc->SetInputOffset({1024});
    op_desc->SetOutputOffset({1024});
    NodePtr node = graph->AddNode(op_desc);  // op_index = 1

    domi::TaskDef *task_def = model_task_def->add_task();
    task_def->set_stream_id(0);
    task_def->set_type(RT_MODEL_TASK_KERNEL);
    domi::KernelDef *kernel_def = task_def->mutable_kernel();
    kernel_def->set_stub_func("stub_func");
    kernel_def->set_args_size(64);
    string args(64, '1');
    kernel_def->set_args(args.data(), 64);
    domi::KernelContext *context = kernel_def->mutable_context();
    context->set_op_index(op_desc->GetId());
    context->set_kernel_type(2);    // ccKernelType::TE
    uint16_t args_offset[9] = {0};
    context->set_args_offset(args_offset, 9 * sizeof(uint16_t));
  }

  {
    OpDescPtr op_desc = CreateOpDesc("memcpy", MEMCPYASYNC);
    op_desc->AddInputDesc(tensor);
    op_desc->AddOutputDesc(tensor);
    op_desc->SetInputOffset({1024});
    op_desc->SetOutputOffset({5120});
    NodePtr node = graph->AddNode(op_desc);  // op_index = 2

    domi::TaskDef *task_def = model_task_def->add_task();
    task_def->set_stream_id(0);
    task_def->set_type(RT_MODEL_TASK_MEMCPY_ASYNC);
    domi::MemcpyAsyncDef *memcpy_async = task_def->mutable_memcpy_async();
    memcpy_async->set_src(1024);
    memcpy_async->set_dst(5120);
    memcpy_async->set_dst_max(512);
    memcpy_async->set_count(1);
    memcpy_async->set_kind(RT_MEMCPY_DEVICE_TO_DEVICE);
    memcpy_async->set_op_index(op_desc->GetId());
  }

  {
    OpDescPtr op_desc = CreateOpDesc("output", NETOUTPUT);
    op_desc->AddInputDesc(tensor);
    op_desc->SetInputOffset({5120});
    op_desc->SetSrcName({"memcpy"});
    op_desc->SetSrcIndex({0});
    AttrUtils::SetInt(op_desc, "used_stream_num", 10);
    NodePtr node = graph->AddNode(op_desc);  // op_index = 3

    domi::TaskDef *task_def = model_task_def->add_task();
    task_def->set_stream_id(0);
    task_def->set_type(RT_MODEL_TASK_HCCL);
    task_def->mutable_kernel_hccl()->set_op_index(op_desc->GetId());
  }
  ret = mm.CheckAndReleaseStreamEventResource(ge_model, 1);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestModelManagerModelManager, record_ts_snapshot_success) {
  const std::string kTriggerFile = "exec_record_trigger";
  const char_t * const kEnvRecordPath = "NPU_COLLECT_PATH";
  const std::string kRecordFilePrefix = "exec_record_";

  // 设置环境变量
  char_t npu_collect_path[MMPA_MAX_PATH] = {};
  mmRealPath(".", &npu_collect_path[0U], MMPA_MAX_PATH);
  mmSetEnv(kEnvRecordPath, &npu_collect_path[0U], 1);

  // 创建trigger文件
  const std::string trigger_file = std::string(&npu_collect_path[0U]) + "/" + kTriggerFile;
  auto trigger_fd = mmOpen(trigger_file.c_str(), M_WRONLY | M_CREAT);
  mmClose(trigger_fd);

  ModelManager *mm = new ModelManager();
  mmSleep(1000U);

  std::string record_file = std::string(&npu_collect_path[0U]) + "/" + kRecordFilePrefix + std::to_string(mmGetPid());
  const auto record_fd = mmOpen(record_file.c_str(), M_RDONLY);
  EXPECT_TRUE(record_fd >= 0);
  mmClose(record_fd);

  delete mm;
  trigger_fd = mmOpen(trigger_file.c_str(), M_RDONLY);
  EXPECT_TRUE(trigger_fd < 0);

  // 清理环境变量
  mmSetEnv(kEnvRecordPath, "", 1);
  // 清理record file
  mmUnlink(record_file.c_str());
}

TEST_F(UtestModelManagerModelManager, record_ts_snapshot_fail) {
  const std::string kTriggerFile = "exec_record_trigger";
  const char_t * const kEnvRecordPath = "NPU_COLLECT_PATH";
  const std::string kRecordFilePrefix = "exec_record_";

  // 设置环境变量
  char_t npu_collect_path[MMPA_MAX_PATH] = {};
  mmRealPath(".", &npu_collect_path[0U], MMPA_MAX_PATH);
  const std::string fail_collect_path = (std::string(&npu_collect_path[0U]) + "/mock_fail");
  mmSetEnv(kEnvRecordPath, fail_collect_path.c_str(), 1);

  // 创建trigger文件
  const std::string trigger_file = fail_collect_path + "/" + kTriggerFile;
  auto trigger_fd = mmOpen(trigger_file.c_str(), M_WRONLY | M_CREAT);
  mmClose(trigger_fd);

  ModelManager *mm = new ModelManager();
  mmSleep(1000U);

  std::string record_file = fail_collect_path + "/" + kRecordFilePrefix + std::to_string(mmGetPid());
  const auto record_fd = mmOpen(record_file.c_str(), M_RDONLY);
  EXPECT_TRUE(record_fd < 0);
  mmClose(record_fd);
  delete mm;
  trigger_fd = mmOpen(trigger_file.c_str(), M_RDONLY);
  EXPECT_TRUE(trigger_fd < 0);

  // 清理环境变量
  mmSetEnv(kEnvRecordPath, "", 1);
}

TEST_F(UtestModelManagerModelManager, DestroyAicpuSessionForInfer) {
  ModelManager mm;
  uint32_t model_id = 0;
  auto ret = mm.DestroyAicpuSessionForInfer(model_id);
  EXPECT_EQ(ret, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);
}

TEST_F(UtestModelManagerModelManager, SetDynamicSize) {
  ModelManager mm;
  uint32_t model_id = 0;
  std::vector<uint64_t> batch_num;
  int32_t dynamic_type = static_cast<int32_t>(FIXED);
  auto ret = mm.SetDynamicSize(model_id, batch_num, dynamic_type);
  EXPECT_EQ(ret, PARAM_INVALID);
}

TEST_F(UtestModelManagerModelManager, DoLoadHybridModelOnline) {
  ModelManager mm;
  uint32_t model_id = 0;
  std::string om_name = "om_name";
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeRootModelPtr ge_root_model = make_shared<GeRootModel>(graph);
  ge_root_model->SetRootGraph(graph);
  hybrid::HybridModel model(ge_root_model);
  model.root_graph_ = graph;

  auto ret = mm.DoLoadHybridModelOnline(model_id, om_name, ge_root_model, listerner);
  EXPECT_EQ(ret, INTERNAL_ERROR);
}

TEST_F(UtestModelManagerModelManager, HandleCommand_Invalid) {
  ModelManager mm;
  uint32_t model_id = 0;
  Command cmd;
  cmd.cmd_params.push_back("modelId");
  cmd.cmd_params.push_back(to_string(model_id));

  auto ret = mm.HandleProfModelSubscribeCommand(cmd);
  EXPECT_EQ(ret, FAILED);

  ret = mm.HandleProfFinalizeCommand(cmd);
  EXPECT_EQ(ret, SUCCESS);

  ret = mm.HandleProfStartCommand(cmd);
  EXPECT_EQ(ret, SUCCESS);

  ret = mm.HandleProfStopCommand(cmd);
  EXPECT_EQ(ret, SUCCESS);

  ret = mm.HandleDumpCommand(cmd);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestModelManagerModelManager, GetSomething_Invalid) {
  ModelManager mm;
  uint32_t model_id = 0;

  std::vector<InputOutputDescInfo> input_desc;
  std::vector<InputOutputDescInfo> output_desc;
  std::vector<uint32_t> inputFormats;
  std::vector<uint32_t> outputFormats;
  bool new_model_desc = false;
  auto ret = mm.GetInputOutputDescInfo(model_id, input_desc, output_desc, inputFormats, outputFormats, new_model_desc);
  EXPECT_EQ(ret, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);

  std::vector<std::vector<int64_t>> batch_info;
  int32_t dynamic_type = static_cast<int32_t>(FIXED);
  ret = mm.GetDynamicBatchInfo(model_id, batch_info, dynamic_type);
  EXPECT_EQ(ret, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);

  ret = mm.GetCombinedDynamicDims(model_id, batch_info);
  EXPECT_EQ(ret, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);

  std::vector<std::string> user_input_shape_order;
  ret = mm.GetUserDesignateShapeOrder(model_id, user_input_shape_order);
  EXPECT_EQ(ret, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);

  std::vector<int64_t> batch_info2;
  ret = mm.GetCurrentShape(model_id, batch_info2, dynamic_type);
  EXPECT_EQ(ret, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);

  std::vector<std::string> dynamic_output_shape_info;
  ret = mm.GetOutputShapeInfo(model_id, dynamic_output_shape_info);
  EXPECT_EQ(ret, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);
}

TEST_F(UtestModelManagerModelManager, test_execute_model2) {
  auto hybrid_model = std::make_shared<hybrid::HybridDavinciModel>();
  ModelManager mm;
  const uint32_t model_id = 1;
  rtStream_t stream = nullptr;
  bool async_mode = true;
  InputData input_data;
  OutputData output_data;
  std::vector<GeTensorDesc> input_desc;
  std::vector<GeTensorDesc> output_desc;

  mm.hybrid_model_map_[model_id] = hybrid_model;
  EXPECT_TRUE(mm.GetHybridModel(model_id) != nullptr);

  mm.ExecuteModel(model_id, stream, async_mode, input_data, input_desc, output_data, output_desc);
}

TEST_F(UtestModelManagerModelManager, LoadClearAicpuSo_Invalid) {
  ModelManager mm;
  uint32_t model_id = 0;
  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);
  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  op_desc->AddInputDesc(tensor);
  op_desc->AddOutputDesc(tensor);
  op_desc->SetInputOffset({1024});
  op_desc->SetOutputOffset({1024});

  std::string so_name = "abc.so";
  bool loaded = false;

  auto ret = mm.LoadCustAicpuSo(op_desc, so_name, loaded);
  EXPECT_EQ(ret, SUCCESS);

  ret = mm.ClearAicpuSo();
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestModelManagerModelManager, testGetModelMemAndWeightSize) {
  ModelManager mm;
  ModelData data;
  LoadStandardModelData(data, 1024U, 2U);
  size_t mem_size;
  size_t weight_size;

  auto ret = mm.GetModelMemAndWeightSize(data, mem_size, weight_size);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestModelManagerModelManager, testGetOrigInputInfo) {
  ModelManager mm;
  const uint32_t model_id = 1;
  uint32_t index = 0;
  OriginInputInfo orig_input_info;

  auto ret = mm.GetOrigInputInfo(model_id, index, orig_input_info);
  EXPECT_EQ(ret, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);
}

TEST_F(UtestModelManagerModelManager, testGetOpDescInfo) {
  ModelManager mm;
  uint32_t device_id = 1;
  uint32_t stream_id = 1;
  uint32_t task_id = 1;
  OpDescInfo op_desc_info;

  auto ret = mm.GetOpDescInfo(device_id, stream_id, task_id, op_desc_info);
  EXPECT_EQ(ret, FAILED);
}

TEST_F(UtestModelManagerModelManager, KernelLaunchEx_Invalid) {
  ModelManager mm;
  aicpu::FWKAdapter::FWKOperateType op_type = aicpu::FWKAdapter::FWKOperateType::FWK_ADPT_KERNEL_DESTROY;
  uint64_t session_id = 0;
  uint32_t model_id = 0;
  uint32_t sub_model_id = 0;
  std::vector<uint64_t> aicpu_kernel = {1, 2, 3};
  mm.model_aicpu_kernel_.insert({string("0_0_0"), aicpu_kernel});

  auto ret = mm.KernelLaunchEx(op_type, session_id, model_id, sub_model_id);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestModelManagerModelManager, DestroyAicpuKernel_Invalid) {
  ModelManager mm;
  uint64_t session_id = 0;
  uint32_t model_id = 0;
  uint32_t sub_model_id = 0;
  std::vector<uint64_t> aicpu_kernel = {1, 2, 3};
  mm.model_aicpu_kernel_.insert({string("0_0_0"), aicpu_kernel});

  auto ret = mm.DestroyAicpuKernel(session_id, model_id, sub_model_id);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestModelManagerModelManager, ReleaseResource_Invalid) {
  ModelManager mm;
  uint64_t need_resource = 1;
  uint64_t free_resource = 0;
  std::string resource_kind = "stream";

  auto ret = mm.ReleaseResource(need_resource, free_resource, resource_kind);
  EXPECT_EQ(ret, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);

  resource_kind = string("event");
  ret = mm.ReleaseResource(need_resource, free_resource, resource_kind);
  EXPECT_EQ(ret, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);
}

TEST_F(UtestModelManagerModelManager, GetModelByCmd_Invalid) {
  uint32_t model_id = std::numeric_limits<uint32_t>::max();
  Command command;
  EXPECT_EQ(ModelManager::GetModelIdByCmd(command, model_id), PARAM_INVALID);
  EXPECT_EQ(model_id, std::numeric_limits<uint32_t>::max());

  command.cmd_params.push_back(string("not model Id"));
  EXPECT_EQ(ModelManager::GetModelIdByCmd(command, model_id), PARAM_INVALID);
  EXPECT_EQ(model_id, std::numeric_limits<uint32_t>::max());
}

TEST_F(UtestModelManagerModelManager, testParserPara) {
  ModelManager mm;
  Command command;
  command.cmd_params.push_back(ge::DUMP_STATUS);
  command.cmd_params.push_back(string("value"));

  auto ret = mm.HandleDumpCommand(command);
  EXPECT_EQ(ret, SUCCESS);
 }

TEST_F(UtestModelManagerModelManager, HandleProfStartCommand_Invalid) {
  ModelManager mm;
  ModelData data;
  LoadStandardModelData(data);
  uint32_t model_id = std::numeric_limits<uint32_t>::max();
  mm.LoadModelOffline(model_id, data);

  Command cmd;
  unsetenv("GE_PROFILING_TO_STD_OUT");

  auto ret = mm.HandleProfStartCommand(cmd);
  EXPECT_EQ(ret, PARAM_INVALID);
  ret = mm.HandleProfStopCommand(cmd);
  EXPECT_EQ(ret, PARAM_INVALID);

  cmd.cmd_params.push_back(string("modelId"));
  cmd.cmd_params.push_back(to_string(model_id));
  cmd.module_index = 0;
  ret = mm.HandleProfStartCommand(cmd);
  EXPECT_EQ(ret, FAILED);
  ret = mm.HandleProfStopCommand(cmd);
  EXPECT_EQ(ret, FAILED);

  for(uint32_t i = 0; i < 1001; i++) {
    cmd.cmd_params.push_back(string("m"));
    cmd.cmd_params.push_back(to_string(model_id));
  }
  ret = mm.HandleProfStartCommand(cmd);
  EXPECT_EQ(ret, PARAM_INVALID);
  ret = mm.HandleProfStopCommand(cmd);
  EXPECT_EQ(ret, PARAM_INVALID);

  EXPECT_EQ(mm.DeleteModel(model_id), SUCCESS);
  setenv("GE_PROFILING_TO_STD_OUT", "1", 1);
}

TEST_F(UtestModelManagerModelManager, testHeadFile) {
  ModelManager mm;
  std::vector<rtExceptionInfo> ex;
  ex = mm.GetExceptionInfos();
  EXPECT_EQ(ex.size(), 0);

  rtExceptionInfo rt_exception_info;
  mm.AddExceptionInfo(rt_exception_info);
  ex = mm.GetExceptionInfos();
  EXPECT_EQ(ex.size(), 1);

  ModelManager::ExceptionCallback(&rt_exception_info);

  bool status = true;
  mm.SetSocketCloseStatus(status);
  EXPECT_TRUE(mm.is_socket_close_);
}

TEST_F(UtestModelManagerModelManager, TestLoadModelWithoutQ) {
  auto root_graph = std::make_shared<ComputeGraph>("root-graph");
  auto root_model = std::make_shared<GeRootModel>(root_graph);
  uint32_t model_id = 1;
  ModelManager model_manager;
  // Failed to get GeModel
  AttrUtils::SetBool(root_graph, ATTR_NAME_DYNAMIC_SHAPE_PARTITIONED, true);
  ASSERT_EQ(model_manager.LoadModelWithoutQ(model_id, root_model, 0), UNSUPPORTED);
  AttrUtils::SetBool(root_graph, ATTR_NAME_DYNAMIC_SHAPE_PARTITIONED, false);


  ASSERT_EQ(model_manager.LoadModelWithoutQ(model_id, root_model, 0), INTERNAL_ERROR);

  // GeModel is null
  root_model->SetSubgraphInstanceNameToModel(root_graph->GetName(), nullptr);
  ASSERT_EQ(model_manager.LoadModelWithoutQ(model_id, root_model, 0), PARAM_INVALID);  // GeModel is null
  root_model->subgraph_instance_name_to_model_[root_graph->GetName()] = std::make_shared<GeModel>();

  // init davinci model failed, model's graph is nullptr
  ASSERT_EQ(model_manager.LoadModelWithoutQ(model_id, root_model, 0), INTERNAL_ERROR);
}

TEST_F(UtestModelManagerModelManager, testEnableExceptionDump) {
  const char_t * const kEnvRecordPath = "NPU_COLLECT_PATH";
  char_t npu_collect_path[MMPA_MAX_PATH] = "valid_path";
  mmSetEnv(kEnvRecordPath, &npu_collect_path[0U], MMPA_MAX_PATH);

  ModelManager mm;
  std::map<std::string, std::string> run_options;
  Status ret = mm.EnableExceptionDump(run_options);
  EXPECT_EQ(ret, 0);
  unsetenv(kEnvRecordPath);
}

}  // namespace ge
