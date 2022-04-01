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

#define private public
#define protected public
#include "common/dump/data_dumper.h"
#include "graph/load/model_manager/davinci_model.h"
#undef private
#undef protected

using namespace std;

namespace ge {
class UtestDataDumper : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

std::vector<void *> stub_get_output_addrs(const RuntimeParam &model_param, ConstOpDescPtr op_desc) {
  std::vector<void *> res;
  res.emplace_back(reinterpret_cast<void *>(23333));
  return res;
}

static ge::OpDescPtr CreateOpDesc(string name = "", string type = "") {
  auto op_desc = std::make_shared<ge::OpDesc>(name, type);
  op_desc->SetStreamId(0);
  op_desc->SetId(0);

  op_desc->SetWorkspace({});
  op_desc->SetWorkspaceBytes({});
  op_desc->SetInputOffset({100, 200});
  op_desc->SetOutputOffset({100, 200});
  return op_desc;
}

TEST_F(UtestDataDumper, LoadDumpInfo_success) {
  RuntimeParam rts_param;
  DataDumper data_dumper(&rts_param);
  data_dumper.SetModelName("test");
  data_dumper.SetModelId(2333);
  std::shared_ptr<OpDesc> op_desc_1(new OpDesc());
  op_desc_1->AddOutputDesc("test", GeTensorDesc());
  data_dumper.SaveDumpTask(0, 0, op_desc_1, 0);
  string dump_mode = "output";
  data_dumper.is_op_debug_ = true;
  data_dumper.dump_properties_.SetDumpMode(dump_mode);
  EXPECT_EQ(data_dumper.LoadDumpInfo(), SUCCESS);
  data_dumper.UnloadDumpInfo();
}

TEST_F(UtestDataDumper, buildtask_success) {
  RuntimeParam rts_param;
  DataDumper data_dumper(&rts_param);
  data_dumper.SetModelName("test");
  data_dumper.SetModelId(2333);
  std::shared_ptr<OpDesc> op_desc_1(new OpDesc());
  op_desc_1->AddOutputDesc("test", GeTensorDesc());
  data_dumper.SaveDumpTask(0, 0, op_desc_1, 0);
  string dump_mode = "all";
  data_dumper.is_op_debug_ = true;
  data_dumper.dump_properties_.SetDumpMode(dump_mode);
  EXPECT_EQ(data_dumper.LoadDumpInfo(), SUCCESS);
  data_dumper.UnloadDumpInfo();
}

TEST_F(UtestDataDumper, BuildtaskInfo_success) {
  RuntimeParam rts_param;
  DataDumper data_dumper(&rts_param);
  data_dumper.SetModelName("test");
  data_dumper.SetModelId(2333);
  std::shared_ptr<OpDesc> op_desc_1(new OpDesc());
  data_dumper.SaveDumpTask(0, 0, op_desc_1, 0);
  string dump_mode = "input";
  data_dumper.is_op_debug_ = true;
  data_dumper.dump_properties_.SetDumpMode(dump_mode);
  EXPECT_EQ(data_dumper.LoadDumpInfo(), SUCCESS);
  data_dumper.UnloadDumpInfo();
}

TEST_F(UtestDataDumper, DumpOutputWithTask_success) {
  RuntimeParam rts_param;
  DataDumper data_dumper(&rts_param);
  data_dumper.SetModelName("test");
  data_dumper.SetModelId(2333);

  toolkit::aicpu::dump::Task task;
  OpDescPtr op_desc = CreateOpDesc("conv", CONVOLUTION);
  GeTensorDesc tensor_0(GeShape(), FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc tensor_1(GeShape(), FORMAT_NCHW, DT_FLOAT);
  int32_t calc_type = 1;
  ge::AttrUtils::SetInt(tensor_1, ATTR_NAME_MEMORY_SIZE_CALC_TYPE, calc_type);
  op_desc->AddOutputDesc(tensor_0);
  op_desc->AddOutputDesc(tensor_1);
  DataDumper::InnerDumpInfo inner_dump_info;
  inner_dump_info.op = op_desc;
  data_dumper.need_generate_op_buffer_ = true;
  Status ret = data_dumper.DumpOutputWithTask(inner_dump_info, task);
  EXPECT_EQ(ret, SUCCESS);
  int64_t task_size = 1;
  data_dumper.GenerateOpBuffer(task_size, task);
}

TEST_F(UtestDataDumper, DumpOutput_test) {
  ComputeGraphPtr graph = make_shared<ComputeGraph>("default");
  RuntimeParam rts_param;

  DataDumper data_dumper(&rts_param);
  data_dumper.SetModelName("test");
  data_dumper.SetModelId(11);
  data_dumper.SetDeviceId(11);
  data_dumper.SaveEndGraphId(0U, 0U);
  data_dumper.SetComputeGraph(graph);
  data_dumper.need_generate_op_buffer_ = true;
  data_dumper.dump_properties_.SetDumpMode(std::string("output"));
  EXPECT_EQ(data_dumper.LoadDumpInfo(), SUCCESS);

  Status retStatus;
  toolkit::aicpu::dump::Task task;
  DataDumper::InnerDumpInfo inner_dump_info;

  OpDescPtr op_desc1 = CreateOpDesc("conv", CONVOLUTION);
  GeTensorDesc tensor1(GeShape(), FORMAT_NCHW, DT_FLOAT);
  ge::AttrUtils::SetStr(tensor1, ATTR_DATA_DUMP_REF, "a");
  op_desc1->AddOutputDesc(tensor1);
  data_dumper.SaveDumpTask(0, 0, op_desc1, 0);
  graph->AddNode(op_desc1);
  inner_dump_info.op = op_desc1;
  retStatus = data_dumper.DumpOutputWithTask(inner_dump_info, task);
  EXPECT_EQ(retStatus, PARAM_INVALID);

  OpDescPtr op_desc2 = CreateOpDesc("conv", CONVOLUTION);
  GeTensorDesc tensor2(GeShape(), FORMAT_NCHW, DT_FLOAT);
  ge::AttrUtils::SetStr(op_desc2, ATTR_DATA_DUMP_REF, "a:b");
  op_desc2->AddOutputDesc(tensor2);
  data_dumper.SaveDumpTask(0, 0, op_desc2, 0);
  graph->AddNode(op_desc2);
  inner_dump_info.op = op_desc2;
  retStatus = data_dumper.DumpOutputWithTask(inner_dump_info, task);
  EXPECT_EQ(retStatus, SUCCESS);

  OpDescPtr op_desc3 = CreateOpDesc("conv", CONVOLUTION);
  GeTensorDesc tensor3(GeShape(), FORMAT_NCHW, DT_FLOAT);
  ge::AttrUtils::SetStr(tensor3, ATTR_DATA_DUMP_REF, "conv:output:1");
  op_desc3->AddOutputDesc(tensor3);
  data_dumper.SaveDumpTask(0, 0, op_desc3, 0);
  graph->AddNode(op_desc3);
  inner_dump_info.op = op_desc3;
  retStatus = data_dumper.DumpOutputWithTask(inner_dump_info, task);
  EXPECT_EQ(retStatus, PARAM_INVALID);
}

TEST_F(UtestDataDumper, DumpOutput_with_memlist) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  RuntimeParam rts_param;

  DataDumper data_dumper(&rts_param);
  data_dumper.SetModelName("test");
  data_dumper.SetModelId(11);
  data_dumper.SaveEndGraphId(0U, 0U);
  data_dumper.SetComputeGraph(graph);
  data_dumper.need_generate_op_buffer_ = true;
  data_dumper.dump_properties_.SetDumpMode(std::string("output"));

  Status retStatus;
  toolkit::aicpu::dump::Task task;
  DataDumper::InnerDumpInfo inner_dump_info;

  ge::OpDescPtr op_desc = CreateOpDesc("conv2", "conv2");
  std::vector<int64_t> in_memory_type_list = {static_cast<int64_t>(RT_MEMORY_L1)};
  std::vector<int64_t> out_memory_type_list = {static_cast<int64_t>(RT_MEMORY_L1)};
  (void)ge::AttrUtils::SetListInt(op_desc, ge::ATTR_NAME_INPUT_MEM_TYPE_LIST, in_memory_type_list);
  (void)ge::AttrUtils::SetListInt(op_desc, ge::ATTR_NAME_OUTPUT_MEM_TYPE_LIST, out_memory_type_list);
  GeTensorDesc tensor1(GeShape(), FORMAT_NCHW, DT_FLOAT);
  op_desc->AddOutputDesc(tensor1);

  data_dumper.SaveDumpTask(0, 0, op_desc, 0);
  graph->AddNode(op_desc);
  inner_dump_info.op = op_desc;
  int64_t *addr = (int64_t *)malloc(1024);
  inner_dump_info.args = reinterpret_cast<uintptr_t>(addr);
  retStatus = data_dumper.DumpOutputWithTask(inner_dump_info, task);
  EXPECT_EQ(retStatus, SUCCESS);

  free(addr);
}

TEST_F(UtestDataDumper, DumpInput_test) {
  ComputeGraphPtr graph = make_shared<ComputeGraph>("default");
  RuntimeParam rts_param;

  DataDumper data_dumper(&rts_param);
  data_dumper.DumpShrink();
  data_dumper.SetModelName("test");
  data_dumper.SetModelId(11);
  data_dumper.SaveEndGraphId(0U, 0U);
  data_dumper.SetComputeGraph(graph);
  data_dumper.need_generate_op_buffer_ = true;
  data_dumper.dump_properties_.SetDumpMode(std::string("input"));

  Status retStatus;
  toolkit::aicpu::dump::Task task;
  DataDumper::InnerDumpInfo inner_dump_info;

  OpDescPtr op_desc = CreateOpDesc("conv", CONVOLUTION);
  GeTensorDesc tensor(GeShape(), FORMAT_NCHW, DT_FLOAT);
  ge::AttrUtils::SetStr(tensor, ATTR_DATA_DUMP_REF, "conv:input:1");
  op_desc->AddInputDesc(tensor);
  data_dumper.SaveDumpTask(0, 0, op_desc, 0);
  graph->AddNode(op_desc);
  inner_dump_info.op = op_desc;
  retStatus = data_dumper.DumpInput(inner_dump_info, task);
  EXPECT_EQ(retStatus, PARAM_INVALID);

  toolkit::aicpu::dump::Input input;
  std::string node_name_index = "a";
  retStatus = data_dumper.DumpRefInput(inner_dump_info, input, 1, node_name_index);
  EXPECT_EQ(retStatus, PARAM_INVALID);

  node_name_index = "conv:input:1";
  retStatus = data_dumper.DumpRefInput(inner_dump_info, input, 1, node_name_index);
  EXPECT_EQ(retStatus, PARAM_INVALID);
}

TEST_F(UtestDataDumper, PrintCheckLog_test) {
  RuntimeParam rts_param;
  DataDumper data_dumper(&rts_param);

  DumpProperties dump_properties1;
  std::string dump_model(DUMP_ALL_MODEL);
  std::string dump_path("/");
  std::string dump_mode("output");
  std::set<std::string> dump_layers;

  std::set<std::string> model_list = data_dumper.GetDumpProperties().GetAllDumpModel();
  data_dumper.SetDumpProperties(dump_properties1);
  std::string key("all");
  data_dumper.PrintCheckLog(key);

  DumpProperties dump_properties2;
  dump_properties2.SetDumpMode(dump_mode);
  dump_properties2.AddPropertyValue(dump_model, dump_layers);
  dump_properties2.SetDumpPath(dump_path);
  data_dumper.SetDumpProperties(dump_properties2);
  data_dumper.PrintCheckLog(dump_model);
}

TEST_F(UtestDataDumper, SaveDumpInput_invalid) {
  RuntimeParam rts_param;
  DataDumper data_dumper(&rts_param);

  data_dumper.SaveDumpInput(nullptr);

  NodePtr op_node = std::make_shared<Node>(nullptr, nullptr);
  data_dumper.SaveDumpInput(op_node);
}


}  // namespace ge
