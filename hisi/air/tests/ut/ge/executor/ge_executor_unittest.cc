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
#include <memory>

#include "common/ge_inner_error_codes.h"
#include "common/types.h"
#include "common/util.h"
#include "runtime/mem.h"
#include "common/util.h"
#include "omg/omg_inner_types.h"
#include "ge_graph_dsl/graph_dsl.h"

#define private public
#define protected public
#include "executor/ge_executor.h"
#include "single_op/stream_resource.h"

#include "common/auth/file_saver.h"
#include "common/debug/log.h"
#include "common/properties_manager.h"
#include "common/types.h"
#include "graph/load/graph_loader.h"
#include "graph/load/model_manager/davinci_model.h"
#include "hybrid/hybrid_davinci_model.h"
#include "graph/load/model_manager/model_manager.h"
#include "graph/load/model_manager/task_info/kernel_task_info.h"
#include "graph/load/model_manager/task_info/kernel_ex_task_info.h"
#include "graph/execute/graph_execute.h"
#include "common/dump/dump_properties.h"
#include "graph/manager/graph_mem_allocator.h"
#include "graph/utils/graph_utils.h"
#include "proto/ge_ir.pb.h"
#include "graph/manager/graph_var_manager.h"
#include "ut/ge/ffts_plus_proto_tools.h"

using namespace std;
using namespace testing;

namespace ge {
class UtestGeExecutor : public testing::Test {
 protected:
  void SetUp() {
    unsetenv("FMK_SYSMODE");
    unsetenv("FMK_DUMP_PATH");
    unsetenv("FMK_USE_FUSION");
    unsetenv("DAVINCI_TIMESTAT_ENABLE");
  }

  void TearDown() {
    EXPECT_TRUE(ModelManager::GetInstance().model_map_.empty());
    EXPECT_TRUE(ModelManager::GetInstance().hybrid_model_map_.empty());
  }
};

class DModelListener : public ge::ModelListener {
 public:
  DModelListener() {
  };
  Status OnComputeDone(uint32_t model_id, uint32_t data_index, uint32_t resultCode,
                       std::vector<ge::Tensor> &outputs) {
    GELOGI("In Call back. OnComputeDone");
    return SUCCESS;
  }
};

namespace {
class MockHybridModelExecutor : public ge::hybrid::HybridDavinciModel {
 public:
  Status Execute(const vector<DataBuffer> &inputs,
                 const vector<GeTensorDesc> &input_desc,
                 vector<DataBuffer> &outputs,
                 vector<GeTensorDesc> &output_desc,
                 const rtStream_t stream) override {
    return SUCCESS;
  }
};
}  // namespace

TEST_F(UtestGeExecutor, finalize) {
  GeExecutor ge_executor;
  Status retStatus;

  GeExecutor::is_inited_ = false;
  retStatus = ge_executor.Finalize();
  EXPECT_EQ(retStatus, SUCCESS);

  GeExecutor::is_inited_ = true;
  ProfilingProperties::Instance().SetLoadProfiling(true);
  retStatus = ge_executor.Finalize();
  EXPECT_EQ(retStatus, SUCCESS);
}

shared_ptr<ge::ModelListener> g_label_call_back(new DModelListener());

TEST_F(UtestGeExecutor, test_single_op_exec) {
  GeExecutor exeutor;
  ModelData model_data;
  string model_name = "1234";

  EXPECT_EQ(exeutor.LoadSingleOp(model_name, model_data, nullptr, nullptr), ACL_ERROR_GE_INTERNAL_ERROR);
  EXPECT_EQ(exeutor.LoadDynamicSingleOp(model_name, model_data, nullptr, nullptr), PARAM_INVALID);
}

TEST_F(UtestGeExecutor, test_ge_initialize) {
  GeExecutor executor;
  EXPECT_EQ(executor.Initialize(), SUCCESS);
  EXPECT_EQ(executor.Initialize(), SUCCESS);
}

TEST_F(UtestGeExecutor, load_data_from_file) {
  GeExecutor ge_executor;
  ModelData model_data;
  Status retStatus;
  string test_smap = "/tmp/" + std::to_string(getpid()) + "_maps";

  ge_executor.is_inited_ = false;
  retStatus = ge_executor.LoadDataFromFile(test_smap, model_data);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.LoadDataFromFile(std::string(""), model_data);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_MODEL_PATH_INVALID);
  EXPECT_EQ(model_data.model_data, nullptr);

  string self_smap = "/proc/" + std::to_string(getpid()) + "/maps";
  string copy_smap = "cp -f " + self_smap + " " + test_smap;
  EXPECT_EQ(system(copy_smap.c_str()), 0);
  retStatus = ge_executor.LoadDataFromFile(test_smap, model_data);
  EXPECT_EQ(retStatus, SUCCESS);
  EXPECT_NE(model_data.model_data, nullptr);

  delete[] static_cast<char *>(model_data.model_data);
  model_data.model_data = nullptr;
}

TEST_F(UtestGeExecutor, load_model_from_data) {
  GeExecutor ge_executor;
  ModelData model_data;
  Status retStatus;
  uint32_t model_id = 0;

  ge_executor.is_inited_ = false;
  retStatus = ge_executor.LoadModelFromData(model_id, model_data, nullptr, 0, nullptr, 0);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  ge_executor.is_inited_ = true;
  model_id = std::numeric_limits<uint32_t>::max();
  retStatus = ge_executor.LoadModelFromData(model_id, model_data, nullptr, 0, nullptr, 0);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_MODEL_DATA_SIZE_INVALID);
}

TEST_F(UtestGeExecutor, load_model_with_Q) {
  GeExecutor ge_executor;
  ModelData model_data;
  uint32_t model_id = 0;
  Status retStatus;
  std::vector<uint32_t> input_queue_ids = {0,1,2};
  std::vector<uint32_t> output_queue_ids = {3,4,5};

  ge_executor.is_inited_ = false;
  retStatus = ge_executor.LoadModelWithQ(model_id, model_data, input_queue_ids, output_queue_ids);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.LoadModelWithQ(model_id, model_data, input_queue_ids, output_queue_ids);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_MODEL_DATA_SIZE_INVALID);

  auto graph = make_shared<ComputeGraph>("graph");
  OpDescPtr op_desc = CreateOpDesc("Add", "Add");
  GeShape shape({2, 16});
  GeTensorDesc tensor_desc(shape);
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddOutputDesc(tensor_desc);
  auto node = graph->AddNode(op_desc);

  shared_ptr<GeRootModel> root_model = std::make_shared<GeRootModel>(graph);
  retStatus = ge_executor.LoadModelWithQ(model_id, root_model, input_queue_ids, output_queue_ids);
  EXPECT_EQ(retStatus, INTERNAL_ERROR);
}

TEST_F(UtestGeExecutor, load_model_without_Q) {
  uint32_t model_id = 0;
  auto graph = make_shared<ComputeGraph>("graph");
  OpDescPtr op_desc = CreateOpDesc("Add", "Add");
  GeShape shape({2, 16});
  GeTensorDesc tensor_desc(shape);
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddOutputDesc(tensor_desc);
  auto node = graph->AddNode(op_desc);

  shared_ptr<GeRootModel> root_model = std::make_shared<GeRootModel>(graph);
  GeExecutor ge_executor;
  ge_executor.is_inited_ = true;
  Status retStatus = ge_executor.LoadModelWithoutQ(model_id, root_model);
  EXPECT_EQ(retStatus, INTERNAL_ERROR);
}

TEST_F(UtestGeExecutor, exec_model) {
  GeExecutor ge_executor;
  ModelBufferData model_buffer;
  ModelData model_data{model_buffer.data.get(), static_cast<uint32_t>(model_buffer.length), 0, "", ""};
  uint32_t model_id = 0;
  Status retStatus;
  uint64_t *stream = nullptr;

  int64_t arg_0 = 127;
  int64_t arg_1 = 100;
  RunModelData input_data;
  input_data.blobs.emplace_back(DataBuffer{&arg_0, sizeof(arg_0), false, 0});
  input_data.blobs.emplace_back(DataBuffer{&arg_1, sizeof(arg_1), false, 0});

  int64_t arg_3 = 111;
  RunModelData output_data;
  output_data.blobs.emplace_back(DataBuffer{&arg_3, sizeof(arg_3), false, 0});

  ge_executor.is_inited_ = false;
  retStatus = ge_executor.ExecModel(model_id, stream, input_data, output_data, false);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.ExecModel(model_id, stream, input_data, output_data, false);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);

  input_data.dynamic_batch_size = 4;
  retStatus = ge_executor.ExecModel(model_id, stream, input_data, output_data, false);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);
}

TEST_F(UtestGeExecutor, exec_dynamic_model) {
  GeExecutor ge_executor;
  ModelBufferData model_buffer;
  ModelData model_data{model_buffer.data.get(), static_cast<uint32_t>(model_buffer.length), 0, "", ""};
  uint32_t model_id = 0;
  uint64_t *stream = nullptr;

  int64_t arg_0 = 127;
  int64_t arg_1 = 100;
  RunModelData input_data;
  input_data.blobs.emplace_back(DataBuffer{&arg_0, sizeof(arg_0), false, 0});
  input_data.blobs.emplace_back(DataBuffer{&arg_1, sizeof(arg_1), false, 0});

  RunModelData output_data;
  output_data.blobs.emplace_back(DataBuffer{nullptr, 0, false, 0});

  GeTensorDesc tensor_desc(GeShape(std::vector<int64_t>{}));
  std::vector<GeTensorDesc> input_desc{tensor_desc};
  std::vector<GeTensorDesc> output_desc{tensor_desc};

  ge_executor.is_inited_ = true;
  ModelManager::GetInstance().hybrid_model_map_[model_id] = make_shared<MockHybridModelExecutor>();
  EXPECT_EQ(ge_executor.ExecModel(model_id, stream, input_data, input_desc, output_data, output_desc, true), SUCCESS);
  EXPECT_EQ(ge_executor.ExecModel(model_id, stream, input_data, input_desc, output_data, output_desc, false), SUCCESS);
  ModelManager::GetInstance().hybrid_model_map_.erase(model_id);
}

TEST_F(UtestGeExecutor, get_mem_and_weight_size1) {
  GeExecutor ge_executor;
  ModelData model_data;
  Status retStatus;

  ge_executor.is_inited_ = false;
  size_t weightSize;
  size_t memSize;
  retStatus = ge_executor.GetMemAndWeightSize("", memSize, weightSize);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.GetMemAndWeightSize("", memSize, weightSize);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_MODEL_PATH_INVALID);

  string test_smap = "/tmp/" + std::to_string(getpid()) + "_maps";
  string self_smap = "/proc/" + std::to_string(getpid()) + "/maps";
  string copy_smap = "cp -f " + self_smap + " " + test_smap;
  EXPECT_EQ(system(copy_smap.c_str()), 0);
  retStatus = ge_executor.GetMemAndWeightSize(test_smap, memSize, weightSize);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_PARAM_INVALID);
}

TEST_F(UtestGeExecutor, get_mem_and_weight_size2) {
  GeExecutor ge_executor;
  ModelData model_data;
  Status retStatus;

  ge_executor.is_inited_ = false;
  size_t weightSize;
  size_t memSize;
  retStatus = ge_executor.GetMemAndWeightSize(&model_data, sizeof(model_data), memSize, weightSize);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.GetMemAndWeightSize(nullptr, sizeof(model_data), memSize, weightSize);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_MODEL_ADDR_INVALID);

  retStatus = ge_executor.GetMemAndWeightSize(&model_data, sizeof(model_data), memSize, weightSize);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_PARAM_INVALID);

  ModelBufferData model_buffer;
  ModelData model_data2{model_buffer.data.get(), static_cast<uint32_t>(model_buffer.length), 0, "", "/tmp/a"};
  retStatus = ge_executor.GetMemAndWeightSize(&model_data2, sizeof(model_data2), memSize, weightSize);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_PARAM_INVALID);
}

TEST_F(UtestGeExecutor, load_single_op) {
  GeExecutor ge_executor;
  SingleOp *single_op;
  Status retStatus;

  ModelBufferData model_buffer;
  ModelData model_data{model_buffer.data.get(), static_cast<uint32_t>(model_buffer.length), 0, "", "/tmp/a"};
  retStatus = ge_executor.LoadSingleOp(std::string("/tmp/a"), model_data, nullptr, &single_op);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_MODEL_DATA_SIZE_INVALID);

  retStatus = ge_executor.ReleaseSingleOpResource(nullptr);
  EXPECT_EQ(retStatus, SUCCESS);
}

TEST_F(UtestGeExecutor, execute_async1) {
  GeExecutor ge_executor;
  Status retStatus;
  int64_t arg_0 = 127;
  int64_t arg_1 = 100;
  std::vector<DataBuffer> inputs;
  std::vector<DataBuffer> outputs;
  inputs.emplace_back(DataBuffer{&arg_0, sizeof(arg_0), false, 0});
  outputs.emplace_back(DataBuffer{&arg_1, sizeof(arg_1), false, 0});

  retStatus = ge_executor.ExecuteAsync(nullptr, inputs, outputs);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  StreamResource *res = new (std::nothrow) StreamResource(1);
  std::mutex stream_mu;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  SingleOp *single_op = new (std::nothrow) SingleOp(res, &stream_mu, stream);

  retStatus = ge_executor.ExecuteAsync(single_op, inputs, outputs);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_PARAM_INVALID);
  delete res;
}

TEST_F(UtestGeExecutor, execute_async2) {
  GeExecutor ge_executor;
  Status retStatus;
  int64_t arg_0 = 127;
  int64_t arg_1 = 100;

  std::vector<GeTensorDesc> input_desc;
  std::vector<GeTensorDesc> output_desc;

  std::vector<DataBuffer> inputs;
  std::vector<DataBuffer> outputs;
  inputs.emplace_back(DataBuffer{&arg_0, sizeof(arg_0), false, 0});
  outputs.emplace_back(DataBuffer{&arg_1, sizeof(arg_1), false, 0});

  std::mutex stream_mu_;
  DynamicSingleOp dynamic_single_op(nullptr, 0, &stream_mu_, nullptr);
  retStatus = ge_executor.ExecuteAsync(&dynamic_single_op, input_desc, inputs, output_desc, outputs);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_PARAM_INVALID);
}

namespace {
class UtestGeExeWithDavModel : public testing::Test {
 public:
  ComputeGraphPtr graph;
  shared_ptr<DavinciModel> model;
  shared_ptr<hybrid::HybridDavinciModel> hybrid_model;
  uint32_t model_id1;
  uint32_t model_id2;

 protected:
  void SetUp() {
    unsetenv("FMK_SYSMODE");
    unsetenv("FMK_DUMP_PATH");
    unsetenv("FMK_USE_FUSION");
    unsetenv("DAVINCI_TIMESTAT_ENABLE");

    VarManager::Instance(0)->Init(0, 0, 0, 0);
    map<string, string> options;
    options[GRAPH_MEMORY_MAX_SIZE] = "1048576";
    VarManager::Instance(0)->SetMemoryMallocSize(options, 1024UL * 1024UL * 1024UL);
    graph = make_shared<ComputeGraph>("default");

    model_id1 = 1;
    model = MakeShared<DavinciModel>(1, g_label_call_back);
    model->SetId(model_id1);
    model->om_name_ = "testom";
    model->name_ = "test";
    ModelManager::GetInstance().InsertModel(model_id1, model);

    model_id2 = 2;
    hybrid_model = MakeShared<hybrid::HybridDavinciModel>();
    model->SetId(model_id2);
    model->om_name_ = "testom_hybrid";
    model->name_ = "test_hybrid";
    ModelManager::GetInstance().InsertModel(model_id2, hybrid_model);

    model->ge_model_ = make_shared<GeModel>();
    model->runtime_param_.mem_base = 0x08000000;
    model->runtime_param_.mem_size = 5120000;
  }
  void TearDown() {
    ModelManager::GetInstance().DeleteModel(model_id1);
    ModelManager::GetInstance().DeleteModel(model_id2);
    EXPECT_TRUE(ModelManager::GetInstance().model_map_.empty());
    EXPECT_TRUE(ModelManager::GetInstance().hybrid_model_map_.empty());
  }
};
}

TEST_F(UtestGeExeWithDavModel, get_cur_shape) {
  GeExecutor ge_executor;
  Status retStatus;
  std::vector<int64_t> batch_info;
  int32_t dynamic_type;

  ge_executor.is_inited_ = false;
  retStatus = ge_executor.GetCurShape(model_id1, batch_info, dynamic_type);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.GetCurShape(-1, batch_info, dynamic_type);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);

  retStatus = ge_executor.GetCurShape(model_id1, batch_info, dynamic_type);
  EXPECT_EQ(retStatus, SUCCESS);
}

TEST_F(UtestGeExeWithDavModel, set_synamic_aipp_data) {
  GeExecutor ge_executor;
  Status retStatus;
  void *dynamic_input_addr = nullptr;
  uint64_t length = 0;
  std::vector<kAippDynamicBatchPara> aipp_batch_para;
  kAippDynamicPara aipp_parms;

  retStatus = ge_executor.SetDynamicAippData(model_id1, dynamic_input_addr,
                                             length, aipp_batch_para, aipp_parms);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_DYNAMIC_INPUT_ADDR_INVALID);

  dynamic_input_addr = malloc(sizeof(kAippDynamicBatchPara)*2);
  retStatus = ge_executor.SetDynamicAippData(model_id1, dynamic_input_addr,
                                             length, aipp_batch_para, aipp_parms);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_AIPP_BATCH_EMPTY);

  kAippDynamicBatchPara data1;
  aipp_batch_para.push_back(data1);
  retStatus = ge_executor.SetDynamicAippData(model_id1, dynamic_input_addr,
                                             length, aipp_batch_para, aipp_parms);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_DYNAMIC_INPUT_LENGTH_INVALID);

  length = sizeof(kAippDynamicBatchPara)*2;
  retStatus = ge_executor.SetDynamicAippData(model_id1, dynamic_input_addr,
                                             length, aipp_batch_para, aipp_parms);
  EXPECT_EQ(retStatus, SUCCESS);

  free(dynamic_input_addr);
}

TEST_F(UtestGeExeWithDavModel, unload_model) {
  GeExecutor ge_executor;
  Status retStatus;

  retStatus = ge_executor.UnloadModel(-1);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);

  retStatus = ge_executor.UnloadModel(model_id2);
  EXPECT_EQ(retStatus, SUCCESS);

  retStatus = ge_executor.UnloadModel(model_id1);
  EXPECT_EQ(retStatus, SUCCESS);

  retStatus = ge_executor.UnloadModel(model_id1);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);
}

TEST_F(UtestGeExeWithDavModel, cmd_handler) {
  GeExecutor ge_executor;
  struct Command cmd;
  Status retStatus;

  retStatus = ge_executor.CommandHandle(cmd);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_COMMAND_HANDLE);

  cmd.cmd_type = "dump";
  cmd.cmd_params.push_back(std::string("off"));
  cmd.module_index = 1;
  retStatus = ge_executor.CommandHandle(cmd);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_COMMAND_HANDLE);
}

TEST_F(UtestGeExeWithDavModel, get_max_used_mem) {
  GeExecutor ge_executor;
  uint32_t max_size;
  Status retStatus;

  ge_executor.is_inited_ = false;
  retStatus = ge_executor.GetMaxUsedMemory(model_id1, max_size);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.GetMaxUsedMemory(model_id1, max_size);
  EXPECT_EQ(retStatus, SUCCESS);
}

TEST_F(UtestGeExeWithDavModel, get_used_design_shape_order) {
  GeExecutor ge_executor;
  std::vector<std::string> user_designate_shape_order;
  Status retStatus;

  ge_executor.is_inited_ = false;
  retStatus = ge_executor.GetUserDesignateShapeOrder(model_id1, user_designate_shape_order);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.GetUserDesignateShapeOrder(-1, user_designate_shape_order);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);

  retStatus = ge_executor.GetUserDesignateShapeOrder(model_id1, user_designate_shape_order);
  EXPECT_EQ(retStatus, SUCCESS);
}

TEST_F(UtestGeExeWithDavModel, get_combine_dynamic_dims) {
  GeExecutor ge_executor;
  std::vector<std::vector<int64_t>> batch_info;
  Status retStatus;

  ge_executor.is_inited_ = false;
  retStatus = ge_executor.GetCombinedDynamicDims(model_id1, batch_info);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.GetCombinedDynamicDims(-1, batch_info);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);

  retStatus = ge_executor.GetCombinedDynamicDims(model_id1, batch_info);
  EXPECT_EQ(retStatus, SUCCESS);
}

TEST_F(UtestGeExeWithDavModel, get_aipp_info) {
  GeExecutor ge_executor;
  uint32_t index = 0;
  AippConfigInfo aipp_info;
  Status retStatus;

  ge_executor.is_inited_ = false;
  retStatus = ge_executor.GetAIPPInfo(model_id1, index, aipp_info);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.GetAIPPInfo(model_id1, index, aipp_info);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_AIPP_NOT_EXIST);
}

TEST_F(UtestGeExeWithDavModel, get_aipp_type) {
  GeExecutor ge_executor;
  uint32_t index = 0;
  InputAippType type = DATA_WITHOUT_AIPP;
  size_t aipp_index;
  Status retStatus;

  ge_executor.is_inited_ = false;
  retStatus = ge_executor.GetAippType(model_id1, index, type, aipp_index);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.GetAippType(model_id1, index, type, aipp_index);
  EXPECT_EQ(retStatus, PARAM_INVALID);
}

TEST_F(UtestGeExeWithDavModel, get_op_attr) {
  OpDescPtr op_desc = CreateOpDesc("test", "test");
  std::vector<std::string> value{"test"};
  ge::AttrUtils::SetListStr(op_desc, ge::ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, value);

  model->SaveSpecifyAttrValues(op_desc);

  GeExecutor ge_executor;
  GeExecutor::is_inited_ = true;
  std::string attr_value;
  auto ret = ge_executor.GetOpAttr(1, "test", ge::ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, attr_value);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(attr_value, "[4]test");
  ret = ge_executor.GetOpAttr(2, "test", ge::ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, attr_value);
  EXPECT_EQ(ret, PARAM_INVALID);
  ret = ge_executor.GetOpAttr(3, "test", ge::ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, attr_value);
  EXPECT_EQ(ret, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);
}

TEST_F(UtestGeExeWithDavModel, get_model_attr) {
  OpDescPtr op_desc = CreateOpDesc("add0", "Add");
  std::vector<std::string> value{"66"};
  ge::AttrUtils::SetListStr(op_desc, ge::ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, value);
  model->SaveSpecifyAttrValues(op_desc);

  GeExecutor ge_executor;
  Status retStatus;
  std::vector<std::string> str_info;

  GeExecutor::is_inited_ = false;
  retStatus = ge_executor.GetModelAttr(model_id1, str_info);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  GeExecutor::is_inited_ = true;
  retStatus = ge_executor.GetModelAttr(-1, str_info);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);

  GeExecutor::is_inited_ = true;
  retStatus = ge_executor.GetModelAttr(model_id1, str_info);
  EXPECT_EQ(retStatus, SUCCESS);
}

TEST_F(UtestGeExeWithDavModel, get_deviceId_by_modelId) {
  GeExecutor ge_executor;
  uint32_t device_id;
  Status retStatus;

  retStatus = ge_executor.GetDeviceIdByModelId(-1, device_id);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);

  retStatus = ge_executor.GetDeviceIdByModelId(model_id1, device_id);
  EXPECT_EQ(retStatus, SUCCESS);
  EXPECT_EQ(device_id, model->device_id_);
}

TEST_F(UtestGeExeWithDavModel, get_batch_info_size) {
  GeExecutor ge_executor;
  size_t shape_count;
  Status retStatus;

  ge_executor.is_inited_ = false;
  retStatus = ge_executor.GetBatchInfoSize(model_id1, shape_count);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.GetBatchInfoSize(model_id1, shape_count);
  EXPECT_EQ(retStatus, SUCCESS);
}

TEST_F(UtestGeExeWithDavModel, get_all_aipp_inputOutput_dims) {
  GeExecutor ge_executor;
  OriginInputInfo info;
  Status retStatus;

  ge_executor.is_inited_ = false;
  retStatus = ge_executor.GetOrigInputInfo(model_id1, 0, info);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.GetOrigInputInfo(model_id1, 0, info);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_AIPP_NOT_EXIST);
}

TEST_F(UtestGeExeWithDavModel, set_dynamic_batch_size) {
  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr data_desc = CreateOpDesc("data", DATA);
  data_desc->AddInputDesc(tensor);
  data_desc->AddOutputDesc(tensor);
  data_desc->SetInputOffset({1024});
  data_desc->SetOutputOffset({1024});
  graph->AddNode(data_desc);

  std::vector<string> inputs = { "NCHW:DT_FLOAT:inputName:TensorSize:3:1,2,8" };
  AttrUtils::SetListStr(data_desc, ATTR_NAME_AIPP_INPUTS, inputs);
  std::vector<string> outputs = { "NCHW:DT_FLOAT:outputName:TensorSize:3:1,2,8" };
  AttrUtils::SetListStr(data_desc, ATTR_NAME_AIPP_OUTPUTS, outputs);
  std::vector<NodePtr> variable_nodes;
  ASSERT_EQ(model->InitIoNodes(graph, variable_nodes), SUCCESS);
  EXPECT_TRUE(variable_nodes.empty());

  GeExecutor ge_executor;
  Status retStatus;
  uint64_t length = 0;
  uint64_t batch_size = 4;

  retStatus = ge_executor.SetDynamicBatchSize(model_id1, nullptr, length, batch_size);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_DYNAMIC_INPUT_ADDR_INVALID);

  constexpr int SIZE = 32;
  uint64_t *dynamic_input_addr = new uint64_t[SIZE];
  ge_executor.is_inited_ = false;
  retStatus = ge_executor.SetDynamicBatchSize(model_id1, dynamic_input_addr, length, batch_size);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_DYNAMIC_INPUT_LENGTH_INVALID);

  length = sizeof(uint64_t)*SIZE;
  retStatus = ge_executor.SetDynamicBatchSize(-1, dynamic_input_addr, length, batch_size);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.SetDynamicBatchSize(model_id1, dynamic_input_addr, length, batch_size);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_DYNAMIC_BATCH_SIZE_INVALID);

  delete[] dynamic_input_addr;
}

TEST_F(UtestGeExeWithDavModel, set_dynamic_image_size) {
  vector<int64_t> dim(4, 4);
  GeTensorDesc tensor(GeShape(dim), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr data_desc = CreateOpDesc("data", DATA);
  data_desc->AddInputDesc(tensor);
  data_desc->AddOutputDesc(tensor);
  data_desc->SetInputOffset({1024});
  data_desc->SetOutputOffset({1024});
  graph->AddNode(data_desc);

  std::vector<string> inputs = { "NCHW:DT_FLOAT:inputName:TensorSize:3:1,2,8" };
  AttrUtils::SetListStr(data_desc, ATTR_NAME_AIPP_INPUTS, inputs);
  std::vector<string> outputs = { "NCHW:DT_FLOAT:outputName:TensorSize:3:1,2,8" };
  AttrUtils::SetListStr(data_desc, ATTR_NAME_AIPP_OUTPUTS, outputs);
  std::vector<NodePtr> variable_nodes;
  ASSERT_EQ(model->InitIoNodes(graph, variable_nodes), SUCCESS);
  EXPECT_TRUE(variable_nodes.empty());

  GeExecutor ge_executor;
  Status retStatus;
  uint64_t length = 0;
  uint64_t height = 0;
  uint64_t width = 0;

  retStatus = ge_executor.SetDynamicImageSize(model_id1, nullptr, length, height, width);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_DYNAMIC_INPUT_ADDR_INVALID);

  constexpr int SIZE = 32;
  uint64_t *dynamic_input_addr = new uint64_t[SIZE];
  retStatus = ge_executor.SetDynamicImageSize(model_id1, dynamic_input_addr, length, height, width);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_DYNAMIC_INPUT_LENGTH_INVALID);

  length = sizeof(uint64_t)*SIZE;
  ge_executor.is_inited_ = false;
  retStatus = ge_executor.SetDynamicImageSize(-1, dynamic_input_addr, length, height, width);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.SetDynamicImageSize(model_id1, dynamic_input_addr, length, height, width);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_DYNAMIC_BATCH_SIZE_INVALID);

  delete[] dynamic_input_addr;
}

TEST_F(UtestGeExeWithDavModel, dynamic_dims_test) {
  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr data_desc = CreateOpDesc("data", DATA);
  data_desc->AddInputDesc(tensor);
  data_desc->AddOutputDesc(tensor);
  data_desc->SetInputOffset({1024});
  data_desc->SetOutputOffset({1024});
  graph->AddNode(data_desc);

  std::vector<string> inputs = { "NCHW:DT_FLOAT:inputName:TensorSize:3:1,2,8" };
  AttrUtils::SetListStr(data_desc, ATTR_NAME_AIPP_INPUTS, inputs);
  std::vector<string> outputs = { "NCHW:DT_FLOAT:outputName:TensorSize:3:1,2,8" };
  AttrUtils::SetListStr(data_desc, ATTR_NAME_AIPP_OUTPUTS, outputs);
  std::vector<NodePtr> variable_nodes;
  ASSERT_EQ(model->InitIoNodes(graph, variable_nodes), SUCCESS);
  EXPECT_TRUE(variable_nodes.empty());

  GeExecutor ge_executor;
  Status retStatus;
  std::vector<uint64_t> dynamic_dims;
  std::vector<uint64_t> cur_dynamic_dims;

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.GetCurDynamicDims(model_id1, dynamic_dims, cur_dynamic_dims);
  EXPECT_EQ(dynamic_dims.empty(), true);
  EXPECT_EQ(cur_dynamic_dims.empty(), true);

  uint64_t length = 0;
  retStatus = ge_executor.SetDynamicDims(model_id1, nullptr, length, dynamic_dims);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_DYNAMIC_INPUT_ADDR_INVALID);

  constexpr int SIZE = 32;
  uint64_t *dynamic_input_addr = new uint64_t[SIZE];
  ge_executor.is_inited_ = false;
  retStatus = ge_executor.SetDynamicDims(model_id1, dynamic_input_addr, length, dynamic_dims);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.SetDynamicDims(-1, dynamic_input_addr, length, dynamic_dims);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_GET_TENSOR_INFO);

  retStatus = ge_executor.SetDynamicDims(model_id1, dynamic_input_addr, length, dynamic_dims);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_DYNAMIC_BATCH_SIZE_INVALID);

  delete[] dynamic_input_addr;
}

TEST_F(UtestGeExeWithDavModel, get_and_set_dims_info) {
  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr data_desc;
  data_desc = CreateOpDesc("data", DATA);
  data_desc->AddInputDesc(tensor);
  data_desc->AddOutputDesc(tensor);
  data_desc->SetInputOffset({1024});
  data_desc->SetOutputOffset({1024});
  graph->AddNode(data_desc);

  std::vector<string> inputs = { "NCHW:DT_FLOAT:inputName:TensorSize:3:1,2,8" };
  AttrUtils::SetListStr(data_desc, ATTR_NAME_AIPP_INPUTS, inputs);
  std::vector<string> outputs = { "NCHW:DT_FLOAT:outputName:TensorSize:3:1,2,8" };
  AttrUtils::SetListStr(data_desc, ATTR_NAME_AIPP_OUTPUTS, outputs);
  std::vector<NodePtr> variable_nodes;
  ASSERT_EQ(model->InitIoNodes(graph, variable_nodes), SUCCESS);
  EXPECT_TRUE(variable_nodes.empty());

  GeExecutor ge_executor;
  Status retStatus;
  std::vector<InputOutputDims> input_dims;
  std::vector<InputOutputDims> output_dims;

  ge_executor.is_inited_ = false;
  retStatus = ge_executor.GetAllAippInputOutputDims(model_id1, 0, input_dims, output_dims);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.GetAllAippInputOutputDims(-1, 0, input_dims, output_dims);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_MODEL_ID_INVALID);

  retStatus = ge_executor.GetAllAippInputOutputDims(model_id1, 0, input_dims, output_dims);
  EXPECT_EQ(retStatus, SUCCESS);
  EXPECT_EQ(input_dims.front().name=="inputName", true);
}

TEST_F(UtestGeExeWithDavModel, get_modelDesc_info) {
  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr data_desc;
  data_desc = CreateOpDesc("data", DATA);
  data_desc->AddInputDesc(tensor);
  data_desc->AddOutputDesc(tensor);
  data_desc->SetInputOffset({1024});
  data_desc->SetOutputOffset({1024});
  graph->AddNode(data_desc);

  std::vector<string> inputs = { "NCHW:DT_FLOAT:TensorName:TensorSize:3:1,2,8" };
  AttrUtils::SetListStr(data_desc, ATTR_NAME_AIPP_INPUTS, inputs);
  std::vector<string> outputs = { "NCHW:DT_FLOAT:TensorName:TensorSize:3:1,2,8" };
  AttrUtils::SetListStr(data_desc, ATTR_NAME_AIPP_OUTPUTS, outputs);
  std::vector<NodePtr> variable_nodes;
  ASSERT_EQ(model->InitIoNodes(graph, variable_nodes), SUCCESS);
  EXPECT_TRUE(variable_nodes.empty());

  GeExecutor ge_executor;
  Status retStatus;
  std::vector<TensorDesc> input_desc;
  std::vector<TensorDesc> output_desc;

  ge_executor.is_inited_ = false;
  retStatus = ge_executor.GetModelDescInfo(model_id1, input_desc, output_desc, false);
  EXPECT_EQ(retStatus, ACL_ERROR_GE_EXEC_NOT_INIT);

  ge_executor.is_inited_ = true;
  retStatus = ge_executor.GetModelDescInfo(model_id1, input_desc, output_desc, false);
  EXPECT_EQ(retStatus, SUCCESS);
  EXPECT_EQ(input_desc.front().GetFormat(), FORMAT_NCHW);
}

TEST_F(UtestGeExecutor, get_op_desc_info) {
  ComputeGraphPtr graph = make_shared<ComputeGraph>("default");
  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor, 512);

  OpDescPtr op_desc = CreateOpDesc("data", DATA);
  op_desc->AddInputDesc(tensor);
  op_desc->AddOutputDesc(tensor);
  op_desc->SetInputOffset({1024});
  op_desc->SetOutputOffset({1024});
  NodePtr node = graph->AddNode(op_desc);

  GeExecutor ge_executor;
  OpDescInfo op_desc_info;
  Status retStatus;

  retStatus = ge_executor.GetOpDescInfo(0, 0, 0, op_desc_info);
  EXPECT_EQ(retStatus, FAILED);
}

TEST_F(UtestGeExecutor, set_dump) {
  GeExecutor ge_executor;
  DumpConfig dump_config;
  Status retStatus;

  retStatus = ge_executor.SetDump(dump_config);
  EXPECT_EQ(retStatus, SUCCESS);
}

TEST_F(UtestGeExecutor, InitFeatureMapAndP2PMem_failed) {
  DavinciModel model(0, g_label_call_back);
  model.is_feature_map_mem_has_inited_ = true;
  EXPECT_EQ(model.InitFeatureMapAndP2PMem(0U, 0U), PARAM_INVALID);
}

TEST_F(UtestGeExecutor, kernel_InitDumpArgs) {
  DavinciModel model(0, g_label_call_back);
  model.om_name_ = "testom";
  model.name_ = "test";
  OpDescPtr op_desc = CreateOpDesc("test", "test");

  std::map<std::string, std::set<std::string>> model_dump_properties_map;
  std::set<std::string> s;
  model_dump_properties_map[DUMP_ALL_MODEL] = s;
  DumpProperties dp;
  dp.model_dump_properties_map_ = model_dump_properties_map;
  model.SetDumpProperties(dp);

  KernelTaskInfo kernel_task_info;
  kernel_task_info.davinci_model_ = &model;
  kernel_task_info.op_desc_ = op_desc;
  kernel_task_info.InitDumpArgs(0);
}

TEST_F(UtestGeExecutor, kernel_ex_InitDumpArgs) {
  DavinciModel model(0, g_label_call_back);
  model.om_name_ = "testom";
  model.name_ = "test";
  OpDescPtr op_desc = CreateOpDesc("test", "test");

  std::map<std::string, std::set<std::string>> model_dump_properties_map;
  std::set<std::string> s;
  model_dump_properties_map[DUMP_ALL_MODEL] = s;
  DumpProperties dp;
  dp.model_dump_properties_map_ = model_dump_properties_map;
  model.SetDumpProperties(dp);

  KernelExTaskInfo kernel_ex_task_info;
  kernel_ex_task_info.davinci_model_ = &model;
  kernel_ex_task_info.InitDumpArgs(nullptr, op_desc);
}

TEST_F(UtestGeExecutor, kernel_ex_InitDumpFlag) {
  DavinciModel model(0, g_label_call_back);
  model.om_name_ = "testom";
  model.name_ = "test";
  OpDescPtr op_desc = CreateOpDesc("test", "test");

  std::map<std::string, std::set<std::string>> model_dump_properties_map;
  std::set<std::string> s;
  model_dump_properties_map[DUMP_ALL_MODEL] = s;
  DumpProperties dp;
  dp.model_dump_properties_map_ = model_dump_properties_map;
  model.SetDumpProperties(dp);

  KernelExTaskInfo kernel_ex_task_info;
  kernel_ex_task_info.davinci_model_ = &model;
  kernel_ex_task_info.InitDumpFlag(op_desc);
}

TEST_F(UtestGeExecutor, execute_graph_with_stream) {
  VarManager::Instance(0)->Init(0, 0, 0, 0);
  map<string, string> options;
  options[GRAPH_MEMORY_MAX_SIZE] = "1048576";
  VarManager::Instance(0)->SetMemoryMallocSize(options, 1024UL * 1024UL * 1024UL);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  AttrUtils::SetInt(graph, "globleworkspace_status", 1);
  AttrUtils::SetInt(graph, "globleworkspace_status_bytes", 1);

  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1);

  shared_ptr<domi::ModelTaskDef> model_task_def = std::make_shared<domi::ModelTaskDef>();
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
    EXPECT_TRUE(AttrUtils::SetListStr(op_desc, ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, std::vector<std::string>{"dump"}));

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
    op_desc->SetSrcName( { "memcpy" } );
    op_desc->SetSrcIndex( { 0 } );
    NodePtr node = graph->AddNode(op_desc);  // op_index = 3
  }

  setenv("SKT_ENABLE", "1", 1);
  const auto model = MakeShared<DavinciModel>(0, nullptr);
  model->Assign(ge_model);
  EXPECT_EQ(model->Init(), SUCCESS);

  EXPECT_EQ(model->input_addrs_list_.size(), 1);
  EXPECT_EQ(model->output_addrs_list_.size(), 1);
  EXPECT_EQ(model->task_list_.size(), 2);

  OutputData output_data;
  vector<Tensor> outputs;
  EXPECT_EQ(model->GenOutputTensorInfo(output_data, outputs), SUCCESS);

  GraphExecutor graph_executer;
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  std::vector<GeTensor> input_tensor;
  std::vector<GeTensor> output_tensor;
  std::vector<InputOutputDescInfo> output_desc;
  InputOutputDescInfo desc0;
  output_desc.push_back(desc0);

  ge_root_model->SetModelId(1001U);
  ModelManager::GetInstance().InsertModel(ge_root_model->GetModelId(), model);
  EXPECT_EQ(graph_executer.ExecuteGraphWithStream(0, nullptr, ge_root_model, input_tensor, output_tensor), ACL_ERROR_GE_PARAM_INVALID);
  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(ge_root_model->GetModelId()), SUCCESS);
  unsetenv("SKT_ENABLE");
}

namespace {
class GraphModelListenerMock : public GraphModelListener {
 public:
  MOCK_METHOD0(ResetResult, uint32_t());
  MOCK_METHOD0(GetResultCode, uint32_t());
};
}

TEST_F(UtestGeExecutor, execute_graph_sync_multi) {
  VarManager::Instance(0)->Init(0, 0, 0, 0);
  map<string, string> options;
  options[GRAPH_MEMORY_MAX_SIZE] = "1048576";
  VarManager::Instance(0)->SetMemoryMallocSize(options, 1024UL * 1024UL * 1024UL);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");

  vector<NamedAttrs> deploy_info;
  NamedAttrs thread_instance;
  (void) thread_instance.SetAttr("_need_return_result", GeAttrValue::CreateFrom<bool>(true));
  (void) thread_instance.SetAttr("_device_id", GeAttrValue::CreateFrom<int64_t>(0));
  deploy_info.push_back(thread_instance);
  NamedAttrs thread_instance2;
  (void) thread_instance2.SetAttr("_need_return_result", GeAttrValue::CreateFrom<bool>(false));
  (void) thread_instance2.SetAttr("_device_id", GeAttrValue::CreateFrom<int64_t>(0));
  deploy_info.push_back(thread_instance2);
  (void) ge::AttrUtils::SetListNamedAttrs(*graph, ATTR_NAME_DEPLOY_INFO, deploy_info);

  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1);

  shared_ptr<domi::ModelTaskDef> model_task_def = std::make_shared<domi::ModelTaskDef>();
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
    EXPECT_TRUE(AttrUtils::SetListStr(op_desc, ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, std::vector<std::string>{"dump"}));

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
    op_desc->SetSrcName( { "memcpy" } );
    op_desc->SetSrcIndex( { 0 } );
    NodePtr node = graph->AddNode(op_desc);  // op_index = 3
  }

  auto mock_listener = MakeShared<GraphModelListenerMock>();
  EXPECT_CALL(*mock_listener, ResetResult).Times(4).WillOnce(Return(0U)).WillOnce(Return(0U)).WillOnce(Return(0U)).WillOnce(Return(FAILED));
  EXPECT_CALL(*mock_listener, GetResultCode).Times(3).WillOnce(Return(0U)).WillOnce(Return(0U)).WillOnce(Return(FAILED));
  {
    shared_ptr<DavinciModel> model = MakeShared<DavinciModel>(0, mock_listener);
    model->SetId(0);

    model->Assign(ge_model);
    EXPECT_EQ(model->Init(), SUCCESS);

    EXPECT_EQ(model->input_addrs_list_.size(), 1);
    EXPECT_EQ(model->output_addrs_list_.size(), 1);
    EXPECT_EQ(model->task_list_.size(), 2);

    OutputData output_data;
    vector<Tensor> outputs;
    EXPECT_EQ(model->GenOutputTensorInfo(output_data, outputs), SUCCESS);
    ModelManager::GetInstance().InsertModel(0U, model);
  }

  {
    shared_ptr<DavinciModel> model = MakeShared<DavinciModel>(1, mock_listener);
    model->SetId(1);

    model->Assign(ge_model);
    EXPECT_EQ(model->Init(), SUCCESS);

    EXPECT_EQ(model->input_addrs_list_.size(), 1);
    EXPECT_EQ(model->output_addrs_list_.size(), 1);
    EXPECT_EQ(model->task_list_.size(), 2);

    OutputData output_data;
    vector<Tensor> outputs;
    EXPECT_EQ(model->GenOutputTensorInfo(output_data, outputs), SUCCESS);
    ModelManager::GetInstance().InsertModel(1U, model);
  }

  GraphExecutor graph_executer;
  InputOutputDescInfo desc0;
  std::vector<InputOutputDescInfo> output_desc{desc0};
  std::vector<GeTensor> input_tensor;
  std::vector<GeTensor> output_tensor;

  {
    const auto ge_root_model = MakeShared<GeRootModel>(graph);
    ge_root_model->SetModelId(0);
    ge_root_model->SetModelId(1);
    EXPECT_EQ(graph_executer.ExecuteGraph(0, ge_root_model, input_tensor, output_tensor), SUCCESS);
  }

  {
    graph->DelAttr(ATTR_NAME_DEPLOY_INFO);
    const auto ge_root_model = MakeShared<GeRootModel>(graph);
    ge_root_model->SetModelId(1);
    EXPECT_EQ(graph_executer.ExecuteGraph(0, ge_root_model, input_tensor, output_tensor), GE_GRAPH_SYNC_MODEL_FAILED); // failed for GetResultCode
    EXPECT_EQ(graph_executer.ExecuteGraph(0, ge_root_model, input_tensor, output_tensor), GE_GRAPH_SYNC_MODEL_FAILED); // failed for ResetResult
  }

  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(0U), SUCCESS);
  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(1U), SUCCESS);
}

TEST_F(UtestGeExecutor, execute_graph_async_multi) {
  VarManager::Instance(0)->Init(0, 0, 0, 0);
  map<string, string> options;
  options[GRAPH_MEMORY_MAX_SIZE] = "1048576";
  VarManager::Instance(0)->SetMemoryMallocSize(options, 1024UL * 1024UL * 1024UL);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");

  vector<NamedAttrs> deploy_info;
  NamedAttrs thread_instance;
  (void) thread_instance.SetAttr("_need_return_result", GeAttrValue::CreateFrom<bool>(true));
  (void) thread_instance.SetAttr("_device_id", GeAttrValue::CreateFrom<int64_t>(0));
  deploy_info.push_back(thread_instance);
  NamedAttrs thread_instance2;
  (void) thread_instance2.SetAttr("_need_return_result", GeAttrValue::CreateFrom<bool>(false));
  (void) thread_instance2.SetAttr("_device_id", GeAttrValue::CreateFrom<int64_t>(0));
  deploy_info.push_back(thread_instance2);
  (void) ge::AttrUtils::SetListNamedAttrs(*graph, ATTR_NAME_DEPLOY_INFO, deploy_info);

  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge_model->SetGraph(GraphUtils::CreateGraphFromComputeGraph(graph));
  AttrUtils::SetInt(ge_model, ATTR_MODEL_MEMORY_SIZE, 10240);
  AttrUtils::SetInt(ge_model, ATTR_MODEL_STREAM_NUM, 1);

  shared_ptr<domi::ModelTaskDef> model_task_def = std::make_shared<domi::ModelTaskDef>();
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
    EXPECT_TRUE(AttrUtils::SetListStr(op_desc, ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, std::vector<std::string>{"dump"}));

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
    op_desc->SetSrcName( { "memcpy" } );
    op_desc->SetSrcIndex( { 0 } );
    NodePtr node = graph->AddNode(op_desc);  // op_index = 3
  }

  {
    auto listener = MakeShared<RunAsyncListener>();
    shared_ptr<DavinciModel> model = MakeShared<DavinciModel>(0, listener);
    model->SetId(0);

    model->Assign(ge_model);
    EXPECT_EQ(model->Init(), SUCCESS);

    EXPECT_EQ(model->input_addrs_list_.size(), 1);
    EXPECT_EQ(model->output_addrs_list_.size(), 1);
    EXPECT_EQ(model->task_list_.size(), 2);

    OutputData output_data;
    vector<Tensor> outputs;
    EXPECT_EQ(model->GenOutputTensorInfo(output_data, outputs), SUCCESS);
    ModelManager::GetInstance().InsertModel(0, model);
  }

  {
    auto listener = MakeShared<RunAsyncListener>();
    shared_ptr<DavinciModel> model = MakeShared<DavinciModel>(1, listener);
    model->SetId(1);

    model->Assign(ge_model);
    EXPECT_EQ(model->Init(), SUCCESS);

    EXPECT_EQ(model->input_addrs_list_.size(), 1);
    EXPECT_EQ(model->output_addrs_list_.size(), 1);
    EXPECT_EQ(model->task_list_.size(), 2);

    OutputData output_data;
    vector<Tensor> outputs;
    EXPECT_EQ(model->GenOutputTensorInfo(output_data, outputs), SUCCESS);
    ModelManager::GetInstance().InsertModel(1, model);
  }

  GraphExecutor graph_executer;
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  ge_root_model->SetModelId(0);
  ge_root_model->SetModelId(1);
  std::vector<ge::Tensor> input_tensor;
  RunAsyncCallback callback_stub = [](Status, std::vector<ge::Tensor> &) {};
  std::vector<InputOutputDescInfo> output_desc;
  InputOutputDescInfo desc0;
  output_desc.push_back(desc0);

  EXPECT_EQ(graph_executer.ExecuteGraphAsync(0, ge_root_model, input_tensor, callback_stub), SUCCESS);
  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(0U), SUCCESS);
  EXPECT_EQ(ModelManager::GetInstance().DeleteModel(1U), SUCCESS);
}
} // namespace ge

