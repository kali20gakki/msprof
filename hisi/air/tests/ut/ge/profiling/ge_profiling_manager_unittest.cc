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

#include <dirent.h>
#include <gtest/gtest.h>
#include <fstream>
#include <map>
#include <string>
#include <securec.h>

#define protected public
#define private public
#include "toolchain/prof_acl_api.h"
#include "common/profiling/profiling_init.h"
#include "common/profiling/profiling_manager.h"
#include "common/profiling/profiling_properties.h"
#include "common/profiling_definitions.h"
#include "common/profiling/command_handle.h"
#include "graph/ge_local_context.h"
#include "framework/common/profiling/ge_profiling.h"
#include "graph/manager/graph_manager.h"
#include "graph/ops_stub.h"
#include "framework/omg/omg_inner_types.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/load/model_manager/model_manager.h"
#include "graph/load/model_manager/davinci_model.h"

using namespace ge;
using namespace std;

namespace {
  enum ProfCommandHandleType {
    kProfCommandhandleInit = 0,
    kProfCommandhandleStart,
    kProfCommandhandleStop,
    kProfCommandhandleFinalize,
    kProfCommandhandleModelSubscribe,
    kProfCommandhandleModelUnsubscribe
  };
  constexpr int32_t TMP_ERROR = -1;       // RT_ERROR

  MsprofGeProfTaskData task_data_actual{};
  MsprofGeProfTensorData tensor_data_actual{};
  int32_t task_data_count = 0;

  int32_t ReporterCallback2(uint32_t moduleId, uint32_t type, void *data, uint32_t len) {
    if (type == static_cast<uint32_t>(MSPROF_REPORTER_HASH)) {
      MsprofHashData *hash_data = reinterpret_cast<MsprofHashData *>(data);
      hash_data->hashId = 22U;
    }
    else {
      ReporterData *report_data = reinterpret_cast<ReporterData *>(data);
      std::string tag(report_data->tag);
      if (tag == "task_desc_info") {
        MsprofGeProfTaskData *task_data_ptr = reinterpret_cast<MsprofGeProfTaskData *>(report_data->data);
        task_data_actual.opName.type = task_data_ptr->opName.type;
        (void)memcpy_s(task_data_actual.opName.data.dataStr, MSPROF_MIX_DATA_STRING_LEN,
                       task_data_ptr->opName.data.dataStr, sizeof(task_data_ptr->opName.data.dataStr));
        task_data_actual.opType.type = task_data_ptr->opType.type;
        task_data_actual.opType.data.hashId = task_data_ptr->opType.data.hashId;
        ++task_data_count;
      } else {
        MsprofGeProfTensorData *tensor_data_ptr = reinterpret_cast<MsprofGeProfTensorData *>(report_data->data);
        tensor_data_actual.tensorData[0].dataType = tensor_data_ptr->tensorData[0].dataType;
        tensor_data_actual.tensorData[0].format = tensor_data_ptr->tensorData[0].format;
        tensor_data_actual.tensorData[0].shape[0] = tensor_data_ptr->tensorData[0].shape[0];
        tensor_data_actual.tensorData[0].shape[1] = tensor_data_ptr->tensorData[0].shape[1];
      }
    }
    return 0;
  }
}

class UtestGeProfilingManager : public testing::Test {
 protected:
  void SetUp() {
    const auto davinci_model = MakeShared<DavinciModel>(2005U, MakeShared<RunAsyncListener>());
    ModelManager::GetInstance().InsertModel(2005U, davinci_model);
  }

  void TearDown() override {
    ProfilingManager::Instance().reporter_callback_ = nullptr;
    EXPECT_EQ(ModelManager::GetInstance().DeleteModel(2005U), SUCCESS);
    EXPECT_TRUE(ModelManager::GetInstance().model_map_.empty());
    EXPECT_TRUE(ModelManager::GetInstance().hybrid_model_map_.empty());
  }
};

int32_t ReporterCallback(uint32_t moduleId, uint32_t type, void *data, uint32_t len) {
  return -1;
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

TEST_F(UtestGeProfilingManager, init_success) {
  setenv("PROFILING_MODE", "true", true);
  Options options;
  options.device_id = 0;
  options.job_id = "0";
  options.profiling_mode = "1";
  options.profiling_options = R"({"result_path":"/data/profiling","training_trace":"on","task_trace":"on","aicpu_trace":"on","fp_point":"Data_0","bp_point":"addn","ai_core_metrics":"ResourceConflictRatio"})";


  struct MsprofGeOptions prof_conf = {{ 0 }};
  bool is_execute_profiling = false;
  Status ret = ProfilingInit::Instance().InitFromOptions(options, prof_conf, is_execute_profiling);
  EXPECT_EQ(ret, ge::SUCCESS);
}

TEST_F(UtestGeProfilingManager, plungin_init_) {
  ProfilingManager::Instance().reporter_callback_ = ReporterCallback;

  Status ret = ProfilingManager::Instance().PluginInit();
  EXPECT_EQ(ret, INTERNAL_ERROR);
  ProfilingManager::Instance().reporter_callback_ = nullptr;
}

TEST_F(UtestGeProfilingManager, Prof_finalize_) {
  ProfilingManager::Instance().reporter_callback_ = ReporterCallback;
  Status ret = ProfilingManager::Instance().ProfFinalize();
  EXPECT_EQ(ret, ge::SUCCESS);
}

TEST_F(UtestGeProfilingManager, report_data_) {
  std::string data = "ge is better than tensorflow.";
  std::string tag_name = "fmk";
  ProfilingManager::Instance().ReportData(0, reinterpret_cast<unsigned char *>(const_cast<char *>(data.c_str())),
                                          sizeof(data), tag_name);
}

TEST_F(UtestGeProfilingManager, get_fp_bp_point_) {
  map<std::string, string> options_map = {
    {OPTION_EXEC_PROFILING_OPTIONS,
    R"({"result_path":"/data/profiling","training_trace":"on","task_trace":"on","aicpu_trace":"on","fp_point":"Data_0","bp_point":"addn","ai_core_metrics":"ResourceConflictRatio"})"}};
  GEThreadLocalContext &context = GetThreadLocalContext();
  context.SetGraphOption(options_map);

  std::string fp_point;
  std::string bp_point;
  ProfilingProperties::Instance().GetFpBpPoint(fp_point, bp_point);
  EXPECT_EQ(fp_point, "Data_0");
  EXPECT_EQ(bp_point, "addn");
}

TEST_F(UtestGeProfilingManager, get_fp_bp_point_empty) {
  // fp bp empty
  map<std::string, string> options_map = {
    { OPTION_EXEC_PROFILING_OPTIONS,
      R"({"result_path":"/data/profiling","training_trace":"on","task_trace":"on","aicpu_trace":"on","ai_core_metrics":"ResourceConflictRatio"})"}};
  GEThreadLocalContext &context = GetThreadLocalContext();
  context.SetGraphOption(options_map);
  std::string fp_point = "fp";
  std::string bp_point = "bp";
  ProfilingProperties::Instance().bp_point_ = "";
  ProfilingProperties::Instance().fp_point_ = "";
  ProfilingProperties::Instance().GetFpBpPoint(fp_point, bp_point);
  EXPECT_EQ(fp_point, "");
  EXPECT_EQ(bp_point, "");
}

TEST_F(UtestGeProfilingManager, set_step_info_success) {
  uint64_t index_id = 0;
  auto stream = (rtStream_t)0x1;
  Status ret = ProfSetStepInfo(index_id, 0, stream);
  EXPECT_EQ(ret, ge::SUCCESS);
  ret = ProfSetStepInfo(index_id, 1, stream);
  EXPECT_EQ(ret, ge::SUCCESS);
}

TEST_F(UtestGeProfilingManager, set_step_info_failed) {
  uint64_t index_id = 0;
  auto stream = (rtStream_t)0x1;
  Status ret = ProfSetStepInfo(index_id, 2, stream);
  EXPECT_EQ(ret, ge::FAILED);
}

TEST_F(UtestGeProfilingManager, handle_subscribe_info) {
  uint32_t prof_type = RT_PROF_CTRL_SWITCH;
  MsprofCommandHandle prof_data;
  prof_data.profSwitch = 0;
  prof_data.modelId = 1;
  prof_data.type = 0;
  domi::GetContext().train_flag = true;
  ProfilingProperties::Instance().SetProfilingLoadOffineFlag(false);
  auto prof_ptr = std::make_shared<MsprofCommandHandle>(prof_data);
  Status ret = ProfCtrlHandle(prof_type, static_cast<void *>(prof_ptr.get()), sizeof(prof_data));
  EXPECT_EQ(ret, ge::SUCCESS);
}

TEST_F(UtestGeProfilingManager, need_handle_start_end) {
  Status ret;
  uint32_t prof_type = RT_PROF_CTRL_SWITCH;

  MsprofCommandHandle prof_data;
  prof_data.profSwitch = 0;
  prof_data.modelId = 1;
  domi::GetContext().train_flag = true;
  ProfilingProperties::Instance().SetProfilingLoadOffineFlag(false);
  auto prof_ptr = std::make_shared<MsprofCommandHandle>(prof_data);

  prof_ptr->type = kProfCommandhandleModelUnsubscribe + 2;
  ret = ProfCtrlHandle(prof_type, static_cast<void *>(prof_ptr.get()), sizeof(prof_data));
  EXPECT_EQ(ret, TMP_ERROR);

  prof_ptr->type = kProfCommandhandleStart;
  prof_ptr->devNums = 0;
  ret = ProfCtrlHandle(prof_type, static_cast<void *>(prof_ptr.get()), sizeof(prof_data));
  EXPECT_EQ(ret, TMP_ERROR);

  prof_ptr->devNums = 2;
  ret = ProfCtrlHandle(prof_type, static_cast<void *>(prof_ptr.get()), sizeof(prof_data));
  EXPECT_EQ(ret, TMP_ERROR);

  prof_ptr->devNums = 1;
  prof_ptr->devIdList[0] = 1;
  ret = ProfCtrlHandle(prof_type, static_cast<void *>(prof_ptr.get()), sizeof(prof_data));
  EXPECT_EQ(ret, TMP_ERROR);
  prof_ptr->devIdList[0] = 0;
  prof_ptr->devIdList[1] = 1;
  ret = ProfCtrlHandle(prof_type, static_cast<void *>(prof_ptr.get()), sizeof(prof_data));
  EXPECT_EQ(ret, RT_ERROR_NONE);
}

TEST_F(UtestGeProfilingManager, handle_ctrl_switch) {
  Status ret;
  uint32_t prof_type = RT_PROF_CTRL_SWITCH;

  MsprofCommandHandle prof_data;
  prof_data.profSwitch = 0;
  prof_data.modelId = 1;
  domi::GetContext().train_flag = true;
  ProfilingProperties::Instance().SetProfilingLoadOffineFlag(false);
  auto prof_ptr = std::make_shared<MsprofCommandHandle>(prof_data);

  prof_ptr->devNums = 1;
  prof_ptr->devIdList[0] = 0;
  prof_ptr->devIdList[1] = 1;

  prof_ptr->type = 4;   // kProfCommandHandleModelSubscribe;
  prof_ptr->profSwitch = -1;
  prof_ptr->modelId = -1;
  ret = ProfCtrlHandle(prof_type, static_cast<void *>(prof_ptr.get()), sizeof(prof_data));
  EXPECT_EQ(ret, SUCCESS);

  prof_ptr->type = 5;
  prof_ptr->profSwitch = 0;
  prof_ptr->modelId = 2005U;
  ret = ProfCtrlHandle(prof_type, static_cast<void *>(prof_ptr.get()), sizeof(prof_data));
  EXPECT_EQ(ret, FAILED);
  ProfilingManager::Instance().SetSubscribeInfo(0, 2005U, false);
  ret = ProfCtrlHandle(prof_type, static_cast<void *>(prof_ptr.get()), sizeof(prof_data));
  EXPECT_EQ(ret, SUCCESS);

  domi::GetContext().train_flag = false;
  ProfilingProperties::Instance().SetProfilingLoadOffineFlag(true);
  prof_ptr->type = 4;
  ret = ProfCtrlHandle(prof_type, static_cast<void *>(prof_ptr.get()), sizeof(prof_data));
  EXPECT_EQ(ret, SUCCESS);

  ret = ProfCtrlHandle(prof_type, static_cast<void *>(prof_ptr.get()), sizeof(prof_data));
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGeProfilingManager, handle_subscribe_info2) {
  uint32_t prof_type = RT_PROF_CTRL_REPORTER;
  MsprofCommandHandle prof_data;
  prof_data.profSwitch = 0;
  prof_data.modelId = 1;
  prof_data.type = 0;
  domi::GetContext().train_flag = true;
  ProfilingProperties::Instance().SetProfilingLoadOffineFlag(false);
  auto prof_ptr = std::make_shared<MsprofCommandHandle>(prof_data);

  Status ret = ProfCtrlHandle(prof_type, nullptr, sizeof(prof_data));
  EXPECT_EQ(ret, ge::FAILED);

  ret = ProfCtrlHandle(prof_type, static_cast<void *>(prof_ptr.get()), sizeof(prof_data));
  EXPECT_EQ(ret, ge::SUCCESS);
}

TEST_F(UtestGeProfilingManager, handle_unsubscribe_info) {
  uint32_t prof_type = kProfCommandhandleModelUnsubscribe;
  MsprofCommandHandle prof_data;
  prof_data.profSwitch = 0;
  prof_data.modelId = 1;
  domi::GetContext().train_flag = true;
  ProfilingProperties::Instance().SetProfilingLoadOffineFlag(false);
  auto &profiling_manager = ge::ProfilingManager::Instance();
  profiling_manager.SetSubscribeInfo(0, 1, true);
  auto prof_ptr = std::make_shared<MsprofCommandHandle>(prof_data);
  Status ret = ProfCtrlHandle(prof_type, static_cast<void *>(prof_ptr.get()), sizeof(prof_data));
  profiling_manager.CleanSubscribeInfo();
}

TEST_F(UtestGeProfilingManager, set_subscribe_info) {
  ProfilingManager::Instance().SetSubscribeInfo(0, 1, true);
  const auto &subInfo = ProfilingManager::Instance().GetSubscribeInfo();
  EXPECT_EQ(subInfo.prof_switch, 0);
  EXPECT_EQ(subInfo.graph_id, 1);
  EXPECT_EQ(subInfo.is_subscribe, true);
}

TEST_F(UtestGeProfilingManager, clean_subscribe_info) {
  ProfilingManager::Instance().CleanSubscribeInfo();
  const auto &subInfo = ProfilingManager::Instance().GetSubscribeInfo();
  EXPECT_EQ(subInfo.prof_switch, 0);
  EXPECT_EQ(subInfo.graph_id, 0);
  EXPECT_EQ(subInfo.is_subscribe, false);
}

TEST_F(UtestGeProfilingManager, get_model_id_success) {
  ProfilingManager::Instance().SetGraphIdToModelMap(0, 1);
  uint32_t model_id = 0;
  Status ret = ProfilingManager::Instance().GetModelIdFromGraph(0, model_id);
  EXPECT_EQ(ret, ge::SUCCESS);
}

TEST_F(UtestGeProfilingManager, get_model_id_failed) {
  ProfilingManager::Instance().SetGraphIdToModelMap(0, 1);
  uint32_t model_id = 0;
  Status ret = ProfilingManager::Instance().GetModelIdFromGraph(10, model_id);
  EXPECT_EQ(ret, ge::FAILED);
}

TEST_F(UtestGeProfilingManager, get_device_id_success) {
  ProfilingManager::Instance().SetGraphIdToDeviceMap(0, 1);
  uint32_t device_id = 0;
  Status ret = ProfilingManager::Instance().GetDeviceIdFromGraph(0, device_id);
  EXPECT_EQ(ret, ge::SUCCESS);
}

TEST_F(UtestGeProfilingManager, get_device_id_failed) {
  ProfilingManager::Instance().SetGraphIdToDeviceMap(0, 1);
  uint32_t device_id = 0;
  Status ret = ProfilingManager::Instance().GetDeviceIdFromGraph(10, device_id);
  EXPECT_EQ(ret, ge::FAILED);
}

TEST_F(UtestGeProfilingManager, start_profiling_failed) {
  const uint64_t module = PROF_OP_DETAIL_MASK | PROF_TRAINING_TRACE;
  std::map<std::string, std::string> cmd_params_map;
  cmd_params_map["key"] = "value";
  Status ret = ProfilingManager::Instance().ProfStartProfiling(module, cmd_params_map);
  EXPECT_EQ(ret, ge::FAILED);
}
TEST_F(UtestGeProfilingManager, prof_step_info_success) {
  ProfilingProperties::Instance().SetLoadProfiling(true);
  uint32_t device_id = 0;
  uint64_t index_id = 1U;
  uint16_t tag_id = 1;
  uint32_t model_id = 1U;
  rtStream_t stream;
  const auto ret =  ProfilingManager::Instance().ProfileStepInfo(index_id, model_id, tag_id, stream, device_id);
  EXPECT_EQ(ge::SUCCESS, ret);
}

void BuildTaskDescInfo(TaskDescInfo &task_info) {
  task_info.op_name = "test";
  std::string op_type (80,'x');
  task_info.op_type = op_type;
  task_info.shape_type = "static";
  task_info.block_dim = 1;
  task_info.task_id = 1;
  task_info.cur_iter_num = 1;
  task_info.task_type = "AI_CPU";
}

TEST_F(UtestGeProfilingManager, prof_task_info_success) {
  TaskDescInfo task_info_1{};
  BuildTaskDescInfo(task_info_1);
  TaskDescInfo task_info_2{};
  BuildTaskDescInfo(task_info_2);
  task_info_2.op_name = "test2";
  task_info_2.shape_type = "test";
  task_info_2.task_type = "test";
  auto &prof_mgr = ProfilingManager::Instance();
  prof_mgr.reporter_callback_ = ReporterCallback2;
  uint32_t model_id =1;
  uint32_t device_id = 1;
  std::vector<TaskDescInfo> task_info_lst;
  task_info_lst.emplace_back(task_info_1);
  task_info_lst.emplace_back(task_info_2);
  prof_mgr.ProfilingTaskDescInfo(model_id, device_id, task_info_lst);
  ASSERT_EQ(task_data_actual.opName.type, MSPROF_MIX_DATA_STRING);
  ASSERT_EQ(task_data_actual.opName.data.dataStr, task_info_1.op_name);
  ASSERT_EQ(task_data_actual.opType.type, MSPROF_MIX_DATA_HASH_ID);
  ASSERT_EQ(task_data_actual.opType.data.hashId, 22U);
  ASSERT_EQ(1, task_data_count);
  task_data_count = 0;
}

TEST_F(UtestGeProfilingManager, prof_op_info_success) {
  TaskDescInfo task_info{};
  BuildTaskDescInfo(task_info);
  std::vector<DataType> data_type(5,DT_INT8);
  task_info.input_data_type = data_type;
  task_info.output_data_type = data_type;
  std::vector<std::vector<int64_t>> shape1 = {{1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}};
  std::vector<std::vector<int64_t>> shape2 = {{1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 2}, {1, 3}};
  task_info.input_shape = shape1;
  task_info.output_shape = shape2;
  std::vector<Format> format(6, FORMAT_NCHW);
  task_info.input_format = format;
  task_info.output_format = format;
  auto &prof_mgr = ProfilingManager::Instance();
  prof_mgr.reporter_callback_ = ReporterCallback2;
  const uint32_t model_id = 1;
  const uint32_t device_id = 1;
  std::vector<TaskDescInfo> task_info_lst;
  task_info_lst.emplace_back(task_info);
  prof_mgr.ProfilingOpInputOutInfo(task_info, model_id, device_id, 0UL, "tensor_data_info");
  ASSERT_EQ(tensor_data_actual.tensorData[0].dataType, DT_INT8);
  std::vector<int64_t> expect_shape = {1, 2};
  for (size_t i = 0U; i<expect_shape.size(); ++i) {
    EXPECT_EQ(tensor_data_actual.tensorData[0].shape[i], expect_shape[i]);
  }
  EXPECT_EQ(tensor_data_actual.tensorData[0].format, FORMAT_NCHW);
}


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
shared_ptr<ge::ModelListener> g_label_call_back(new DModelListener());

namespace {


class UtestGeProfilingManager1 : public testing::Test {
 public:
    ComputeGraphPtr graph;
    shared_ptr<DavinciModel> model;
    uint32_t model_id;

 protected:
  void SetUp() {
    graph = make_shared<ComputeGraph>("default");

    model_id = 1;
    model = MakeShared<DavinciModel>(1, g_label_call_back);
    model->SetId(model_id);
    model->om_name_ = "testom";
    model->name_ = "test";
    ModelManager::GetInstance().InsertModel(model_id, model);

    model->ge_model_ = make_shared<GeModel>();
    model->runtime_param_.mem_base = 0x08000000;
    model->runtime_param_.mem_size = 5120000;
  }
  void TearDown() {
    ModelManager::GetInstance().DeleteModel(model_id);
    ProfilingManager::Instance().reporter_callback_ = nullptr;
  }
};
}

TEST_F(UtestGeProfilingManager1, prof_model_unsubscribe) {
  Status retStatus;

  retStatus = ProfilingManager::Instance().ProfModelUnsubscribe(model->GetDeviceId());
  EXPECT_EQ(retStatus, SUCCESS);

  retStatus = ModelManager::GetInstance().ModelSubscribe(1);
  EXPECT_EQ(retStatus, SUCCESS);

  retStatus = ProfilingManager::Instance().ProfInit(PROF_MODEL_LOAD_MASK);
  EXPECT_EQ(retStatus, PARAM_INVALID);

  retStatus = ProfilingManager::Instance().ProfInit(1);
  EXPECT_EQ(retStatus, PARAM_INVALID);
}

TEST_F(UtestGeProfilingManager, prof_start_profiling) {
  Status retStatus;
  const uint64_t module = 1;
  std::map<std::string, std::string> config_para;

  config_para["devNums"] = "ab";
  retStatus = ProfilingManager::Instance().ProfStartProfiling(module, config_para);
  EXPECT_EQ(retStatus, FAILED);

  config_para["devNums"] = "12356890666666";
  retStatus = ProfilingManager::Instance().ProfStartProfiling(module, config_para);
  EXPECT_EQ(retStatus, FAILED);

  config_para["devNums"] = "-1";
  retStatus = ProfilingManager::Instance().ProfStartProfiling(module, config_para);
  EXPECT_EQ(retStatus, FAILED);

  config_para["devNums"] = "1";
  retStatus = ProfilingManager::Instance().ProfStartProfiling(module, config_para);
  EXPECT_EQ(retStatus, FAILED);

  config_para["devIdList"] = "ab";
  retStatus = ProfilingManager::Instance().ProfStartProfiling(module, config_para);
  EXPECT_EQ(retStatus, FAILED);

  config_para["devIdList"] = "12356890666666";
  retStatus = ProfilingManager::Instance().ProfStartProfiling(module, config_para);
  EXPECT_EQ(retStatus, FAILED);

  config_para["devNums"] = "4";
  config_para["devIdList"] = "0,1,2,8";
  retStatus = ProfilingManager::Instance().ProfStartProfiling(module, config_para);
  EXPECT_EQ(retStatus, SUCCESS);

  retStatus = ProfilingManager::Instance().ProfStartProfiling(PROF_MODEL_EXECUTE_MASK, config_para);
  EXPECT_EQ(retStatus, SUCCESS);
  ProfilingManager::Instance().RecordUnReportedModelId(1);
  retStatus = ProfilingManager::Instance().ProfStartProfiling(PROF_MODEL_LOAD_MASK, config_para);
  EXPECT_EQ(retStatus, SUCCESS);
}

TEST_F(UtestGeProfilingManager, prof_stop_profiling) {
  Status retStatus;
  const uint64_t module = 1;
  std::map<std::string, std::string> config_para;

  config_para["devNums"] = "ab";
  retStatus = ProfilingManager::Instance().ProfStopProfiling(module, config_para);
  EXPECT_EQ(retStatus, FAILED);

  config_para["devNums"] = "12356890666666";
  retStatus = ProfilingManager::Instance().ProfStopProfiling(module, config_para);
  EXPECT_EQ(retStatus, FAILED);

  config_para["devNums"] = "-1";
  retStatus = ProfilingManager::Instance().ProfStopProfiling(module, config_para);
  EXPECT_EQ(retStatus, FAILED);

  config_para["devNums"] = "1";
  retStatus = ProfilingManager::Instance().ProfStopProfiling(module, config_para);
  EXPECT_EQ(retStatus, FAILED);

  config_para["devIdList"] = "ab";
  retStatus = ProfilingManager::Instance().ProfStopProfiling(module, config_para);
  EXPECT_EQ(retStatus, FAILED);

  config_para["devIdList"] = "12356890666666";
  retStatus = ProfilingManager::Instance().ProfStopProfiling(module, config_para);
  EXPECT_EQ(retStatus, FAILED);

  config_para["devNums"] = "4";
  config_para["devIdList"] = "0,1,2,8";
  retStatus = ProfilingManager::Instance().ProfStopProfiling(module, config_para);
  EXPECT_EQ(retStatus, SUCCESS);

  retStatus = ProfilingManager::Instance().ProfStopProfiling(PROF_MODEL_EXECUTE_MASK, config_para);
  EXPECT_EQ(retStatus, SUCCESS);
}

TEST_F(UtestGeProfilingManager, registe_element) {
  int64_t first_idx = -1;
  int64_t second_idx = -1;
  ProfilingProperties::Instance().SetLoadProfiling(true);
  ProfilingProperties::Instance().SetOpDetailProfiling(true);
  ProfilingManager::Instance().RegisterElement(first_idx, "tiling1");
  ProfilingManager::Instance().RegisterElement(second_idx, "tiling2");
  ProfilingProperties::Instance().SetLoadProfiling(false);
  ProfilingProperties::Instance().SetOpDetailProfiling(false);
  EXPECT_EQ(first_idx + 1, second_idx);
}

