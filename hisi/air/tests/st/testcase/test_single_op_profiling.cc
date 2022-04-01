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
#include "inc/framework/common/profiling_definitions.h"
#include "inc/framework/common/ge_types.h"
#include "executor/single_op/single_op.h"
#include "executor/single_op/task/tbe_task_builder.h"
#include "common/profiling/profiling_manager.h"
#include "common/tbe_handle_store/tbe_handle_store.h"

#include "st/stubs/utils/bench_env.h"
#include "st/stubs/utils/model_data_builder.h"
#include "st/stubs/utils/graph_factory.h"
#include "st/stubs/utils/tensor_descs.h"
#include "st/stubs/utils/data_buffers.h"
#include "st/stubs/utils/profiling_test_data.h"

#include <gtest/gtest.h>

namespace ge {
using namespace profiling;
class SingleOpProfilingSt : public testing::Test {
 protected:
  void TearDown() override {
    profiling::ProfilingContext::GetInstance().Reset();
    profiling::ProfilingContext::GetInstance().SetDisable();
    ProfilingProperties::Instance().ClearProperties();
  }
};

static int32_t g_call_profiling_time[10] = {0};
static int32_t MsprofReportCallback(uint32_t moduleId, uint32_t type, void *data, uint32_t len) {
  struct MsprofHashData *hash_data;
  char *str;
  switch (type) {
    case MSPROF_REPORTER_REPORT:
      g_call_profiling_time[MSPROF_REPORTER_REPORT]++;
      break;
    case MSPROF_REPORTER_INIT:
      g_call_profiling_time[MSPROF_REPORTER_INIT]++;
      break;
    case MSPROF_REPORTER_UNINIT:
      g_call_profiling_time[MSPROF_REPORTER_UNINIT]++;
      break;
    case MSPROF_REPORTER_DATA_MAX_LEN:
      g_call_profiling_time[MSPROF_REPORTER_DATA_MAX_LEN]++;
      break;
    case MSPROF_REPORTER_HASH:
      hash_data = reinterpret_cast<struct MsprofHashData *>(data);
      str = reinterpret_cast<char *>(hash_data->data);
      hash_data->hashId = std::hash<std::string>{}(str);
      g_call_profiling_time[MSPROF_REPORTER_HASH]++;
      break;
    default:
      break;
  }
  return 0;
};

static void ClearMsprofReportData() {
  ProfilingProperties::Instance().SetOpDetailProfiling(false);
  (void)memset(g_call_profiling_time, 0, sizeof(g_call_profiling_time));
  ProfilingManager::Instance().SetMsprofReporterCallback(nullptr);
}

TEST_F(SingleOpProfilingSt, DynamicSingleOpProfiling) {
  profiling::ProfilingContext::GetInstance().SetEnable();

  BenchEnv::Init();
  uint8_t model_data[8192];
  ge::ModelData modelData{.model_data = model_data};
  ModelDataBuilder(modelData).AddGraph(GraphFactory::SingeOpGraph()).AddTask(2, 2).Build();

  auto input_desc = TensorDescs(1).Value();
  auto input_buffers = DataBuffers(1).Value();
  auto output_desc = TensorDescs(1).Value();
  auto output_buffers = DataBuffers(1).Value();

  ge::DynamicSingleOp *single_op = nullptr;
  std::vector<char> kernelBin;
  TBEKernelPtr tbe_kernel = std::make_shared<ge::OpKernelBin>("name/transdata", std::move(kernelBin));
  auto holder = std::unique_ptr<KernelHolder>(new (std::nothrow) KernelHolder("0/_tvmbin", tbe_kernel));
  KernelBinRegistry::GetInstance().AddKernel("0/_tvmbin", std::move(holder));
  ASSERT_EQ(ge::GeExecutor::LoadDynamicSingleOpV2("dynamic_op2", modelData, nullptr, &single_op, 0), SUCCESS);
  ASSERT_EQ(single_op->ExecuteAsync(input_desc, input_buffers, output_desc, output_buffers), SUCCESS);

  ProfilingData().HasRecord(kUpdateShape).HasRecord(kTiling).HasRecord(kRtKernelLaunch);
}

TEST_F(SingleOpProfilingSt, HybridSingeOpGraphProfiling) {
  profiling::ProfilingContext::GetInstance().SetEnable();

  BenchEnv::Init();
  uint8_t model_data[8192];
  ge::ModelData modelData{.model_data = model_data};
  ModelDataBuilder(modelData).AddGraph(GraphFactory::HybridSingeOpGraph()).AddTask(2, 2)
  .AddTask(2, 4)
  .AddTask(2, 4)
  .AddTask(2, 5)
  .AddTask(2, 5)
  .Build();

  auto input_desc = TensorDescs(2).Value();
  auto input_buffers = DataBuffers(2).Value();
  auto output_desc = TensorDescs(1).Value();
  auto output_buffers = DataBuffers(1).Value();

  ge::DynamicSingleOp *singleOp = nullptr;
  ASSERT_EQ(ge::GeExecutor::LoadDynamicSingleOpV2("dynamic_op", modelData, nullptr, &singleOp, 4), SUCCESS);
  ASSERT_EQ(ge::GeExecutor::ExecuteAsync(singleOp, input_desc, input_buffers, output_desc, output_buffers), SUCCESS);
  ProfilingData().HasRecord(kInferShape).HasRecord(kUpdateShape).HasRecord(kTiling).HasRecord(kRtKernelLaunch);
}

TEST_F(SingleOpProfilingSt, HybridSingeOpGraphProfilingStartThenExecAndStop) {
  profiling::ProfilingContext::GetInstance().SetEnable();
  BenchEnv::Init();
  ClearMsprofReportData();
  uint8_t model_data[8192];
  ge::ModelData modelData{.model_data = model_data};
  ModelDataBuilder(modelData).AddGraph(GraphFactory::HybridSingeOpGraph()).AddTask(2, 2)
  .AddTask(2, 4)
  .AddTask(2, 4)
  .AddTask(2, 5)
  .AddTask(2, 5)
  .Build();

  auto input_desc = TensorDescs(2).Value();
  auto input_buffers = DataBuffers(2).Value();
  auto output_desc = TensorDescs(1).Value();
  auto output_buffers = DataBuffers(1).Value();

  ProfilingProperties::Instance().SetLoadProfiling(true);
  ge::DynamicSingleOp *singleOp = nullptr;
  const std::string kConfigNumsdev = "devNums";
  const std::string kConfigDevIdList = "devIdList";
  const uint64_t kAllSwitchOn = 0xffffffffffffffffULL;
  const string &key = kConfigNumsdev;
  const string &val = "1";
  std::map<std::string, std::string> config_para;
  EXPECT_EQ(ProfilingManager::Instance().ProfStartProfiling(kAllSwitchOn, config_para), FAILED);
  config_para.insert(std::pair<string, string>{key, val});
  config_para.insert(std::pair<string, string>{kConfigDevIdList, "0"});
  // first start profiling and second execute single op
  ProfilingManager::Instance().SetMsprofReporterCallback(MsprofReportCallback);
  EXPECT_EQ(ProfilingManager::Instance().ProfStartProfiling(kAllSwitchOn, config_para), SUCCESS);
  // check result after profiling start
  ASSERT_EQ(ProfilingProperties::Instance().IsDynamicShapeProfiling(), true);
  ASSERT_EQ(ge::GeExecutor::LoadDynamicSingleOpV2("dynamic_op", modelData, nullptr, &singleOp, 4), SUCCESS);
  ASSERT_EQ(ge::GeExecutor::ExecuteAsync(singleOp, input_desc, input_buffers, output_desc, output_buffers), SUCCESS);
  ProfilingData().HasRecord(kInferShape).HasRecord(kUpdateShape).HasRecord(kTiling).HasRecord(kRtKernelLaunch);
  // transdata1, matmul, transdata2, data1, data2, net_output has register 6 nodes
  ASSERT_EQ(g_call_profiling_time[MSPROF_REPORTER_HASH] > 0, true);
  int32_t current_profiling_time = g_call_profiling_time[MSPROF_REPORTER_HASH];
  // transdata1, matmul, transdata2 each has 5 types report
  ASSERT_EQ(g_call_profiling_time[MSPROF_REPORTER_REPORT] != 0, true);
  const auto last_call_profiling_time = g_call_profiling_time[MSPROF_REPORTER_REPORT];
  ProfilingManager::Instance().ProfStopProfiling(kAllSwitchOn, config_para);
  ProfilingProperties::Instance().SetLoadProfiling(false);
  ASSERT_EQ(ge::GeExecutor::ExecuteAsync(singleOp, input_desc, input_buffers, output_desc, output_buffers), SUCCESS);
  // stop profiling, num not increase
  ASSERT_EQ(g_call_profiling_time[MSPROF_REPORTER_HASH], current_profiling_time);
  ASSERT_EQ(g_call_profiling_time[MSPROF_REPORTER_REPORT] == last_call_profiling_time, true);
}

TEST_F(SingleOpProfilingSt, HybridSingeOpGraphProfilingExecThenStart) {
  profiling::ProfilingContext::GetInstance().SetEnable();
  ClearMsprofReportData();
  BenchEnv::Init();
  uint8_t model_data[8192];
  ge::ModelData modelData{.model_data = model_data};
  ModelDataBuilder(modelData).AddGraph(GraphFactory::HybridSingeOpGraph()).AddTask(2, 2)
      .AddTask(2, 4)
      .AddTask(2, 4)
      .AddTask(2, 5)
      .AddTask(2, 5)
      .Build();

  auto input_desc = TensorDescs(2).Value();
  auto input_buffers = DataBuffers(2).Value();
  auto output_desc = TensorDescs(1).Value();
  auto output_buffers = DataBuffers(1).Value();

  ProfilingProperties::Instance().SetLoadProfiling(true);
  ge::DynamicSingleOp *singleOp = nullptr;
  const std::string kConfigNumsdev = "devNums";
  const std::string kConfigDevIdList = "devIdList";
  const uint64_t kAllSwitchOn = 0xffffffffffffffffULL;
  const string &key = kConfigNumsdev;
  const string &val = "1";
  std::map<std::string, std::string> config_para;
  config_para.insert(std::pair<string, string>{key, val});
  config_para.insert(std::pair<string, string>{kConfigDevIdList, "0"});
  // first start profiling and second execute single op
  ProfilingManager::Instance().SetMsprofReporterCallback(MsprofReportCallback);
  // check switch before profiling start
  ASSERT_EQ(ProfilingProperties::Instance().IsDynamicShapeProfiling(), false);
  ASSERT_EQ(ge::GeExecutor::LoadDynamicSingleOpV2("dynamic_op", modelData, nullptr, &singleOp, 4), SUCCESS);
  ASSERT_EQ(singleOp->ExecuteAsync(input_desc, input_buffers, output_desc, output_buffers), SUCCESS);
  ProfilingData().HasRecord(kInferShape).HasRecord(kUpdateShape).HasRecord(kTiling).HasRecord(kRtKernelLaunch);
  ASSERT_EQ(g_call_profiling_time[MSPROF_REPORTER_REPORT], 6);
  ASSERT_EQ(g_call_profiling_time[MSPROF_REPORTER_HASH], 0);
  ProfilingManager::Instance().ProfStartProfiling(kAllSwitchOn, config_para);
  // transdata1, matmul, transdata2, data1, data2, net_output has register 6 nodes
  ASSERT_EQ(g_call_profiling_time[MSPROF_REPORTER_HASH] > 0, true);
  // start profiling can not report data
  ASSERT_EQ(g_call_profiling_time[MSPROF_REPORTER_REPORT], 6);
}
}
