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

#include "fixpipe_addinputstrategy.h"
#include <memory>
#include <sstream>
#include "common/aicore_util_constants.h"
#include "common/configuration.h"
#include "common/fe_fp16_t.h"
#include "common/op_info_common.h"
#include "common/util/platform_info.h"
#include "fixpipe_common.h"
#include "fixpipe_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/quant_host_cpu_op_common.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/fusion_pass_registry.h"

namespace fe {
constexpr int16_t MAXINT8_VALUE = 127;
constexpr uint32_t BITSHIFT_4_BYTESIZE = 32;
constexpr uint32_t BITSHIFT_3_BYTESIZE = 24;
constexpr uint32_t BITSHIFT_37 = 37;
const std::string ATTR_OUTDTYPE = "dequantouttype";
template <typename T>
Status FixPipeAddInputBase::UpdateSalarInput(ge::GeTensorDescPtr tensor_desc, T value,
                                             ge::GeTensorPtr tensornode, const ge::DataType &data_type) const {
  size_t data_size = CalDSize(data_type);
  if (data_size == 0) {
    return FAILED;
  }
  ge::GeShape shape{};
  FE_LOGD("value = %f data size = %zu", static_cast<float>(value), data_size);
  tensor_desc->SetDataType(data_type);
  tensor_desc->SetOriginDataType(data_type);
  tensor_desc->SetShape(shape);
  tensor_desc->SetOriginShape(shape);
  tensor_desc->SetFormat(ge::FORMAT_ND);
  tensor_desc->SetOriginFormat(ge::FORMAT_ND);
  if (tensornode->SetData(reinterpret_cast<uint8_t *>(&value), data_size) != SUCCESS) {
    return FAILED;
  }
  return SUCCESS;
}

uint64_t FixPipeAddInputBase::TransM1Scale(const float &src_value) const {
  uint32_t value = 0;
  if (memcpy_s(&value, sizeof(uint32_t), &src_value, sizeof(float)) != 0) {
    REPORT_FE_ERROR("[GraphOpt][FixPipeAddInputBase][TransM1Scale] Fail to memcpy_s");
    return 0;
  }
  uint64_t tmp_data = static_cast<uint64_t>(value) & 0x000000000FFFFE000;
  return tmp_data;
}

uint64_t FixPipeAddInputBase::SetM1OfQuant(const float &scale, const float &offset, const ge::NodePtr node) const {
  uint32_t offset_tmp_tmp = static_cast<uint32_t>(offset);
  uint64_t offset_tmp = static_cast<uint64_t>(offset_tmp_tmp);
  uint64_t tmp_data = 0;
  auto output_descptr = node->GetOpDesc()->MutableOutputDesc(0);
  if (output_descptr == nullptr) {
    return 0;
  }
  ge::DataType data_type = output_descptr->GetDataType();
  if (data_type == ge::DT_UINT16) {
    tmp_data = TransM1Scale(scale) +
               ((offset_tmp >> BITSHIFT_3_BYTESIZE) & 0xFF) +
               (((offset_tmp & 0x1FF) << BITSHIFT_37) & 0x3FE000000000);
  } else if (data_type == ge::DT_UINT8) {
    tmp_data = TransM1Scale(scale) + (((offset_tmp & 0x1FF) << BITSHIFT_37) & 0x3FE000000000);
  } else if (data_type == ge::DT_INT4) {
    tmp_data = TransM1Scale(scale) + (((offset_tmp & 0x1F) << BITSHIFT_37) & 0x3E000000000) + 0x400000000000;
  } else if (data_type == ge::DT_INT16) {
    tmp_data = TransM1Scale(scale) + ((offset_tmp >> BITSHIFT_3_BYTESIZE) & 0xFF) +
               (((offset_tmp & 0x1FF) << BITSHIFT_37) & 0x3FE000000000) +
               0x400000000000;
  } else if (data_type == ge::DT_INT8) {
    tmp_data = TransM1Scale(scale) + (((offset_tmp & 0x1FF) << BITSHIFT_37) & 0x3FE000000000) + 0x400000000000;
  }
  return tmp_data;
}

uint64_t FixPipeAddInputBase::SetM3OfQuant(const float &scale, const float &offset, const ge::NodePtr node) const {
  uint32_t offset_tmp_tmp = static_cast<uint32_t>(offset);
  uint64_t offset_tmp = static_cast<uint64_t>(offset_tmp_tmp);
  uint64_t tmp_data = 0;
  auto output_descptr = node->GetOpDesc()->MutableOutputDesc(0);
  if (output_descptr == nullptr) {
    return 0;
  }
  ge::DataType data_type = output_descptr->GetDataType();
  if (data_type == ge::DT_UINT16) {
    uint64_t tmp_offset = static_cast<uint64_t>((offset_tmp >> BITSHIFT_3_BYTESIZE) && 0xFF);
    tmp_data = TransM1Scale(scale) + (offset_tmp & 0x1FF) + ((tmp_offset << BITSHIFT_4_BYTESIZE) & 0xFF00000000);
  } else if (data_type == ge::DT_UINT8) {
    tmp_data = TransM1Scale(scale) + (offset_tmp & 0x1FF);
  } else if (data_type == ge::DT_INT4) {
    tmp_data = TransM1Scale(scale) + (offset_tmp & 0x1F) + 0x200;
  } else if (data_type == ge::DT_INT16) {
    uint64_t tmp_offset = static_cast<uint64_t>((offset_tmp >> BITSHIFT_3_BYTESIZE) && 0xFF);
    tmp_data = TransM1Scale(scale) + (offset_tmp & 0x1FF) +
               ((tmp_offset << BITSHIFT_4_BYTESIZE) & 0xFF00000000) + 0x200;
  } else if (data_type == ge::DT_INT8) {
    tmp_data = TransM1Scale(scale) + (offset_tmp & 0x1FF) + 0x200;
  }
  return tmp_data;
}

ge::NodePtr FixPipeAddInputBase::CreateNewDataNodeOnly(ge::ComputeGraph &graph, ge::GeTensorDescPtr tensor_desc,
                                                       ge::GeTensorPtr tensornode, const std::string &op_name,
                                                       std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("fixpipenode name = %s", op_name.c_str());
  ge::OpDescPtr newdatadesc = nullptr;
  FE_MAKE_SHARED(newdatadesc = std::make_shared<ge::OpDesc>(op_name, CONSTANT), return nullptr);
  (void)ge::AttrUtils::SetBool(newdatadesc, "_is_single_op", ATTRTRUE);
  (void)ge::AttrUtils::SetBool(newdatadesc, ge::ATTR_NAME_IS_ORIGINAL_INPUT, ATTRTRUE);
  if (newdatadesc->AddOutputDesc(*tensor_desc) != ge::GRAPH_SUCCESS) {
    FE_LOGD("AddOutputDesc Failed");
    return nullptr;
  }
  if (!ge::AttrUtils::SetTensor(newdatadesc, ge::ATTR_NAME_WEIGHTS, tensornode)) {
    FE_LOGD("SetTensor = null");
    return nullptr;
  }
  ge::NodePtr newdatanode = graph.AddNode(newdatadesc);
  new_nodes.push_back(newdatanode);
  return newdatanode;
}

Status FixPipeAddInputBase::CreateNewDataNodeDirect(ge::ComputeGraph &graph, ge::GeTensorDescPtr tensor_desc,
                                                    ge::GeTensorPtr tensornode,
                                                    const FixPipeFunctionParamPtr &functpara,
                                                    std::vector<ge::NodePtr> &new_nodes) const {
  ge::NodePtr fixpipenode = functpara->GetFixpipeNode();
  int input_index = functpara->GetParaIndex();
  std::string op_name = fixpipenode->GetName() + "_" +  functpara->GetInputName();
  auto newdatanode = CreateNewDataNodeOnly(graph, tensor_desc, tensornode, op_name, new_nodes);
  if (newdatanode->GetOutDataAnchor(0) == nullptr || fixpipenode->GetInDataAnchor(input_index) == nullptr) {
    FE_LOGD("GetOutDataAnchor GetInDataAnchor = null ");
    return FAILED;
  }
  if (ge::GraphUtils::AddEdge(newdatanode->GetOutDataAnchor(0), fixpipenode->GetInDataAnchor(input_index)) !=
      ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FixpipePss][RelkDataEdge] Fail to add edge between src node[%s] and dst node[%s].",
                    newdatanode->GetName().c_str(), fixpipenode->GetName().c_str());
    return FAILED;
  }
  new_nodes.push_back(newdatanode);
  return SUCCESS;
}

bool FixPipeAddInputBase::IsScalar(const ge::GeShape &origin_shape) const {
  if (origin_shape.GetShapeSize() == 0 || origin_shape.GetShapeSize() == 1) {
    return true;
  }
  return false;
}

Status FixPipeAddInputBase::CreateAndUpdateVectorMulsInput(ge::ComputeGraph &graph,
                                                           const FixPipeFunctionParamPtr &functpara,
                                                           const FixPipeNodeInfo &postfuzednode,
                                                           const FixPipeNodeInfo &tofuzednode,
                                                           std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("CreateAndUpdateVectorMulsInput start");
  auto tofuze_node = tofuzednode.GetNode();
  auto post_fuze_node = postfuzednode.GetNode();
  if (HasInput1Node(tofuze_node) != SUCCESS || HasInput1Node(post_fuze_node) != SUCCESS) {
    return FAILED;
  }
  ge::NodePtr fixpipenode = functpara->GetFixpipeNode();
  ge::OpDescPtr newopdesc = nullptr;
  int input_index = functpara->GetParaIndex();
  FE_MAKE_SHARED(newopdesc = std::make_shared<ge::OpDesc>(
                     fixpipenode->GetName() + "VectorMuls" + functpara->GetInputName(), "VectorMuls"),
                 return FAILED);
  (void)ge::AttrUtils::SetBool(newopdesc, "_is_single_op", ATTRTRUE);
  auto tufuzenode_inputdesc = tofuze_node->GetOpDesc()->MutableInputDesc(1);
  auto postnode_inputdesc = post_fuze_node->GetOpDesc()->MutableInputDesc(1);
  if (tufuzenode_inputdesc == nullptr || postnode_inputdesc == nullptr) {
    return FAILED;
  }
  ge::GeShape dequant_input_shape = tufuzenode_inputdesc->GetOriginShape();
  ge::GeShape prelu_input_shape = postnode_inputdesc->GetOriginShape();
  ge::GeTensorDesc out_tensor_desc = tufuzenode_inputdesc->Clone();
  out_tensor_desc.SetDataType(ge::DT_FLOAT);
  out_tensor_desc.SetOriginDataType(ge::DT_FLOAT);
  if (IsScalar(dequant_input_shape)) {
    out_tensor_desc.SetShape(postnode_inputdesc->GetShape());
    out_tensor_desc.SetOriginShape(prelu_input_shape);
  } else {
    out_tensor_desc.SetShape(tufuzenode_inputdesc->GetShape());
    out_tensor_desc.SetOriginShape(dequant_input_shape);
  }
  newopdesc->AddOutputDesc("y", out_tensor_desc);
  newopdesc->AddInputDesc(X1INPUTNAME, *(tufuzenode_inputdesc));
  newopdesc->AddInputDesc(X2INPUTNAME, *(postnode_inputdesc));
  ge::NodePtr newopnode = graph.AddNode(newopdesc);
  auto inputnode = tofuze_node->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode();
  auto anotherinput = post_fuze_node->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode();
  fixpipenode->GetOpDesc()->UpdateInputDesc(input_index, out_tensor_desc);
  if (!AddEdges(inputnode->GetOutDataAnchor(0), newopnode->GetInDataAnchor(0), inputnode, newopnode) ||
      !AddEdges(anotherinput->GetOutDataAnchor(0), newopnode->GetInDataAnchor(1), anotherinput, newopnode) ||
      !AddEdges(newopnode->GetOutDataAnchor(0), fixpipenode->GetInDataAnchor(input_index), newopnode, fixpipenode)) {
    return FAILED;
  }
  new_nodes.push_back(newopnode);
  return SUCCESS;
}

template <typename T>
Status FixPipeAddInputBase::CreateAndUpdateVectorMulScalarInput(ge::ComputeGraph &graph,
                                                                const FixPipeFunctionParamPtr &functpara,
                                                                const FixPipeNodeInfo &prefuzednode, const T &value,
                                                                std::vector<ge::NodePtr> &new_nodes) const {
  auto pre_fuzed_node = prefuzednode.GetNode();
  if (HasInput1Node(pre_fuzed_node) != SUCCESS) {
    return FAILED;
  }
  ge::NodePtr fixpipenode = functpara->GetFixpipeNode();
  ge::OpDescPtr newopdesc = nullptr;
  int input_index = functpara->GetParaIndex();
  FE_MAKE_SHARED(newopdesc = std::make_shared<ge::OpDesc>(
                     fixpipenode->GetName() + "VectorMulScalar" + functpara->GetInputName(), "VectorMulScalar"),
                 return FAILED);

  (void)ge::AttrUtils::SetBool(newopdesc, "_is_single_op", ATTRTRUE);
  auto prenode_inputdesc = pre_fuzed_node->GetOpDesc()->MutableInputDesc(1);
  if (prenode_inputdesc == nullptr) {
    return FAILED;
  }
  ge::GeTensorDesc out_tensor_desc = prenode_inputdesc->Clone();
  if (functpara->GetDataType() != ge::DT_UNDEFINED) {
    out_tensor_desc.SetDataType(functpara->GetDataType());
    out_tensor_desc.SetOriginDataType(functpara->GetDataType());
  }
  newopdesc->AddOutputDesc("y", out_tensor_desc);
  newopdesc->AddInputDesc(X1INPUTNAME, *(prenode_inputdesc));
  ge::AttrUtils::SetFloat(newopdesc, ATTR_SCALA, static_cast<float>(value));
  FE_LOGD("DealDequantInput scale = %f", static_cast<float>(value));
  ge::NodePtr newopnode = graph.AddNode(newopdesc);
  auto inputnode = pre_fuzed_node->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode();
  fixpipenode->GetOpDesc()->UpdateInputDesc(input_index, out_tensor_desc);
  if (!AddEdges(inputnode->GetOutDataAnchor(0), newopnode->GetInDataAnchor(0), inputnode, newopnode)) {
    return FAILED;
  }
  if (!AddEdges(newopnode->GetOutDataAnchor(0), fixpipenode->GetInDataAnchor(input_index), newopnode, fixpipenode)) {
    return FAILED;
  }
  new_nodes.push_back(newopnode);
  return SUCCESS;
}

Status FixPipeAddInputBase::CloneVectorInput(const FixPipeNodeInfo &tofuzednode,
                                             const FixPipeFunctionParamPtr &functpara) const {
  ge::NodePtr fixpipenode = functpara->GetFixpipeNode();
  auto tofuze_node = tofuzednode.GetNode();
  auto tufuzenode_inputdesc = tofuze_node->GetOpDesc()->MutableInputDesc(1);
  if (tufuzenode_inputdesc == nullptr) {
    return FAILED;
  }
  int input_index = functpara->GetParaIndex();
  ge::GeTensorDesc cur_tensor_desc = tufuzenode_inputdesc->Clone();
  if (HasInput1Node(tofuze_node) != SUCCESS) {
    return FAILED;
  }
  auto inputnode = tofuze_node->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode();
  if (functpara->GetDataType() != ge::DT_UNDEFINED &&
      cur_tensor_desc.GetDataType() != functpara->GetDataType()) {
    cur_tensor_desc.SetDataType(functpara->GetDataType());
    cur_tensor_desc.SetOriginDataType(functpara->GetDataType());
  }
  fixpipenode->GetOpDesc()->UpdateInputDesc(input_index, cur_tensor_desc);
  if (!AddEdges(inputnode->GetOutDataAnchor(0), fixpipenode->GetInDataAnchor(input_index), inputnode, fixpipenode)) {
    return FAILED;
  }
  return SUCCESS;
}

template <typename T>
Status FixPipeAddInputBase::CreateAndUpdateSalarInput(ge::ComputeGraph &graph, const FixPipeFunctionParamPtr &functpara,
                                                      T value, const ge::DataType &data_type,
                                                      std::vector<ge::NodePtr> &new_nodes) const {
  ge::NodePtr fixpipenode = functpara->GetFixpipeNode();
  int input_index = functpara->GetParaIndex();
  ge::GeTensorDescPtr tensor_desc = fixpipenode->GetOpDesc()->MutableInputDesc(input_index);
  bool has_desc = true;
  if (tensor_desc == nullptr) {
    FE_LOGD("input = %d", input_index);
    ge::GeShape shape{};
    ge::GeTensorDesc fakedesc(shape, ge::FORMAT_ND, data_type);
    FE_MAKE_SHARED(tensor_desc = std::make_shared<ge::GeTensorDesc>(fakedesc), return FAILED);
    has_desc = false;
  }
  ge::GeTensorPtr tensor_node = nullptr;
  FE_MAKE_SHARED(tensor_node = std::make_shared<ge::GeTensor>(), return FAILED);
  tensor_node->SetTensorDesc(*tensor_desc);
  if (UpdateSalarInput<T>(tensor_desc, value, tensor_node, data_type) != SUCCESS) {
    return FAILED;
  }
  if (CreateNewDataNodeDirect(graph, tensor_desc, tensor_node, functpara, new_nodes) != SUCCESS) {
    return FAILED;
  }
  if (!has_desc) {
    fixpipenode->GetOpDesc()->UpdateInputDesc(input_index, *tensor_desc);
  }
  return SUCCESS;
}

void FixPipeAddInputBase::SetClipValue6(ge::ComputeGraph &graph, const FixPipeFunctionParamPtr &functpara,
                                        ge::DataType dst_datatype, std::vector<ge::NodePtr> &new_nodes) const {
  if (dst_datatype == ge::DT_FLOAT) {
    float clipvalue = relu6_value;
    CreateAndUpdateSalarInput<float>(graph, functpara, clipvalue, dst_datatype, new_nodes);
  } else if (dst_datatype == ge::DT_FLOAT16) {
    float relu6value_tmp = relu6_value;
    fe::fp16_t clipvalue(relu6value_tmp);
    FE_LOGD("clipvalue = %u", clipvalue.ToUInt16());
    CreateAndUpdateSalarInput<fe::fp16_t>(graph, functpara, clipvalue, dst_datatype, new_nodes);
  } else if (dst_datatype == ge::DT_INT8 || dst_datatype == ge::DT_INT4) {
    dst_datatype = ge::DT_FLOAT16;
    float relu6value_tmp = relu6_value;
    fe::fp16_t clipvalue(relu6value_tmp);
    FE_LOGD("clipvalue = %u", clipvalue.ToUInt16());
    CreateAndUpdateSalarInput<fe::fp16_t>(graph, functpara, clipvalue, dst_datatype, new_nodes);
  }
}

ge::NodePtr FixPipeAddInputBase::GetQuantScaleOffset(const FixPipePassInfo &match_pass,
                                                     const uint32_t &index,
                                                     float &scale, float &offset) const {
  if (!CheckIsInVector(match_pass.m_opnodes, index)) {
    return nullptr;
  }
  auto node = match_pass.m_opnodes[index].GetNode();
  if (node == nullptr) {
    return nullptr;
  }
  (void)ge::AttrUtils::GetFloat(node->GetOpDesc(), ATTR_SCALE, scale);
  (void)ge::AttrUtils::GetFloat(node->GetOpDesc(), ATTR_OFFSET, offset);
  return node;
}

// quant
Status FixPipeAddInputStrategy21::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy21 precove quant");
  float scale = 0.0;
  float offset = 0.0;
  auto first_node = GetQuantScaleOffset(match_pass, functpara->GetFirstIndex(), scale, offset);
  if (first_node == nullptr) {
    return FAILED;
  }
  uint64_t tmp_data = SetM1OfQuant(scale, offset, first_node);
  CreateAndUpdateSalarInput<uint64_t>(graph, functpara, tmp_data, ge::DT_UINT64, new_nodes);
  return SUCCESS;
}

// dequant/requant
Status FixPipeAddInputStrategy22::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy22 dequant requant");
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetFirstIndex())) {
    return FAILED;
  }
  auto prefuzednode = match_pass.m_opnodes[functpara->GetFirstIndex()];
  auto pre_fuzed_node = prefuzednode.GetNode();
  if (HasInput1Node(pre_fuzed_node) != SUCCESS) {
    return FAILED;
  }
  ge::NodePtr fixpipenode = functpara->GetFixpipeNode();
  ge::OpDescPtr newopdesc = nullptr;
  int input_index = functpara->GetParaIndex();
  auto prenode_inputdesc = pre_fuzed_node->GetOpDesc()->MutableInputDesc(1);
  auto prenode_outdesc = pre_fuzed_node->GetOpDesc()->MutableOutputDesc(0);
  if (prenode_inputdesc == nullptr || prenode_outdesc == nullptr) {
    return FAILED;
  }
  ge::GeTensorDesc pre_tensor_desc = prenode_inputdesc->Clone();
  FE_MAKE_SHARED(newopdesc = std::make_shared<ge::OpDesc>(
                     fixpipenode->GetName() + "SetM1Dequant" + functpara->GetInputName(), "SetM1Dequant"),
                 return FAILED);
  (void)ge::AttrUtils::SetBool(newopdesc, "_is_single_op", ATTRTRUE);
  pre_tensor_desc.SetDataType(ge::DT_UINT64);
  pre_tensor_desc.SetOriginDataType(ge::DT_UINT64);
  newopdesc->AddOutputDesc("y", pre_tensor_desc);
  newopdesc->AddInputDesc(X1INPUTNAME, *(prenode_inputdesc));
  float offset = 0.0;
  if (ge::AttrUtils::GetFloat(pre_fuzed_node->GetOpDesc(), ATTR_OFFSET, offset)) {
    ge::AttrUtils::SetFloat(newopdesc, ATTR_OFFSET, offset);
    FE_LOGD("offset value = %f", offset);
  }
  (void)ge::AttrUtils::SetInt(newopdesc, ATTR_OUTDTYPE, static_cast<int>(prenode_outdesc->GetDataType()));
  ge::NodePtr newopnode = graph.AddNode(newopdesc);
  auto inputnode = pre_fuzed_node->GetInDataAnchor(1)->GetPeerOutAnchor()->GetOwnerNode();
  fixpipenode->GetOpDesc()->UpdateInputDesc(input_index, pre_tensor_desc);
  if (!AddEdges(inputnode->GetOutDataAnchor(0), newopnode->GetInDataAnchor(0), inputnode, newopnode)) {
    return FAILED;
  }
  if (!AddEdges(newopnode->GetOutDataAnchor(0), fixpipenode->GetInDataAnchor(input_index), newopnode, fixpipenode)) {
    return FAILED;
  }
  new_nodes.push_back(newopnode);
  return SUCCESS;
}

// prelu+quant
Status FixPipeAddInputStrategy31::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy31 preact prlu+quant");
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetSecondIndex()) ||
      !CheckIsInVector(match_pass.m_opnodes, functpara->GetFirstIndex())) {
    return FAILED;
  }
  auto second_node = match_pass.m_opnodes[functpara->GetSecondIndex()].GetNode();
  if (second_node == nullptr) {
    return FAILED;
  }
  if (second_node->GetOpDesc()->MutableInputDesc(1) == nullptr) {
    return FAILED;
  }
  float scale = 0.0;
  float offset = 0.0;
  auto first_node = match_pass.m_opnodes[functpara->GetFirstIndex()].GetNode();
  if (first_node == nullptr) {
    return FAILED;
  }
  (void)ge::AttrUtils::GetFloat(first_node->GetOpDesc(), ATTR_SCALE, scale);
  (void)ge::AttrUtils::GetFloat(first_node->GetOpDesc(), ATTR_OFFSET, offset);
  CreateAndUpdateVectorMulScalarInput<float>(graph, functpara, match_pass.m_opnodes[functpara->GetSecondIndex()], scale,
                                             new_nodes);
  return SUCCESS;
}

// prelu+dequant/requant
Status FixPipeAddInputStrategy32::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy32 preact prelu+derequant");
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetSecondIndex()) ||
      !CheckIsInVector(match_pass.m_opnodes, functpara->GetFirstIndex())) {
    return FAILED;
  }
  CreateAndUpdateVectorMulsInput(graph, functpara, match_pass.m_opnodes[functpara->GetSecondIndex()],
                                 match_pass.m_opnodes[functpara->GetFirstIndex()], new_nodes);
  return SUCCESS;
}

// prelu
Status FixPipeAddInputStrategy33::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy33 preact, prelu");
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetFirstIndex())) {
    return FAILED;
  }
  CloneVectorInput(match_pass.m_opnodes[functpara->GetFirstIndex()], functpara);
  return SUCCESS;
}

// lrelu+quant
Status FixPipeAddInputStrategy34::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy34 preact lrelu+quant");
  float scale = 0.0;
  float offset = 0.0;
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetSecondIndex()) ||
      !CheckIsInVector(match_pass.m_opnodes, functpara->GetFirstIndex())) {
    return FAILED;
  }
  auto first_node = GetQuantScaleOffset(match_pass, functpara->GetFirstIndex(), scale, offset);
  if (first_node == nullptr) {
    return FAILED;
  }
  auto second_node = match_pass.m_opnodes[functpara->GetSecondIndex()].GetNode();
  if (second_node == nullptr) {
    return FAILED;
  }
  float attr_negative_slope_a = 0.0;
  (void)ge::AttrUtils::GetFloat(second_node->GetOpDesc(), ATTR_NEGATIVE_SLOPE, attr_negative_slope_a);
  attr_negative_slope_a *= scale;
  CreateAndUpdateSalarInput<float>(graph, functpara, attr_negative_slope_a, ge::DT_FLOAT, new_nodes);
  return SUCCESS;
}

// lrelu+dequant/requant
Status FixPipeAddInputStrategy35::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy35 preact lrelu+derequant");
  float attr_negative_slope_a = 0.0;
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetSecondIndex()) ||
      !CheckIsInVector(match_pass.m_opnodes, functpara->GetFirstIndex())) {
    return FAILED;
  }
  auto second_node = match_pass.m_opnodes[functpara->GetSecondIndex()].GetNode();
  if (second_node == nullptr) {
    return FAILED;
  }
  (void)ge::AttrUtils::GetFloat(second_node->GetOpDesc(), ATTR_NEGATIVE_SLOPE, attr_negative_slope_a);
  CreateAndUpdateVectorMulScalarInput<float>(graph, functpara, match_pass.m_opnodes[functpara->GetFirstIndex()],
                                             attr_negative_slope_a, new_nodes);
  return SUCCESS;
}

// lrlu
Status FixPipeAddInputStrategy36::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy36 preactalone, lrelu");
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetFirstIndex())) {
    return FAILED;
  }
  auto first_node = match_pass.m_opnodes[functpara->GetFirstIndex()].GetNode();
  if (first_node == nullptr) {
    return FAILED;
  }
  if (first_node->GetOpDesc()->MutableOutputDesc(0) == nullptr) {
    return FAILED;
  }
  float attr_negative_slope_a = 0.0;
  (void)ge::AttrUtils::GetFloat(first_node->GetOpDesc(), ATTR_NEGATIVE_SLOPE, attr_negative_slope_a);
  FE_LOGD("slop_a = %f index = %u op type = %s", attr_negative_slope_a, functpara->GetFirstIndex(),
          first_node->GetType().c_str());
  CreateAndUpdateSalarInput<float>(graph, functpara, attr_negative_slope_a, ge::DT_FLOAT, new_nodes);
  return SUCCESS;
}

// cast + prelu
Status FixPipeAddInputStrategy37::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy37 preact, cast+prelu");
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetSecondIndex())) {
    return FAILED;
  }
  CloneVectorInput(match_pass.m_opnodes[functpara->GetSecondIndex()], functpara);
  return SUCCESS;
}

// cast + lrlu
Status FixPipeAddInputStrategy38::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy38 preactalone, cast+lrelu");
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetSecondIndex())) {
    return FAILED;
  }
  auto second_node = match_pass.m_opnodes[functpara->GetSecondIndex()].GetNode();
  if (second_node == nullptr) {
    return FAILED;
  }
  if (second_node->GetOpDesc()->MutableOutputDesc(0) == nullptr) {
    return FAILED;
  }
  float attr_negative_slope_a = 0.0;
  (void)ge::AttrUtils::GetFloat(second_node->GetOpDesc(), ATTR_NEGATIVE_SLOPE, attr_negative_slope_a);
  CreateAndUpdateSalarInput<float>(graph, functpara, attr_negative_slope_a, ge::DT_FLOAT, new_nodes);
  return SUCCESS;
}

// relu6
Status FixPipeAddInputStrategy41::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy41 clipvalue, relu6");
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetFirstIndex())) {
    return FAILED;
  }
  auto first_node = match_pass.m_opnodes[functpara->GetFirstIndex()].GetNode();
  if (first_node == nullptr) {
    return FAILED;
  }
  auto firstnode_outputdesc = first_node->GetOpDesc()->MutableOutputDesc(0);
  if (firstnode_outputdesc == nullptr) {
    return FAILED;
  }
  ge::DataType dst_datatype = firstnode_outputdesc->GetDataType();
  SetClipValue6(graph, functpara, dst_datatype, new_nodes);
  return SUCCESS;
}

// relu6+quant
Status FixPipeAddInputStrategy42::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy42 clipvalue relu6+ quant");
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetSecondIndex())) {
    return FAILED;
  }
  auto second_node = match_pass.m_opnodes[functpara->GetSecondIndex()].GetNode();
  if (second_node == nullptr) {
    return FAILED;
  }
  auto secondnode_outputdesc = second_node->GetOpDesc()->MutableOutputDesc(0);
  if (secondnode_outputdesc == nullptr) {
    return FAILED;
  }
  float scale = 0.0;
  float offset = 0.0;
  (void)ge::AttrUtils::GetFloat(second_node->GetOpDesc(), ATTR_SCALE, scale);
  (void)ge::AttrUtils::GetFloat(second_node->GetOpDesc(), ATTR_OFFSET, offset);
  ge::DataType dst_datatype = secondnode_outputdesc->GetDataType();
  SetClipValue6(graph, functpara, dst_datatype, new_nodes);
  return SUCCESS;
}

// relu6+dequant
Status FixPipeAddInputStrategy43::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy43 clipvalue relu6+dequant");
  float offset = 0.0;
  float scale = 0.0;
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetFirstIndex())) {
    return FAILED;
  }
  auto first_node = match_pass.m_opnodes[functpara->GetFirstIndex()].GetNode();
  if (first_node == nullptr) {
    return FAILED;
  }
  ge::OpDescPtr dequantdesc = first_node->GetOpDesc();
  if (dequantdesc == nullptr) {
    return FAILED;
  }
  auto dequantoutdesc = dequantdesc->MutableOutputDesc(0);
  if (dequantoutdesc == nullptr) {
    return FAILED;
  }
  ge::DataType dst_datatype = dequantoutdesc->GetDataType();
  if (ge::AttrUtils::GetFloat(dequantdesc, ATTR_OFFSET, offset) &&
      ge::AttrUtils::GetFloat(dequantdesc, ATTR_SCALE, scale)) {
    if (dst_datatype == ge::DT_FLOAT) {
      float relu6value = relu6_value * scale + offset;
      CreateAndUpdateSalarInput<float>(graph, functpara, relu6value, dst_datatype, new_nodes);
    } else if (dst_datatype == ge::DT_FLOAT16) {
      float value = relu6_value * scale + offset;
      uint16_t value_tmp = static_cast<uint16_t>(value);
      fe::fp16_t relu6value = static_cast<fe::fp16_t>(value_tmp);
      CreateAndUpdateSalarInput<fe::fp16_t>(graph, functpara, relu6value, dst_datatype, new_nodes);
    } else if (dst_datatype == ge::DT_INT8 || dst_datatype == ge::DT_INT4) {
      int16_t relu6value_tmp = static_cast<int16_t>(relu6_value * scale + offset);
      int8_t relu6value = 0;
      if (relu6value_tmp > MAXINT8_VALUE) {
        relu6value = MAXINT8_VALUE;
      } else {
        relu6value = static_cast<int8_t>(relu6value_tmp);
      }
      CreateAndUpdateSalarInput<int8_t>(graph, functpara, relu6value, dst_datatype, new_nodes);
    }
  } else {
    SetClipValue6(graph, functpara, dst_datatype, new_nodes);
  }
  return SUCCESS;
}

// cast+relu6
Status FixPipeAddInputStrategy44::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy44 clipvalue, cast+relu6");
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetSecondIndex())) {
    return FAILED;
  }
  auto second_node = match_pass.m_opnodes[functpara->GetSecondIndex()].GetNode();
  if (second_node == nullptr) {
    return FAILED;
  }
  auto second_outdesc = second_node->GetOpDesc()->MutableOutputDesc(0);
  if (second_outdesc == nullptr) {
    return 0;
  }
  ge::DataType dst_datatype = second_outdesc->GetDataType();
  SetClipValue6(graph, functpara, dst_datatype, new_nodes);
  return SUCCESS;
}

// quant
Status FixPipeAddInputStrategy51::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy51 post quant");
  float scale = 0.0;
  float offset = 0.0;
  auto first_node = GetQuantScaleOffset(match_pass, functpara->GetFirstIndex(), scale, offset);
  if (first_node == nullptr) {
    return FAILED;
  }
  uint64_t tmp_data = SetM3OfQuant(scale, offset, first_node);
  CreateAndUpdateSalarInput<uint64_t>(graph, functpara, tmp_data, ge::DT_UINT64, new_nodes);
  return SUCCESS;
}

// prelu+quant
Status FixPipeAddInputStrategy61::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy61 postact prlu+quant");
  float scale = 0.0;
  float offset = 0.0;
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetSecondIndex()) ||
      !CheckIsInVector(match_pass.m_opnodes, functpara->GetFirstIndex())) {
    return FAILED;
  }
  auto second_node = match_pass.m_opnodes[functpara->GetSecondIndex()].GetNode();
  if (second_node == nullptr) {
    return FAILED;
  }
  (void)ge::AttrUtils::GetFloat(second_node->GetOpDesc(), ATTR_SCALE, scale);
  (void)ge::AttrUtils::GetFloat(second_node->GetOpDesc(), ATTR_OFFSET, offset);
  CreateAndUpdateVectorMulScalarInput<float>(graph, functpara, match_pass.m_opnodes[functpara->GetFirstIndex()], scale,
                                             new_nodes);
  return SUCCESS;
}

// prelu
Status FixPipeAddInputStrategy62::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy62 postactalone, prelu");
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetFirstIndex())) {
    return FAILED;
  }
  CloneVectorInput(match_pass.m_opnodes[functpara->GetFirstIndex()], functpara);
  return SUCCESS;
}

// lrelu+quant
Status FixPipeAddInputStrategy63::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy63 postact lrelu+quant");
  float scale = 0.0;
  float offset = 0.0;
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetSecondIndex()) ||
      !CheckIsInVector(match_pass.m_opnodes, functpara->GetFirstIndex())) {
    return FAILED;
  }
  auto first_node = match_pass.m_opnodes[functpara->GetFirstIndex()].GetNode();
  if (first_node == nullptr) {
    return FAILED;
  }
  auto second_node = GetQuantScaleOffset(match_pass, functpara->GetSecondIndex(), scale, offset);
  if (second_node == nullptr) {
    return FAILED;
  }
  float attr_negative_slope_a = 0.0;
  (void)ge::AttrUtils::GetFloat(first_node->GetOpDesc(), ATTR_NEGATIVE_SLOPE, attr_negative_slope_a);
  attr_negative_slope_a *= scale;
  CreateAndUpdateSalarInput<float>(graph, functpara, attr_negative_slope_a, ge::DT_FLOAT, new_nodes);
  return SUCCESS;
}

// lrelu
Status FixPipeAddInputStrategy64::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy64 postactalone, lrelu");
  float attr_negative_slope_a = 0.0;
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetFirstIndex())) {
    return FAILED;
  }
  auto first_node = match_pass.m_opnodes[functpara->GetFirstIndex()].GetNode();
  if (first_node == nullptr) {
    return FAILED;
  }
  (void)ge::AttrUtils::GetFloat(first_node->GetOpDesc(), ATTR_NEGATIVE_SLOPE, attr_negative_slope_a);
  CreateAndUpdateSalarInput<float>(graph, functpara, attr_negative_slope_a, ge::DT_FLOAT, new_nodes);
  return SUCCESS;
}

// eltwise
Status FixPipeAddInputStrategy81::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy81 eltwise scale");
  float scale_tmp = 0.0;
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetFirstIndex())) {
    return FAILED;
  }
  auto first_node = match_pass.m_opnodes[functpara->GetFirstIndex()].GetNode();
  if (first_node == nullptr) {
    return FAILED;
  }
  (void)ge::AttrUtils::GetFloat(first_node->GetOpDesc(), ATTR_SCALE, scale_tmp);
  fe::fp16_t insert_value(scale_tmp);
  CreateAndUpdateSalarInput<fe::fp16_t>(graph, functpara, insert_value, ge::DT_FLOAT16, new_nodes);
  return SUCCESS;
}

// eltwise
Status FixPipeAddInputStrategy91::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                             const FixPipeFunctionParamPtr &functpara,
                                             std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategy91 eltwise offset");
  float offset_a = 0.0;
  if (!CheckIsInVector(match_pass.m_opnodes, functpara->GetFirstIndex())) {
    return FAILED;
  }
  auto first_node = match_pass.m_opnodes[functpara->GetFirstIndex()].GetNode();
  if (first_node == nullptr) {
    return FAILED;
  }
  (void)ge::AttrUtils::GetFloat(first_node->GetOpDesc(), ATTR_OFFSET, offset_a);
  if (functpara->GetDataType() == ge::DT_INT8 ||
      functpara->GetDataType() == ge::DT_INT4) {
    CreateAndUpdateSalarInput<int8_t>(graph, functpara, static_cast<int8_t>(offset_a),
                                      functpara->GetDataType(), new_nodes);
  } else {
    CreateAndUpdateSalarInput<float>(graph, functpara, offset_a, ge::DT_FLOAT, new_nodes);
  }
  return SUCCESS;
}

Status FixPipeAddInputStrategyDefault::DoAddInput(ge::ComputeGraph &graph, const FixPipePassInfo &match_pass,
                                                  const FixPipeFunctionParamPtr &functpara,
                                                  std::vector<ge::NodePtr> &new_nodes) const {
  FE_LOGD("FixPipeAddInputStrategyDefault");
  return SUCCESS;
}
}  // namespace fe
