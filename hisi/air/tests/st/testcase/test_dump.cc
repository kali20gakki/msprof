/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#include "external/ge/ge_api.h"
#include "framework/executor/ge_executor.h"
#include "session/session_manager.h"
#include "common/dump/dump_manager.h"

using namespace std;
namespace ge {
class DumpTest : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
  static void BuildGraphWithDumpOptions(const std::map<std::string, std::string> &dump_options) {
    std::map<std::string, std::string> options;
    //  OPTION_EXEC_DUMP_STEP
    //OPTION_EXEC_DUMP_MODE
    EXPECT_EQ(GEInitialize(options), SUCCESS);

    std::map<std::string, std::string> session_options;
    session_options = dump_options;
    Session session(session_options);

    GraphId graph_id = 1;
    const auto compute_graph = MakeShared<ComputeGraph>("test_graph");
    Graph graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);

    EXPECT_EQ(session.AddGraph(graph_id, graph), SUCCESS);

    vector<Tensor> inputs;
    vector<InputTensorInfo> tensors;
    EXPECT_EQ(session.BuildGraph(graph_id, inputs), FAILED);

    EXPECT_EQ(session.RemoveGraph(graph_id), SUCCESS);
    EXPECT_EQ(GEFinalize(), SUCCESS);
  }
};

TEST_F(DumpTest, TestSetDumpStatusByOption) {
  std::map<std::string, std::string> dump_options;
  dump_options.emplace(OPTION_EXEC_ENABLE_DUMP, "1");
  dump_options.emplace(OPTION_EXEC_ENABLE_DUMP_DEBUG, "0");
  dump_options.emplace(OPTION_EXEC_DUMP_PATH, "./");
  dump_options.emplace(OPTION_EXEC_DUMP_STEP, "0|5|10-20");
  dump_options.emplace(OPTION_EXEC_DUMP_MODE, "all");
  BuildGraphWithDumpOptions(dump_options);
}

TEST_F(DumpTest, TestSetDumpDebugByOption) {
  std::map<std::string, std::string> dump_options;
  dump_options.emplace(OPTION_EXEC_ENABLE_DUMP_DEBUG, "1");
  dump_options.emplace(OPTION_EXEC_DUMP_PATH, "./");
  dump_options.emplace(OPTION_EXEC_DUMP_DEBUG_MODE, OP_DEBUG_AICORE); // OP_DEBUG_ATOMIC /  OP_DEBUG_ALL
  BuildGraphWithDumpOptions(dump_options);

  dump_options[OPTION_EXEC_DUMP_DEBUG_MODE] = OP_DEBUG_ATOMIC;
  BuildGraphWithDumpOptions(dump_options);

  dump_options[OPTION_EXEC_DUMP_DEBUG_MODE] = OP_DEBUG_ALL;
  BuildGraphWithDumpOptions(dump_options);
}

TEST_F(DumpTest, TestSetDumpStatusByCmd) {
  Command dump_command;
  dump_command.cmd_type = "dump";
  dump_command.cmd_params = {DUMP_STATUS};
  GeExecutor ge_executor;
  EXPECT_EQ(ge_executor.CommandHandle(dump_command), ACL_ERROR_GE_COMMAND_HANDLE);

  // cmd params saved as: { key1, val1, key2, val2, key3, val3 }
  dump_command.cmd_params = {DUMP_FILE_PATH, "./",
                             DUMP_STATUS, "on",
                             DUMP_MODEL, "om",
                             DUMP_MODE, "all",
                             DUMP_LAYER, "relu"};
  EXPECT_EQ(ge_executor.CommandHandle(dump_command), SUCCESS);

//  DumpManager::GetInstance().GetDumpProperties(kInferSessionId).IsSingleOpNeedDump();

  dump_command.cmd_params = {DUMP_STATUS, "off"};
  EXPECT_EQ(ge_executor.CommandHandle(dump_command), SUCCESS);
}

TEST_F(DumpTest, TestSetDumpStatusByApi) {
  GeExecutor ge_executor;
  {
    ge::DumpConfig dump_cfg;
    dump_cfg.dump_path = "./dump/";
    dump_cfg.dump_mode = "all";
    dump_cfg.dump_status = "on";
    dump_cfg.dump_op_switch = "on";
    EXPECT_EQ(ge_executor.SetDump(dump_cfg), SUCCESS);
    EXPECT_TRUE(DumpManager::GetInstance().GetDumpProperties(kInferSessionId).IsDumpOpen());
    EXPECT_FALSE(DumpManager::GetInstance().GetDumpProperties(kInferSessionId).IsOpDebugOpen());
  }

  {
    ge::DumpConfig dump_cfg;
    dump_cfg.dump_status = "off";
    dump_cfg.dump_debug = "off";
    EXPECT_EQ(ge_executor.SetDump(dump_cfg), SUCCESS);
    EXPECT_FALSE(DumpManager::GetInstance().GetDumpProperties(kInferSessionId).IsDumpOpen());
  }
}

TEST_F(DumpTest, TestSetDumpDebugByApi) {
  GeExecutor ge_executor;
  {
    ge::DumpConfig dump_cfg;
    dump_cfg.dump_path = "./dump/";
    dump_cfg.dump_mode = "all";
    dump_cfg.dump_debug = "on";
    EXPECT_EQ(ge_executor.SetDump(dump_cfg), SUCCESS);
    EXPECT_FALSE(DumpManager::GetInstance().GetDumpProperties(kInferSessionId).IsDumpOpen());
    EXPECT_TRUE(DumpManager::GetInstance().GetDumpProperties(kInferSessionId).IsOpDebugOpen());
    EXPECT_EQ(DumpManager::GetInstance().GetDumpProperties(kInferSessionId).GetOpDebugMode(), 3);
  }

  {
    ge::DumpConfig dump_cfg;
    EXPECT_EQ(ge_executor.SetDump(dump_cfg), SUCCESS);
    EXPECT_FALSE(DumpManager::GetInstance().GetDumpProperties(kInferSessionId).IsDumpOpen());
  }
}

TEST_F(DumpTest, TestGetDumpCfgByApi) {
  {
    DumpConfig dump_config;
    std::map<std::string, std::string> options;
    options[OPTION_EXEC_ENABLE_DUMP] = "1";
    options[OPTION_EXEC_DUMP_MODE] = "all";
    bool ret = DumpManager::GetCfgFromOption(options, dump_config);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(dump_config.dump_status, "device_on");
    EXPECT_EQ(dump_config.dump_mode, "all");
  }
}
}  // namespace ge


