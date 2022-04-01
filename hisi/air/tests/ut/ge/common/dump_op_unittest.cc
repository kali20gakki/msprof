/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#define protected public
#define private public
#include "common/dump/dump_op.h"
#include "common/debug/log.h"
#include "common/ge_inner_error_codes.h"
#include "common/dump/dump_properties.h"
#include "graph/utils/attr_utils.h"
#include "graph/debug/ge_attr_define.h"
#undef private
#undef protected

namespace ge {
class UTEST_dump_op : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UTEST_dump_op, launch_dump_op_success) {
  DumpOp dump_op;
  DumpProperties dump_properties;
  OpDescPtr op_desc = std::make_shared<OpDesc>("GatherV2", "GatherV2");
  std::set<std::string> temp;
  dump_properties.model_dump_properties_map_.emplace("model1", temp);
  dump_properties.enable_dump_ = "1";
  dump_op.SetDynamicModelInfo("model1", "model2", 1);
  dump_op.SetDumpInfo(dump_properties, op_desc, {}, {}, nullptr);
  auto ret = dump_op.LaunchDumpOp();
  EXPECT_EQ(ret, ge::SUCCESS);
}

TEST_F(UTEST_dump_op, launch_dump_op_success_2) {
  DumpOp dump_op;
  DumpProperties dump_properties;
  OpDescPtr op_desc = std::make_shared<OpDesc>("GatherV2", "GatherV2");
  std::set<std::string> temp;
  dump_properties.model_dump_properties_map_.emplace("model1", temp);
  dump_properties.enable_dump_ = "1";
  dump_op.SetDynamicModelInfo("modle2", "model2", 1);
  dump_op.SetDumpInfo(dump_properties, op_desc, {}, {}, nullptr);
  auto ret = dump_op.LaunchDumpOp();
  EXPECT_EQ(ret, ge::SUCCESS);
}

TEST_F(UTEST_dump_op, launch_dump_op_output) {
  DumpOp dump_op;
  DumpProperties dump_properties;
  OpDescPtr op_desc = std::make_shared<OpDesc>("NetOutput", "NetOutput");
  GeTensorDesc output_tensor(GeShape(), ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
  op_desc->AddOutputDesc(output_tensor);

  std::set<std::string> temp;
  std::vector<uintptr_t> input_addrs;
  std::vector<uintptr_t> output_addrs;
  dump_properties.model_dump_properties_map_.emplace("model1", temp);
  dump_properties.enable_dump_ = "1";
  dump_properties.dump_mode_ = "output";
  dump_op.SetDynamicModelInfo("model1", "model2", 1);
  dump_op.SetDumpInfo(dump_properties, op_desc, input_addrs, output_addrs, nullptr);
  auto ret = dump_op.LaunchDumpOp();
  EXPECT_EQ(ret, ACL_ERROR_GE_INTERNAL_ERROR);
}

TEST_F(UTEST_dump_op, launch_dump_op_input) {
  DumpOp dump_op;
  DumpProperties dump_properties;

  OpDescPtr op_desc = std::make_shared<ge::OpDesc>("conv", "conv");
  op_desc->SetStreamId(0);
  op_desc->SetId(0);

  op_desc->SetWorkspace({});
  op_desc->SetWorkspaceBytes({});
  op_desc->SetInputOffset({100, 200});
  op_desc->SetOutputOffset({100, 200});

  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  ge::AttrUtils::SetStr(tensor, ATTR_DATA_DUMP_REF, "conv:input:1");
  op_desc->AddInputDesc(tensor);

  std::set<std::string> temp;
  dump_properties.model_dump_properties_map_.emplace("model1", temp);
  dump_properties.enable_dump_ = "1";
  dump_properties.dump_mode_ = "input";
  EXPECT_EQ(dump_op.global_step_, 0U);
  EXPECT_EQ(dump_op.loop_per_iter_, 0U);
  EXPECT_EQ(dump_op.loop_cond_, 0U);
  dump_op.SetLoopAddr(2U, 2U, 2U);
  EXPECT_EQ(dump_op.global_step_, 2U);
  EXPECT_EQ(dump_op.loop_per_iter_, 2U);
  EXPECT_EQ(dump_op.loop_cond_, 2U);
  dump_op.SetDynamicModelInfo("model1", "model2", 1);

  std::vector<uintptr_t> input_addrs;
  std::vector<uintptr_t> output_addrs;
  int64_t *addr_in = (int64_t *)malloc(1024);
  int64_t *addr_out = (int64_t *)malloc(1024);
  input_addrs.push_back(reinterpret_cast<uintptr_t>(addr_in));
  output_addrs.push_back(reinterpret_cast<uintptr_t>(addr_out));
  dump_op.SetDumpInfo(dump_properties, op_desc, input_addrs, output_addrs, nullptr);
  auto ret = dump_op.LaunchDumpOp();
  EXPECT_EQ(ret, ge::SUCCESS);

  free(addr_in);
  free(addr_out);
}

TEST_F(UTEST_dump_op, launch_dump_op_all) {
  DumpOp dump_op;
  DumpProperties dump_properties;
  OpDescPtr op_desc = std::make_shared<OpDesc>("GatherV2", "GatherV2");

  std::set<std::string> temp;
  dump_properties.model_dump_properties_map_.emplace("model1", temp);
  dump_properties.enable_dump_ = "1";
  dump_properties.dump_mode_ = "all";
  dump_op.SetDynamicModelInfo("model1", "model2", 1);
  dump_op.SetDumpInfo(dump_properties, op_desc, {}, {}, nullptr);
  auto ret = dump_op.LaunchDumpOp();
  EXPECT_EQ(ret, ge::SUCCESS);
}
}  // namespace ge