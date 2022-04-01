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
#include <gmock/gmock.h>
#include <thread>
#include "common/ge_inner_attrs.h"

#define protected public
#define private public
#include "executor/engine_daemon.h"
#undef private
#undef protected

#include "graph/build/graph_builder.h"
#include "ge/ge_api.h"
#include "runtime/rt.h"
#include "depends/runtime/src/runtime_stub.h"

using namespace std;
using namespace ::testing;

namespace ge {
namespace {
class MockRuntime : public RuntimeStub {
 public:
  MOCK_METHOD5(rtEschedWaitEvent, int32_t(int32_t, uint32_t, uint32_t, int32_t, rtEschedEventSummary_t * ));
};
}  // namespace
class EngineDaemonTest : public testing::Test {
 protected:
  void SetUp() override {
  }
  void TearDown() override {
    RuntimeStub::Reset();
  }
};

TEST_F(EngineDaemonTest, TestEngineDaemon) {
  EngineDaemon engine_daemon;
  auto device_id = std::to_string(0);
  auto queue_id = std::to_string(0);
  auto event_group_id = std::to_string(1);
  const std::string process_name = "npu_executor";
  const char_t *argv[] = {
      process_name.c_str(),
      "BufferGroupName",
      queue_id.c_str(),
      device_id.c_str(),
      event_group_id.c_str(),
  };
  EXPECT_EQ(engine_daemon.InitializeWithArgs(5, (char **)argv), SUCCESS);
  engine_daemon.Finalize();
}

TEST_F(EngineDaemonTest, TestEngineDaemonWaitEvent) {
  EngineDaemon engine_daemon;
  ExecutorEventType event_type;
  bool is_timeout = false;
  auto mock_runtime = std::make_shared<MockRuntime>();
  RuntimeStub::SetInstance(mock_runtime);
  EXPECT_CALL(*mock_runtime, rtEschedWaitEvent).
      WillOnce(Return(ACL_ERROR_RT_REPORT_TIMEOUT)).
      WillOnce(Return(RT_FAILED)).
      WillRepeatedly(Return(SUCCESS));
  EXPECT_EQ(engine_daemon.WaitEvent(event_type, is_timeout), SUCCESS);
  EXPECT_TRUE(is_timeout);
  EXPECT_EQ(engine_daemon.WaitEvent(event_type, is_timeout), FAILED);
  EXPECT_EQ(engine_daemon.WaitEvent(event_type, is_timeout), SUCCESS);
  EXPECT_FALSE(is_timeout);
}

TEST_F(EngineDaemonTest, TestInitProfilingFromOption) {
  EngineDaemon engine_daemon;
  std::map<std::string, std::string> options;
  options[kProfilingDeviceConfigData] = "test1";
  auto ret = engine_daemon.InitProfilingFromOption(options);
  EXPECT_EQ(ret, SUCCESS);
  options[kProfilingIsExecuteOn] = "1";
  ret = engine_daemon.InitProfilingFromOption(options);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(EngineDaemonTest, TestInitDumpFromOption) {
  EngineDaemon engine_daemon;
  std::map<std::string, std::string> options;
  options[OPTION_EXEC_ENABLE_DUMP] = "1";
  auto ret = engine_daemon.InitDumpFromOption(options);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(EngineDaemonTest, TestFinalizeMaintenance) {
  EngineDaemon engine_daemon;
  engine_daemon.is_dump_inited_ = true;
  auto ret = engine_daemon.FinalizeMaintenance();
  EXPECT_EQ(ret, SUCCESS);
}

}  // namespace ge


