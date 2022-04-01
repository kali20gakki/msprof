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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v200_not_requant_fusion_pass.h"
#include <cmath>
#include <string>
#include <vector>
#include "securec.h"
#include "common/math_util.h"
#include "common/configuration.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/quant_host_cpu_op_common.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v200_requant_util.h"

namespace fe {

static const string PATTERN_DEQUANT      = "dequant";
static const string PATTERN_QUANT        = "quant";
static const string PATTERN_ELTWISE      = "eltwise";

static const string ADD                  = "Add";
static const string DEQUANTS16           = "AscendDequantS16";
static const string REQUANTS16           = "AscendRequantS16";
static const string ATTR_DUALOUT         = "dual_output";

static const uint64_t UINT64_NUM_ZERO    = 0;
static const uint32_t DOUBLE_OUTPUT      = 2;
static const uint32_t SECONDE_OPDESC_INDEX = 2;

void V200NotRequantFusionPass::RecordNotRequantOutputAnchorMap(vector<ge::NodePtr> &relu_nodes,
                                                               vector<ge::NodePtr> &fuse_nodes) {
  if (relu_nodes.empty()) {
    for (auto requant_node : fuse_nodes) {
      RecordOutputAnchorMap(requant_node);
    }
  }

  for (auto relu_node : relu_nodes) {
    if (CheckNeedRemoveRelu(relu_node)) {
      RecordOutputAnchorMap(relu_node);
    } else {
      fuse_nodes.push_back(relu_node);
    }
  }
}

Status V200NotRequantFusionPass::NotRequantPattern1(ge::ComputeGraph &graph, ge::NodePtr dequant_node,
                                                    vector<ge::NodePtr> &new_nodes, Mapping &mapping) {
  Status ret;
  vector<ge::NodePtr> dequants;
  vector<ge::NodePtr> quants;
  vector<ge::NodePtr> relus;
  int requanted_flg;
  unsigned int relu_del = 0;
  float scale_quant = 1;

  if (ge::AttrUtils::GetInt(dequant_node->GetOpDesc(), ATTR_REQUANTED, requanted_flg)) {
    FE_LOGD("This node [%s] has beed requanted, skip it.", dequant_node->GetName().c_str());
    return NOT_CHANGED;
  }

  dequants.push_back(dequant_node);

  for (auto node_ptr : dequant_node->GetOutAllNodes()) {
    if (CheckReluValid(node_ptr)) {
      if (CheckNeedRemoveRelu(node_ptr)) {
        relu_del++;
      }
      relus.push_back(node_ptr);
    }
  }

  FE_LOGD("reluDel is %u, size of dequant out node %zu.", relu_del, dequant_node->GetOutAllNodes().size());
  bool del_relu_flag = (relu_del && (relu_del == dequant_node->GetOutAllNodes().size()));
  if (!del_relu_flag) {
    relus.clear();
  }
  // add mapping for datadump to get all original nodes before fusion.
  if (relus.size() > 0) {
    for (size_t i = 0; i < relus.size(); i++) {
      std::shared_ptr<OpDesc> op(new (std::nothrow) OpDesc());
      FE_CHECK(op == nullptr,
               REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][NotReQntPtn1] create op is nullptr"), return FAILED);
      std::string relu_name = "relu";
      op->id = relu_name.append(to_string(i));
      mapping[op].push_back(relus[i]);
    }
  }
  new_nodes.push_back(dequant_node);

  if (!ge::AttrUtils::SetBool(dequant_node->GetOpDesc(), ATTR_DEL_RELU, del_relu_flag)) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][NotReQntPtn1] set relu del failed! node name: %s",
                    dequant_node->GetName().c_str());
    return FAILED;
  }
  FE_LOGD("relu size is %zu, quant size %zu, relu_del_num %d", relus.size(), dequants.size(), relu_del);
  if (DealDequantNotRequantV200(graph, dequants, scale_quant, new_nodes) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][NotReQntPtn1] deal dequant nodes failed");
    return FAILED;
  }

  RecordNotRequantOutputAnchorMap(relus, new_nodes);
  ret = DealRelu(graph, relus, scale_quant);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][NotReQntPtn1] deal relu nodes failed");
    return FAILED;
  }

  quants.clear();
  ret = TagNodes(quants, dequants, 0);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][NotReQntPtn1] tag nodes failed");
    return FAILED;
  }

  /* if fusion op only contains requant and RequantHostCpuOp, RequantHostCpuOp
     will be folded finally. host op do not need to record it to fusion op. */
  RemoveHostOpFromFusionNode(new_nodes);
  return SUCCESS;
}

Status V200NotRequantFusionPass::JudgeAndExtractQuantParas(vector<ge::NodePtr> dequant_nodes, vector<float> &scale_deq,
                                                           vector<int8_t> &offset_w, vector<int8_t> &N) const {
  vector<vector<uint64_t>> vec_deq_args;
  uint64_t deq_co = 0;
  vector<uint32_t> vec_n_args;
  for (size_t i = 0; i < dequant_nodes.size(); i++) {
    vector<ge::GeTensorPtr> weights = ge::OpDescUtils::MutableWeights(dequant_nodes[i]);
    if (weights.size() < 1) {
      FE_LOGI("weights get failed.");
      return NOT_CHANGED;
    }

    ge::GeTensorPtr deq_scale = weights[0];
    FE_CHECK(deq_scale == nullptr,
             REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][JdgExtractQntPara] deq_scale is nullptr."),
             return PARAM_INVALID);

    // translate deq_scale to scale_deq[32:63], N[24:31], offset_w[16:23]
    std::uint8_t *data = const_cast<uint8_t *>(deq_scale->GetData().data());
    uint64_t *deq_scale_data = reinterpret_cast<uint64_t *>(data);
    uint64_t scale_size = deq_scale->GetData().size() / sizeof(uint64_t);
    FE_CHECK(deq_scale_data == nullptr,
             REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][JdgExtractQntPara] deqScaleData is nullptr"),
             return PARAM_INVALID);
    const ge::GeShape &deq_scale_shape = deq_scale->GetTensorDesc().GetShape();
    if (deq_scale_shape.GetDimNum() != 1) {
      REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][JdgExtractQntPara] deq_scale shape error, shape is %zu.",
                      deq_scale_shape.GetDimNum());
      return PARAM_INVALID;
    }
    deq_co = deq_scale_shape.GetDim(0);
    FE_LOGD("Op[%s], deq_scale dim is %lu, scale_size is %lu.",
            dequant_nodes[i]->GetName().c_str(), deq_co, scale_size);

    vector<uint64_t> deq_args;
    for (uint64_t j = 0; j < deq_co && j < scale_size; j++) {
      deq_args.push_back(deq_scale_data[j]);
    }
    vec_deq_args.push_back(deq_args);
  }

  // check if dequnat args of all dequant are equal
  for (unsigned int i = 1; i < vec_deq_args.size(); i++) {
    int index = 0;
    std::vector<uint64_t> args = vec_deq_args.at(i);
    for (auto arg : args) {
      if (vec_deq_args.at(0).at(index) != arg) {
        return NOT_CHANGED;
      }
      index++;
    }
  }
  if (vec_deq_args.empty()) {
    FE_LOGW("vec_deq_args is empty.");
    return FAILED;
  }
  vector<uint64_t> deq_args = vec_deq_args[0];
  for (uint64_t i = 0; i < deq_co; i++) {
    uint32_t scale_deq_value = GET_DEQUANT_SCALE_DEQ(deq_args[i]);
    float scale = 0;
    if (memcpy_s(&scale, sizeof(float), &scale_deq_value, sizeof(uint32_t)) != 0) {
      FE_LOGW("memcpy_s not success!");
      return FAILED;
    }
    scale_deq.push_back(scale);
    offset_w.push_back(GET_DEQUANT_OFFSET_W(deq_args[i]));
    N.push_back(GET_DEQUANT_N(deq_args[i]));
  }
  return SUCCESS;
}
Status V200NotRequantFusionPass::DealHighPerformanceDeqaunt(ge::ComputeGraph &graph, vector<ge::NodePtr> dequant_nodes,
                                                            vector<float> &scale_deq, vector<int8_t> &N,
                                                            vector<ge::NodePtr> &fusion_nodes) const {
  std::vector<int8_t> offset_w;
  Status res = JudgeAndExtractQuantParas(dequant_nodes, scale_deq, offset_w, N);
  if (res != SUCCESS) {
    FE_LOGW("Juge or extract failed.");
    return res;
  }
  for (auto dequant_node : dequant_nodes) {
    auto dequant_op = dequant_node->GetOpDesc();
    // change op type from AscendDequant to AscendDequantS16
    dequant_op->SetType(DEQUANTS16);
    fusion_nodes.push_back(dequant_node);
    // put offset_w from dequant to cube
    auto cube_node = dequant_node->GetInAllNodes().at(0);
    FE_CHECK(cube_node == nullptr,
             REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][DealHighPfmDeqnt] cubeNode is null."), return FAILED);
    // cube node has bias input, and bias have done bias optimization before
    vector<int32_t> tmp_n;
    for (auto n : N) {
      tmp_n.push_back(n);
    }
    if (cube_node->GetInAllNodes().size() >= 3) {
      auto bias_node = cube_node->GetInAllNodes().at(2);
      if (bias_node->GetType() != QUANT_BIAS_OPTIMIZATION) {
        REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][DealHighPfmDeqnt] bias not do bias optimize, cube node [%s].",
                        cube_node->GetName().c_str());
        return FAILED;
      }
      bias_node->GetOpDesc()->SetType(QUANT_BIAS_OPTIMIZATIONS16);
      fusion_nodes.push_back(bias_node);
      ge::AttrUtils::SetListInt(bias_node->GetOpDesc(), DEQ_N_VALUE, tmp_n);
      ge::AttrUtils::SetBool(bias_node->GetOpDesc(), OUTPUTS16, true);
      ge::GraphUtils::RemoveEdge(bias_node->GetOutDataAnchor(0), cube_node->GetInDataAnchor(2));
      dequant_node->AddLinkFrom("x1", bias_node);
      // delete cube bias input
      ge::NodeUtils::ClearInDataAnchor(cube_node, cube_node->GetInDataAnchor(2));
      ge::OpDescUtils::ClearInputDesc(cube_node->GetOpDesc(), static_cast<uint32_t>(SECONDE_OPDESC_INDEX));
      // for cube node, need add offset_w input if which is not zero
      // update dtype
      bias_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_INT16);
      bias_node->GetOpDesc()->MutableOutputDesc(0)->SetOriginDataType(ge::DT_INT16);
      dequant_node->GetOpDesc()->MutableInputDesc(2)->SetDataType(ge::DT_INT16);
      dequant_node->GetOpDesc()->MutableInputDesc(2)->SetOriginDataType(ge::DT_INT16);
    }
    // relu flag
    bool relu_flag = false;
    bool dequant_has_relu = dequant_node->GetOutAllNodes().size() == 1 &&
                            (dequant_node->GetOutAllNodes().at(0)->GetType() == RELU ||
                             dequant_node->GetOutAllNodes().at(0)->GetType() == LEAKY_RELU);
    if (dequant_has_relu) {
      relu_flag = CheckNeedRemoveRelu(dequant_node->GetOutAllNodes().at(0));
      if (relu_flag) {
        (void)ge::AttrUtils::SetBool(dequant_op, ATTR_RELU_FLAG, true);
        auto relu_node = dequant_node->GetOutAllNodes().at(0);
        if (graph.RemoveNode(relu_node) == ge::GRAPH_FAILED) {
          REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][DealHighPfmDeqnt] reluNode remove failed.");
          return FAILED;
        }
      } else {
        (void)ge::AttrUtils::SetBool(dequant_op, ATTR_RELU_FLAG, false);
      }
    }
    // set N deq_scale_tensor[32:35]
    vector<ge::GeTensorPtr> weights = ge::OpDescUtils::MutableWeights(dequant_node);
    if (weights.size() < 1) {
      FE_LOGI("weights get failed.");
      return NOT_CHANGED;
    }

    ge::GeTensorPtr scale_input = weights[0];

    std::uint8_t *data = const_cast<uint8_t *>(scale_input->GetData().data());
    uint64_t *deq_scale_data = reinterpret_cast<uint64_t *>(data);
    int scale_size = scale_input->GetData().size() / sizeof(uint64_t);
    FE_CHECK(deq_scale_data == nullptr,
             REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][DealHighPfmDeqnt] deqScaleData is nullptr"),
             return PARAM_INVALID);

    for (int64_t i = 0; i < scale_size; i++) {
      deq_scale_data[i] = N[i] - 1;
      deq_scale_data[i] = deq_scale_data[i] << SCALE_BIT_OFFSET;
      // set relu flag
      if (relu_flag) {
        deq_scale_data[i] = deq_scale_data[i] | 0x0000800000000000;
      } else {
        deq_scale_data[i] = deq_scale_data[i] & 0xffff7fffffffffff;
      }
    }

    if (scale_input->SetData(reinterpret_cast<const uint8_t *>(deq_scale_data), scale_size * sizeof(uint64_t)) !=
        SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][DealHighPfmDeqnt] set scale data failed!");
      return FAILED;
    }
    // set dequants16 output dtype int16
    dequant_node->GetOpDesc()->MutableOutputDesc(0)->SetDataType(ge::DT_INT16);
    dequant_node->GetOpDesc()->MutableOutputDesc(0)->SetOriginDataType(ge::DT_INT16);
  }
  return SUCCESS;
}

Status V200NotRequantFusionPass::CreateRequantS16BasedOnEltwise(ge::ComputeGraph &graph,
                                                                const ge::OpDescPtr &eltwise_op,
                                                                ge::NodePtr &requants16_node, bool dual_output,
                                                                vector<ge::NodePtr> &fusion_nodes) const {
  // create requants16 node
  ge::OpDescPtr requant_s16_op = nullptr;
  FE_MAKE_SHARED(requant_s16_op = std::make_shared<ge::OpDesc>(eltwise_op->GetName(), REQUANTS16), return FAILED);
  ge::GeTensorDesc requant_s16_in_desc0 = eltwise_op->GetInputDesc(0);
  ge::GeTensorDesc requant_s16_in_desc1;
  requant_s16_in_desc0.SetDataType(ge::DT_INT16);
  requant_s16_in_desc0.SetOriginDataType(ge::DT_INT16);
  requant_s16_in_desc1.SetDataType(ge::DT_UINT64);
  requant_s16_in_desc1.SetOriginDataType(ge::DT_UINT64);
  requant_s16_in_desc1.SetFormat(ge::FORMAT_NCHW);
  requant_s16_in_desc1.SetOriginFormat(ge::FORMAT_NCHW);
  ge::GeTensorDesc requant_s16_in_desc2 = eltwise_op->GetInputDesc(1);
  requant_s16_in_desc2.SetDataType(ge::DT_INT16);
  requant_s16_in_desc2.SetOriginDataType(ge::DT_INT16);
  if (requant_s16_op->AddInputDesc(requant_s16_in_desc0) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][CrtReqntS16] add input x for requants16 fail.");
    return FAILED;
  }
  if (requant_s16_op->AddInputDesc(requant_s16_in_desc1) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][CrtReqntS16] add input deq_scale for requants16 fail.");
    return FAILED;
  }
  if (requant_s16_op->AddInputDesc(requant_s16_in_desc2) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][CrtReqntS16] add input x1 for requants16 fail.");
    return FAILED;
  }

  ge::GeTensorDesc requant_s16_out_desc0;
  if (requant_s16_op->AddOutputDesc(requant_s16_out_desc0) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][CrtReqntS16] add output y for requants16 fail.");
    return FAILED;
  }
  if (dual_output) {
    ge::GeTensorDesc requant_s16_out_desc1;
    if (requant_s16_op->AddOutputDesc(requant_s16_out_desc1) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][CrtReqntS16] add output y1 for requants16 fail.");
      return FAILED;
    }
  }
  requants16_node = graph.AddNode(requant_s16_op);
  FE_CHECK(requants16_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][CrtReqntS16] requants16Node is nullptr"), return PARAM_INVALID);

  static std::atomic<uint64_t> atomic_name_id(0);
  auto name_id = atomic_name_id.fetch_add(1);
  // create const node for deq_scale input
  ge::OpDescPtr const_op_desc = nullptr;
  FE_MAKE_SHARED(const_op_desc = std::make_shared<ge::OpDesc>("requantS16_scale_" + std::to_string(name_id), CONSTANT),
                 return FAILED);
  ge::GeTensorDesc const_out_desc;
  const_out_desc.SetDataType(ge::DT_UINT64);
  const_out_desc.SetOriginDataType(ge::DT_UINT64);
  const_out_desc.SetFormat(ge::FORMAT_NCHW);
  const_out_desc.SetOriginFormat(ge::FORMAT_NCHW);
  if (const_op_desc->AddOutputDesc(const_out_desc) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][CrtReqntS16] AddOutputDesc failed!");
    return FAILED;
  }
  ge::NodePtr const_node = graph.AddNode(const_op_desc);
  FE_CHECK(const_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][CrtReqntS16] constNode is nullptr"), return PARAM_INVALID);

  // deq_scale is the name of the 2nd input of requants16 in IR
  Status res = ge::GraphUtils::AddEdge(const_node->GetOutDataAnchor(0), requants16_node->GetInDataAnchor(1));
  if (res != SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOpt][V200NotReqntFus][CrtReqntS16] ConvNode[%s]: add edge between new const node and requant "
        "node failed!",
        requants16_node->GetName().c_str());
    return res;
  }
  fusion_nodes.push_back(requants16_node);
  fusion_nodes.push_back(const_node);
  return SUCCESS;
}

Status V200NotRequantFusionPass::UpdateRequants16Node(const int &co,
                                                      const std::unique_ptr<uint64_t[]> &scale64_data,
                                                      const ge::NodePtr &requants16_node,
                                                      const float &offset_quant,
                                                      const bool &relu_flag,
                                                      const bool &dual_output) const {
  std::vector<int64_t> dim;
  dim.push_back(static_cast<int64_t>(co));
  ge::GeShape shape(dim);
  ge::GeTensorDesc ScaleDesc(shape, ge::FORMAT_NCHW, ge::DT_UINT64);
  ge::GeTensorPtr scale_ptr = nullptr;
  vector<ge::GeTensorPtr> weights_requant;
  FE_MAKE_SHARED((scale_ptr = std::make_shared<ge::GeTensor>(ScaleDesc, reinterpret_cast<uint8_t *>(scale64_data.get()),
                                                             co * sizeof(uint64_t))),
                 return PARAM_INVALID);

  weights_requant.push_back(scale_ptr);
  ge::OpDescUtils::SetWeights(requants16_node, weights_requant);
  requants16_node->GetOpDesc()->MutableInputDesc(1)->SetShape(shape);
  requants16_node->GetOpDesc()->MutableInputDesc(1)->SetOriginShape(shape);
  ge::NodePtr req_scale = requants16_node->GetInAllNodes().at(1);
  req_scale->GetOpDesc()->MutableOutputDesc(0)->SetShape(shape);
  req_scale->GetOpDesc()->MutableOutputDesc(0)->SetOriginShape(shape);
  if (!ge::AttrUtils::SetFloat(requants16_node->GetOpDesc(), ATTR_OFFSET, offset_quant)) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][UpdReqnt16Nd] set ATTR_OFFSET failed!");
    return FAILED;
  }
  if (!ge::AttrUtils::SetBool(requants16_node->GetOpDesc(), ATTR_DUALOUT, dual_output)) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][UpdReqnt16Nd] set ATTR_DUALOUT failed!");
    return FAILED;
  }
  if (!ge::AttrUtils::SetBool(requants16_node->GetOpDesc(), ATTR_RELU_FLAG, relu_flag)) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][UpdReqnt16Nd] set relu_flag failed!");
    return FAILED;
  }

  // update requants16 shape
  for (size_t index = 0; index < requants16_node->GetAllOutDataAnchors().size(); index++) {
    ge::NodePtr next_node = requants16_node->GetOutDataAnchor(static_cast<int>(index))->GetPeerInDataAnchors().at(0)
                            ->GetOwnerNode();
    auto next_id = requants16_node->GetOutDataAnchor(static_cast<int>(index))->GetPeerInDataAnchors().at(0)->GetIdx();
    auto input_desc = next_node->GetOpDesc()->GetInputDesc(next_id);
    if (index == 0) {
      input_desc.SetDataType(ge::DT_INT8);
      input_desc.SetOriginDataType(ge::DT_INT8);
    } else {
      input_desc.SetDataType(ge::DT_INT16);
      input_desc.SetOriginDataType(ge::DT_INT16);
    }
    (void)requants16_node->GetOpDesc()->UpdateOutputDesc(static_cast<uint32_t>(index), input_desc);
  }
  return SUCCESS;
}

Status V200NotRequantFusionPass::InitRequantS16Op(ge::NodePtr requants16_node, ge::NodePtr quant_node,
                                                  std::vector<float> &Scale_deq, std::vector<int8_t> &N, bool relu_flag,
                                                  bool dual_output) const {
  int co = Scale_deq.size();
  ge::OpDescPtr requant_s16_op = requants16_node->GetOpDesc();

  float_t scale_quant;
  if (!ge::AttrUtils::GetFloat(quant_node->GetOpDesc(), ATTR_SCALE, scale_quant)) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][InitReqntS16Op] get attr scale failed!");
    return FAILED;
  }
  float_t offset_quant;
  if (!ge::AttrUtils::GetFloat(quant_node->GetOpDesc(), ATTR_OFFSET, offset_quant)) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][InitReqntS16Op] get attr offset failed!");
    return FAILED;
  }
  std::unique_ptr<uint64_t[]> scale64_data(new (std::nothrow) uint64_t[co]());
  FE_CHECK(scale64_data == nullptr,
           REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][InitReqntS16Op] biasInt16Data is nullptr"),
           return PARAM_INVALID);
  if (NnSet(co, UINT64_NUM_ZERO, *reinterpret_cast<uint64_t *>(scale64_data.get())) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][InitReqntS16Op] NnSet failed.");
    return FAILED;
  }

  int8_t tmp_offset_quant = static_cast<int8_t>(offset_quant);
  for (int i = 0; i < co; i++) {
    float_t scale_req;
    FE_FLOAT_MULCHECK(Scale_deq[i], scale_quant);
    FE_FLOAT_MULCHECK(Scale_deq[i] * scale_quant, pow(SCALE_BASE, N[i]));
    scale_req = Scale_deq[i] * scale_quant * pow(SCALE_BASE, N[i]);
    scale64_data[i] = reinterpret_cast<uint32_t &>(scale_req);
    // set bias s9
    FE_LOGD("Node[%s]:offset is %d", quant_node->GetName().c_str(), tmp_offset_quant);
    if (offset_quant >= 0) {
      uint64_t tmp_offset = static_cast<uint64_t>(tmp_offset_quant);
      FE_LOGD("Node[%s]:uint64 offset value is %ld", quant_node->GetName().c_str(), tmp_offset);
      scale64_data[i] = scale64_data[i] | ((tmp_offset << 37) & 0x00001fe000000000);
    } else {
      uint64_t tmp_offset = static_cast<uint64_t>(tmp_offset_quant);
      FE_LOGD("Node[%s]:uint64 offset value is %ld", quant_node->GetName().c_str(), tmp_offset);
      scale64_data[i] = scale64_data[i] | ((tmp_offset << 37) & 0x00001fe000000000) | 0x0000200000000000;
    }
    // set int/uint
    scale64_data[i] = scale64_data[i] | 0x0000400000000000;
  }

  // update requants16 shape
  if (UpdateRequants16Node(co, scale64_data, requants16_node, offset_quant, relu_flag, dual_output) != SUCCESS) {
    return FAILED;
  }
  return SUCCESS;
}

Status V200NotRequantFusionPass::ChangeEdgeToRequantS16(ge::NodePtr requants16_node, ge::NodePtr eltwise_node) const {
  // change edge to requants16
  if (ge::GraphUtils::AddEdge(eltwise_node->GetInDataAnchor(0)->GetPeerOutAnchor(),
                              requants16_node->GetInDataAnchor(0)) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][ChgEgToReqntS16] change in edge from %s 1th to %s 1th failed.",
                    eltwise_node->GetName().c_str(),
                    requants16_node->GetName().c_str());
    return FAILED;
  }

  if (ge::GraphUtils::AddEdge(eltwise_node->GetInDataAnchor(1)->GetPeerOutAnchor(),
                              requants16_node->GetInDataAnchor(SECONDE_OPDESC_INDEX)) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][ChgEgToReqntS16] change in edge from %s 2th to %s 3th failed.",
                    eltwise_node->GetName().c_str(),
                    requants16_node->GetName().c_str());
    return FAILED;
  }

  bool flag = ((eltwise_node->GetOutAllNodes().at(0)->GetType() == RELU) ||
               (eltwise_node->GetOutAllNodes().at(0)->GetType() == LEAKY_RELU));
  ge::NodePtr t_node = flag ? eltwise_node->GetOutAllNodes().at(0) : eltwise_node;
  FE_LOGI("requants16 output size is %zu.", requants16_node->GetAllOutAnchors().size());

  auto in_anchors = t_node->GetOutDataAnchor(0)->GetPeerInDataAnchors();
  for (auto in_anchor : in_anchors) {
    if (in_anchor->GetOwnerNode()->GetType() == ASCEND_QUANT) {
      for (auto cube_node : in_anchor->GetOwnerNode()->GetOutAllNodes()) {
        cube_node->GetInDataAnchor(0)->UnlinkAll();

        if (ge::GraphUtils::AddEdge(requants16_node->GetOutDataAnchor(0), cube_node->GetInDataAnchor(0)) != SUCCESS) {
          REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][ChgEgToReqntS16] change edge from %s 1th in to %s 1th out \
                          failed.", cube_node->GetName().c_str(), requants16_node->GetName().c_str());
          return FAILED;
        }
      }
    } else if (in_anchor->GetOwnerNode()->GetType() == ADD || in_anchor->GetOwnerNode()->GetType() == ELTWISE) {
      FE_LOGD("requants16 has eltwise output.");
      if (requants16_node->GetAllOutAnchors().size() != DOUBLE_OUTPUT) {
        REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][ChgEgToReqntS16] requants16 node %s output is not 2.",
                        requants16_node->GetName().c_str());
        return FAILED;
      }
      in_anchor->UnlinkAll();
      if (ge::GraphUtils::AddEdge(requants16_node->GetOutDataAnchor(1), in_anchor) != SUCCESS) {
        REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][ChgEgToReqntS16] add edge from %s 1th in to %s 2th out failed.",
                        t_node->GetName().c_str(), requants16_node->GetName().c_str());
        return FAILED;
      }
    }
  }

  return SUCCESS;
}

Status V200NotRequantFusionPass::RequantS16MultOutput(ge::NodePtr eltwise_node, bool &mult_output, bool &relu_flag,
                                                      ge::NodePtr &relu_node) const {
  mult_output = false;
  relu_flag = false;
  auto next_node = eltwise_node->GetOutAllNodes().at(0);
  if (next_node->GetType() == RELU || next_node->GetType() == LEAKY_RELU) {
    relu_flag = CheckNeedRemoveRelu(next_node);
    relu_node = next_node;
  } else {
    next_node = eltwise_node;
  }
  auto quant_nodes = next_node->GetOutAllNodes();
  for (auto node : quant_nodes) {
    if (node->GetType() == ADD || node->GetType() == ELTWISE) {
      mult_output = true;
      break;
    }
  }
  return SUCCESS;
}

Status V200NotRequantFusionPass::DealHighPerformanceEltwiseQuant(ge::ComputeGraph &graph, ge::NodePtr eltwise_node,
                                                                 vector<ge::NodePtr> &quants,
                                                                 std::vector<float> &Scale_deq, std::vector<int8_t> &N,
                                                                 vector<ge::NodePtr> &fusion_nodes) const {
  bool relu_flag = false;
  bool dual_output = false;
  ge::NodePtr requants16_node = nullptr;
  ge::NodePtr relu_node = nullptr;
  auto eltwise_op = eltwise_node->GetOpDesc();

  if (quants.empty()) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][DealHighPfmEltQnt] There is no quant node, can not do \
                    requants16 optimize.");
    return FAILED;
  }

  (void)RequantS16MultOutput(eltwise_node, dual_output, relu_flag, relu_node);

  if (CreateRequantS16BasedOnEltwise(graph, eltwise_op, requants16_node, dual_output, fusion_nodes) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][DealHighPfmEltQnt] Create requants16 op fail.");
    return FAILED;
  }
  FE_CHECK(requants16_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][DealHighPfmEltQnt] requants16Node is nullptr"),
           return PARAM_INVALID);

  if (ChangeEdgeToRequantS16(requants16_node, eltwise_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][DealHighPfmEltQnt] change edge to requants16 op fail.");
    return FAILED;
  }

  ge::NodePtr quant_node = quants.at(0);
  if (InitRequantS16Op(requants16_node, quant_node, Scale_deq, N, relu_flag, dual_output) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][DealHighPfmEltQnt] Init requants16 op fail.");
    return FAILED;
  }

  // delete node
  if (relu_flag) {
    if (graph.RemoveNode(relu_node) == ge::GRAPH_FAILED) {
      REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][DealHighPfmEltQnt] reluNode remove failed");
      return FAILED;
    }
  }
  for (auto quant_node_inner : quants) {
    if (graph.RemoveNode(quant_node_inner) == ge::GRAPH_FAILED) {
      REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][DealHighPfmEltQnt] quantNode remove failed");
      return FAILED;
    }
  }
  if (graph.RemoveNode(eltwise_node) == ge::GRAPH_FAILED) {
    REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][DealHighPfmEltQnt] eltwiseNode remove failed");
    return FAILED;
  }
  return SUCCESS;
}

Status V200NotRequantFusionPass::ParseQuantsOfNotRequantPattern2(ge::NodePtr eltwise_node,
                                                                 vector<ge::NodePtr> &quants) const {
  bool has_relu = false;
  for (auto node_ptr : eltwise_node->GetOutAllNodes()) {
    if (node_ptr->GetType() == RELU || node_ptr->GetType() == LEAKY_RELU) {
      if (!CheckNeedRemoveRelu(node_ptr)) {
        return NOT_CHANGED;
      }
      has_relu = true;
      for (auto node_next : node_ptr->GetOutAllNodes()) {
        if (node_next->GetType() == ASCEND_QUANT) {
          quants.push_back(node_next);
        } else if (node_next->GetType() == ADD || node_next->GetType() == ELTWISE) {
          continue;
        } else {
          return NOT_CHANGED;
        }
      }
    } else if (node_ptr->GetType() == ASCEND_QUANT) {
      if (has_relu) {
        return NOT_CHANGED;
      }
      quants.push_back(node_ptr);
    } else if (node_ptr->GetType() == ADD || node_ptr->GetType() == ELTWISE) {
      // do nothing to eltwise node
    } else {
      return NOT_CHANGED;
    }
  }
  return SUCCESS;
}

Status V200NotRequantFusionPass::ParseDequantsOfNotRequantPattern2(ge::NodePtr eltwise_node,
                                                                   vector<ge::NodePtr> &dequants) const {
  float negative_slope = 0;

  for (auto node_ptr : eltwise_node->GetInAllNodes()) {
    if (node_ptr->GetType() == RELU || node_ptr->GetType() == LEAKY_RELU) {
      if (node_ptr->GetType() == LEAKY_RELU) {
        if (!ge::AttrUtils::GetFloat(node_ptr->GetOpDesc(), ATTR_NEGATIVE_SLOPE, negative_slope)) {
          REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][ParseDeqntOfNotReqntPtn2] leakyRelu node does not have \
                          negative slope attr!");
          return FAILED;
        }
        if (!FloatEqual(negative_slope, 0.0)) {
          FE_LOGD("leakyRelu has negative slope.");
          return NOT_CHANGED;
        }
      }
      for (auto input_node : node_ptr->GetInAllNodes()) {
        if (input_node->GetType() != ASCEND_DEQUANT && input_node->GetType() != REQUANTS16) {
          FE_LOGD("Node[%s]'s optype is [%s], no need to do fusion.", input_node->GetName().c_str(),
                  input_node->GetType().c_str());
          return NOT_CHANGED;
        }
        if (input_node->GetType() == ASCEND_DEQUANT) {
          dequants.push_back(input_node);
          FE_LOGD("Node[%s]'s optype is [%s], record it.", input_node->GetName().c_str(),
                  input_node->GetType().c_str());
        }
      }
    } else if (node_ptr->GetType() == ASCEND_DEQUANT) {
      dequants.push_back(node_ptr);
      FE_LOGD("Node[%s]'s optype is [%s], record it.", node_ptr->GetName().c_str(), node_ptr->GetType().c_str());
    } else if (node_ptr->GetType() == REQUANTS16) {
      FE_LOGD("Node[%s]'s optype is [%s], no need to record.", node_ptr->GetName().c_str(),
              node_ptr->GetType().c_str());
    } else {
      FE_LOGD("Node[%s]'s optype is [%s], no need to do fusion.", node_ptr->GetName().c_str(),
              node_ptr->GetType().c_str());
      return NOT_CHANGED;
    }
  }
  return SUCCESS;
}

Status V200NotRequantFusionPass::ParseNotRequantPattern2(ge::NodePtr eltwise_node, vector<ge::NodePtr> &quants,
                                                         vector<ge::NodePtr> &dequants) const {
  int requanted_flg = 0;
  Status ret = ParseQuantsOfNotRequantPattern2(eltwise_node, quants);
  if (ret != SUCCESS) {
    FE_LOGD("ParseQuantsOfNotRequantPattern2 abnormal, return %d.", ret);
    return ret;
  }

  ret = ParseDequantsOfNotRequantPattern2(eltwise_node, dequants);
  if (ret != SUCCESS) {
    FE_LOGD("ParseDequantsOfNotRequantPattern2 abnormal, return %d.", ret);
    return ret;
  }

  std::string quant_mode;
  for (auto dequant_node : dequants) {
    // dequant node should be single reference
    if (dequant_node->GetOutDataNodes().size() != 1) {
      return NOT_CHANGED;
    }
    if (dequant_node->GetInAllNodes().empty()) {
      return NOT_CHANGED;
    }
    // get deqaunt node quant mode
    (void)ge::AttrUtils::GetStr(dequant_node->GetOpDesc(), STR_QUANT_MODE, quant_mode);
    if (quant_mode != STR_QUANT_HIGH_PERFORMANCE) {
      FE_LOGD("Node[%s] quant mode is not high performance.", dequant_node->GetName().c_str());
      return NOT_CHANGED;
    }
  }
  for (auto node : quants) {
    if (ge::AttrUtils::GetInt(node->GetOpDesc(), ATTR_REQUANTED, requanted_flg)) {
      FE_LOGI("this node has beed requanted, skip");
      return NOT_CHANGED;
    }
  }

  for (auto node : dequants) {
    if (ge::AttrUtils::GetInt(node->GetOpDesc(), ATTR_REQUANTED, requanted_flg)) {
      FE_LOGI("this node has beed requanted, skip");
      return NOT_CHANGED;
    }
  }
  return SUCCESS;
}

Status V200NotRequantFusionPass::NotRequantPattern0(ge::ComputeGraph &graph, ge::NodePtr eltwise_node,
                                                    vector<ge::NodePtr> &fusion_nodes) const {
  Status ret;
  vector<ge::NodePtr> quants;
  vector<ge::NodePtr> dequants;
  vector<float> scale_deq;
  std::vector<int8_t> N;
  ret = ParseNotRequantPattern2(eltwise_node, quants, dequants);
  if (ret != SUCCESS) {
    FE_LOGD("ParseNotRequantPattern2 abnormal, return %d.", ret);
    return ret;
  }

  ret = DealHighPerformanceDeqaunt(graph, dequants, scale_deq, N, fusion_nodes);
  if (ret != SUCCESS) {
    FE_LOGD("DealHighPerformanceDeqaunt abnormal, return %d.", ret);
    return ret;
  }

  ret = DealHighPerformanceEltwiseQuant(graph, eltwise_node, quants, scale_deq, N, fusion_nodes);
  if (ret != SUCCESS) {
    FE_LOGD("DealHighPerformanceEltwiseQuant abnormal, return %d.", ret);
    return ret;
  }
  return SUCCESS;
}

/*
 * ===========================Before Fusion===============================
 * ##########pattern0################         ##########pattern1##########
 *      dequant        dequant
 *         |              |
 *         |              |
 *            Eltwise/Add
 *                |
 *                |                               dequant(single node)
 *            ---------                                     |
 *           |         |                               relu(optional)
 *           |       quant
 *           |         |
 *       Eltwise/Add conv2d
 *
 *
 * ===========================After Fusion================================
 *
 * ##########pattern0################         ##########pattern1##########
 *     dequants16     dequants16
 *         |              |
 *         |              |
 *            requants16
 *           |         |
 *           |         |                          dequant(single node)
 *           |         |
 *     int16 |         | int8
 *           |       conv2d
 *           |
 *       Eltwise/Add
 *
 */
vector<FusionPattern *> V200NotRequantFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;

  const ISAArchVersion isa_arch_version = Configuration::Instance(AI_CORE_NAME).GetIsaArchVer();
  if (isa_arch_version == ISAArchVersion::EN_ISA_ARCH_V200) {
    FusionPattern *pattern0 = new(std::nothrow) FusionPattern("notRequantPassPattern0");
    FE_CHECK(pattern0 == nullptr,
             REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][DfnPtn] new FusionPattern object failed!"), return patterns);
    pattern0->AddOpDesc(PATTERN_ELTWISE, {ELTWISE, ADD}).SetOutput(PATTERN_ELTWISE);
    patterns.push_back(pattern0);
  }

  FusionPattern *pattern1 = new (std::nothrow) FusionPattern("notRequantPassPattern1");
  FE_CHECK(pattern1 == nullptr,
           REPORT_FE_ERROR("[GraphOpt][V200NotReqntFus][DfnPtn] new FusionPattern object failed!"), return patterns);
  pattern1->AddOpDesc(PATTERN_DEQUANT, {ASCEND_DEQUANT}).SetOutput(PATTERN_DEQUANT);
  patterns.push_back(pattern1);

  return patterns;
}

Status V200NotRequantFusionPass::Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &fusion_nodes) {
  ge::NodePtr dequant_node = GetNodeFromMapping(PATTERN_DEQUANT, mapping);
  ge::NodePtr eltwise_node = GetNodeFromMapping(PATTERN_ELTWISE, mapping);

  // eltwise + cube
  if (eltwise_node.get()) {
    return NotRequantPattern0(graph, eltwise_node, fusion_nodes);
  }
  if (dequant_node.get()) {
    ClearOutputAnchorMap();
    return NotRequantPattern1(graph, dequant_node, fusion_nodes, mapping);
  }

  return NOT_CHANGED;
}

void V200NotRequantFusionPass::RemoveHostOpFromFusionNode(vector<ge::NodePtr> &new_nodes) const {
  if (new_nodes.size() == 2) {
    for (auto iter = new_nodes.begin(); iter != new_nodes.end();) {
      ge::NodePtr node = *iter;
      if (node->GetOpDesc()->GetType() == REQUANT_HOST_CPU_OP) {
        iter = new_nodes.erase(iter);
      } else {
        iter++;
      }
    }
  }
}
}  // namespace fe
