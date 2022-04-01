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

#include <gtest/gtest.h>

#include <mutex>
#include <chrono>
#define private public
#include "external/ge/ge_api.h"
#undef private
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/node_adapter.h"
#include "framework/common/types.h"
#include "graph/utils/op_desc_utils.h"
#include "ge_graph_dsl/assert/check_utils.h"

#include "ge_graph_dsl/graph_dsl.h"
#include "ge_graph_dsl/assert/graph_assert.h"
#include "utils/graph_factory.h"
#include "graph/manager/graph_var_manager.h"
#include "ge_running_env/tensor_utils.h"
#include "ge_running_env/fake_graph_optimizer.h"
#include "ge_running_env/ge_running_env_faker.h"
#include "graph/utils/tensor_adapter.h"
#include "graph/utils/graph_utils.h"
#include "utils/synchronizer.h"

using namespace std;
using namespace ge;
namespace {
///
///      Variable(2, 3, 4, 5)                   Relu(1,2,3,4,5)
///         /            \                             |
///    TransData      TransData                    TransData
///        |              |                            |
///  Relu(1,2,3,4,5)  Relu(1,2,3,4,5)            Variable(2, 3, 4, 5)
///         \            /                             |
///            NetOutput -------------------------------
///
Graph BuildVariableGraph(bool has_copy_from_attr = false) {
  GeTensorDesc tensor_4_desc(ge::GeShape({2,3,4,5}), FORMAT_NCHW, DT_INT32);
  GeTensorDesc tensor_5_desc(ge::GeShape({1,2,3,4,5}), FORMAT_NC1HWC0, DT_INT32);

  auto var1 = std::make_shared<OpDesc>("var1", VARIABLE);
  auto var2 = std::make_shared<OpDesc>("var2", VARIABLE);
  var1->AddInputDesc(tensor_4_desc);
  var1->AddOutputDesc(tensor_4_desc);
  var2->AddInputDesc(tensor_4_desc);
  var2->AddOutputDesc(tensor_4_desc);
  if (has_copy_from_attr) {
    (void)AttrUtils::SetStr(var1, "_copy_from_var_node", "var2");
    (void)AttrUtils::SetBool(var1, "_copy_value", false);
  }

  auto trans1 = std::make_shared<OpDesc>("transdata1", TRANSDATA);
  trans1->AddInputDesc("x", tensor_4_desc);
  trans1->AddOutputDesc("x", tensor_5_desc);

  auto trans2 = std::make_shared<OpDesc>("transdata2", TRANSDATA);
  trans2->AddInputDesc("x", tensor_4_desc);
  trans2->AddOutputDesc("x", tensor_5_desc);

  auto trans3 = std::make_shared<OpDesc>("transdata3", TRANSDATA);
  trans3->AddInputDesc(tensor_5_desc);
  trans3->AddOutputDesc(tensor_4_desc);

  auto data1 = std::make_shared<OpDesc>("data1", DATA);
  data1->AddInputDesc(tensor_5_desc);
  data1->AddOutputDesc(tensor_5_desc);

  auto relu1 = std::make_shared<OpDesc>("relu1", RELU);
  relu1->AddInputDesc(tensor_5_desc);
  relu1->AddOutputDesc(tensor_5_desc);

  auto relu2 = std::make_shared<OpDesc>("relu2", RELU);
  relu2->AddInputDesc(tensor_5_desc);
  relu2->AddOutputDesc(tensor_5_desc);

  auto relu3 = std::make_shared<OpDesc>("relu3", RELU);
  relu3->AddInputDesc(tensor_5_desc);
  relu3->AddOutputDesc(tensor_5_desc);

  auto var_ref = std::make_shared<OpDesc>("var_ref", VARIABLE);
  AttrUtils::SetStr(var_ref, REF_VAR_SRC_VAR_NAME, "var1");
  var_ref->AddInputDesc(tensor_4_desc);
  var_ref->AddOutputDesc(tensor_4_desc);

  DEF_GRAPH(g1) {
    CHAIN(NODE(var1)->NODE(trans1)->NODE(relu1)->NODE("output", NETOUTPUT));
    CHAIN(NODE(var1)->NODE(trans2)->NODE(relu2)->NODE("output"));
    CHAIN(NODE(data1)->NODE(relu3)->NODE(trans3)->NODE(var_ref)->NODE("output"));
    CHAIN(NODE(var2)->NODE("output"));
  };
  return ToGeGraph(g1);
}

///
///   Variable       Const
///  [2, 3, 4, 5]    [1,2,3,4,5]             
///            \     /                               
///             Reshape                 
///                |                                         
///        Relu[1,2,3,4,5]         
///                |                                     
///            NetOutput 
///
Graph BuildVariableAndReshapeGraph() {
  int64_t dims_size = 1;
  vector<int64_t> data_vec = {5};
  for_each(data_vec.begin(), data_vec.end(), [&](int64_t &data) { dims_size *= data; });
  vector<int32_t> data_value_vec = {1, 2, 3, 4, 5};
  GeTensorDesc data_tensor_desc(GeShape(data_vec), FORMAT_NCHW, DT_INT32);
  GeTensorPtr data_tensor = std::make_shared<GeTensor>(data_tensor_desc, (uint8_t *) data_value_vec.data(),
                                                       data_value_vec.size() * sizeof(int32_t));

  auto var1 = OP_CFG(VARIABLE).TensorDesc(FORMAT_NCHW, DT_INT32, {2, 3, 4, 5}).InCnt(0).OutCnt(1);
  auto const1 = OP_CFG(CONSTANT).TensorDesc(FORMAT_NCHW, DT_INT32, {5}).InCnt(0).OutCnt(1).Weight(data_tensor);
  auto reshape = OP_CFG(RESHAPE).InCnt(2).OutCnt(1).TensorDesc(FORMAT_NCHW, DT_INT32, {1, 2, 3, 4, 5});
  auto relu = OP_CFG(RELU).InCnt(1).OutCnt(1).TensorDesc(FORMAT_NCHW, DT_INT32, {1, 2, 3, 4, 5});
  auto netoutput = OP_CFG(NETOUTPUT).InCnt(1).OutCnt(1).TensorDesc(FORMAT_NCHW, DT_INT32, {1, 2, 3, 4, 5});

  DEF_GRAPH(g1) {
    CHAIN(NODE("var1", var1)
              ->EDGE(0, 0)
              ->NODE("reshape", reshape)
              ->NODE("relu", relu)
              ->NODE("netoutput", netoutput));
    CHAIN(NODE("const1", const1)->EDGE(0, 1)->NODE("reshape"));
  };
  return ToGeGraph(g1);
}


/*
 *                 netoutput
 *                /c  |c    \c
 *      transdata2   /       \
 *     /       |c   /         \
 *    |      assign1         assign2
 *    |      /     \         /    \
 *   transdata1   const1  var2    const2
 *      |
*     var1
 */
Graph BuildVarWriteNoOutputRefGraph1() {
  std::vector<int64_t> shape = {2,2,3,2};  // HWCN
  auto data_tensor = GenerateTensor(shape);
  DEF_GRAPH(var_init) {
    auto var1 = OP_CFG(VARIABLE)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("var1");
    auto var2 = OP_CFG(VARIABLE)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("var2");
    auto assign1 = OP_CFG(ASSIGNVARIABLEOP)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .InCnt(2)
        .Build("assign1");
    auto assign2 = OP_CFG(ASSIGNVARIABLEOP)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .InCnt(2)
        .Build("assign2");
    auto const1 = OP_CFG(CONSTANT)
        .TensorDesc(FORMAT_ND, DT_FLOAT, shape)
        .Weight(data_tensor)
        .InCnt(1)
        .OutCnt(1)
        .Build("const1");
    auto const2 = OP_CFG(CONSTANT)
        .Weight(data_tensor).TensorDesc(FORMAT_HWCN, DT_FLOAT, shape)
        .InCnt(1)
        .OutCnt(1)
        .Build("const2");
    auto transdata1 = OP_CFG(TRANSDATA)
        .InCnt(1)
        .OutCnt(1)
        .Build("transdata1");
    auto transdata2 = OP_CFG(TRANSDATA)
        .InCnt(1)
        .OutCnt(1)
        .Build("transdata2");
    auto netoutput = OP_CFG(NETOUTPUT)
        .Build("netoutput");


    assign1->MutableInputDesc(0)->SetRefPortByIndex(std::vector<uint32_t>({0}));
    assign2->MutableInputDesc(0)->SetRefPortByIndex(std::vector<uint32_t>({0}));

    CHAIN(NODE(var1)->NODE(transdata1)->NODE(assign1));
    CHAIN(NODE(transdata1)->NODE(transdata2));
    CHAIN(NODE(const1)->EDGE(0, 1)->NODE(assign1));
    CHAIN(NODE(var2)->NODE(assign2));
    CHAIN(NODE(const2)->EDGE(0, 1)->NODE(assign2));

    CTRL_CHAIN(NODE(assign1)->NODE(netoutput));
    CTRL_CHAIN(NODE(assign2)->NODE(netoutput));
    CTRL_CHAIN(NODE(assign1)->NODE(transdata2)->NODE(netoutput));
  };

  return ToGeGraph(var_init);
}
}  // namespace
class VariableAccSt : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {  }
};

TEST_F(VariableAccSt, test_variable_ref) {
  // build graph
  Graph graph = BuildVariableGraph();
  Graph graph2 = BuildVariableGraph(true);

  // new session & add graph
  map<AscendString, AscendString> options;
  Session session(options);
  auto ret = session.AddGraph(2, graph, options);
  EXPECT_EQ(ret, SUCCESS);
  ret = session.AddGraph(3, graph2, options);
  EXPECT_EQ(ret, SUCCESS);

  std::vector<InputTensorInfo> inputs;
  ret = session.BuildGraph(2, inputs);
  EXPECT_EQ(ret, SUCCESS);

  ret = session.BuildGraph(3, inputs);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(VariableAccSt, test_variable_and_reshape_fusion) {
  // build graph
  Graph graph = BuildVariableAndReshapeGraph();
  DUMP_GRAPH_WHEN("OptimizeStage1_1");

  // new session & add graph
  map<AscendString, AscendString> options;
  Session session(options);
  auto ret = session.AddGraph(2, graph, options);
  EXPECT_EQ(ret, SUCCESS);
  // build input tensor
  std::vector<InputTensorInfo> inputs;
  // build_graph through session
  ret = session.BuildGraph(2, inputs);
  EXPECT_EQ(ret, SUCCESS);
  // check result
  CHECK_GRAPH(OptimizeStage1_1) {
    auto var_node = graph->FindNode("var1");
    ASSERT_EQ(var_node->GetOutAllNodes().at(0)->GetName(), "relu");
  };
}

void Fake5DNodeEngine(GeRunningEnvFaker &ge_env) {
  auto ffo = MakeShared<FakeFormatsOptimizer>();
  ffo->OpFormatByType(
      CONV2D, {
          .input_formats = {
              {FORMAT_NC1HWC0, GeShape(std::vector<int64_t>({8,1,16,16,16}))},
              {FORMAT_FRACTAL_Z, GeShape(std::vector<int64_t>({4,1,16,16}))},
          },
          .output_formats = {
              {FORMAT_NC1HWC0, GeShape(std::vector<int64_t>({8,1,16,16,16}))}
          }
      });
  ge_env.InstallDefault();
  ge_env.Install(FakeEngine("TestForVarAcc").GraphOptimizer("FormatOp", ffo));
}

void RunAndCheckInitVarGraph(Session &session) {
  auto var_init_graph = GraphFactory::BuildVarInitGraph1();
  session.AddGraph(1, var_init_graph);
  std::vector<ge::Tensor> g1_inputs;
  Synchronizer sync;
  auto ret = session.RunGraphAsync(1, g1_inputs, [&sync](Status run_ret, std::vector<ge::Tensor> &) {
    EXPECT_EQ(run_ret, SUCCESS);
    sync.OnDone();
  });
  EXPECT_EQ(ret, SUCCESS);

  sync.WaitFor(60);
  ASSERT_TRUE(VarManagerPool::Instance().GetVarManager(session.sessionId_)->IsVarExist("var1"));
  ASSERT_TRUE(VarManagerPool::Instance().GetVarManager(session.sessionId_)->IsVarExist("var2"));
  GeTensorDesc td1;
  EXPECT_EQ(VarManagerPool::Instance().GetVarManager(session.sessionId_)->GetCurVarDesc("var1", td1), SUCCESS);
  // var1由ND格式的const做初始化，因此其格式被推为ND
  EXPECT_EQ(td1.GetFormat(), FORMAT_ND);
  EXPECT_EQ(td1.GetShape().GetDims(), std::vector<int64_t>({2,2,3,2}));
  EXPECT_EQ(td1.GetOriginShape().GetDims(), std::vector<int64_t>({2,2,3,2}));
  GeTensorDesc td2;
  EXPECT_EQ(VarManagerPool::Instance().GetVarManager(session.sessionId_)->GetCurVarDesc("var2", td2), SUCCESS);
  // var2由HWCN格式的const做初始化，因此其格式被推为HWCN
  EXPECT_EQ(td2.GetFormat(), FORMAT_HWCN);
  EXPECT_EQ(td2.GetShape().GetDims(), std::vector<int64_t>({2,2,3,2}));
  EXPECT_EQ(td2.GetOriginShape().GetDims(), std::vector<int64_t>({2,2,3,2}));
}

void RunAndCheckCheckpointGraph(Session &session, bool is_5d) {
  std::vector<ge::Tensor> g1_inputs;
  Synchronizer sync;
  GeTensorDesc td1;
  GeTensorDesc td2;

  auto ckp_graph = GraphFactory::BuildCheckpointGraph1();
  auto ret = session.AddGraph(2, ckp_graph);
  EXPECT_EQ(ret, SUCCESS);
  ret = session.RunGraphAsync(2, g1_inputs, [&sync](Status run_ret, std::vector<ge::Tensor> &) {
    EXPECT_EQ(run_ret, SUCCESS);
    sync.OnDone();
  });
  EXPECT_EQ(ret, SUCCESS);

  sync.WaitFor(60);
  EXPECT_EQ(VarManagerPool::Instance().GetVarManager(session.sessionId_)->GetCurVarDesc("var1", td1), SUCCESS);
  if (is_5d) {
    EXPECT_EQ(td1.GetFormat(), FORMAT_FRACTAL_Z);
    EXPECT_EQ(td1.GetShape().GetDims(), std::vector<int64_t>({4,1,16,16}));
  } else {
    EXPECT_EQ(td1.GetFormat(), FORMAT_ND);
    EXPECT_EQ(td1.GetShape().GetDims(), std::vector<int64_t>({2,2,3,2}));
  }
  EXPECT_EQ(td1.GetOriginShape().GetDims(), std::vector<int64_t>({2,2,3,2}));
  EXPECT_EQ(VarManagerPool::Instance().GetVarManager(session.sessionId_)->GetCurVarDesc("var2", td2), SUCCESS);
  EXPECT_EQ(td2.GetFormat(), FORMAT_ND);  // 这里行为有些奇怪，变量管理器的当前格式由原来的HWCN改成ND了
  EXPECT_EQ(td2.GetShape().GetDims(), std::vector<int64_t>({2,2,3,2}));
  EXPECT_EQ(td2.GetOriginShape().GetDims(), std::vector<int64_t>({2,2,3,2}));
}

void RunAndCheckTrainGraph(Session &session) {
  std::vector<ge::Tensor> g1_inputs;
  Synchronizer sync;
  GeTensorDesc td1;
  GeTensorDesc td2;

  auto train_graph = GraphFactory::BuildVarTrainGraph1();
  g1_inputs.emplace_back(TensorAdapter::AsTensor(*GenerateTensor({8,3,16,16})));
  g1_inputs.emplace_back(TensorAdapter::AsTensor(*GenerateTensor({})));
  auto ret = session.AddGraph(3, train_graph);
  EXPECT_EQ(ret, SUCCESS);

  ret = session.RunGraphAsync(3, g1_inputs, [&sync](Status run_ret, std::vector<ge::Tensor> &) {
    EXPECT_EQ(run_ret, SUCCESS);
    sync.OnDone();
  });
  EXPECT_EQ(ret, SUCCESS);

  sync.WaitFor(60);
  EXPECT_EQ(VarManagerPool::Instance().GetVarManager(session.sessionId_)->GetCurVarDesc("var1", td1), SUCCESS);
  EXPECT_EQ(td1.GetFormat(), FORMAT_FRACTAL_Z);
  EXPECT_EQ(td1.GetShape().GetDims(), std::vector<int64_t>({4,1,16,16}));
  EXPECT_EQ(td1.GetOriginShape().GetDims(), std::vector<int64_t>({2,2,3,2}));
  EXPECT_EQ(VarManagerPool::Instance().GetVarManager(session.sessionId_)->GetCurVarDesc("var2", td2), SUCCESS);
  EXPECT_EQ(td2.GetShape().GetDims(), std::vector<int64_t>({2,2,3,2}));
  EXPECT_EQ(td2.GetOriginShape().GetDims(), std::vector<int64_t>({2,2,3,2}));
}

/*
 * 用例场景：变量格式发生变化时，包含该变量的、之前下发的图，需要做重编译
 * 步骤：
 * step 1. 下发一张变量初始化图，初始化变量var1和var2，变量格式为常规ND
 * 期望：变量在变量管理器中创建成功
 * step 2. 下发一张checkpoint图，将变量var1和var2作为返回值返回
 * step 3. 下发一张训练图，var1作为权重连接到Conv2D
 * 期望： var1格式变更为FZ；var1按照新格式分配新的dev地址；var1的数据从device上被拷贝上来，在host完成格式转换后，按照新地址拷贝回dev
 * step 4. 判断step 2中的checkpoint图是否需要重编译
 * 期望：checkpoint图需要重编译
 * step 5. 删除ckp图，重新下发一次ckp图
 * 期望：ckp图上var1的变量格式为FZ，并且插入了FZ->HWCN的转换算子后作为图的输出结果返回
 */
TEST_F(VariableAccSt, variable_modified_when_second_graph_then_run_first_graph) {
  GeRunningEnvFaker ge_env;
  Fake5DNodeEngine(ge_env);

  std::map<AscendString, AscendString> options;
  Session session(options);

  RunAndCheckInitVarGraph(session);
  RunAndCheckCheckpointGraph(session, false);
  RunAndCheckTrainGraph(session);
  EXPECT_TRUE(session.IsGraphNeedRebuild(2));
  session.RemoveGraph(2);
  RunAndCheckCheckpointGraph(session, true);

  ge_env.Reset();
  ge_env.InstallDefault();
}

TEST_F(VariableAccSt, variable_write_edge_insert_transdata) {}

/*
 * 用例场景：本用例测试变量回边写入场景，在写入算子没有输出ref时，最终图是正确的
 * 步骤：
 * step 1. 下发一张变量初始化图，初始化变量var1和var2，变量格式为常规ND
 * 期望：变量在变量管理器中创建成功
 * step 2. 下发一张训练图，var1作为权重连接到Conv2D
 * 期望： var1格式变更为FZ；var1按照新格式分配新的dev地址；var1的数据从device上被拷贝上来，在host完成格式转换后，按照新地址拷贝回dev
 * step 3. 下发一张训练图2，var1、var2连接到一个不支持FZ的、会写入var的算子
 * 期望：var1应该有一个较为复杂的写入图结构（详见代码校验）
 */
TEST_F(VariableAccSt, variable_write_edge_does_not_have_output) {
  GeRunningEnvFaker ge_env;
  Fake5DNodeEngine(ge_env);

  std::map<AscendString, AscendString> options;
  Session session(options);

  RunAndCheckInitVarGraph(session);
  RunAndCheckTrainGraph(session);

  // step 3
  auto var_write_graph = GraphFactory::BuildVarWriteNoOutputRefGraph1();
  session.AddGraph(4, var_write_graph);
  std::vector<ge::Tensor> g1_inputs;
  Synchronizer sync;

  mmSetEnv("DUMP_GE_GRAPH", "1", 1);

  auto ret = session.RunGraphAsync(4, g1_inputs, [&sync](Status run_ret, std::vector<ge::Tensor> &) {
    EXPECT_EQ(run_ret, SUCCESS);
    sync.OnDone();
  });
  EXPECT_EQ(ret, SUCCESS);

  // todo check topo

  ge_env.Reset();
  ge_env.InstallDefault();
}

TEST_F(VariableAccSt, test_variable_is_initialized) {
  int64_t dims_size = 1;
  vector<int64_t> data_vec = {5};
  for_each(data_vec.begin(), data_vec.end(), [&](int64_t &data) { dims_size *= data; });
  vector<int32_t> data_value_vec = {1, 2, 3, 4, 5};
  GeTensorDesc data_tensor_desc(GeShape(data_vec), FORMAT_NCHW, DT_INT32);
  GeTensorPtr data_tensor = std::make_shared<GeTensor>(data_tensor_desc, (uint8_t *)data_value_vec.data(),
                                                       data_value_vec.size() * sizeof(int32_t));


  auto var1 = OP_CFG(VARIABLE).TensorDesc(FORMAT_NCHW, DT_INT32, {2, 3, 4, 5}).InCnt(0).OutCnt(1);
  auto var_init = OP_CFG(VARISINITIALIZEDOP).TensorDesc(FORMAT_NCHW, DT_INT32, {2, 3, 4, 5}).InCnt(1).OutCnt(1);
  auto const1 = OP_CFG(CONSTANT).TensorDesc(FORMAT_NCHW, DT_INT32, {2, 3, 4, 5}).InCnt(0).OutCnt(1);
  auto const2 = OP_CFG(CONSTANT).TensorDesc(FORMAT_NCHW, DT_INT32, {5}).InCnt(0).OutCnt(1).Weight(data_tensor);
  auto reshape = OP_CFG(RESHAPE).InCnt(2).OutCnt(1).TensorDesc(FORMAT_NCHW, DT_INT32, {1, 2, 3, 4, 5});
  auto relu = OP_CFG(RELU).InCnt(1).OutCnt(1).TensorDesc(FORMAT_NCHW, DT_INT32, {1, 2, 3, 4, 5});
  auto netoutput = OP_CFG(NETOUTPUT).InCnt(1).OutCnt(1).TensorDesc(FORMAT_NCHW, DT_INT32, {1, 2, 3, 4, 5});

  DEF_GRAPH(g1) {
    CHAIN(NODE("var_init", var_init)
              ->EDGE(0, 0)
              ->NODE("reshape", reshape)
              ->NODE("relu", relu)
              ->NODE("netoutput", netoutput));
    CHAIN(NODE("const2", const2)->EDGE(0, 1)->NODE("reshape"));
  };

  DEF_GRAPH(g2) {
    CHAIN(NODE("const1", const1)
              ->NODE("var_init", var_init)
              ->EDGE(0, 0)
              ->NODE("reshape", reshape)
              ->NODE("relu", relu)
              ->NODE("netoutput", netoutput));
    CHAIN(NODE("const2", const2)->EDGE(0, 1)->NODE("reshape"));
  };

  DEF_GRAPH(g3) {
    CHAIN(NODE("var1", var1)
              ->NODE("var_init", var_init)
              ->EDGE(0, 0)
              ->NODE("reshape", reshape)
              ->NODE("relu", relu)
              ->NODE("netoutput", netoutput));
    CHAIN(NODE("const2", const2)->EDGE(0, 1)->NODE("reshape"));
  };

  auto graph = ToGeGraph(g1);
  DUMP_GRAPH_WHEN("OptimizeStage1_1");

  map<AscendString, AscendString> options;
  Session session(options);
  auto ret = session.AddGraph(2, graph, options);
  EXPECT_EQ(ret, SUCCESS);
  std::vector<InputTensorInfo> inputs;
  ret = session.BuildGraph(2, inputs);
  // VARISINITIALIZEDOP must have input
  EXPECT_NE(ret, SUCCESS);

  graph = ToGeGraph(g2);
  ret = session.AddGraph(3, graph, options);
  EXPECT_EQ(ret, SUCCESS);
  ret = session.BuildGraph(3, inputs);
  // VARISINITIALIZEDOP input must be varibale
  EXPECT_NE(ret, SUCCESS);

  graph = ToGeGraph(g3);
  ret = session.AddGraph(4, graph, options);
  EXPECT_EQ(ret, SUCCESS);
  ret = session.BuildGraph(4, inputs);
  EXPECT_EQ(ret, SUCCESS);
}
