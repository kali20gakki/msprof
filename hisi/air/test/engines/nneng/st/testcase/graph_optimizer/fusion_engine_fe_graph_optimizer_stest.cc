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
#include "graph_optimizer/fe_graph_optimizer.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "common/configuration.h"
#include "graph/ge_tensor.h"
#include "graph/op_desc.h"
#include "graph/compute_graph.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/tensor_utils.h"
#include "array_ops.h"
#include "selection_ops.h"
#include "matrix_calculation_ops.h"
#include "elewise_calculation_ops.h"
#include "reduce_ops.h"
#include "common/util/op_info_util.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include <graph_optimizer/fe_graph_optimizer.h>
#include "ops_kernel_store/fe_ops_kernel_info_store.h"
#include "ops_store/sub_op_info_store.h"
#include "ops_store/ops_kernel_manager.h"
#include <fusion_rule_manager/fusion_rule_data/fusion_rule_pattern.h>
#include "graph_optimizer/graph_fusion/graph_replace.h"
#include "./ge_context.h"
#include "./ge_local_context.h"

#include "graph_optimizer/heavy_format_propagation/heavy_format_propagation.h"
#include "ge/ge_api_types.h"
#include "common/lxfusion_json_util.h"
#include "adapter/common/op_store_adapter_manager.h"

#include "ops_kernel_store/fe_ops_kernel_info_store.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "common/graph/fe_graph_utils.h"
#include "common/configuration.h"
#include <vector>
#include "common/fe_inner_attr_define.h"
#include "ge/ge_api_types.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/tuning_utils.h"
#include "graph/node.h"

#undef protected
#undef private

using namespace testing;
using namespace ge;
using namespace fe;

using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;
using FEGraphOptimizerPtr = std::shared_ptr<fe::FEGraphOptimizer>;
using OpStoreAdapterPtr = std::shared_ptr<fe::OpStoreAdapter>;

class OptimizeUtilitySTStub: public ge::OptimizeUtility {
 public:
  OptimizeUtilitySTStub() {}
  virtual ~OptimizeUtilitySTStub() override {}

  ge::Status InferShape(ComputeGraph &compute_graph) override{
    return ge::SUCCESS;
  }

  ge::Status InferShape(const ComputeGraphPtr &compute_graph) override {
    return ge::SUCCESS;
  }
};

bool checkIsRegistered(const te::TbeOpInfo &op_info, bool &val) {
  val = true;
  return true;
}

bool checkIsNotRegistered(const te::TbeOpInfo &op_info, bool &val) {
  val = false;
  return true;
}

bool checkIsRegisteredException(const te::TbeOpInfo &op_info, bool &val) {
  val = false;
  return false;
}

bool teGeneralize(const te::TbeOpInfo &op_info,
    const te::TE_GENERALIZE_TYPE &general_type, const ge::NodePtr &node) {
  std::vector<int64_t> shape_vec;
  auto op_desc = node->GetOpDesc();
  auto tensor_desc_x = op_desc->MutableInputDesc("x");
  shape_vec = tensor_desc_x->GetShape().GetDims();
  if (general_type == te::REGISTER_FUNC) {
    for (auto &i : shape_vec) {
      i = -1;
    }
  } else if (general_type == te::DEFAULT_TBE_OP_INFO) {
    for (int i = 0; i < shape_vec.size()-1; ++i) {
      shape_vec[i] = -1;
    }
  } else {
    shape_vec[0] = -1;
  }
  tensor_desc_x->SetShape(ge::GeShape(shape_vec));
  tensor_desc_x->SetOriginShape(ge::GeShape(shape_vec));
  return true;
}

bool teGeneralizeException(const te::TbeOpInfo &op_info,
    const te::TE_GENERALIZE_TYPE &general_type, const ge::NodePtr &node) {
  return false;
}

static tune::Status LxFusionFinalizeFunc1(const ge::ComputeGraph &){
  return tune::SUCCESS;
}

tune::Status LxFusionRecoveryFunc1(ge::ComputeGraph &, const std::vector<ge::NodePtr> &, std::vector<ge::NodePtr> *,
                                   std::vector<ge::NodePtr> *){
  return tune::SUCCESS;
}

class STEST_fusion_engine_fe_graph_optimizer : public testing::Test {
public:
  FEOpsKernelInfoStorePtr ops_info_store;
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
  std::vector<FEOpsStoreInfo> cfg_info_;
  SplitOptimizer split_optimizer;
  ConcatOptimizer concat_optimizer;
  RefRelationsPtr reflection_builder_ptr_;
  TbeOpStoreAdapterPtr tbe_op_store_adapter;
  shared_ptr<fe::SubOpsStore> sub_ops_store_ptr;
  shared_ptr<fe::SubOpInfoStore> sub_ops_kernel_ptr;
  FEGraphOptimizerPtr fe_graph_optimizer_;
  FEOpsKernelInfoStorePtr ops_kernel_info_store_ptr_;
  GraphFusionPtr graph_fusion_ptr_;
  NodePtr MakeNode(const ComputeGraphPtr &graph, uint32_t in_num, uint32_t out_num, string name, string type) {
    GeTensorDesc test_desc(GeShape(), FORMAT_NCHW, DT_FLOAT);
    auto op_desc = std::make_shared<OpDesc>(name, type);
    for (auto i = 0; i < in_num; ++i) {
      op_desc->AddInputDesc(test_desc);
    }
    for (auto i = 0; i < out_num; ++i) {
      op_desc->AddOutputDesc(test_desc);
    }
    return graph->AddNode(op_desc);
  }
protected:
  void SetUp() {
    tbe_op_store_adapter = std::make_shared<TbeOpStoreAdapter>();
    reflection_builder_ptr_ = std::make_shared<ge::RefRelations>();
    op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace("tbe_op_adapter", tbe_op_store_adapter);
    ops_info_store = std::make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_);
    sub_ops_store_ptr = make_shared<fe::SubOpsStore>(op_store_adapter_manager_ptr_);

    OptimizeUtilitySTStub *optimize_utility_stub = new OptimizeUtilitySTStub();

    ops_kernel_info_store_ptr_ = std::make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, fe::AI_CORE_NAME);
    RuleMgrPtr fusion_rule_mgr_ptr_ = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
    PassMgrPtr fusion_pass_mgr_ptr_ = std::make_shared<FusionPassManager>();
    FusionPriorityMgrPtr fusion_priority_mgr_ptr_ = std::make_shared<FusionPriorityManager>(
        fe::AI_CORE_NAME, fusion_pass_mgr_ptr_, fusion_rule_mgr_ptr_);
    graph_fusion_ptr_ = std::make_shared<GraphFusion>(fusion_rule_mgr_ptr_, ops_kernel_info_store_ptr_,
                                                      fusion_pass_mgr_ptr_, fusion_priority_mgr_ptr_);
    graph_fusion_ptr_->SetEngineName(fe::AI_CORE_NAME);
    fe_graph_optimizer_ = make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_, op_store_adapter_manager_ptr_,
                                                        fe::AI_CORE_NAME);
    std::map<std::string, std::string> options;
    fe_graph_optimizer_->Initialize(options, optimize_utility_stub);
    fe_graph_optimizer_->graph_fusion_ptr_ = graph_fusion_ptr_;

    FEOpsStoreInfo TIK_CUSTOM_OPINFO_STUB = {
            1,
            "tik-custom",
            EN_IMPL_CUSTOM_TIK,
            "./air/test/engines/nneng/st/stub/fe_config/tik_custom_opinfo",
            ""
    };
    FEOpsStoreInfo TBE_CUSTOM_OPINFO_STUB = {
            2,
            "tbe-custom",
            EN_IMPL_CUSTOM_TBE,
            "./air/test/engines/nneng/st/stub/fe_config/tbe_custom_opinfo",
            ""
    };
    FEOpsStoreInfo TIK_OPINFO_STUB = {
            5,
            "tik-builtin",
            EN_IMPL_HW_TIK,
            "./air/test/engines/nneng/st/stub/fe_config/tik_opinfo",
            ""
    };
    FEOpsStoreInfo TBE_OPINFO_STUB = {
            6,
            "tbe-builtin",
            EN_IMPL_HW_TBE,
            "./air/test/engines/nneng/st/stub/fe_config/tbe_opinfo",
            ""
    };
    FEOpsStoreInfo RL_OPINFO_STUB = {
            7,
            "rl-built",
            EN_IMPL_RL,
            "./air/test/engines/nneng/st/stub/fe_config/rl_opinfo",
            ""
    };

    sub_ops_store_ptr->SetSubStoreType("tbe-builtin");
    sub_ops_store_ptr->SetSubStoreInfo(TBE_OPINFO_STUB);
    sub_ops_store_ptr->InitializeSubStore(fe::AI_CORE_NAME);

    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(TBE_OPINFO_STUB);
    Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);

    sub_ops_kernel_ptr = std::make_shared<fe::SubOpInfoStore>(TBE_OPINFO_STUB);
    sub_ops_kernel_ptr->Initialize(fe::AI_CORE_NAME);
    OpsKernelManager::Instance(fe::AI_CORE_NAME).sub_ops_kernel_map_.emplace("tbe-builtin", sub_ops_kernel_ptr);

    cfg_info_.push_back(TIK_CUSTOM_OPINFO_STUB);
    cfg_info_.push_back(TBE_CUSTOM_OPINFO_STUB);
    cfg_info_.push_back(TIK_OPINFO_STUB);
    cfg_info_.push_back(TBE_OPINFO_STUB);
    cfg_info_.push_back(RL_OPINFO_STUB);

    options.insert(std::pair<std::string, std::string>("ge.shape_generalized_build_mode", SHAPE_GENERALIZED));
    ge::GetThreadLocalContext().SetGlobalOption(options);
    OpsKernelManager::Instance(fe::AI_CORE_NAME).Finalize();
    std::map<std::string, std::string> options1;
    ops_info_store->Initialize(options1);

  }

  void TearDown() {
    sub_ops_store_ptr->FinalizeSubStore();
    sub_ops_store_ptr.reset();
    sub_ops_kernel_ptr->Finalize();
    sub_ops_kernel_ptr.reset();
    ops_info_store->Finalize();
  }

  static void CreateBatchNormGraph(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);

    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 32};
    GeShape shape(dims);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_FRACTAL_Z);
    in_desc2.SetOriginFormat(FORMAT_FRACTAL_Z);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("x", in_desc2);
    data->AddOutputDesc("x", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_NHWC);
    out_desc2.SetOriginFormat(FORMAT_NHWC);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("y", out_desc2);
    std::vector<bool> is_in_const_vec = {false};
    bn_op->SetIsInputConst(is_in_const_vec);

    ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetBool(bn_op, ge::ATTR_NAME_NOTASK, true);
    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr data_node = graph->AddNode(data);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), bn_node->GetInDataAnchor(0));
  }

  static void CreateSingleNodeGraph(ComputeGraphPtr graph) {
    OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Activation");
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    vector<int64_t> dims = {1, 2, 3, 4};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetOriginFormat(FORMAT_NCHW);
    in_desc1.SetDataType(DT_FLOAT16);
    relu_op->AddInputDesc("x", in_desc1);
    data->AddOutputDesc("x", in_desc1);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetOriginFormat(FORMAT_HWCN);
    out_desc1.SetDataType(DT_FLOAT16);
    relu_op->AddOutputDesc("y", out_desc1);

    ge::AttrUtils::SetInt(relu_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    NodePtr relu_node = graph->AddNode(relu_op);
    NodePtr data_node = graph->AddNode(data);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
  }

  static void CreateSingleNodeGraph2(ComputeGraphPtr graph) {
    OpDescPtr max_pool_op = std::make_shared<OpDesc>("maxpool", "MaxPoolV3");
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    vector<int64_t> dims = {1, 2, 3, 4};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetOriginFormat(FORMAT_NCHW);
    in_desc1.SetDataType(DT_FLOAT16);
    max_pool_op->AddInputDesc("x", in_desc1);
    data->AddOutputDesc("x", in_desc1);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetOriginFormat(FORMAT_HWCN);
    out_desc1.SetDataType(DT_FLOAT16);
    max_pool_op->AddOutputDesc("y", out_desc1);

    ge::AttrUtils::SetInt(max_pool_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    NodePtr relu_node = graph->AddNode(max_pool_op);
    NodePtr data_node = graph->AddNode(data);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
  }

  static void CreateComplexGraph(ComputeGraphPtr graph, string data_type) {
    shared_ptr<ge::OpDesc> op_desc_ptr = make_shared<ge::OpDesc>("tbe_conv2d", "Convolution");

    vector<ge::GeTensorPtr> weights;
    vector<int64_t> dim_weight = {256, 256, 512};
    uint32_t data_size = 256 * 256 * 512;
    GeShape shape_weight(dim_weight);
    GeTensorDesc weight_desc(shape_weight);
    GeTensor tensor(weight_desc);
    if (data_type == "float") {
      weight_desc.SetDataType(DT_FLOAT);
      vector<float> weight(data_size, 0.01);
      vector<uint8_t> buffer(data_size * 4);
      memcpy(buffer.data(), weight.data(), data_size * 4);
      tensor.SetData(std::move(buffer));
    } else if (data_type == "int32") {
      weight_desc.SetDataType(DT_INT32);
      vector<int32_t> weight(data_size, 1);
      vector<uint8_t> buffer(data_size * 4);
      memcpy(buffer.data(), weight.data(), data_size * 4);
      tensor.SetData(std::move(buffer));
    }
    GeTensorPtr tensor_ptr = make_shared<GeTensor>(tensor);
    weights.emplace_back(tensor_ptr);

    OpDescPtr weight_op_desc1 = std::make_shared<OpDesc>("w1", "Const");
    weight_op_desc1->AddInputDesc("x", weight_desc);
    weight_op_desc1->AddOutputDesc("z", weight_desc);
    std::vector<bool> w_input_const;
    w_input_const.emplace_back(false);
    weight_op_desc1->SetIsInputConst(w_input_const);

    int64_t int_value = 1;
    float float_value = 2.0;
    bool bool_value = false;
    string str_value = "abc";
    vector<int64_t> int_vec{1, 2, 3};
    vector<int64_t> rint_vec;
    vector<float> float_vec{4.0, 5.0, 6.0};
    vector<float> rfloat_vec;
    vector<bool> bool_vec{false, true, true};
    vector<bool> rbool_vec;
    std::vector<string> str_vec{"a", "b", "c"};
    AttrUtils::SetInt(op_desc_ptr, "transposX", int_value);
    AttrUtils::SetFloat(op_desc_ptr, "transposY", float_value);
    AttrUtils::SetBool(op_desc_ptr, "attrBool", bool_value);
    AttrUtils::SetStr(op_desc_ptr, "attrStr", str_value);
    AttrUtils::SetListInt(op_desc_ptr, "attrListInt", int_vec);
    AttrUtils::SetListFloat(op_desc_ptr, "attrListFloat", float_vec);
    AttrUtils::SetListBool(op_desc_ptr, "attrListBool", bool_vec);
    AttrUtils::SetListStr(op_desc_ptr, "attrListStr", str_vec);

    AttrUtils::SetInt(weight_op_desc1, "transposX", int_value);
    AttrUtils::SetFloat(weight_op_desc1, "transposY", float_value);
    AttrUtils::SetBool(weight_op_desc1, "attrBool", bool_value);
    AttrUtils::SetStr(weight_op_desc1, "attrStr", str_value);
    AttrUtils::SetListInt(weight_op_desc1, "attrListInt", int_vec);
    AttrUtils::SetListFloat(weight_op_desc1, "attrListFloat", float_vec);
    AttrUtils::SetListBool(weight_op_desc1, "attrListBool", bool_vec);
    AttrUtils::SetListStr(weight_op_desc1, "attrListStr", str_vec);

    ge::DataType set_dtype = ge::DT_FLOAT;
    std::vector<int64_t> shape_vec{256, 256, 512};
    ge::GeShape shape_desc = ge::GeShape(shape_vec);

    shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
    input0_desc_ptr->SetDataType(set_dtype);
    input0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr->AddInputDesc("x", input0_desc_ptr->Clone());

    shared_ptr<ge::GeTensorDesc> input1_desc_ptr = make_shared<ge::GeTensorDesc>();
    if (data_type == "float") {
      input1_desc_ptr->SetDataType(DT_FLOAT);
    } else if (data_type == "int32") {
      input1_desc_ptr->SetDataType(DT_INT32);
    }
    input1_desc_ptr->SetShape(shape_desc);
    op_desc_ptr->AddInputDesc("y", input1_desc_ptr->Clone());

    std::vector<bool> is_input_const;
    is_input_const.emplace_back(false);
    is_input_const.emplace_back(true);
    op_desc_ptr->SetIsInputConst(is_input_const);

    shared_ptr<ge::GeTensorDesc> output_desc_ptr = make_shared<ge::GeTensorDesc>();
    output_desc_ptr->SetDataType(set_dtype);
    output_desc_ptr->SetShape(shape_desc);
    op_desc_ptr->AddOutputDesc("z", output_desc_ptr->Clone());

    AttrUtils::SetInt(op_desc_ptr, "imply_type", EN_IMPL_HW_TBE);
    NodePtr weight_node1 = graph->AddNode(weight_op_desc1);
    NodePtr conv_node = graph->AddNode(op_desc_ptr);
    op_desc_ptr->SetName("conv2");
    NodePtr conv_next_node = graph->AddNode(op_desc_ptr);
    GraphUtils::AddEdge(weight_node1->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(weight_node1->GetOutDataAnchor(0), conv_next_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0), conv_next_node->GetInDataAnchor(0));
    OpDescUtils::SetWeights(*conv_node, weights);
    OpDescUtils::SetWeights(*conv_next_node, weights);
  }

  static void CreateSimpleGraph(ComputeGraphPtr graph) {
    shared_ptr<ge::OpDesc> op_desc_ptr = make_shared<ge::OpDesc>("tbe_conv2d", "conv");

    int64_t int_value = 1;
    float float_value = 2.0;
    bool bool_value = false;
    string str_value = "abc";
    vector<int64_t> int_vec{1, 2, 3};
    vector<int64_t> rint_vec;
    vector<float> float_vec{4.0, 5.0, 6.0};
    vector<float> rfloat_vec;
    vector<bool> bool_vec{false, true, true};
    vector<bool> rbool_vec;
    std::vector<string> str_vec{"a", "b", "c"};
    AttrUtils::SetInt(op_desc_ptr, "transposX", int_value);
    AttrUtils::SetFloat(op_desc_ptr, "transposY", float_value);
    AttrUtils::SetBool(op_desc_ptr, "attrBool", bool_value);
    AttrUtils::SetStr(op_desc_ptr, "attrStr", str_value);
    AttrUtils::SetListInt(op_desc_ptr, "attrListInt", int_vec);
    AttrUtils::SetListFloat(op_desc_ptr, "attrListFloat", float_vec);
    AttrUtils::SetListBool(op_desc_ptr, "attrListBool", bool_vec);
    AttrUtils::SetListStr(op_desc_ptr, "attrListStr", str_vec);

    ge::DataType set_dtype = ge::DT_FLOAT16;
    std::vector<int64_t> shape_vec{256, 256, 512};
    ge::GeShape shape_desc = ge::GeShape(shape_vec);

    shared_ptr<ge::GeTensorDesc> input0_desc_ptr = make_shared<ge::GeTensorDesc>();
    input0_desc_ptr->SetDataType(set_dtype);
    input0_desc_ptr->SetShape(shape_desc);
    op_desc_ptr->AddInputDesc("x", input0_desc_ptr->Clone());

    shared_ptr<ge::GeTensorDesc> input1_desc_ptr = make_shared<ge::GeTensorDesc>();
    input1_desc_ptr->SetDataType(set_dtype);
    input1_desc_ptr->SetShape(shape_desc);
    op_desc_ptr->AddInputDesc("y", input1_desc_ptr->Clone());

    std::vector<bool> is_input_const;
    is_input_const.emplace_back(false);
    is_input_const.emplace_back(true);
    op_desc_ptr->SetIsInputConst(is_input_const);

    shared_ptr<ge::GeTensorDesc> output_desc_ptr = make_shared<ge::GeTensorDesc>();
    output_desc_ptr->SetDataType(set_dtype);
    output_desc_ptr->SetShape(shape_desc);
    op_desc_ptr->AddOutputDesc("z", output_desc_ptr->Clone());

    AttrUtils::SetInt(op_desc_ptr, "imply_type", EN_IMPL_HW_TBE);
    NodePtr conv_node = graph->AddNode(op_desc_ptr);
    op_desc_ptr->SetName("conv2");
    NodePtr conv_next_node = graph->AddNode(op_desc_ptr);
    GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0), conv_next_node->GetInDataAnchor(0));
  }

  static ComputeGraphPtr BuildGraph(vector<Operator> inputs, string graph_name) {
    Graph graph(graph_name);
    vector<Operator> outputs{};
    graph.SetInputs(inputs).SetOutputs(outputs);
    return GraphUtils::GetComputeGraph(graph);
  }
  /*
        data1 data2         data3 data4
           \  /               /\  /
           conv             /   add
            |             /      |
      /   /   \  \      /        |
    mean neg SSDDetectionOutput PReLU
    (out1) \    /    \           |
            \  /       \       split
             sub         \     /  \
               \           mul   pooling
                 \        /  \   (out4)
                   \    /     \
                  FloorDiv activation
                   (out2)    (out3)
   */
/*    static ComputeGraphPtr GenGraphTest1()
    {
        auto data1 = op::Data();
        auto data2 = op::Data();
        auto data3 = op::Data();
        auto data4 = op::Data();
        auto conv = op::Conv2D().set_input_x(data1).set_input_filter(data2);
        auto add = op::Add().set_input_x1(data3).set_input_x2(data4);
        auto mean = op::Mean().set_input_x(conv);
        auto neg = op::Neg().set_input_x(conv);
        auto detect_out = op::SSDDetectionOutput().set_input_x1(conv).set_input_x2(conv).set_input_x3(data3);
        auto prelu = op::PReLU().set_input_x(add);
        auto sub = op::Sub().set_input_x1(neg).set_input_x2(detect_out, "y1");
        auto split = op::SplitD().set_input_input_value(prelu).create_dynamic_output_output_data(2).set_attr_split_dim(0).set_attr_num_split(1);
        auto mul = op::Mul().set_input_x(detect_out, "y2").set_input_y(split, "output_data0");
        auto pool = op::Pooling().set_input_x(split, "output_data1");
        auto div = op::FloorDiv().set_input_x(sub).set_input_y(mul);
        auto activation = op::Activation().set_input_x(mul);

        return BuildGraph({data1, data2, data3, data4}, "graph_test1");
    }
*/
  static ComputeGraphPtr GenGraphBiasAdd() {
    auto data1_feature = op::Data();
    auto data2_weight = op::Data();
    auto data3_bias = op::Data();

    auto matmul = op::MatMul().set_input_x1(data1_feature).set_input_x2(data2_weight);
    auto biasadd = op::BiasAdd().set_input_bias(data3_bias).set_input_x(matmul);

    auto y = op::Data().set_input_x(biasadd);

    return BuildGraph({data1_feature, data2_weight, data3_bias}, "graph_biasadd");
  }

  static ComputeGraphPtr SquareSumV1() {
    auto data1 = op::Data("Input0");
    auto square0 = op::Square("Square0").set_input_x(data1);
    auto sum0 = op::ReduceSum("Sum0").set_input_x(square0);
    auto data2 = op::Data("DataOut0").set_input_x(sum0);
    ComputeGraphPtr tmp_graph = BuildGraph({data1}, "graph_SquareSumV1");
    ge::NodePtr sq = tmp_graph->FindNode("Square0");
    string stream_label = "All_Cast_t";
    ge::AttrUtils::SetStr(sq->GetOpDesc(), "_stream_label", stream_label);
    return tmp_graph;
  }

  static ComputeGraphPtr GenGraphSoftGrad() {
    auto data1_grad_softmax = op::Data();
    auto data2_softmax = op::Data();

    auto mul1 = op::Mul().set_input_x1(data1_grad_softmax).set_input_x2(data2_softmax);
    auto sum0 = op::ReduceSum().set_input_x(mul1);
    auto mul2 = op::Mul().set_input_x1(sum0).set_input_x2(data2_softmax);

    auto grad_x = op::Data().set_input_x(mul2);

    return BuildGraph({data1_grad_softmax, data2_softmax}, "graph_softmaxgrad");
  }

  static void CreateTwoOpDescGraph(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
    OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Activation");
    OpDescPtr max_op = std::make_shared<OpDesc>("max", "Maximum");

    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 4};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetDataType(DT_FLOAT16);
    relu_op->AddInputDesc("x", in_desc1);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetDataType(DT_FLOAT16);
    relu_op->AddOutputDesc("y", out_desc1);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_FRACTAL_Z);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("x", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_NHWC);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("y", out_desc2);
    std::vector<bool> is_in_const_vec = {false};
    bn_op->SetIsInputConst(is_in_const_vec);

    GeTensorDesc in_desc3(shape);
    in_desc3.SetFormat(FORMAT_FRACTAL_Z);
    in_desc3.SetDataType(DT_FLOAT16);
    max_op->AddInputDesc("x", in_desc3);

    GeTensorDesc in_desc4(shape);
    in_desc4.SetFormat(FORMAT_FRACTAL_Z);
    in_desc4.SetDataType(DT_FLOAT16);
    max_op->AddInputDesc("y", in_desc4);

    GeTensorDesc out_desc3(shape);
    out_desc3.SetFormat(FORMAT_NHWC);
    out_desc3.SetDataType(DT_FLOAT16);
    max_op->AddOutputDesc("z", out_desc3);
    std::vector<bool> is_input_const_vec = {false, true};
    max_op->SetIsInputConst(is_input_const_vec);


    ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(relu_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(max_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));

    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr relu_node = graph->AddNode(relu_op);
    NodePtr max_node = graph->AddNode(max_op);
    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0), max_node->GetInDataAnchor(0));
  }

  static void CreateSplitOpDescGraph(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
    OpDescPtr split_op = std::make_shared<OpDesc>("split", "SplitD");
    OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Relu");
    // add descriptor
    vector<int64_t> dims = {1, 2};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_FRACTAL_NZ);
    in_desc1.SetOriginFormat(FORMAT_ND);
    in_desc1.SetOriginShape(shape);
    in_desc1.SetDataType(DT_FLOAT16);
    split_op->AddInputDesc("x", in_desc1);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetOriginShape(shape);
    out_desc1.SetDataType(DT_FLOAT16);
    split_op->AddOutputDesc("y", out_desc1);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_FRACTAL_Z);
    in_desc2.SetOriginShape(shape);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("x", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_NHWC);
    out_desc2.SetOriginShape(shape);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("y", out_desc2);


    GeTensorDesc in_desc4(shape);
    in_desc4.SetFormat(FORMAT_NCHW);
    in_desc4.SetOriginShape(shape);
    in_desc4.SetDataType(DT_FLOAT16);
    relu_op->AddInputDesc("x", in_desc4);

    GeTensorDesc out_desc4(shape);
    out_desc4.SetFormat(FORMAT_HWCN);
    out_desc4.SetOriginShape(shape);
    out_desc4.SetDataType(DT_FLOAT16);
    relu_op->AddOutputDesc("y", out_desc4);

    std::vector<bool> is_in_const_vec = {false};
    bn_op->SetIsInputConst(is_in_const_vec);

    ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(split_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    (void)ge::AttrUtils::SetInt(split_op, SPLIT_DIM, -4);
    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr split_node = graph->AddNode(split_op);
    NodePtr relu_node = graph->AddNode(relu_op);
    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0),
                        split_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(split_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
  }

  static void CreateConstSplitOpDescGraph(ComputeGraphPtr graph) {
    OpDescPtr const_op = std::make_shared<OpDesc>("const", "Const");
    OpDescPtr split_op = std::make_shared<OpDesc>("split", "SplitD");
    OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Relu");
    // add descriptor
    vector<int64_t> dims = {1, 2};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetOriginFormat(FORMAT_NCHW);
    in_desc1.SetOriginShape(shape);
    in_desc1.SetDataType(DT_FLOAT16);
    split_op->AddInputDesc("x", in_desc1);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_NCHW);
    out_desc1.SetOriginShape(shape);
    out_desc1.SetDataType(DT_FLOAT16);
    split_op->AddOutputDesc("y", out_desc1);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_NCHW);
    out_desc2.SetOriginShape(shape);
    out_desc2.SetDataType(DT_FLOAT16);
    const_op->AddOutputDesc("y", out_desc2);


    GeTensorDesc in_desc4(shape);
    in_desc4.SetFormat(FORMAT_NCHW);
    in_desc4.SetOriginShape(shape);
    in_desc4.SetDataType(DT_FLOAT16);
    relu_op->AddInputDesc("x", in_desc4);

    GeTensorDesc out_desc4(shape);
    out_desc4.SetFormat(FORMAT_NCHW);
    out_desc4.SetOriginShape(shape);
    out_desc4.SetDataType(DT_FLOAT16);
    relu_op->AddOutputDesc("y", out_desc4);

    ge::AttrUtils::SetInt(const_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(split_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    (void)ge::AttrUtils::SetInt(split_op, SPLIT_DIM, 0);
    NodePtr const_node = graph->AddNode(const_op);
    NodePtr split_node = graph->AddNode(split_op);
    NodePtr relu_node = graph->AddNode(relu_op);
    GraphUtils::AddEdge(const_node->GetOutDataAnchor(0),
                        split_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(split_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
  }

  static void CreateConcatOpDescGraph(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
    OpDescPtr shape_op = std::make_shared<OpDesc>("shape", "Shape");
    OpDescPtr concat_op = std::make_shared<OpDesc>("concat", "ConcatD");
    OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Relu");
    // add descriptor
    vector<int64_t> dims = {1, 2};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_FRACTAL_NZ);
    in_desc1.SetOriginFormat(FORMAT_ND);
    in_desc1.SetOriginShape(shape);
    in_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("x", in_desc1);

    GeTensorDesc in_desc11(shape);
    in_desc11.SetFormat(FORMAT_NCHW);
    in_desc11.SetOriginShape(shape);
    in_desc11.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("z", in_desc11);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetOriginShape(shape);
    out_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddOutputDesc("y", out_desc1);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_FRACTAL_Z);
    in_desc2.SetOriginShape(shape);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("x", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_NHWC);
    out_desc2.SetOriginShape(shape);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("y", out_desc2);

    GeTensorDesc in_desc3(shape);
    in_desc3.SetFormat(FORMAT_NCHW);
    in_desc3.SetOriginShape(shape);
    in_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddInputDesc("x", in_desc3);

    GeTensorDesc out_desc3(shape);
    out_desc3.SetFormat(FORMAT_HWCN);
    out_desc3.SetOriginShape(shape);
    out_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddOutputDesc("y", out_desc3);

    GeTensorDesc in_desc4(shape);
    in_desc4.SetFormat(FORMAT_NCHW);
    in_desc4.SetOriginShape(shape);
    in_desc4.SetDataType(DT_FLOAT16);
    relu_op->AddInputDesc("x", in_desc4);

    GeTensorDesc out_desc4(shape);
    out_desc4.SetFormat(FORMAT_HWCN);
    out_desc4.SetOriginShape(shape);
    out_desc4.SetDataType(DT_FLOAT16);
    relu_op->AddOutputDesc("y", out_desc4);

    std::vector<bool> is_in_const_vec = {false};
    bn_op->SetIsInputConst(is_in_const_vec);

    ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(concat_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    (void) ge::AttrUtils::SetInt(concat_op, CONCAT_DIM, -4);
    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr concat_node = graph->AddNode(concat_op);
    NodePtr shape_node = graph->AddNode(shape_op);
    NodePtr relu_node = graph->AddNode(relu_op);

    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(shape_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(concat_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
  }

  static void CreateConcatOpDescGraph2(ComputeGraphPtr graph) {
    OpDescPtr placeholder_op =
            std::make_shared<OpDesc>("placeholder", "PlaceHolder");
    OpDescPtr shape_op = std::make_shared<OpDesc>("shape", "Shape");
    OpDescPtr concat_op = std::make_shared<OpDesc>("concat", "ConcatD");

    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 32};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("x", in_desc1);

    GeTensorDesc in_desc11(shape);
    in_desc11.SetFormat(FORMAT_NCHW);
    in_desc11.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("z", in_desc11);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddOutputDesc("y", out_desc1);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_FRACTAL_Z);
    in_desc2.SetDataType(DT_FLOAT16);
    placeholder_op->AddInputDesc("x", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_NHWC);
    out_desc2.SetDataType(DT_FLOAT16);
    placeholder_op->AddOutputDesc("y", out_desc2);

    GeTensorDesc in_desc3(shape);
    in_desc3.SetFormat(FORMAT_NCHW);
    in_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddInputDesc("x", in_desc3);

    GeTensorDesc out_desc3(shape);
    out_desc3.SetFormat(FORMAT_HWCN);
    out_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddOutputDesc("y", out_desc3);

    std::vector<bool> is_in_const_vec = {false};
    placeholder_op->SetIsInputConst(is_in_const_vec);

    ge::AttrUtils::SetInt(placeholder_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(concat_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    (void) ge::AttrUtils::SetInt(concat_op, CONCAT_DIM, 0);
    NodePtr placeholder_node = graph->AddNode(placeholder_op);
    NodePtr concat_node = graph->AddNode(concat_op);
    NodePtr shape_node = graph->AddNode(shape_op);
    GraphUtils::AddEdge(placeholder_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(shape_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(1));
  }

  static void CreateConcatOpDescGraph3(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
    OpDescPtr shape_op = std::make_shared<OpDesc>("shape", "Shape");
    OpDescPtr concat_op = std::make_shared<OpDesc>("concat", "ConcatD");

    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 32};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("x", in_desc1);

    GeTensorDesc in_desc11(shape);
    in_desc11.SetFormat(FORMAT_NCHW);
    in_desc11.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("z", in_desc11);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddOutputDesc("y", out_desc1);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_FRACTAL_Z);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("x", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_NHWC);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("y", out_desc2);

    GeTensorDesc in_desc3(shape);
    in_desc3.SetFormat(FORMAT_NCHW);
    in_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddInputDesc("x", in_desc3);

    GeTensorDesc out_desc3(shape);
    out_desc3.SetFormat(FORMAT_HWCN);
    out_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddOutputDesc("y", out_desc3);

    std::vector<bool> is_in_const_vec = {false};
    bn_op->SetIsInputConst(is_in_const_vec);

    ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(concat_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    (void) ge::AttrUtils::SetInt(concat_op, CONCAT_DIM, 0);
    ge::AttrUtils::SetBool(bn_op, ge::ATTR_NAME_CONTINUOUS_INPUT, true);
    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr concat_node = graph->AddNode(concat_op);
    NodePtr shape_node = graph->AddNode(shape_op);
    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(shape_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(1));
  }

  static void CreateConcatOpDescGraph4(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
    OpDescPtr shape_op = std::make_shared<OpDesc>("shape", "Shape");
    OpDescPtr concat_op = std::make_shared<OpDesc>("concat", "ConcatD");

    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 32};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("x", in_desc1);

    GeTensorDesc in_desc11(shape);
    in_desc11.SetFormat(FORMAT_NCHW);
    in_desc11.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("z", in_desc11);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddOutputDesc("y", out_desc1);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_FRACTAL_Z);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("x", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_NHWC);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("y", out_desc2);

    GeTensorDesc in_desc3(shape);
    in_desc3.SetFormat(FORMAT_NCHW);
    in_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddInputDesc("x", in_desc3);

    GeTensorDesc out_desc3(shape);
    out_desc3.SetFormat(FORMAT_HWCN);
    out_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddOutputDesc("y", out_desc3);

    std::vector<bool> is_in_const_vec = {false};
    bn_op->SetIsInputConst(is_in_const_vec);

    ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(concat_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    (void) ge::AttrUtils::SetInt(concat_op, CONCAT_DIM, 0);
    ge::AttrUtils::SetBool(bn_op, ge::ATTR_NAME_CONTINUOUS_OUTPUT, true);
    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr concat_node = graph->AddNode(concat_op);
    NodePtr shape_node = graph->AddNode(shape_op);
    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(shape_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(1));
  }

  static void CreateConcatOpDescGraph5(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
    OpDescPtr shape_op = std::make_shared<OpDesc>("shape", "Shape");
    OpDescPtr concat_op = std::make_shared<OpDesc>("concat", "ConcatD");

    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 32};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("x", in_desc1);

    GeTensorDesc in_desc11(shape);
    in_desc11.SetFormat(FORMAT_NCHW);
    in_desc11.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("z", in_desc11);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddOutputDesc("y", out_desc1);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_FRACTAL_Z);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("x", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_NHWC);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("y", out_desc2);

    GeTensorDesc in_desc3(shape);
    in_desc3.SetFormat(FORMAT_NCHW);
    in_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddInputDesc("x", in_desc3);

    GeTensorDesc out_desc3(shape);
    out_desc3.SetFormat(FORMAT_HWCN);
    out_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddOutputDesc("y", out_desc3);

    std::vector<bool> is_in_const_vec = {false};
    bn_op->SetIsInputConst(is_in_const_vec);

    ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(concat_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    (void) ge::AttrUtils::SetInt(concat_op, CONCAT_DIM, 0);
    ge::AttrUtils::SetBool(bn_op, ge::ATTR_NAME_REFERENCE, true);
    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr concat_node = graph->AddNode(concat_op);
    NodePtr shape_node = graph->AddNode(shape_op);
    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(shape_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(1));
  }

  static void CreateConcatOpDescGraph6(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
    OpDescPtr shape_op = std::make_shared<OpDesc>("shape", "Shape");
    OpDescPtr concat_op = std::make_shared<OpDesc>("concat", "ConcatD");
    OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Relu");

    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 32};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("x", in_desc1);

    GeTensorDesc in_desc11(shape);
    in_desc11.SetFormat(FORMAT_NCHW);
    in_desc11.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("z", in_desc11);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddOutputDesc("y", out_desc1);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_FRACTAL_Z);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("x", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_NHWC);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("y", out_desc2);

    GeTensorDesc in_desc3(shape);
    in_desc3.SetFormat(FORMAT_NCHW);
    in_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddInputDesc("x", in_desc3);

    GeTensorDesc out_desc3(shape);
    out_desc3.SetFormat(FORMAT_HWCN);
    out_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddOutputDesc("y", out_desc3);

    GeTensorDesc in_desc4(shape);
    in_desc4.SetFormat(FORMAT_NCHW);
    in_desc4.SetDataType(DT_FLOAT16);
    relu_op->AddInputDesc("x", in_desc4);

    GeTensorDesc out_desc4(shape);
    out_desc4.SetFormat(FORMAT_HWCN);
    out_desc4.SetDataType(DT_FLOAT16);
    relu_op->AddOutputDesc("y", out_desc4);

    std::vector<bool> is_in_const_vec = {false};
    bn_op->SetIsInputConst(is_in_const_vec);

    ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(concat_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    (void) ge::AttrUtils::SetInt(concat_op, CONCAT_DIM, 0);
    ge::AttrUtils::SetBool(bn_op, ge::ATTR_NAME_NOTASK, true);
    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr concat_node = graph->AddNode(concat_op);
    NodePtr shape_node = graph->AddNode(shape_op);
    NodePtr relu_node = graph->AddNode(relu_op);
    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(shape_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(concat_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
  }

  static void CreateConcatOpDescGraph7(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
    OpDescPtr concat_op = std::make_shared<OpDesc>("concat", "ConcatD");
    OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Relu");
    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 32};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("x", in_desc1);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddOutputDesc("y", out_desc1);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_FRACTAL_Z);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("x", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_NHWC);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("y", out_desc2);
    std::vector<bool> is_in_const_vec = {false};
    bn_op->SetIsInputConst(is_in_const_vec);
    GeTensorDesc in_desc4(shape);
    in_desc4.SetFormat(FORMAT_NCHW);
    in_desc4.SetDataType(DT_FLOAT16);
    relu_op->AddInputDesc("x", in_desc4);

    GeTensorDesc out_desc4(shape);
    out_desc4.SetFormat(FORMAT_HWCN);
    out_desc4.SetDataType(DT_FLOAT16);
    relu_op->AddOutputDesc("y", out_desc4);

    ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(concat_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    (void) ge::AttrUtils::SetInt(concat_op, CONCAT_DIM, 0);
    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr concat_node = graph->AddNode(concat_op);
    NodePtr relu_node = graph->AddNode(relu_op);
    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(concat_node->GetOutDataAnchor(0),
                        relu_node->GetInDataAnchor(0));
  }

  static void CreateConcatOpDescGraph8(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
    OpDescPtr shape_op = std::make_shared<OpDesc>("shape", "Shape");
    OpDescPtr concat_op = std::make_shared<OpDesc>("concat", "ConcatD");

    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 32};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("x", in_desc1);

    GeTensorDesc in_desc11(shape);
    in_desc11.SetFormat(FORMAT_NCHW);
    in_desc11.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("z", in_desc11);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddOutputDesc("y", out_desc1);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_FRACTAL_Z);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("x", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_NHWC);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("y", out_desc2);

    GeTensorDesc in_desc3(shape);
    in_desc3.SetFormat(FORMAT_NCHW);
    in_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddInputDesc("x", in_desc3);

    GeTensorDesc out_desc3(shape);
    out_desc3.SetFormat(FORMAT_HWCN);
    out_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddOutputDesc("y", out_desc3);

    std::vector<bool> is_in_const_vec = {false};
    bn_op->SetIsInputConst(is_in_const_vec);

    ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(concat_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    (void) ge::AttrUtils::SetInt(concat_op, CONCAT_DIM, 1);
    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr concat_node = graph->AddNode(concat_op);
    NodePtr shape_node = graph->AddNode(shape_op);
    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(shape_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(1));
  }

  static void CreateConcatOpDescGraph9(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
    OpDescPtr shape_op = std::make_shared<OpDesc>("shape", "Shape");
    OpDescPtr concat_op = std::make_shared<OpDesc>("concat", "ConcatD");

    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 32};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("x", in_desc1);

    GeTensorDesc in_desc11(shape);
    in_desc11.SetFormat(FORMAT_NCHW);
    in_desc11.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("z", in_desc11);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddOutputDesc("y", out_desc1);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_FRACTAL_Z);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("x", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_NHWC);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("y", out_desc2);

    GeTensorDesc in_desc3(shape);
    in_desc3.SetFormat(FORMAT_NCHW);
    in_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddInputDesc("x", in_desc3);

    GeTensorDesc out_desc3(shape);
    out_desc3.SetFormat(FORMAT_HWCN);
    out_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddOutputDesc("y", out_desc3);

    std::vector<bool> is_in_const_vec = {false};
    bn_op->SetIsInputConst(is_in_const_vec);

    ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(concat_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    (void) ge::AttrUtils::SetInt(concat_op, CONCAT_DIM, 0);
    vector<int64_t> output_index;
    output_index.push_back(0);
    (void) ge::AttrUtils::SetListInt(bn_op, ge::ATOMIC_ATTR_OUTPUT_INDEX,
                                     output_index);
    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr concat_node = graph->AddNode(concat_op);
    NodePtr shape_node = graph->AddNode(shape_op);
    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(shape_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(1));
  }

  static void CreateConcatOpDescGraph10(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
    OpDescPtr concat_op = std::make_shared<OpDesc>("concat", "ConcatD");

    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 32};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("x", in_desc1);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddOutputDesc("y", out_desc1);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_FRACTAL_Z);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("x", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_NHWC);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("y", out_desc2);
    std::vector<bool> is_in_const_vec = {false};
    bn_op->SetIsInputConst(is_in_const_vec);

    ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(concat_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    (void) ge::AttrUtils::SetInt(concat_op, CONCAT_DIM, 0);
    ge::AttrUtils::SetBool(bn_op, ge::ATTR_NAME_NOTASK, true);
    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr concat_node = graph->AddNode(concat_op);
    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(0));
  }

  static void CreateConcatOpDescGraph11(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
    OpDescPtr shape_op = std::make_shared<OpDesc>("shape", "Shape");
    OpDescPtr concat_op = std::make_shared<OpDesc>("concat", "ConcatD");

    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 32};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("x", in_desc1);

    GeTensorDesc in_desc11(shape);
    in_desc11.SetFormat(FORMAT_NCHW);
    in_desc11.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("z", in_desc11);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddOutputDesc("y", out_desc1);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_FRACTAL_Z);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("x", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_NHWC);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("y", out_desc2);

    GeTensorDesc in_desc3(shape);
    in_desc3.SetFormat(FORMAT_NCHW);
    in_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddInputDesc("x", in_desc3);

    GeTensorDesc out_desc3(shape);
    out_desc3.SetFormat(FORMAT_HWCN);
    out_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddOutputDesc("y", out_desc3);

    std::vector<bool> is_in_const_vec = {false};
    bn_op->SetIsInputConst(is_in_const_vec);

    ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(concat_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    (void) ge::AttrUtils::SetInt(concat_op, CONCAT_DIM, 0);
    ge::AttrUtils::SetBool(shape_op, ge::ATTR_NAME_REFERENCE, true);
    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr concat_node = graph->AddNode(concat_op);
    NodePtr shape_node = graph->AddNode(shape_op);
    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(shape_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(1));
  }

  static void CreateConcatOpDescGraph12(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
    OpDescPtr shape_op = std::make_shared<OpDesc>("shape", "Shape");
    OpDescPtr concat_op = std::make_shared<OpDesc>("concat", "ConcatD");
    OpDescPtr end_op = std::make_shared<OpDesc>("end", "End");

    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 32};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("x", in_desc1);

    GeTensorDesc in_desc11(shape);
    in_desc11.SetFormat(FORMAT_NCHW);
    in_desc11.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("z", in_desc11);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddOutputDesc("y", out_desc1);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_FRACTAL_Z);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("x", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_NHWC);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("y", out_desc2);

    GeTensorDesc in_desc3(shape);
    in_desc3.SetFormat(FORMAT_NCHW);
    in_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddInputDesc("x", in_desc3);

    GeTensorDesc out_desc3(shape);
    out_desc3.SetFormat(FORMAT_HWCN);
    out_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddOutputDesc("y", out_desc3);

    GeTensorDesc in_desc4(shape);
    in_desc4.SetFormat(FORMAT_NCHW);
    in_desc4.SetDataType(DT_FLOAT16);
    end_op->AddInputDesc("x", in_desc4);

    GeTensorDesc out_desc4(shape);
    out_desc4.SetFormat(FORMAT_HWCN);
    out_desc4.SetDataType(DT_FLOAT16);
    end_op->AddOutputDesc("y", out_desc4);

    std::vector<bool> is_in_const_vec = {false};
    bn_op->SetIsInputConst(is_in_const_vec);

    ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(concat_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    (void) ge::AttrUtils::SetInt(concat_op, CONCAT_DIM, 0);
    ge::AttrUtils::SetBool(bn_op, ge::ATTR_NAME_NOTASK, true);
    ge::AttrUtils::SetStr(end_op, "parentOpType", "NetOutput");
    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr concat_node = graph->AddNode(concat_op);
    NodePtr shape_node = graph->AddNode(shape_op);
    NodePtr end_node = graph->AddNode(end_op);
    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(shape_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(concat_node->GetOutDataAnchor(0),
                        end_node->GetInDataAnchor(0));
  }

  static void CreateConcatOpDescGraph13(ComputeGraphPtr graph) {
    OpDescPtr placeholder_op1 = std::make_shared<OpDesc>("placeholder1", "PlaceHolder");
    OpDescPtr placeholder_op2 = std::make_shared<OpDesc>("placeholder2", "PlaceHolder");
    OpDescPtr concat_op = std::make_shared<OpDesc>("concat", "ConcatD");
    // add descriptor
    vector<int64_t> ori_dims = {1, 32, 3, 32};
    GeShape ori_shape(ori_dims);
    vector<int64_t> dims = {1, 1, 3, 32, 32};
    GeShape shape(dims);
    GeTensorDesc desc;
    desc.SetOriginShape(ori_shape);
    desc.SetShape(shape);
    desc.SetFormat(FORMAT_NC1HWC0);
    desc.SetOriginFormat(FORMAT_NCHW);
    desc.SetDataType(DT_INT8);
    concat_op->AddInputDesc("x", desc);
    concat_op->AddInputDesc("z", desc);
    concat_op->AddOutputDesc("y", desc);
    placeholder_op1->AddInputDesc("x", desc);
    placeholder_op1->AddOutputDesc("y", desc);
    placeholder_op2->AddInputDesc("x", desc);
    placeholder_op2->AddOutputDesc("y", desc);

    std::vector<bool> is_in_const_vec = {false};
    placeholder_op1->SetIsInputConst(is_in_const_vec);
    placeholder_op2->SetIsInputConst(is_in_const_vec);
    ge::AttrUtils::SetInt(placeholder_op1, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(placeholder_op2, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(concat_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    (void) ge::AttrUtils::SetInt(concat_op, CONCAT_DIM, 1);

    NodePtr placeholder_node1 = graph->AddNode(placeholder_op1);
    NodePtr placeholder_node2 = graph->AddNode(placeholder_op2);
    NodePtr concat_node = graph->AddNode(concat_op);

    GraphUtils::AddEdge(placeholder_node1->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(placeholder_node2->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(1));

  }

 static void CreateConcatOpDescGraph14(ComputeGraphPtr graph) {
    OpDescPtr placeholder_op1 = std::make_shared<OpDesc>("placeholder1", "PlaceHolder");
    OpDescPtr placeholder_op2 = std::make_shared<OpDesc>("placeholder2", "PlaceHolder");
    OpDescPtr concat_op = std::make_shared<OpDesc>("concat", "ConcatD");
    // add descriptor
    vector<int64_t> ori_dims1 = {1, 32, 3, 32};
    GeShape ori_shape1(ori_dims1);
    vector<int64_t> dims1 = {1, 1, 3, 32, 32};
    GeShape shape1(dims1);
    GeTensorDesc desc1;
    desc1.SetOriginShape(ori_shape1);
    desc1.SetShape(shape1);
    desc1.SetFormat(FORMAT_NC1HWC0);
    desc1.SetOriginFormat(FORMAT_NCHW);
    desc1.SetDataType(DT_INT8);
    
    vector<int64_t> ori_dims2 = {1, 2, 3, 32};
    GeShape ori_shape2(ori_dims2);
    vector<int64_t> dims2 = {1, 1, 3, 32, 32};
    GeShape shape2(dims2);
    GeTensorDesc desc2;
    desc2.SetOriginShape(ori_shape2);
    desc2.SetShape(shape2);
    desc2.SetFormat(FORMAT_NC1HWC0);
    desc2.SetOriginFormat(FORMAT_NCHW);
    desc2.SetDataType(DT_INT8);  
    concat_op->AddInputDesc("x", desc1);
    concat_op->AddInputDesc("z", desc2);
    concat_op->AddOutputDesc("y", desc1);
    placeholder_op1->AddInputDesc("x", desc1);
    placeholder_op1->AddOutputDesc("y", desc1);
    placeholder_op2->AddInputDesc("x", desc2);
    placeholder_op2->AddOutputDesc("y", desc2);

    std::vector<bool> is_in_const_vec = {false};
    placeholder_op1->SetIsInputConst(is_in_const_vec);
    placeholder_op2->SetIsInputConst(is_in_const_vec);
    ge::AttrUtils::SetInt(placeholder_op1, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(placeholder_op2, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(concat_op, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    (void) ge::AttrUtils::SetInt(concat_op, CONCAT_DIM, 1);

    NodePtr placeholder_node1 = graph->AddNode(placeholder_op1);
    NodePtr placeholder_node2 = graph->AddNode(placeholder_op2);
    NodePtr concat_node = graph->AddNode(concat_op);

    GraphUtils::AddEdge(placeholder_node1->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(placeholder_node2->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(1));
  }
  static void CreateConv2dGraph(ComputeGraphPtr graph) {
    OpDescPtr conv2d = std::make_shared<OpDesc>("conv2d", CONV2D);
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);

    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 3};
    GeShape shape(dims);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_NHWC);
    in_desc2.SetOriginFormat(FORMAT_NHWC);
    in_desc2.SetDataType(DT_FLOAT16);
    conv2d->AddInputDesc("x", in_desc2);
    data->AddOutputDesc("x", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_NHWC);
    out_desc2.SetOriginFormat(FORMAT_NHWC);
    out_desc2.SetDataType(DT_FLOAT16);
    conv2d->AddOutputDesc("y", out_desc2);
    std::vector<bool> is_in_const_vec = {false};
    conv2d->SetIsInputConst(is_in_const_vec);

    ge::AttrUtils::SetInt(conv2d, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetBool(conv2d, ge::ATTR_NAME_NOTASK, true);
    NodePtr bn_node = graph->AddNode(conv2d);
    NodePtr data_node = graph->AddNode(data);
    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), bn_node->GetInDataAnchor(0));
  }
 static void CreateConcatOpDescGraph15(ComputeGraphPtr graph) {
    OpDescPtr placeholder_op1 = std::make_shared<OpDesc>("placeholder1", "PlaceHolder");
    OpDescPtr placeholder_op2 = std::make_shared<OpDesc>("placeholder2", "PlaceHolder");
    OpDescPtr placeholder_op3 = std::make_shared<OpDesc>("placeholder3", "PlaceHolder");
    OpDescPtr concat_op1 = std::make_shared<OpDesc>("concat1", "ConcatD");
    OpDescPtr concat_op2 = std::make_shared<OpDesc>("concat2", "ConcatD");
    // add descriptor
    vector<int64_t> ori_dims1 = {1, 32, 3, 32};
    GeShape ori_shape1(ori_dims1);
    vector<int64_t> dims1 = {1, 1, 3, 32, 32};
    GeShape shape1(dims1);
    GeTensorDesc desc1;
    desc1.SetOriginShape(ori_shape1);
    desc1.SetShape(shape1);
    desc1.SetFormat(FORMAT_NC1HWC0);
    desc1.SetOriginFormat(FORMAT_NCHW);
    desc1.SetDataType(DT_INT8);
    
    vector<int64_t> ori_dims2 = {1, 32, 3, 32};
    GeShape ori_shape2(ori_dims2);
    vector<int64_t> dims2 = {1, 1, 3, 32, 32};
    GeShape shape2(dims2);
    GeTensorDesc desc2;
    desc2.SetOriginShape(ori_shape2);
    desc2.SetShape(shape2);
    desc2.SetFormat(FORMAT_NC1HWC0);
    desc2.SetOriginFormat(FORMAT_NCHW);
    desc2.SetDataType(DT_INT8);
    
    vector<int64_t> ori_dims3 = {1, 32, 3, 32};
    GeShape ori_shape3(ori_dims3);
    vector<int64_t> dims3 = {1, 1, 3, 32, 32};
    GeShape shape3(dims3);
    GeTensorDesc desc3;
    desc3.SetOriginShape(ori_shape3);
    desc3.SetShape(shape3);
    desc3.SetFormat(FORMAT_NC1HWC0);
    desc3.SetOriginFormat(FORMAT_NCHW);
    desc3.SetDataType(DT_INT8);

    concat_op1->AddInputDesc("x", desc1);
    concat_op1->AddInputDesc("z", desc2);
    concat_op1->AddOutputDesc("y", desc1);

    concat_op2->AddInputDesc("x", desc1);
    concat_op2->AddInputDesc("z", desc3);
    concat_op2->AddOutputDesc("y", desc1);

    placeholder_op1->AddInputDesc("x", desc1);
    placeholder_op1->AddOutputDesc("y", desc1);
    placeholder_op2->AddInputDesc("x", desc2);
    placeholder_op2->AddOutputDesc("y", desc2);
    placeholder_op3->AddInputDesc("x", desc3);
    placeholder_op3->AddOutputDesc("y", desc3);

    std::vector<bool> is_in_const_vec = {false};
    placeholder_op1->SetIsInputConst(is_in_const_vec);
    placeholder_op2->SetIsInputConst(is_in_const_vec);
    placeholder_op3->SetIsInputConst(is_in_const_vec);
    ge::AttrUtils::SetInt(placeholder_op1, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(placeholder_op2, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(placeholder_op3, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_TBE));

    ge::AttrUtils::SetInt(concat_op1, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    (void) ge::AttrUtils::SetInt(concat_op1, CONCAT_DIM, 1);
    ge::AttrUtils::SetInt(concat_op2, FE_IMPLY_TYPE,
                          static_cast<int>(EN_IMPL_HW_GENERAL_CCE));
    (void) ge::AttrUtils::SetInt(concat_op2, CONCAT_DIM, 1);

    NodePtr placeholder_node1 = graph->AddNode(placeholder_op1);
    NodePtr placeholder_node2 = graph->AddNode(placeholder_op2);
    NodePtr placeholder_node3 = graph->AddNode(placeholder_op3);
    NodePtr concat_node1 = graph->AddNode(concat_op1);
    NodePtr concat_node2 = graph->AddNode(concat_op2);

    GraphUtils::AddEdge(placeholder_node1->GetOutDataAnchor(0),
                        concat_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(placeholder_node2->GetOutDataAnchor(0),
                        concat_node1->GetInDataAnchor(1));
    GraphUtils::AddEdge(placeholder_node1->GetOutDataAnchor(0),
                        concat_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(placeholder_node3->GetOutDataAnchor(0),
                        concat_node2->GetInDataAnchor(1));
  }
  static void CreateConcatOpDescGraph16(ComputeGraphPtr graph) {
    OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
    OpDescPtr shape_op = std::make_shared<OpDesc>("shape", "Shape");
    OpDescPtr reshape_op1 = std::make_shared<OpDesc>("reshape1", "Reshape");
    OpDescPtr concat_op = std::make_shared<OpDesc>("concat", "ConcatD");
    OpDescPtr reshape_op2 = std::make_shared<OpDesc>("reshape2", "Reshape");
    OpDescPtr end_op = std::make_shared<OpDesc>("end", "End");

    // add descriptor
    vector<int64_t> dims = {1, 2, 3, 32};
    GeShape shape(dims);

    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("x", in_desc1);

    GeTensorDesc in_desc11(shape);
    in_desc11.SetFormat(FORMAT_NCHW);
    in_desc11.SetDataType(DT_FLOAT16);
    concat_op->AddInputDesc("z", in_desc11);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_NCHW);
    out_desc1.SetDataType(DT_FLOAT16);
    concat_op->AddOutputDesc("y", out_desc1);

    GeTensorDesc in_desc2(shape);
    in_desc2.SetFormat(FORMAT_NCHW);
    in_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("x", in_desc2);

    GeTensorDesc out_desc2(shape);
    out_desc2.SetFormat(FORMAT_NCHW);
    out_desc2.SetDataType(DT_FLOAT16);
    bn_op->AddOutputDesc("y", out_desc2);

    GeTensorDesc in_desc3(shape);
    in_desc3.SetFormat(FORMAT_NCHW);
    in_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddInputDesc("x", in_desc3);
    reshape_op1->AddInputDesc("x", in_desc3);

    GeTensorDesc out_desc3(shape);
    out_desc3.SetFormat(FORMAT_NCHW);
    out_desc3.SetDataType(DT_FLOAT16);
    shape_op->AddOutputDesc("y", out_desc3);
    reshape_op1->AddOutputDesc("y", out_desc3);

    GeTensorDesc in_desc4(shape);
    in_desc4.SetFormat(FORMAT_NCHW);
    in_desc4.SetDataType(DT_FLOAT16);
    end_op->AddInputDesc("x", in_desc4);
    reshape_op2->AddInputDesc("x", in_desc4);

    GeTensorDesc out_desc4(shape);
    out_desc4.SetFormat(FORMAT_NCHW);
    out_desc4.SetDataType(DT_FLOAT16);
    end_op->AddOutputDesc("y", out_desc4);
    reshape_op2->AddOutputDesc("y", out_desc4);

    std::vector<bool> is_in_const_vec = {false};
    bn_op->SetIsInputConst(is_in_const_vec);

    ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(shape_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(reshape_op1, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(reshape_op2, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));
    ge::AttrUtils::SetInt(concat_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));
    (void)ge::AttrUtils::SetInt(concat_op, CONCAT_DIM, 0);
    ge::AttrUtils::SetStr(end_op, "parentOpType", "NetOutput");
    NodePtr bn_node = graph->AddNode(bn_op);
    NodePtr concat_node = graph->AddNode(concat_op);
    NodePtr shape_node = graph->AddNode(shape_op);
    NodePtr reshape_node1 = graph->AddNode(reshape_op1);
    NodePtr end_node = graph->AddNode(end_op);
    NodePtr reshape_node2 = graph->AddNode(reshape_op2);
    GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(shape_node->GetOutDataAnchor(0),
                        reshape_node1->GetInDataAnchor(0));
    GraphUtils::AddEdge(reshape_node1->GetOutDataAnchor(0),
                        concat_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(concat_node->GetOutDataAnchor(0),
                        reshape_node2->GetInDataAnchor(0));
    GraphUtils::AddEdge(reshape_node2->GetOutDataAnchor(0),
                        end_node->GetInDataAnchor(0));
  }

  static ComputeGraphPtr CreateCastReluCastGraph6() {
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test1");
    OpDescPtr op_desc_cast1 = std::make_shared<OpDesc>("cast1", "Cast");
    OpDescPtr op_desc_cast3 = std::make_shared<OpDesc>("cast3", "Cast");
    OpDescPtr op_desc_cast4 = std::make_shared<OpDesc>("loss_scale/gradients/fp32_vars/conv2d_15/Conv2D_grad/Conv2DBackpropInput_dilation", "Cast");
    OpDescPtr op_desc_relu = std::make_shared<OpDesc>("relu", "Relu");
    OpDescPtr op_desc_cast2 = std::make_shared<OpDesc>("loss_scale/gradients/fp32_vars/conv2d_15/Conv2D_grad/Conv2DBackpropInput_dilation", "Cast");
    OpDescPtr op_desc_output = std::make_shared<OpDesc>("output", "NetOutput");
    OpDescPtr op_desc_input = std::make_shared<OpDesc>("other", "Other");

    //add descriptor
    vector<int64_t> dim_a = {8, 4, 16, 16};
    GeShape shape_a(dim_a);
    GeTensorDesc tensor_desc_a(shape_a);
    tensor_desc_a.SetFormat(FORMAT_NCHW);
    tensor_desc_a.SetOriginFormat(FORMAT_NCHW);
    tensor_desc_a.SetDataType(DT_FLOAT16);
    tensor_desc_a.SetOriginDataType(DT_FLOAT);

    vector<int64_t> dim_b = {1, 4, 64, 64};
    GeShape shape_b(dim_b);
    GeTensorDesc tensor_desc_b(shape_b);
    tensor_desc_b.SetFormat(FORMAT_NCHW);
    tensor_desc_b.SetOriginFormat(FORMAT_NCHW);
    tensor_desc_b.SetDataType(DT_FLOAT);
    tensor_desc_b.SetOriginDataType(DT_FLOAT);

    vector<int64_t> dim_c = {1, 4, 64, 64};
    GeShape shape_c(dim_c);
    GeTensorDesc tensor_desc_c(shape_c);
    tensor_desc_c.SetFormat(FORMAT_NCHW);
    tensor_desc_c.SetOriginFormat(FORMAT_NCHW);
    tensor_desc_c.SetDataType(DT_FLOAT);
    tensor_desc_c.SetOriginDataType(DT_FLOAT);

    //vector<int64_t> dim_d;
    GeShape shape_d(dim_a);
    GeTensorDesc tensor_desc_d(shape_d);
    tensor_desc_d.SetFormat(FORMAT_NCHW);
    tensor_desc_d.SetOriginFormat(FORMAT_NCHW);
    tensor_desc_d.SetDataType(DT_FLOAT16);
    tensor_desc_d.SetOriginDataType(DT_FLOAT);

    op_desc_input->AddOutputDesc(tensor_desc_a);

    op_desc_cast1->AddInputDesc(tensor_desc_a);
    op_desc_cast1->AddOutputDesc(tensor_desc_b);

    op_desc_cast3->AddInputDesc(tensor_desc_c);
    op_desc_cast3->AddOutputDesc(tensor_desc_d);

    op_desc_cast4->AddInputDesc(tensor_desc_c);
    op_desc_cast4->AddOutputDesc(tensor_desc_c);

    op_desc_relu->AddInputDesc(tensor_desc_b);
    op_desc_relu->AddOutputDesc(tensor_desc_c);

    op_desc_cast2->AddInputDesc(tensor_desc_c);
    op_desc_cast2->AddOutputDesc(tensor_desc_d);

    op_desc_output->AddInputDesc(tensor_desc_d);
    op_desc_output->AddInputDesc(tensor_desc_d);
    op_desc_output->AddInputDesc(tensor_desc_c);

    NodePtr node_cast1 = graph->AddNode(op_desc_cast1);
    NodePtr node_cast3 = graph->AddNode(op_desc_cast3);
    NodePtr node_cast4 = graph->AddNode(op_desc_cast4);
    NodePtr node_relu = graph->AddNode(op_desc_relu);
    NodePtr node_cast2 = graph->AddNode(op_desc_cast2);
    NodePtr node_netoutput = graph->AddNode(op_desc_output);
    NodePtr node_other = graph->AddNode(op_desc_input);
    (void)ge::AttrUtils::SetInt(node_cast1->GetOpDesc(), kThreadScopeId, 1);
    (void)ge::AttrUtils::SetInt(node_cast3->GetOpDesc(), kThreadScopeId, 2);
    (void)AttrUtils::SetInt(node_cast1->GetOpDesc(), FE_IMPLY_TYPE, 1);
    (void)AttrUtils::SetInt(node_cast3->GetOpDesc(), FE_IMPLY_TYPE, 1);
    GraphUtils::AddEdge(node_other->GetOutDataAnchor(0), node_cast1->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_cast1->GetOutDataAnchor(0), node_relu->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_relu->GetOutDataAnchor(0), node_cast2->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_relu->GetOutDataAnchor(0), node_cast3->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_relu->GetOutDataAnchor(0), node_cast4->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_cast2->GetOutDataAnchor(0), node_netoutput->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_cast3->GetOutDataAnchor(0), node_netoutput->GetInDataAnchor(1));
    GraphUtils::AddEdge(node_cast4->GetOutDataAnchor(0), node_netoutput->GetInDataAnchor(2));

    return graph;
  }
};

TEST_F(STEST_fusion_engine_fe_graph_optimizer, shape_and_value_generalize_success)
{
    auto graph = std::make_shared<ComputeGraph>("test");
    CreateConcatOpDescGraph10(graph);
    vector<int64_t> dims = {1, 2, 3, 32};
    vector<int64_t> shape_vec;
    OptimizeUtilitySTStub *optimize_utility_stub = new OptimizeUtilitySTStub();
    auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_info_store, op_store_adapter_manager_ptr_);
    fe_graph_optimizer_ptr->optimize_utility_ = optimize_utility_stub;
    tbe_op_store_adapter->CheckIsTbeGeneralizeFuncRegistered = checkIsRegistered;
    tbe_op_store_adapter->TeGeneralize = teGeneralize;
    Status status = fe_graph_optimizer_ptr->ShapeAndValueGeneralize(*(graph.get()));

    EXPECT_EQ(fe::SUCCESS, status);
    for (auto node : graph->GetDirectNode()) {
      auto op_desc = node->GetOpDesc();
      if (op_desc->GetType() != "ConcatD") {
        continue;
      }
      auto tensor_desc = op_desc->MutableInputDesc("x");
      shape_vec = tensor_desc->GetShape().GetDims();
    }
    EXPECT_EQ(shape_vec, dims);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, shape_and_value_generalize_success1)
{
    auto graph = std::make_shared<ComputeGraph>("test");
    CreateSimpleGraph(graph);
    std::vector<int64_t> dims = {256, 256, 512};
    std::vector<int64_t> shape_vec;
    OptimizeUtilitySTStub *optimize_utility_stub = new OptimizeUtilitySTStub();
    auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_info_store, op_store_adapter_manager_ptr_);
    fe_graph_optimizer_ptr->optimize_utility_ = optimize_utility_stub;
    tbe_op_store_adapter->CheckIsTbeGeneralizeFuncRegistered = checkIsRegistered;
    tbe_op_store_adapter->TeGeneralize = teGeneralize;
    Status status = fe_graph_optimizer_ptr->ShapeAndValueGeneralize(*(graph.get()));
    EXPECT_EQ(fe::SUCCESS, status);
    for (auto node : graph->GetDirectNode()) {
      auto op_desc = node->GetOpDesc();
      auto tensor_desc_x = op_desc->MutableInputDesc("x");
      auto tensor_desc_y = op_desc->MutableInputDesc("y");
      shape_vec = tensor_desc_x->GetShape().GetDims();
      EXPECT_EQ(shape_vec, dims);
      shape_vec = tensor_desc_y->GetShape().GetDims();
      EXPECT_EQ(shape_vec, dims);
    }
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, shape_and_value_generalize_success2)
{
    auto graph = std::make_shared<ComputeGraph>("test");
    CreateSingleNodeGraph(graph);
    std::vector<int64_t> dims = {1, 2, 3, 4};
    std::vector<int64_t> shape_vec;
    OptimizeUtilitySTStub *optimize_utility_stub = new OptimizeUtilitySTStub();
    auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_info_store, op_store_adapter_manager_ptr_);
    fe_graph_optimizer_ptr->optimize_utility_ = optimize_utility_stub;
    tbe_op_store_adapter->CheckIsTbeGeneralizeFuncRegistered = checkIsRegistered;
    tbe_op_store_adapter->TeGeneralize = teGeneralize;
    Status status = fe_graph_optimizer_ptr->ShapeAndValueGeneralize(*(graph.get()));
    EXPECT_EQ(fe::SUCCESS, status);
    for (auto node : graph->GetDirectNode()) {
      if (node->GetType() == fe::DATA) {
        continue;
      }
      auto op_desc = node->GetOpDesc();
      auto tensor_desc_x = op_desc->MutableInputDesc("x");
      shape_vec = tensor_desc_x->GetShape().GetDims();
    }
    EXPECT_EQ(shape_vec, dims);
    auto graph2 = std::make_shared<ComputeGraph>("test");
    CreateSingleNodeGraph2(graph2);

    status = fe_graph_optimizer_ptr->ShapeAndValueGeneralize(*(graph2.get()));
    EXPECT_EQ(fe::SUCCESS, status);
    for (auto node : graph2->GetDirectNode()) {
      if (node->GetType() == fe::DATA) {
        continue;
      }
      auto op_desc = node->GetOpDesc();
      auto tensor_desc_x = op_desc->MutableInputDesc("x");
      shape_vec = tensor_desc_x->GetShape().GetDims();
    }
    EXPECT_EQ(shape_vec, dims);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, shape_and_value_generalize_fail)
{
    auto graph = std::make_shared<ComputeGraph>("test");
    CreateBatchNormGraph(graph);
    vector<int64_t> shape_vec;
    auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_info_store, op_store_adapter_manager_ptr_);
    OptimizeUtilitySTStub *optimize_utility_stub = new OptimizeUtilitySTStub();
    fe_graph_optimizer_ptr->optimize_utility_ = optimize_utility_stub;
    tbe_op_store_adapter->CheckIsTbeGeneralizeFuncRegistered = checkIsRegisteredException;
    tbe_op_store_adapter->TeGeneralize = teGeneralizeException;
    Status status = fe_graph_optimizer_ptr->ShapeAndValueGeneralize(*(graph.get()));
    EXPECT_EQ(fe::FAILED, status);
}

Status OptimizerConfigurationStub(Configuration *This, std::string& graph_file_path) {
    graph_file_path = "./air/test/engines/nneng/st/testcase/graph_optimizer/gen_graph.json";
    return fe::SUCCESS;
}
Status OptimizerConfigurationStubBiasAddSoftMaxGrad(Configuration *This, std::string& graph_file_path) {
    graph_file_path = "./air/test/engines/nneng/st/testcase/graph_optimizer/bias_add_softmaxgrad.json";
    return fe::SUCCESS;
}

Status OptimizerConfigurationStubSquareSumV1(Configuration *This, std::string& graph_file_path) {
  graph_file_path = "./air/test/engines/nneng/st/testcase/graph_optimizer/SquareSumOut.json";
  return fe::SUCCESS;
}
std::string GetBuiltInFusionConfigFilePathStubs() {
  std::string switch_priority_file_path = "./air/test/engines/nneng/st/stub/fe_config/fusion_config.json";

  return  switch_priority_file_path;
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, init_opcompiler)
{
  auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_info_store, op_store_adapter_manager_ptr_);
  Status status = fe_graph_optimizer_ptr->InitializeAllOpCompiler();

  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, finalize_success1)
{
  auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_, op_store_adapter_manager_ptr_, AI_CORE_NAME);
  Status status = fe_graph_optimizer_ptr->Finalize();

  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, finalize_success2)
{
  auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_, op_store_adapter_manager_ptr_, AI_CORE_NAME);
  fe_graph_optimizer_ptr->init_flag_ = true;
  Status status = fe_graph_optimizer_ptr->Finalize();

  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, refresh_parameters)
{
  auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_,
                                op_store_adapter_manager_ptr_, AI_CORE_NAME);
  Status status = fe_graph_optimizer_ptr->RefreshParameters();

  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, optimize_original_graph_failed)
{
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateTwoOpDescGraph(graph);
  auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_,
                                op_store_adapter_manager_ptr_, AI_CORE_NAME);
  Status status = fe_graph_optimizer_ptr->OptimizeOriginalGraph(*graph);

  EXPECT_EQ(fe::FAILED, status);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, optimize_original_graph_success)
{
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateTwoOpDescGraph(graph);
  auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_,
                                op_store_adapter_manager_ptr_, AI_CORE_NAME);
  fe_graph_optimizer_ptr->init_flag_ = true;
  fe_graph_optimizer_ptr->graph_fusion_ptr_ = graph_fusion_ptr_;
  Status status = fe_graph_optimizer_ptr->OptimizeOriginalGraph(*graph);

  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, concat_split_optimizer_success1)
{
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateTwoOpDescGraph(graph);
  auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_,
                                op_store_adapter_manager_ptr_, AI_CORE_NAME);
  ConcatOptimizerPtr concat_optimizer_ptr_ = std::make_shared<ConcatOptimizer>();
  SplitOptimizerPtr split_optimizer_ptr_ = std::make_shared<SplitOptimizer>();
  bool need_set_virtual_op = true;
  Status status = fe_graph_optimizer_ptr->ConcatSplitOptimizer(*graph, need_set_virtual_op);

  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, concat_split_optimizer_success2)
{
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateTwoOpDescGraph(graph);
  auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_,
                                op_store_adapter_manager_ptr_, AI_CORE_NAME);
  ConcatOptimizerPtr concat_optimizer_ptr_ = std::make_shared<ConcatOptimizer>();
  SplitOptimizerPtr split_optimizer_ptr_ = std::make_shared<SplitOptimizer>();
  bool need_set_virtual_op = false;
  Status status = fe_graph_optimizer_ptr->ConcatSplitOptimizer(*graph, need_set_virtual_op);

  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, post_process_after_compiling_op_success)
{
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateTwoOpDescGraph(graph);
  auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_, op_store_adapter_manager_ptr_, AI_CORE_NAME);
  GraphCommPtr graph_comm_ptr = std::make_shared<GraphComm>(fe::AI_CORE_NAME);
  FusionPassMgrPtr fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  RuleMgrPtr fusion_rule_mgr_ptr_ = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
  PassMgrPtr fusion_pass_mgr_ptr_ = std::make_shared<FusionPassManager>();
  FusionPriorityMgrPtr fusion_priority_mgr_ptr_ = std::make_shared<FusionPriorityManager>(fe::AI_CORE_NAME, fusion_pass_mgr_ptr_, fusion_rule_mgr_ptr_);
  BufferFusionPtr buffer_fusion_ptr_ = std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr,
                                      fusion_priority_mgr_ptr_);
  std::vector<ge::NodePtr> buff_fus_compile_failed_nodes;
  fe_graph_optimizer_ptr->space_size_calculator_ptr_ = std::make_shared<SpaceSizeCalculator>();
  Status status = fe_graph_optimizer_ptr->PostProcessAfterCompilingOp(*graph, buffer_fusion_ptr_, buff_fus_compile_failed_nodes);

  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, optimize_fused_graph_after_graph_slice_success)
{
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateTwoOpDescGraph(graph);
  auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_, op_store_adapter_manager_ptr_, AI_CORE_NAME);
  fe_graph_optimizer_ptr->init_flag_ = true;
  OpCompilerPtr op_compiler_ptr = make_shared<OpCompiler>("compiler_name", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  fe_graph_optimizer_ptr->op_compiler_ptr_.push_back(op_compiler_ptr);
  fe_graph_optimizer_ptr->space_size_calculator_ptr_ = std::make_shared<SpaceSizeCalculator>();
  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_ = std::make_shared<FusionPassManager>();
  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_->lx_fusion_finalize_ = LxFusionFinalizeFunc1;
  Status status = fe_graph_optimizer_ptr->OptimizeFusedGraphAfterGraphSlice(*graph);
  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, optimize_fused_graph_after_graph_slice_with_compiled_fusionop_success)
{
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateTwoOpDescGraph(graph);
  auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_, op_store_adapter_manager_ptr_, AI_CORE_NAME);
  fe_graph_optimizer_ptr->init_flag_ = true;
  OpCompilerPtr op_compiler_ptr = make_shared<OpCompiler>("compiler_name", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  fe_graph_optimizer_ptr->op_compiler_ptr_.push_back(op_compiler_ptr);
  fe_graph_optimizer_ptr->space_size_calculator_ptr_ = std::make_shared<SpaceSizeCalculator>();
  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_ = std::make_shared<FusionPassManager>();

  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_->lx_fusion_finalize_ = LxFusionFinalizeFunc1;

  for (auto& node : graph->GetDirectNode()) {
    ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
    (void)ge::AttrUtils::SetBool(op_desc_ptr, ATTR_NAME_IS_COMPIED_FUSION_OP, true);
    break;
  }
  Status status = fe_graph_optimizer_ptr->OptimizeFusedGraphAfterGraphSlice(*graph);

  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, optimize_stream_graph_success)
{
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateTwoOpDescGraph(graph);
  auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_, op_store_adapter_manager_ptr_, AI_CORE_NAME);
  fe_graph_optimizer_ptr->init_flag_ = true;
  fe_graph_optimizer_ptr->l2_optimize_ptr_ = std::make_shared<L2Optimizer>(AI_CORE_NAME);
  ge::RunContext context_;
  Status status = fe_graph_optimizer_ptr->OptimizeStreamGraph(*graph, context_);

  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, optimize_whole_graph_success)
{
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateTwoOpDescGraph(graph);
  auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_, op_store_adapter_manager_ptr_, AI_CORE_NAME);
  RuleMgrPtr fusion_rule_mgr_ptr_ = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
  PassMgrPtr fusion_pass_mgr_ptr_ = std::make_shared<FusionPassManager>();
  FusionPriorityMgrPtr fusion_priority_mgr_ptr_ = std::make_shared<FusionPriorityManager>(fe::AI_CORE_NAME, fusion_pass_mgr_ptr_, fusion_rule_mgr_ptr_);
  fe_graph_optimizer_ptr->fusion_priority_mgr_ptr_ = fusion_priority_mgr_ptr_;
  Status status = fe_graph_optimizer_ptr->OptimizeWholeGraph(*graph);

  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_atomic_add_info_success)
{
    auto graph = std::make_shared<ComputeGraph>("test");
    CreateTwoOpDescGraph(graph);

    auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_info_store, op_store_adapter_manager_ptr_);
    //fe_graph_optimizer_ptr->init_flag_ = true;
    for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == OP_TYPE_PLACE_HOLDER ||
    op_type == OP_TYPE_END) {
        continue;
    }
    ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
    if (!ge::AttrUtils::HasAttr(op_desc_ptr, FE_IMPLY_TYPE)) {
        continue;
    }
    int tmp_imply_type = -1;
    ge::AttrUtils::GetInt(op_desc_ptr, FE_IMPLY_TYPE, tmp_imply_type);
    OpImplType op_impl_type = (OpImplType)tmp_imply_type;
    if (op_desc_ptr->GetName() == "batchnormal") {
        std::vector<uint32_t> tmp_output_index {1, 0, 0};
        bool output_index = ge::AttrUtils::SetListInt(op_desc_ptr, TBE_OP_ATOMIC_OUTPUT_INDEX, tmp_output_index);

        std::vector<int64_t> tmp_wk_index {1, 1, 1};
        bool atomic = ge::AttrUtils::SetListInt(op_desc_ptr, TBE_OP_ATOMIC_WORKSPACE_INDEX, tmp_wk_index);
        op_desc_ptr->SetWorkspaceBytes({32, 32, 32});
        EXPECT_EQ(output_index, true);
        EXPECT_EQ(atomic, true);
    }
    if (op_desc_ptr->GetName() == "relu") {
        ge::AttrUtils::SetInt(op_desc_ptr, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
        std::vector<uint32_t> tmp_output_index {1};
        bool output_index2 = ge::AttrUtils::SetListInt(op_desc_ptr, TBE_OP_ATOMIC_OUTPUT_INDEX, tmp_output_index);

        std::vector<int64_t> tmp_wk_index {1, 1, 1};
        bool atomic2 = ge::AttrUtils::SetListInt(op_desc_ptr, TBE_OP_ATOMIC_WORKSPACE_INDEX, tmp_wk_index);
        op_desc_ptr->SetWorkspaceBytes({32, 32, 32});
        EXPECT_EQ(output_index2, true);
        EXPECT_EQ(atomic2, true);
    }
    }
    Status status = fe_graph_optimizer_ptr->SetAtomicAddInfo(*(graph.get()));
    EXPECT_EQ(fe::SUCCESS, status);
}
TEST_F(STEST_fusion_engine_fe_graph_optimizer, optimize_original_judge_c04_success)
{
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateConv2dGraph(graph);
  FEOpsKernelInfoStorePtr ops_info_store;
  std::make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_);
  auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_info_store, op_store_adapter_manager_ptr_);
  fe_graph_optimizer_ptr->format_dtype_setter_ptr_ =
  std::make_shared<FormatDtypeSetter>(AI_CORE_NAME, op_store_adapter_manager_ptr_);
  fe_graph_optimizer_ptr->op_impl_type_judge_ptr_ =
  std::make_shared<OpImplTypeJudge>(AI_CORE_NAME, ops_kernel_info_store_ptr_);
  fe_graph_optimizer_ptr->op_axis_update_desc_ptr_ =
  std::make_shared<OpAxisUpdateDesc>(AI_CORE_NAME);
  RuleMgrPtr fusion_rule_mgr_ptr_ = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
  PassMgrPtr fusion_pass_mgr_ptr_ = std::make_shared<FusionPassManager>();
  FusionPriorityMgrPtr fusion_priority_mgr_ptr_ = std::make_shared<FusionPriorityManager>(
      fe::AI_CORE_NAME, fusion_pass_mgr_ptr_, fusion_rule_mgr_ptr_);
  op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();

  fe_graph_optimizer_ptr->ops_kernel_info_store_ptr_ =
  std::make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_, fe::AI_CORE_NAME);

  fe_graph_optimizer_ptr->graph_fusion_ptr_ = std::make_shared<GraphFusion>(fusion_rule_mgr_ptr_,
                                                                            ops_kernel_info_store_ptr_,fusion_pass_mgr_ptr_, fusion_priority_mgr_ptr_);
  fe_graph_optimizer_ptr->space_size_calculator_ptr_ = std::make_shared<SpaceSizeCalculator>();
  fe_graph_optimizer_ptr->op_setter_ptr_ = std::make_shared<OpSetter>(AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status status = fe_graph_optimizer_ptr->OptimizeOriginalGraphJudgeInsert(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
}
TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_atomic_add_info_success2)
{
    auto graph = std::make_shared<ComputeGraph>("test");
    CreateTwoOpDescGraph(graph);

    auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_info_store, op_store_adapter_manager_ptr_);
    //fe_graph_optimizer_ptr->init_flag_ = true;
    for (auto node : graph->GetDirectNode()) {
        string op_type = node->GetType();
        if (op_type == OP_TYPE_PLACE_HOLDER ||
        op_type == OP_TYPE_END) {
            continue;
        }
        ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
        if (!ge::AttrUtils::HasAttr(op_desc_ptr, FE_IMPLY_TYPE)) {
        continue;
        }
        int tmp_imply_type = -1;
        ge::AttrUtils::GetInt(op_desc_ptr, FE_IMPLY_TYPE, tmp_imply_type);
        OpImplType op_impl_type = (OpImplType)tmp_imply_type;

        if (op_desc_ptr->GetName() == "batchnormal") {
            std::vector<int64_t> tmp_wk_index {0, 0};
            bool atomic = ge::AttrUtils::SetListInt(op_desc_ptr, TBE_OP_ATOMIC_WORKSPACE_FLAG, tmp_wk_index);
            op_desc_ptr->SetWorkspaceBytes({32, 32});
            EXPECT_EQ(atomic, true);
        }
        if (op_desc_ptr->GetName() == "relu") {
            ge::AttrUtils::SetInt(op_desc_ptr, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
            std::vector<uint32_t> tmp_output_index {0, 1};
            bool output_index = ge::AttrUtils::SetListInt(op_desc_ptr, TBE_OP_ATOMIC_OUTPUT_INDEX, tmp_output_index);
            EXPECT_EQ(output_index, true);
        }
    }
    Status status = fe_graph_optimizer_ptr->SetAtomicAddInfo(*(graph.get()));
    EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, optimize_original_graph_judge_insert_success)
{
    auto graph = std::make_shared<ComputeGraph>("test");
    CreateBatchNormGraph(graph);
    auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_info_store, op_store_adapter_manager_ptr_);
    fe_graph_optimizer_ptr->format_dtype_setter_ptr_ =
    std::make_shared<FormatDtypeSetter>(AI_CORE_NAME, op_store_adapter_manager_ptr_);
    fe_graph_optimizer_ptr->op_impl_type_judge_ptr_ = 
    std::make_shared<OpImplTypeJudge>(AI_CORE_NAME, ops_kernel_info_store_ptr_);
    fe_graph_optimizer_ptr->op_axis_update_desc_ptr_ = 
    std::make_shared<OpAxisUpdateDesc>(AI_CORE_NAME);
    RuleMgrPtr fusion_rule_mgr_ptr_ = std::make_shared<FusionRuleManager>(ops_kernel_info_store_ptr_);
    PassMgrPtr fusion_pass_mgr_ptr_ = std::make_shared<FusionPassManager>();
    FusionPriorityMgrPtr fusion_priority_mgr_ptr_ = std::make_shared<FusionPriorityManager>(
          fe::AI_CORE_NAME, fusion_pass_mgr_ptr_, fusion_rule_mgr_ptr_);

    fe_graph_optimizer_ptr->graph_fusion_ptr_ = std::make_shared<GraphFusion>(fusion_rule_mgr_ptr_,
          ops_kernel_info_store_ptr_,fusion_pass_mgr_ptr_, fusion_priority_mgr_ptr_);
    fe_graph_optimizer_ptr->space_size_calculator_ptr_ = std::make_shared<SpaceSizeCalculator>();
    fe_graph_optimizer_ptr->op_setter_ptr_ = std::make_shared<OpSetter>(AI_CORE_NAME, op_store_adapter_manager_ptr_);
    Status status = fe_graph_optimizer_ptr->OptimizeOriginalGraphJudgeInsert(*(graph.get()));
    EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, shape_and_value_generalize_nonfuzzy)
{
    auto graph = std::make_shared<ComputeGraph>("test");
    CreateBatchNormGraph(graph);
    vector<int64_t> shape_vec;
    auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_info_store, op_store_adapter_manager_ptr_);
    tbe_op_store_adapter->CheckIsTbeGeneralizeFuncRegistered = checkIsRegisteredException;
    tbe_op_store_adapter->TeGeneralize = teGeneralizeException;

    std::map<std::string, std::string> options;
    options.insert(std::pair<std::string, std::string>("ge.shape_generalized_build_mode", "shape_precise"));
    ge::GetThreadLocalContext().SetGlobalOption(options);

    Status status = fe_graph_optimizer_ptr->ShapeAndValueGeneralize(*(graph.get()));
    EXPECT_EQ(fe::SUCCESS, status);

    std::map<std::string, std::string> options1;
    options1.insert(std::pair<std::string, std::string>("ge.shape_generalized_build_mode", SHAPE_GENERALIZED));
    ge::GetThreadLocalContext().SetGlobalOption(options1);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, shape_and_value_generalize_fuzzy)
{
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateBatchNormGraph(graph);
  vector<int64_t> shape_vec;
  tbe_op_store_adapter->CheckIsTbeGeneralizeFuncRegistered = checkIsRegisteredException;
  tbe_op_store_adapter->TeGeneralize = teGeneralizeException;

  std::map<std::string, std::string> options;
  options.insert(std::pair<std::string, std::string>("ge.shape_generalized_build_mode", "shape_generalized"));
  ge::GetThreadLocalContext().SetGlobalOption(options);

  Status status = fe_graph_optimizer_->ShapeAndValueGeneralize(*(graph.get()));
  EXPECT_EQ(fe::FAILED, status);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_fusion_virtual_op_success1) {
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateConcatOpDescGraph(graph);
  Status status = concat_optimizer.SetFusionVirtualOp(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == "ConcatD") {
      bool no_task = false;
      ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, no_task);
      EXPECT_EQ(no_task, false);
    }
  }
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_fusion_virtual_op_success1_split) {
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateSplitOpDescGraph(graph);
  Status status = split_optimizer.SetFusionVirtualOp(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == "SplitD") {
      bool no_task = false;
      ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, no_task);
      EXPECT_EQ(no_task, false);
    }
  }
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_fusion_virtual_op_success2_split) {
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateConstSplitOpDescGraph(graph);
  Status status = split_optimizer.SetFusionVirtualOp(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == "SplitD") {
      bool no_task = false;
      ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, no_task);
      EXPECT_EQ(no_task, false);
    }
  }
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_fusion_virtual_op_failed1) {
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateConcatOpDescGraph2(graph);
  Status status = concat_optimizer.SetFusionVirtualOp(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == "ConcatD") {
      bool no_task = false;
      ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, no_task);
      EXPECT_EQ(no_task, false);
    }
  }
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_fusion_virtual_op_failed2) {
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateConcatOpDescGraph3(graph);
  Status status = concat_optimizer.SetFusionVirtualOp(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == "ConcatD") {
      bool no_task = false;
      ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, no_task);
      EXPECT_EQ(no_task, false);
    }
  }
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_fusion_virtual_op_failed3) {
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateConcatOpDescGraph4(graph);
  Status status = concat_optimizer.SetFusionVirtualOp(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == "ConcatD") {
      bool no_task = false;
      ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, no_task);
      EXPECT_EQ(no_task, false);
    }
  }
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_fusion_virtual_op_failed4) {
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateConcatOpDescGraph5(graph);
  Status status = concat_optimizer.SetFusionVirtualOp(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == "ConcatD") {
      bool no_task = false;
      ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, no_task);
      EXPECT_EQ(no_task, false);
    }
  }
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_fusion_virtual_op_failed5) {
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateConcatOpDescGraph6(graph);
  Status status = concat_optimizer.SetFusionVirtualOp(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == "ConcatD") {
      bool no_task = false;
      ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, no_task);
      EXPECT_EQ(no_task, false);
    }
  }
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_fusion_virtual_op_success6) {
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateConcatOpDescGraph7(graph);
  Status status = concat_optimizer.SetFusionVirtualOp(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == "ConcatD") {
      bool no_task = false;
      ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, no_task);
      EXPECT_EQ(no_task, true);
    }
  }
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_fusion_virtual_op_failed7) {
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateConcatOpDescGraph8(graph);
  Status status = concat_optimizer.SetFusionVirtualOp(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == "ConcatD") {
      bool no_task = false;
      ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, no_task);
      EXPECT_EQ(no_task, false);
    }
  }
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_fusion_virtual_op_failed8) {
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateConcatOpDescGraph9(graph);
  Status status = concat_optimizer.SetFusionVirtualOp(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == "ConcatD") {
      bool no_task = false;
      ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, no_task);
      EXPECT_EQ(no_task, false);
    }
  }
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_fusion_virtual_op_success9) {
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateConcatOpDescGraph10(graph);
  Status status = concat_optimizer.SetFusionVirtualOp(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == "ConcatD") {
      bool no_task = false;
      ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, no_task);
      EXPECT_EQ(no_task, false);
    }
  }
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_fusion_virtual_op_failed10) {
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateConcatOpDescGraph11(graph);
  Status status = concat_optimizer.SetFusionVirtualOp(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == "ConcatD") {
      bool no_task = false;
      ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, no_task);
      EXPECT_EQ(no_task, false);
    }
  }
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_fusion_virtual_op_failed11) {
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateConcatOpDescGraph12(graph);
  Status status = concat_optimizer.SetFusionVirtualOp(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == "ConcatD") {
      bool no_task = false;
      ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, no_task);
      EXPECT_EQ(no_task, false);
    }
  }
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_fusion_virtual_op_failed12) {
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateConcatOpDescGraph13(graph);
  Status status = concat_optimizer.SetFusionVirtualOp(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == "ConcatV2D" || op_type == "ConcatD") {
      bool no_task = false;
      ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, no_task);
      EXPECT_EQ(no_task, false);
    }
  }
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_fusion_virtual_op_failed13) {
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateConcatOpDescGraph14(graph);
  Status status = concat_optimizer.SetFusionVirtualOp(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == "ConcatD" || op_type == "ConcatV2D") {
      bool no_task = false;
      ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, no_task);
      EXPECT_EQ(no_task, false);
    }
  }
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_fusion_virtual_op_failed14) {
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateConcatOpDescGraph15(graph);
  Status status = concat_optimizer.SetFusionVirtualOp(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  int32_t notask_node_num = 0;
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == "ConcatD" || op_type == "ConcatV2D" ) {
      bool no_task = false;
      ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, no_task);   
      if (no_task) {
        notask_node_num += 1;
      }
    }    
  }
  EXPECT_EQ(notask_node_num, 0);
}


TEST_F(STEST_fusion_engine_fe_graph_optimizer, set_fusion_virtual_op_success_reshape) {
  auto graph = std::make_shared<ComputeGraph>("test");
  CreateConcatOpDescGraph16(graph);
  Status status = concat_optimizer.SetFusionVirtualOp(*(graph.get()));
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto node : graph->GetDirectNode()) {
    string op_type = node->GetType();
    if (op_type == "ConcatD") {
      bool no_task = false;
      ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, no_task);
      EXPECT_EQ(no_task, false);
    }
  }
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, optimize_CloseRcCache) {
  OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
  OpDescPtr transdata = std::make_shared<OpDesc>("transdata", "TransData");
  OpDescPtr cast = std::make_shared<OpDesc>("cast", "Cast");
  OpDescPtr relu = std::make_shared<OpDesc>("relu", "Relu");
  vector<int64_t> dim = {-1, 4, 1, 4};
  GeShape shape(dim);
  GeTensorDesc tenosr_desc_4d_fp16(shape, FORMAT_NCHW, DT_FLOAT16);
  GeTensorDesc tenosr_desc_4d_fp32(shape, FORMAT_NCHW, DT_FLOAT);
  GeTensorDesc tenosr_desc_5d_fp16(shape, FORMAT_NCHW, DT_FLOAT16);
  GeTensorDesc tenosr_desc_5d_fp32(shape, FORMAT_NCHW, DT_FLOAT);

  data->AddOutputDesc(tenosr_desc_4d_fp32);
  cast->AddInputDesc(tenosr_desc_4d_fp32);
  cast->AddOutputDesc(tenosr_desc_4d_fp16);
  transdata->AddInputDesc(tenosr_desc_4d_fp16);
  transdata->AddOutputDesc(tenosr_desc_5d_fp16);
  relu->AddInputDesc(tenosr_desc_5d_fp16);
  relu->AddOutputDesc(tenosr_desc_5d_fp16);

  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr data_node = graph->AddNode(data);
  NodePtr transdata_node = graph->AddNode(transdata);
  NodePtr cast_node = graph->AddNode(cast);
  NodePtr relu_node = graph->AddNode(relu);

  GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), cast_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(cast_node->GetOutDataAnchor(0), transdata_node->GetInDataAnchor(0));
  GraphUtils::AddEdge(transdata_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
  auto fe_graph_optimizer_ = std::make_shared<FEGraphOptimizer>(ops_info_store, op_store_adapter_manager_ptr_);
  Configuration::Instance(fe::AI_CORE_NAME).soc_version_ = "Ascend910A";
  Status ret = fe_graph_optimizer_->CloseRcCache(*graph);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, add_test)
{
auto graph = std::make_shared<ComputeGraph>("test");
CreateBatchNormGraph(graph);

ComputeGraphPtr parent_graph = std::make_shared<ComputeGraph>("parent_graph");
auto parent_const = MakeNode(parent_graph, 0, 1, "parent_const", "Const");
auto parent_case = MakeNode(parent_graph, 3, 1, "parent_case", "Case");
auto parent_output = MakeNode(parent_graph, 1, 0, "parent_output", "NetOutput");

GeTensorDesc tensor_desc(GeShape({1,3,224,224}), FORMAT_NCHW, DT_FLOAT);

parent_const->GetOpDesc()->UpdateOutputDesc(0, tensor_desc);
parent_case->GetOpDesc()->UpdateInputDesc(0, tensor_desc);
parent_case->GetOpDesc()->UpdateInputDesc(1, tensor_desc);
parent_case->GetOpDesc()->UpdateInputDesc(2, tensor_desc);
parent_case->GetOpDesc()->UpdateOutputDesc(0, tensor_desc);

GraphUtils::AddEdge(parent_const->GetOutDataAnchor(0), parent_case->GetInDataAnchor(0));
GraphUtils::AddEdge(parent_const->GetOutDataAnchor(0), parent_case->GetInDataAnchor(1));
GraphUtils::AddEdge(parent_const->GetOutDataAnchor(0), parent_case->GetInDataAnchor(2));
GraphUtils::AddEdge(parent_case->GetOutDataAnchor(0), parent_output->GetInDataAnchor(0));

ComputeGraphPtr sub_graph = std::make_shared<ComputeGraph>("sub_graph");
auto data0 = MakeNode(sub_graph, 1, 1, "data0", "Data");
data0->GetOpDesc()->UpdateInputDesc(0, tensor_desc);
data0->GetOpDesc()->UpdateOutputDesc(0, tensor_desc);
auto data1 = MakeNode(sub_graph, 1, 1, "data1", "Data");
data1->GetOpDesc()->UpdateInputDesc(0, tensor_desc);
data1->GetOpDesc()->UpdateOutputDesc(0, tensor_desc);
auto data2 = MakeNode(sub_graph, 1, 1, "data2", "Data");
data2->GetOpDesc()->UpdateInputDesc(0, tensor_desc);
data2->GetOpDesc()->UpdateOutputDesc(0, tensor_desc);
(void)AttrUtils::SetInt(data0->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 0);
(void)AttrUtils::SetInt(data1->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 1);
(void)AttrUtils::SetInt(data2->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, 2);

sub_graph->SetParentNode(parent_case);
sub_graph->SetParentGraph(parent_graph);
parent_graph->AddSubgraph(sub_graph->GetName(), sub_graph);

graph_fusion_ptr_->Fusion(*(parent_graph.get()));
graph_fusion_ptr_->TagNoConstFolding(*(parent_graph.get()));
graph_fusion_ptr_->JudgeQuantMode(*(parent_graph.get()));
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, optimize_RefreshParameters) {
  std::map<std::string, std::string> options;
  options.emplace(ge::ENABLE_SMALL_CHANNEL, "1");
  ge::GetThreadLocalContext().SetGlobalOption(options);
  fe_graph_optimizer_->RefreshParameters();
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, buffer_fusion_process_1) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GraphCommPtr graph_comm_ptr = std::make_shared<GraphComm>("AIcoreEngine");

  BufferFusionMode tmp_buffer_fusion_mode = Configuration::Instance(fe::AI_CORE_NAME).buffer_fusion_mode_;
  BufferOptimize tmp_buffer_optimize = Configuration::Instance(fe::AI_CORE_NAME).buffer_optimize_;
  Configuration::Instance(fe::AI_CORE_NAME).buffer_optimize_ = EN_OFF_OPTIMIZE;
  Configuration::Instance(fe::AI_CORE_NAME).buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;
  LxFusionOptimizeResult buffer_ret = LxFusionOptimizeResult::NO_FUSION_STRATEGY;
  Status ret = fe_graph_optimizer_->BufferFusionProcess(*graph, graph_comm_ptr, buffer_ret);
  EXPECT_EQ(ret, fe::SUCCESS);
  Configuration::Instance(fe::AI_CORE_NAME).buffer_optimize_ = tmp_buffer_optimize;
  Configuration::Instance(fe::AI_CORE_NAME).buffer_fusion_mode_ = tmp_buffer_fusion_mode;
}

TEST_F(STEST_fusion_engine_fe_graph_optimizer, get_op_compiler_fail) {
  std::string build_mode_value;
  std::string step_mode_value;
  auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_, op_store_adapter_manager_ptr_, AI_CORE_NAME);
  OpCompilerPtr op_compiler_ptr = make_shared<OpCompiler>("compiler_name", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  fe_graph_optimizer_ptr->op_compiler_ptr_.push_back(op_compiler_ptr);
  OpCompilerPtr op_compiler;
  Status status = fe_graph_optimizer_ptr->GetOpCompiler(build_mode_value, step_mode_value, op_compiler);
  EXPECT_EQ(status, fe::FAILED);
}
