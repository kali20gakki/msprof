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
#include "generator/ge_generator.h"
#include "graph/utils/tensor_utils.h"
#include "graph/attr_value.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph/operator_factory_impl.h"
#include "graph/passes/graph_builder_utils.h"
#include "graph/manager/graph_manager.h"
#include "all_ops.h"
#include "init/gelib.h"
#include "opskernel_manager/ops_kernel_builder_manager.h"
#include "register/ops_kernel_builder_registry.h"
#include "opskernel_manager/ops_kernel_manager.h"
using namespace std;

namespace ge {
const char *const kKernelLibName = "DNN_VM_GE_LOCAL";
class UtestGeGenerator : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}

  class FakeOpsKernelInfoStore : public OpsKernelInfoStore {
   public:
    FakeOpsKernelInfoStore(){supported_ = true;};
    bool supported_;

   private:
    Status Initialize(const std::map<std::string, std::string> &options) override {
      return SUCCESS;
    };
    Status Finalize() override {
      return SUCCESS;
    };
    bool CheckSupported(const OpDescPtr &op_desc, std::string &reason) const override {
      return supported_;
    };
    void GetAllOpsKernelInfo(std::map<std::string, ge::OpInfo> &infos) const override {};
  };

  class FakeOpsKernelBuilder : public OpsKernelBuilder {
   public:
    FakeOpsKernelBuilder() = default;
   private:
    Status Initialize(const map<std::string, std::string> &options) override {
      return SUCCESS;
    };
    Status Finalize() override {
      return SUCCESS;
    };
    Status CalcOpRunningParam(Node &node) override {
      return SUCCESS;
    };
    Status GenerateTask(const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) override {
      domi::TaskDef task_def;
      tasks.push_back(task_def);
      return SUCCESS;
    };
  };

  void InitGeLib() {
    map<string, string> options;
    Status ret = ge::GELib::Initialize(options);
    EXPECT_EQ(ret, SUCCESS);
    auto instance_ptr = ge::GELib::GetInstance();
    EXPECT_NE(instance_ptr, nullptr);

    //  SchedulerConf conf;
    SchedulerConf scheduler_conf;
    scheduler_conf.name = kKernelLibName;
    scheduler_conf.cal_engines[kKernelLibName] = std::make_shared<EngineConf>();
    scheduler_conf.cal_engines[kKernelLibName]->name = kKernelLibName;
    scheduler_conf.cal_engines[kKernelLibName]->scheduler_id = kKernelLibName;
    map<string, SchedulerConf> scheduler_confs;
    scheduler_confs["scheduler"] = scheduler_conf;
    instance_ptr->DNNEngineManagerObj().schedulers_[kKernelLibName] = scheduler_conf;

    OpsKernelInfoStorePtr ops_kernel_info_store_ptr = std::make_shared<FakeOpsKernelInfoStore>();
    OpsKernelManager::GetInstance().ops_kernel_store_.emplace(kKernelLibName, ops_kernel_info_store_ptr);
    OpsKernelBuilderPtr fake_builder = std::make_shared<FakeOpsKernelBuilder>();
    OpsKernelBuilderRegistry::GetInstance().kernel_builders_[kKernelLibName] = fake_builder;
    OpInfo op_info;
    op_info.engine = kKernelLibName;
    op_info.opKernelLib = kKernelLibName;
    OpsKernelManager &ops_kernel_manager = instance_ptr->OpsKernelManagerObj();
    ops_kernel_manager.ops_kernel_info_[DATA].emplace_back(op_info);
    ops_kernel_manager.ops_kernel_info_[ADD].emplace_back(op_info);
    ops_kernel_manager.ops_kernel_info_[NETOUTPUT].emplace_back(op_info);
  }

  void FinalizeGeLib() {
    auto instance_ptr = ge::GELib::GetInstance();
    if (instance_ptr != nullptr) {
      instance_ptr->Finalize();
    }
  }
};

namespace {
ComputeGraphPtr MakeGraph() {
  ge::ut::GraphBuilder builder("graph");
  auto data = builder.AddNode("data", "Data", 1, 1);
  auto addn1 = builder.AddNode("addn1", "AddN", 1, 1);
  builder.AddDataEdge(data, 0, addn1, 0);
  return builder.GetGraph();
}

static NamedAttrs CreateNamedAttrs(const string &name, std::map<string, GeAttrValue> map) {
  NamedAttrs named_attrs;
  named_attrs.SetName(name);
  for (auto it : map) {
    named_attrs.SetAttr(it.first, it.second);
  }
  return named_attrs;
}
}  // namespace

/*
TEST_F(UtestGeGenerator, test_build_single_op_offline) {
  GeTensorDesc tensor_desc(GeShape(), FORMAT_NCHW, DT_FLOAT);
  TensorUtils::SetSize(tensor_desc, 512);

  shared_ptr<OpDesc> op_desc = make_shared<OpDesc>("Add", "add");
  EXPECT_EQ(op_desc->AddInputDesc(tensor_desc), GRAPH_SUCCESS);
  EXPECT_EQ(op_desc->AddInputDesc(tensor_desc), GRAPH_SUCCESS);
  EXPECT_EQ(op_desc->AddOutputDesc(tensor_desc), GRAPH_SUCCESS);

  GeTensor tensor(tensor_desc);
  const vector<GeTensor> inputs = { tensor, tensor };
  const vector<GeTensor> outputs = { tensor };

  // not Initialize, impl is null.
  GeGenerator generator;
  EXPECT_EQ(generator.BuildSingleOpModel(op_desc, inputs, outputs, "offline_"), PARAM_INVALID);

  // const map<string, string> &options
  generator.Initialize({});
  EXPECT_EQ(generator.BuildSingleOpModel(op_desc, inputs, outputs, "offline_"), GE_GENERATOR_GRAPH_MANAGER_BUILD_GRAPH_FAILED);
}
*/

graphStatus TestFunc(Operator &op) { return 0; }
graphStatus TestFunc1(Operator &op) { return 1; }
TEST_F(UtestGeGenerator, test_infer_format_for_single_op) {
  ComputeGraphPtr compute_graph = MakeShared<ComputeGraph>("graph_name"); 
  auto graph = GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  OperatorFactoryImpl::RegisterInferFormatFunc("Add", TestFunc);
  shared_ptr<OpDesc> op_desc = std::make_shared<OpDesc>("add", "add");
  compute_graph->AddNode(op_desc);
  GeGenerator generator;
  EXPECT_EQ(generator.InferFormatForSingleOp(op_desc, graph), SUCCESS);
  shared_ptr<OpDesc> op_desc1 = std::make_shared<OpDesc>("Add", "Add");
  compute_graph->AddNode(op_desc1);
  EXPECT_EQ(generator.InferFormatForSingleOp(op_desc1, graph), SUCCESS);
  OperatorFactoryImpl::RegisterInferFormatFunc("MatMulV2", TestFunc1);
  shared_ptr<OpDesc> op_desc2 = std::make_shared<OpDesc>("MatMulV2", "MatMulV2");
  GeTensorDesc tensor_desc;
  EXPECT_EQ(op_desc2->AddInputDesc(tensor_desc), GRAPH_SUCCESS);
  EXPECT_EQ(op_desc2->AddInputDesc(tensor_desc), GRAPH_SUCCESS);
  EXPECT_EQ(op_desc2->AddInputDesc(tensor_desc), GRAPH_SUCCESS);
  EXPECT_EQ(op_desc2->AddInputDesc(tensor_desc), GRAPH_SUCCESS);
  EXPECT_EQ(op_desc2->AddInputDesc(tensor_desc), GRAPH_SUCCESS);
  EXPECT_EQ(op_desc2->AddOutputDesc(tensor_desc), GRAPH_SUCCESS);
  EXPECT_EQ(op_desc2->AddOutputDesc(tensor_desc), GRAPH_SUCCESS);
  compute_graph->AddNode(op_desc2);
  EXPECT_EQ(generator.InferFormatForSingleOp(op_desc2, graph), FAILED);
}

TEST_F(UtestGeGenerator, test_build_single_op_online) {
  GeTensorDesc tensor_desc;
  shared_ptr<OpDesc> op_desc = std::make_shared<OpDesc>("Add", "add");
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddOutputDesc(tensor_desc);

  GeTensor tensor(tensor_desc);
  const vector<GeTensor> inputs = { tensor, tensor };
  const vector<GeTensor> outputs = { tensor };

  GeGenerator generator;
  generator.Initialize({});
  ModelBufferData model_buffer;
  EXPECT_EQ(generator.BuildSingleOpModel(op_desc, inputs, outputs, ENGINE_AIVECTOR, false, model_buffer), FAILED);
  const vector<GeTensor> inputs1;
  EXPECT_EQ(generator.BuildSingleOpModel(op_desc, inputs1, outputs, ENGINE_AIVECTOR, false, model_buffer), FAILED);
}

TEST_F(UtestGeGenerator, test_check_aicore) {
  GeGenerator generator;
  generator.Initialize({});
  auto graph = MakeGraph();
  EXPECT_EQ(generator.CheckNoAicore(graph), true);
}

TEST_F(UtestGeGenerator, test_graph_manager) {
  GraphManager graph_manager;
  GraphPartitioner graph_partitioner;

  auto root_graph = MakeGraph();
  auto sub_graph = MakeGraph();
  root_graph->AddSubGraph(sub_graph);

  auto sgi = MakeShared<SubGraphInfo>();
  // set engine name
  sgi->SetEngineName("AIcoreEngine");
  sgi->SetSubGraph(sub_graph);

  auto sgi_gelocal = MakeShared<SubGraphInfo>();
  // set engine name
  sgi_gelocal->SetEngineName("GELOCAL");
  sgi_gelocal->SetSubGraph(sub_graph);

  graph_partitioner.graph_2_input_subgraph_[root_graph] = sgi_gelocal;
  graph_partitioner.graph_2_subgraph_list_.insert({root_graph, {sgi, sgi_gelocal}});
  graph_partitioner.graph_2_subgraph_list_.insert({sub_graph, {sgi, sgi_gelocal}});
  EXPECT_EQ(graph_manager.ConvertGraphToFile(root_graph, graph_partitioner, "./"), GRAPH_SUCCESS);
}

TEST_F(UtestGeGenerator, test_set_model_name) {
  GeGenerator generator;
  generator.Initialize({});
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>(ComputeGraph("graph"));
  (void)AttrUtils::SetBool(graph, "_dynamic_shape_partitioned", true);
  ge_root_model->root_graph_ = std::move(graph);
  EXPECT_EQ(generator.SetModelNameForDump(ge_root_model), SUCCESS);
}

TEST_F(UtestGeGenerator, test_remove_const) {
  GeGenerator generator;
  GeTensorDesc tensor_desc;
  GeTensor tensor(tensor_desc);
  const vector<GeTensor> inputs = {tensor};
  vector<GeTensor> outputs;
  generator.RemoveConst(inputs, outputs);
}

TEST_F(UtestGeGenerator, test_generate_online_model) {
  GeTensorDesc tensor_desc;
  GeTensor tensor(tensor_desc);
  const vector<GeTensor> inputs = { tensor, tensor };
  auto compute_graph = MakeGraph();
  compute_graph->TopologicalSorting();
  Graph graph = ge::GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  GeGenerator generator;
  generator.Initialize({});
  std::string name;
  EXPECT_NE(generator.GenerateOfflineModel(graph, name, inputs), SUCCESS);
}

TEST_F(UtestGeGenerator, test_create_generalized_build_attrs) {
  GeGenerator generator;
  auto ret = generator.Initialize({});
  ASSERT_EQ(ret, SUCCESS);
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>();
  ge_root_model->root_graph_ = MakeGraph();
  NodePtr data_node = ge_root_model->root_graph_->FindNode("data");
  ASSERT_NE(data_node, nullptr);
  ASSERT_NE(data_node->GetOpDesc(), nullptr);
  auto in_desc = data_node->GetOpDesc()->MutableInputDesc(0);

  GeTensorDesc tensor_desc(GeShape({1, 2}));
  GeTensor tensor(tensor_desc);

  // 1. input shape all known
  {
    in_desc->SetShape(GeShape({1, 2}));
    in_desc->SetOriginShape(GeShape({1, 2}));
    const vector<GeTensor> inputs = {tensor, tensor};
    const vector<GeTensor> outputs = {tensor};
    const vector<pair<string, string>> inputs_name_type = {{"data", DATA}, {"", CONSTANT}};
    std::vector<NamedAttrs> generalized_attrs;
    ret = generator.CreateGeneralizedBuildAttrs(ge_root_model, inputs, outputs, inputs_name_type, generalized_attrs);
    EXPECT_EQ(ret, SUCCESS);
    EXPECT_EQ(generalized_attrs.size(), 0);
  }

  // 2. input and output are empty
  {
    const vector<GeTensor> inputs;
    const vector<GeTensor> outputs;
    const vector<pair<string, string>> inputs_name_type = {{"data", DATA}};
    std::vector<NamedAttrs> generalized_attrs;
    in_desc->SetShape(GeShape({-1, -1}));
    in_desc->SetOriginShape(GeShape({-1, -1}));
    in_desc->SetShapeRange({{1, -1}, {1, -1}});
    in_desc->SetOriginShapeRange({{1, -1}, {1, -1}});
    ret = generator.CreateGeneralizedBuildAttrs(ge_root_model, inputs, outputs, inputs_name_type, generalized_attrs);
    EXPECT_EQ(ret, SUCCESS);
    EXPECT_EQ(generalized_attrs.size(), 1);
    std::vector<NamedAttrs> input_res_attrs;
    AttrUtils::GetListNamedAttrs(generalized_attrs.at(0), ATTR_NAME_FUZZ_INPUTS_SUPPORTED_ATTRS, input_res_attrs);
    EXPECT_EQ(input_res_attrs.size(), 1);
    std::vector<NamedAttrs> output_res_attrs;
    AttrUtils::GetListNamedAttrs(generalized_attrs.at(0), ATTR_NAME_FUZZ_OUTPUTS_SUPPORTED_ATTRS, output_res_attrs);
    EXPECT_EQ(output_res_attrs.size(), 0);
    std::vector<NamedAttrs> tensors;
    AttrUtils::GetListNamedAttrs(input_res_attrs.at(0), "tensor", tensors);
    EXPECT_EQ(tensors.size(), 1);
    std::vector<int64_t> shape;
    ret = tensors.at(0).GetItem("shape").GetValue<std::vector<int64_t>>(shape);
    EXPECT_EQ(ret, SUCCESS);
    EXPECT_EQ(shape, std::vector<int64_t>({-1, -1}));
  }

  // 3. normal
  {
    const vector<GeTensor> inputs = {tensor};
    const vector<GeTensor> outputs = {tensor};
    const vector<pair<string, string>> inputs_name_type = {{"data", DATA}};
    std::vector<NamedAttrs> generalized_attrs;
    in_desc->SetShape(GeShape({-1, -1}));
    in_desc->SetOriginShape(GeShape({-1, -1}));
    in_desc->SetShapeRange({{1, -1}, {1, -1}});
    in_desc->SetOriginShapeRange({{1, -1}, {1, -1}});
    ret = generator.CreateGeneralizedBuildAttrs(ge_root_model, inputs, outputs, inputs_name_type, generalized_attrs);
    EXPECT_EQ(ret, SUCCESS);
    EXPECT_EQ(generalized_attrs.size(), 1);
    // check input res attrs
    {
      std::vector<NamedAttrs> input_res_attrs;
      AttrUtils::GetListNamedAttrs(generalized_attrs.at(0), ATTR_NAME_FUZZ_INPUTS_SUPPORTED_ATTRS, input_res_attrs);
      EXPECT_EQ(input_res_attrs.size(), 1);
      std::vector<NamedAttrs> tensors;
      AttrUtils::GetListNamedAttrs(input_res_attrs.at(0), "tensor", tensors);
      EXPECT_EQ(tensors.size(), 1);
      std::vector<int64_t> shape;
      ret = tensors.at(0).GetItem("shape").GetValue<std::vector<int64_t>>(shape);
      EXPECT_EQ(ret, SUCCESS);
      EXPECT_EQ(shape, std::vector<int64_t>({-1, -1}));
      std::vector<std::vector<int64_t>> shape_range;
      ret = tensors.at(0).GetItem("shapeRange").GetValue<std::vector<std::vector<int64_t>>>(shape_range);
      EXPECT_EQ(ret, SUCCESS);
      EXPECT_EQ(shape_range, std::vector<std::vector<int64_t>>({{1, -1}, {1, -1}}));
    }

    // check output res attrs
    {
      GeAttrValue::LIST_NAMED_ATTRS output_res_attrs;
      AttrUtils::GetListNamedAttrs(generalized_attrs.at(0), ATTR_NAME_FUZZ_OUTPUTS_SUPPORTED_ATTRS, output_res_attrs);
      EXPECT_EQ(output_res_attrs.size(), 1);
      GeAttrValue::LIST_NAMED_ATTRS tensors;
      AttrUtils::GetListNamedAttrs(output_res_attrs.at(0), "tensor", tensors);
      EXPECT_EQ(tensors.size(), 1);
      GeAttrValue::LIST_INT shape;
      ret = tensors.at(0).GetItem("shape").GetValue<GeAttrValue::LIST_INT>(shape);
      EXPECT_EQ(ret, SUCCESS);
      EXPECT_EQ(shape, GeAttrValue::LIST_INT({-2}));
    }
  }

  // 4. normal with value
  {
    const vector<GeTensor> inputs = {tensor};
    const vector<GeTensor> outputs = {tensor};
    const vector<pair<string, string>> inputs_name_type = {{"data", DATA}};
    std::vector<NamedAttrs> generalized_attrs;
    in_desc->SetShape(GeShape({-1, -1}));
    in_desc->SetOriginShape(GeShape({-1, -1}));
    in_desc->SetShapeRange({{1, -1}, {1, -1}});
    in_desc->SetOriginShapeRange({{1, -1}, {1, -1}});
    AttrUtils::SetBool(in_desc, ATTR_NAME_VALUE_DEPEND, true);
    AttrUtils::SetTensor(in_desc, ATTR_NAME_VALUE, tensor);
    ret = generator.CreateGeneralizedBuildAttrs(ge_root_model, inputs, outputs, inputs_name_type, generalized_attrs);
    EXPECT_EQ(ret, SUCCESS);
    EXPECT_EQ(generalized_attrs.size(), 1);
    std::vector<NamedAttrs> input_res_attrs;
    AttrUtils::GetListNamedAttrs(generalized_attrs.at(0), ATTR_NAME_FUZZ_INPUTS_SUPPORTED_ATTRS, input_res_attrs);
    EXPECT_EQ(input_res_attrs.size(), 1);
    std::vector<NamedAttrs> tensors;
    AttrUtils::GetListNamedAttrs(input_res_attrs.at(0), "tensor", tensors);
    EXPECT_EQ(tensors.size(), 1);
    bool has_value = AttrUtils::HasAttr(tensors.at(0), "value");
    EXPECT_EQ(has_value, true);
  }

  // 5. normal with value range
  {
    const vector<GeTensor> inputs = {tensor};
    const vector<GeTensor> outputs = {tensor};
    const vector<pair<string, string>> inputs_name_type = {{"data", DATA}};
    std::vector<NamedAttrs> generalized_attrs;
    in_desc->SetShape(GeShape({-1, -1}));
    in_desc->SetOriginShape(GeShape({-1, -1}));
    in_desc->SetShapeRange({{1, -1}, {1, -1}});
    in_desc->SetOriginShapeRange({{1, -1}, {1, -1}});
    in_desc->SetValueRange({{1, 256}, {1, 256}});
    ret = generator.CreateGeneralizedBuildAttrs(ge_root_model, inputs, outputs, inputs_name_type, generalized_attrs);
    EXPECT_EQ(ret, SUCCESS);
    EXPECT_EQ(generalized_attrs.size(), 1);
    std::vector<NamedAttrs> input_res_attrs;
    AttrUtils::GetListNamedAttrs(generalized_attrs.at(0), ATTR_NAME_FUZZ_INPUTS_SUPPORTED_ATTRS, input_res_attrs);
    EXPECT_EQ(input_res_attrs.size(), 1);
    std::vector<NamedAttrs> tensors;
    AttrUtils::GetListNamedAttrs(input_res_attrs.at(0), "tensor", tensors);
    EXPECT_EQ(tensors.size(), 1);
    std::vector<std::vector<int64_t>> value_range;
    ret = tensors.at(0).GetItem("value_range").GetValue<std::vector<std::vector<int64_t>>>(value_range);
    EXPECT_EQ(ret, SUCCESS);
    EXPECT_EQ(value_range, std::vector<std::vector<int64_t>>({{1, 256}, {1, 256}}));
  }
}

TEST_F(UtestGeGenerator, test_create_generalized_build_attrs_failed) {
  GeGenerator generator;
  auto ret = generator.Initialize({});
  ASSERT_EQ(ret, SUCCESS);
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>();
  ge_root_model->root_graph_ = MakeGraph();
  NodePtr data_node = ge_root_model->root_graph_->FindNode("data");
  ASSERT_NE(data_node, nullptr);
  ASSERT_NE(data_node->GetOpDesc(), nullptr);
  auto in_desc = data_node->GetOpDesc()->MutableInputDesc(0);

  GeTensorDesc tensor_desc(GeShape({1, 2}));
  GeTensor tensor(tensor_desc);

  // 1. input size is not same with input nodes
  {
    const vector<GeTensor> inputs = {tensor, tensor, tensor};
    const vector<GeTensor> outputs = {tensor};
    const vector<pair<string, string>> inputs_name_type = {{"data", DATA}, {"", CONSTANT}};
    std::vector<NamedAttrs> generalized_attrs;
    ret = generator.CreateGeneralizedBuildAttrs(ge_root_model, inputs, outputs, inputs_name_type, generalized_attrs);
    EXPECT_EQ(ret, INTERNAL_ERROR);
  }

  // 2. missing data node
  {
    const vector<GeTensor> inputs = {tensor};
    const vector<GeTensor> outputs = {tensor};
    const vector<pair<string, string>> inputs_name_type = {{"data_missing", DATA}};
    std::vector<NamedAttrs> generalized_attrs;
    ret = generator.CreateGeneralizedBuildAttrs(ge_root_model, inputs, outputs, inputs_name_type, generalized_attrs);
    EXPECT_EQ(ret, INTERNAL_ERROR);
  }
}

TEST_F(UtestGeGenerator, test_build_single_op_online_success) {
  InitGeLib();
  GeTensorDesc tensor_desc;
  shared_ptr<OpDesc> op_desc = std::make_shared<OpDesc>("Add", "Add");
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddOutputDesc(tensor_desc);

  GeTensor tensor(tensor_desc);
  const vector<GeTensor> inputs = { tensor, tensor };
  const vector<GeTensor> outputs = { tensor };

  GeGenerator generator;
  generator.Initialize({});
  ModelBufferData model_buffer;
  Status ret = generator.BuildSingleOpModel(op_desc, inputs, outputs, "file_name", false);

  AttrUtils::SetBool(op_desc, "_AllShape", true);
  generator.BuildSingleOpModel(op_desc, inputs, outputs, "file_name", false);
  FinalizeGeLib();
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGeGenerator, test_get_single_op_build_stage_graph_success) {
  InitGeLib();
  GeTensorDesc tensor_desc;
  shared_ptr<OpDesc> op_desc = std::make_shared<OpDesc>("Add", "Add");
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddOutputDesc(tensor_desc);

  GeTensor tensor(tensor_desc);
  const vector<GeTensor> inputs = { tensor, tensor };
  const vector<GeTensor> outputs = { tensor };

  GeGenerator generator;
  generator.Initialize({});
  ModelBufferData model_buffer;
  ComputeGraphPtr compute_graph = nullptr;

  Status ret = generator.BuildSingleOpModel(op_desc, inputs, outputs, ENGINE_SYS, false, model_buffer,
                                            GraphStage::GRAPH_STAGE_FUZZ, compute_graph);
  EXPECT_EQ(ret, SUCCESS);
  int64_t graph_stage = static_cast<int64_t>(GraphStage::GRAPH_STAGE_RESERVED);
  bool graph_has_been_added = false; 
  // test attr has been cleared
  EXPECT_EQ(AttrUtils::GetInt(compute_graph, kGraphDumpStage, graph_stage), false);
  EXPECT_EQ(AttrUtils::GetBool(compute_graph, ATTR_NAME_GRAPH_HAS_BEEN_ADDED, graph_has_been_added), false);

  FinalizeGeLib();
}

TEST_F(UtestGeGenerator, GenerateInfershapeGraphNull) {
  auto &instance = GeGenerator::GetInstance();
  Graph graph("graph");
  EXPECT_EQ(instance.GenerateInfershapeGraph(graph), PARAM_INVALID);
}

TEST_F(UtestGeGenerator, GenerateInfershapeGraph) {
  auto &instance = GeGenerator::GetInstance();
  instance.Initialize({});
  Graph graph("graph");
  EXPECT_EQ(instance.GenerateInfershapeGraph(graph), GE_GENERATOR_GRAPH_MANAGER_ADD_GRAPH_FAILED);
}

TEST_F(UtestGeGenerator, BuildSingleOpModel) {
  auto &instance = GeGenerator::GetInstance();
  instance.Initialize({});
  Graph graph("graph");
  OpDescPtr op_desc = std::make_shared<OpDesc>("name", "type");
  std::vector<GeTensor> inputs;
  std::vector<GeTensor> outputs;
  OpEngineType engine_type = ENGINE_SYS;
  ModelBufferData model_buff;
  EXPECT_NE(instance.BuildSingleOpModel(op_desc, inputs, outputs, engine_type, model_buff), SUCCESS);
  EXPECT_EQ(instance.Finalize(), SUCCESS);
}

TEST_F(UtestGeGenerator, GenerateModel) {
  InitGeLib();
  auto &instance = GeGenerator::GetInstance();
  std::map<std::string, std::string> options;
  options["ge.buildMode"] = BUILD_MODE_TUNING;
  instance.Initialize(options);
  auto compute_graph = MakeGraph();
  compute_graph->TopologicalSorting();
  Graph graph = ge::GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  std::string file_name_prefix = "prefix";
  std::vector<GeTensor> inputs;
  ModelBufferData model;
  bool is_offline = true;
  EXPECT_EQ(instance.GenerateModel(graph, file_name_prefix, inputs, model, is_offline), SUCCESS);
  EXPECT_EQ(instance.Finalize(), SUCCESS);
  FinalizeGeLib();
}

TEST_F(UtestGeGenerator, BuildSingleOp) {
  InitGeLib();
  auto &instance = GeGenerator::GetInstance();
  std::map<std::string, std::string> options;
  instance.Initialize(options);
  GeTensorDesc tensor_desc;
  shared_ptr<OpDesc> op_desc = std::make_shared<OpDesc>("Add", "add");
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddOutputDesc(tensor_desc);
  GeTensor tensor(tensor_desc);
  vector<GeTensor> inputs = { tensor, tensor };
  vector<GeTensor> outputs = { tensor };
  std::string model_file_name = "online";
  OpEngineType engine_type = ENGINE_AICORE;
  ModelBufferData model_buff;
  ComputeGraphPtr comp_graph;
  bool is_offline = false;
  int32_t compile_flag = 0;
  GraphStage graph_stage = GraphStage::GRAPH_STAGE_FUZZ;
  EXPECT_NE(instance.BuildSingleOp(op_desc, inputs, outputs, model_file_name, engine_type,
                                   model_buff, comp_graph, is_offline, compile_flag, graph_stage), SUCCESS);
  EXPECT_EQ(instance.Finalize(), SUCCESS);
  FinalizeGeLib();
}

TEST_F(UtestGeGenerator, BuildSingleOpAttr_unregst_oppath) {
  InitGeLib();
  auto &instance = GeGenerator::GetInstance();
  std::map<std::string, std::string> options;
  instance.Initialize(options);
  GeTensorDesc tensor_desc;
  shared_ptr<OpDesc> op_desc = std::make_shared<OpDesc>("Add", "add");
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddOutputDesc(tensor_desc);
  op_desc->SetAttr("_unregst_oppath", AnyValue::CreateFrom<int>(1));
  GeTensor tensor(tensor_desc);
  vector<GeTensor> inputs = { tensor, tensor };
  vector<GeTensor> outputs = { tensor };
  std::string model_file_name = "online";
  OpEngineType engine_type = ENGINE_AICORE;
  ModelBufferData model_buff;
  ComputeGraphPtr comp_graph;
  bool is_offline = false;
  int32_t compile_flag = 0;
  GraphStage graph_stage = GraphStage::GRAPH_STAGE_FUZZ;
  EXPECT_NE(instance.BuildSingleOp(op_desc, inputs, outputs, model_file_name, engine_type,
                                   model_buff, comp_graph, is_offline, compile_flag, graph_stage), SUCCESS);
  EXPECT_EQ(instance.Finalize(), SUCCESS);
  FinalizeGeLib();
}

TEST_F(UtestGeGenerator, BuildSingleOpOpInfo) {
  InitGeLib();
  auto &instance = GeGenerator::GetInstance();
  std::map<std::string, std::string> options;
  instance.Initialize(options);
  GeTensorDesc tensor_desc;
  shared_ptr<OpDesc> op_desc = std::make_shared<OpDesc>("Add", "add");
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddOutputDesc(tensor_desc);
  GeTensor tensor(tensor_desc);
  vector<GeTensor> inputs = { tensor, tensor };
  vector<GeTensor> outputs = { tensor };
  std::string model_file_name = "online";
  OpEngineType engine_type = ENGINE_AICORE;
  ModelBufferData model_buff;
  ComputeGraphPtr comp_graph;
  bool is_offline = false;
  int32_t compile_flag = 0;
  GraphStage graph_stage = GraphStage::GRAPH_STAGE_FUZZ;
  OpsKernelManager &ops_kernel_manager = ge::GELib::GetInstance()->OpsKernelManagerObj();
  std::vector<OpInfo> vec;
  OpInfo oi;
  ops_kernel_manager.ops_kernel_info_[op_desc->GetType()] = vec;

  EXPECT_NE(instance.BuildSingleOp(op_desc, inputs, outputs, model_file_name, engine_type,
                                   model_buff, comp_graph, is_offline, compile_flag, graph_stage), SUCCESS);
  oi.engine = "AIcoreEngine";
  oi.opKernelLib = "opKernelLib";
  vec.push_back(oi);
  ops_kernel_manager.ops_kernel_info_[op_desc->GetType()] = vec;
  auto p = std::make_shared<FakeOpsKernelInfoStore>();
  ops_kernel_manager.ops_kernel_store_["opKernelLib"] = p;
  EXPECT_EQ(instance.BuildSingleOp(op_desc, inputs, outputs, model_file_name, engine_type,
                                   model_buff, comp_graph, is_offline, compile_flag, graph_stage), SUCCESS);
  p->supported_ = false;
  ops_kernel_manager.ops_kernel_store_[oi.opKernelLib] = p;
  EXPECT_NE(instance.BuildSingleOp(op_desc, inputs, outputs, model_file_name, engine_type,
                                   model_buff, comp_graph, is_offline, compile_flag, graph_stage), SUCCESS);
  EXPECT_EQ(instance.Finalize(), SUCCESS);
  FinalizeGeLib();
}

TEST_F(UtestGeGenerator, BuildSingleOpOpInfoNoLib) {
  InitGeLib();
  auto &instance = GeGenerator::GetInstance();
  std::map<std::string, std::string> options;
  instance.Initialize(options);
  GeTensorDesc tensor_desc;
  shared_ptr<OpDesc> op_desc = std::make_shared<OpDesc>("Add", "add");
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddOutputDesc(tensor_desc);
  GeTensor tensor(tensor_desc);
  vector<GeTensor> inputs = { tensor, tensor };
  vector<GeTensor> outputs = { tensor };
  std::string model_file_name = "online";
  OpEngineType engine_type = ENGINE_AICORE;
  ModelBufferData model_buff;
  ComputeGraphPtr comp_graph;
  bool is_offline = false;
  int32_t compile_flag = 0;
  GraphStage graph_stage = GraphStage::GRAPH_STAGE_FUZZ;
  OpsKernelManager &ops_kernel_manager = ge::GELib::GetInstance()->OpsKernelManagerObj();
  std::vector<OpInfo> vec;
  OpInfo oi;
  oi.engine = "AIcoreEngine";
  oi.opKernelLib = "";
  vec.push_back(oi);
  ops_kernel_manager.ops_kernel_info_[op_desc->GetType()] = vec;
  EXPECT_NE(instance.BuildSingleOp(op_desc, inputs, outputs, model_file_name, engine_type,
                                   model_buff, comp_graph, is_offline, compile_flag, graph_stage), SUCCESS);
  EXPECT_EQ(instance.Finalize(), SUCCESS);
  FinalizeGeLib();
}

TEST_F(UtestGeGenerator, CheckForSingleOp) {
  InitGeLib();
  auto &instance = GeGenerator::GetInstance();
  std::map<std::string, std::string> options;
  instance.Initialize(options);

  GeTensorDesc tensor_desc;
  shared_ptr<OpDesc> op_desc = std::make_shared<OpDesc>("Add", "add");
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddOutputDesc(tensor_desc);
  GeTensor tensor(tensor_desc);
  vector<GeTensor> inputs = { tensor };
  vector<GeTensor> outputs = { tensor };
  EXPECT_EQ(instance.CheckForSingleOp(op_desc, inputs, outputs), PARAM_INVALID);
  inputs.push_back(tensor);
  outputs.push_back(tensor);
  EXPECT_EQ(instance.CheckForSingleOp(op_desc, inputs, outputs), PARAM_INVALID);
  EXPECT_EQ(instance.Finalize(), SUCCESS);
  FinalizeGeLib();
}

TEST_F(UtestGeGenerator, GenerateModelAndDumpBuildGraph) {
  InitGeLib();
  auto &instance = GeGenerator::GetInstance();
  std::map<std::string, std::string> options;
  options["ge.tuningPath"] = "./after_build.txt";
  instance.Initialize(options);
  auto compute_graph = MakeGraph();
  compute_graph->TopologicalSorting();
  Graph graph = ge::GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  std::string file_name_prefix = "prefix";
  std::vector<GeTensor> inputs;
  ModelBufferData model;
  bool is_offline = true;
  EXPECT_EQ(instance.GenerateModel(graph, file_name_prefix, inputs, model, is_offline), SUCCESS);
  EXPECT_EQ(instance.Finalize(), SUCCESS);
  FinalizeGeLib();
}

}  // namespace ge
