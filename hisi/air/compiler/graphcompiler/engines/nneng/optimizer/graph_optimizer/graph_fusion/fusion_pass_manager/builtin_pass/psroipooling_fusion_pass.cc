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

#include "psroipooling_fusion_pass.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "common/fe_log.h"
#include "common/op_info_common.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/shape_format_transfer/transfer_shape_according_to_format.h"

using namespace ge;
namespace fe {
static const string PATTERN_PSROI = "psRoiPooling";
static const string PSROIPOOLING = "PSROIPooling";
static const string OUTPUT_DIM = "output_dim";
static const string GROUP_SIZE = "group_size";
static const size_t BIAS_INDEX = 2;

PSROIPoolingFusionPass::PSROIPoolingFusionPass() : output_dim_(-1), group_size_(-1) {}

PSROIPoolingFusionPass::~PSROIPoolingFusionPass() {}

vector<FusionPattern *> PSROIPoolingFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;
  FusionPattern *pattern = new (std::nothrow) FusionPattern("PSROIPoolingFusion");
  FE_CHECK(pattern == nullptr, REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][DefPtn] new an object failed."),
           return patterns);

  pattern->AddOpDesc(PATTERN_PSROI, {PSROIPOOLING}).SetOutput(PATTERN_PSROI);
  patterns.push_back(pattern);

  return patterns;
}

Status PSROIPoolingFusionPass::Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &new_nodes) {
  FE_LOGI("Enter PSROIPoolingFusionPass");
  ge::NodePtr psroi_node = GetNodeFromMapping(PATTERN_PSROI, mapping);
  FE_CHECK(psroi_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][DefPtn] Node PSROIPooling is null, fusion failed."),
           return PARAM_INVALID);

  if (CheckParameter(psroi_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][DefPtn] Check PSROIPooling param failed.");
    return PARAM_INVALID;
  }

  ge::NodePtr pre_node_ptr = psroi_node->GetInAllNodes().at(0);
  // if input node of psroipooling is conv2d,
  // and conv2d's out is only to psroipooling,
  // insert swapci before psroipooling. else,
  // insert swapco after conv2d's weight.
  if (pre_node_ptr->GetOutDataNodes().size() == 1 && pre_node_ptr->GetType() == CONV2D) {
    /*
           Data(input)
                \
                 \
                  v
      Const(filter)--->Conv2d--->PSROIPooling--->output
                 ^                   ^
                /                   /
               /                  rois
            Const(bias)
    */
    FE_LOGI("Input from conv and out data size is one, insert SwapCo.");
    return SwapCoFuison(graph, pre_node_ptr, psroi_node, new_nodes);
  } else {
    /*
      input--->PSROIPooling--->output
                 ^
                /
             rois
    */
    FE_LOGI(
        "Input not from conv or out data size greater than one,"
        " insert SwapCi.");
    return SwapCiFuison(graph, pre_node_ptr, psroi_node, new_nodes);
  }
}

Status PSROIPoolingFusionPass::CheckParameter(const ge::NodePtr &psroi_node_ptr) {
  // get psroipooling node inputs.
  Node::Vistor<NodePtr> in_nodes = psroi_node_ptr->GetInAllNodes();
  if (in_nodes.size() != 2) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][ChkParm] PSROIPooling input nodes num(%zu) != 2",
                    in_nodes.size());
    return PARAM_INVALID;
  }

  ge::OpDescPtr psroi_op_desc_ptr = psroi_node_ptr->GetOpDesc();
  FE_CHECK(psroi_op_desc_ptr == nullptr,
           REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][ChkParm] Node:%s's OpDesc is null, fusion failed.",
                           psroi_node_ptr->GetName().c_str()),
           return PARAM_INVALID);

  // get output_dim value and keep it to output_dim_
  int64_t output_dim = -1;
  if (!ge::AttrUtils::GetInt(psroi_op_desc_ptr, OUTPUT_DIM, output_dim)) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][ChkParm] Op[name=%s,type=%s]: get output_dim failed.",
                    psroi_op_desc_ptr->GetName().c_str(), psroi_op_desc_ptr->GetType().c_str());
    return FAILED;
  } else {
    output_dim_ = output_dim;
  }

  // get group_size value and keep it to group_size_
  int64_t group_size = -1;
  if (!ge::AttrUtils::GetInt(psroi_op_desc_ptr, GROUP_SIZE, group_size)) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][ChkParm] Op[name=%s,type=%s]: get group_size failed.",
                    psroi_op_desc_ptr->GetName().c_str(), psroi_op_desc_ptr->GetType().c_str());
    return FAILED;
  } else {
    group_size_ = group_size;
  }

  return SUCCESS;
}

Status PSROIPoolingFusionPass::SetAttrValueForNewNode(const ge::OpDescPtr &psroi_op_desc_ptr,
                                                      const ge::OpDescPtr &new_op_desc_ptr) const {
  // get and update output_dim
  ge::GeAttrValue output_dim_value;
  if (psroi_op_desc_ptr->GetAttr(OUTPUT_DIM, output_dim_value) == ge::GRAPH_FAILED) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SetAttrVal] Get attr %s from node %s error", OUTPUT_DIM.c_str(),
                    psroi_op_desc_ptr->GetName().c_str());
    return GRAPH_REPLACE_UPDATE_ATTR_FAILED;
  }

  if (new_op_desc_ptr->SetAttr(OUTPUT_DIM, output_dim_value) == ge::GRAPH_FAILED) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SetAttrVal] Set attr %s to node %s error", OUTPUT_DIM.c_str(),
                    new_op_desc_ptr->GetName().c_str());
    return GRAPH_REPLACE_UPDATE_ATTR_FAILED;
  }
  // get and update group_size
  ge::GeAttrValue group_size_value;
  if (psroi_op_desc_ptr->GetAttr(GROUP_SIZE, group_size_value) == ge::GRAPH_FAILED) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SetAttrVal] Get attr %s from node %s error", GROUP_SIZE.c_str(),
                    psroi_op_desc_ptr->GetName().c_str());
    return GRAPH_REPLACE_UPDATE_ATTR_FAILED;
  }

  if (new_op_desc_ptr->SetAttr(GROUP_SIZE, group_size_value) == ge::GRAPH_FAILED) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SetAttrVal] Set attr %s to node %s error", GROUP_SIZE.c_str(),
                    new_op_desc_ptr->GetName().c_str());
    return GRAPH_REPLACE_UPDATE_ATTR_FAILED;
  }

  return SUCCESS;
}

Status PSROIPoolingFusionPass::SwapCiFuison(ge::ComputeGraph &graph, const ge::NodePtr &pre_node_ptr,
                                            const ge::NodePtr &psroi_node_ptr,
                                            vector<ge::NodePtr> &new_nodes) {
  // get previous op desc of psroi node
  ge::OpDescPtr pre_op_desc_ptr = pre_node_ptr->GetOpDesc();
  FE_CHECK(pre_op_desc_ptr == nullptr,
           REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCiFus] Node:%s's OpDesc is null, fusion failed.",
                           pre_node_ptr->GetName().c_str()),
           return PARAM_INVALID);

  // Get psroi op desc
  ge::OpDescPtr psroi_op_desc_ptr = psroi_node_ptr->GetOpDesc();

  // create swapci opdesc
  ge::OpDescPtr swap_ci_ptr = nullptr;
  FE_MAKE_SHARED(swap_ci_ptr = std::make_shared<ge::OpDesc>(psroi_op_desc_ptr->GetName(), SWAPCI), return FAILED);

  swap_ci_ptr->SetType(SWAPCI);
  swap_ci_ptr->SetName(swap_ci_ptr->GetName() + "_SwapCi");
  // update output_dim and group_size
  if (SetAttrValueForNewNode(psroi_op_desc_ptr, swap_ci_ptr) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCiFus] Update output_dim and group_size failed.");
    return FAILED;
  }

  // get input desc info of swapci
  ge::GeTensorDesc swap_ci_input_tensor_desc;
  if (GetSwapInputTensorDesc(pre_op_desc_ptr, swap_ci_input_tensor_desc) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCiFus] Create SwapCi input op_desc failed, fusion failed.");
    return FAILED;
  }

  // get output desc info of swapci
  ge::GeTensorDesc swap_ci_output_tensor_desc;
  if (GetSwapCiOutputTensorDesc(psroi_op_desc_ptr, swap_ci_input_tensor_desc,
                                swap_ci_output_tensor_desc) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCiFus] Create SwapCi output op_desc failed, fusion failed.");
    return FAILED;
  }

  // add input and output desc to swapci
  swap_ci_ptr->AddInputDesc("SwapCiInput", swap_ci_input_tensor_desc);
  swap_ci_ptr->AddOutputDesc("SwapCiOutput", swap_ci_output_tensor_desc);

  // update input of psroi
  if (UpdatePSROIInput(psroi_op_desc_ptr, swap_ci_output_tensor_desc) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCiFus] Update PSROI input op_desc failed, fusion failed.");
    return FAILED;
  }

  // update output of psroi
  if (UpdatePSROIOutput(psroi_op_desc_ptr) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCiFus] Update PSROI output op_desc failed, fusion failed.");
    return FAILED;
  }

  // add swapci node to graph
  ge::NodePtr swap_ci_node_ptr = graph.AddNode(swap_ci_ptr);
  FE_CHECK(swap_ci_node_ptr == nullptr,
           REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCiFus] fusionNode: swap_ci_node_ptr is null, fusion failed."),
           return FAILED);
  new_nodes.push_back(swap_ci_node_ptr);
  // delete edge of prenode and psroinode
  if (ge::GraphUtils::RemoveEdge(pre_node_ptr->GetOutDataAnchor(0), psroi_node_ptr->GetInDataAnchor(0)) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCiFus] Remove input edge from fused node:%s.",
                    psroi_node_ptr->GetName().c_str());
    return FAILED;
  }

  // add the original edge of prenode to swapci
  if (ge::GraphUtils::AddEdge(pre_node_ptr->GetOutDataAnchor(0), swap_ci_node_ptr->GetInDataAnchor(0)) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCiFus] Add edge from fused node:%s to fusion node:%s failed.",
                    psroi_node_ptr->GetName().c_str(), psroi_node_ptr->GetName().c_str());
    return FAILED;
  }

  // add the output of swapci edge to psroi
  if (ge::GraphUtils::AddEdge(swap_ci_node_ptr->GetOutDataAnchor(0), psroi_node_ptr->GetInDataAnchor(0)) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCiFus] Add edge from node:%s to node:%s failed.",
                    psroi_node_ptr->GetName().c_str(), psroi_node_ptr->GetName().c_str());
    return FAILED;
  }

  return SUCCESS;
}

Status PSROIPoolingFusionPass::SwapCoFuison(ge::ComputeGraph &graph, const ge::NodePtr &conv_node_ptr,
                                            const ge::NodePtr &psroi_node_ptr,
                                            vector<ge::NodePtr> &new_nodes) {
  // check conv op desc is null or not
  ge::OpDescPtr conv_op_desc_ptr = conv_node_ptr->GetOpDesc();
  FE_CHECK(conv_op_desc_ptr == nullptr,
           REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCoFus] Node:%s's OpDesc is null, fusion failed.",
                           conv_node_ptr->GetName().c_str()),
           return PARAM_INVALID);

  ge::OpDescPtr psroi_op_desc_ptr = psroi_node_ptr->GetOpDesc();
  FE_CHECK(psroi_op_desc_ptr == nullptr,
           REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCoFus] Node:%s's OpDesc is null, fusion failed.",
                           psroi_node_ptr->GetName().c_str()),
           return PARAM_INVALID);

  // get conv node inputs.
  Node::Vistor<NodePtr> input_nodes = conv_node_ptr->GetInAllNodes();
  if (input_nodes.size() < 2) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCoFus] Convolution input nodes num(%zu) is less than 2",
                    input_nodes.size());
    return PARAM_INVALID;
  }

  std::vector<ge::NodePtr> weight_nodes;
  ge::NodePtr weight_node_ptr = input_nodes.at(1);
  weight_nodes.emplace_back(weight_node_ptr);
  if (input_nodes.size() > BIAS_INDEX) {
    ge::NodePtr bias_node_ptr = input_nodes.at(BIAS_INDEX);
    weight_nodes.emplace_back(bias_node_ptr);
  }

  for (size_t i = 0; i < weight_nodes.size(); i++) {
    // create swapco opdesc
    auto node_ptr = weight_nodes.at(i);
    uint32_t weight_index = i + 1;
    ge::OpDescPtr swap_co_op_desc_ptr = nullptr;
    FE_MAKE_SHARED(swap_co_op_desc_ptr = std::make_shared<ge::OpDesc>(psroi_op_desc_ptr->GetName(), SWAPCO),
                   return FAILED);
    swap_co_op_desc_ptr->SetType(SWAPCO);
    swap_co_op_desc_ptr->SetName(swap_co_op_desc_ptr->GetName() + "_SwapCo" + std::to_string(weight_index));
    // update output_dim and group_size
    if (SetAttrValueForNewNode(psroi_op_desc_ptr, swap_co_op_desc_ptr) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCoFus] Update output_dim and group_size failed.");
      return FAILED;
    }

    // get swapco input
    ge::GeTensorDesc swap_co_input_tensor_desc;
    if (GetSwapInputTensorDesc(node_ptr->GetOpDesc(), swap_co_input_tensor_desc) !=
        SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCoFus] Create SwapCo input op_desc failed, fusion failed.");
      return FAILED;
    }

    // get swapco output
    ge::GeTensorDesc swap_co_output_tensor_desc;
    if (GetSwapCoOutputTensorDesc(conv_op_desc_ptr, swap_co_input_tensor_desc, weight_index,
                                  swap_co_output_tensor_desc) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCoFus] Create SwapCo output op_desc failed, fusion failed.");
      return FAILED;
    }

    // update weight and output of conv2d
    if (UpdateConv2DWeightAndOut(conv_op_desc_ptr, weight_index, swap_co_output_tensor_desc) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCoFus] Update conv2d weight op_desc failed, fusion failed.");
      return FAILED;
    }

    // update input of psroi
    if (UpdatePSROIInput(psroi_op_desc_ptr, conv_op_desc_ptr->GetOutputDesc(0)) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCoFus] Update PSROI input op_desc failed, fusion failed.");
      return FAILED;
    }

    // update output origin shape of pad
    swap_co_op_desc_ptr->AddInputDesc("x", swap_co_input_tensor_desc);
    swap_co_op_desc_ptr->AddOutputDesc("y", swap_co_output_tensor_desc);

    // add SwapCo node to graph
    ge::NodePtr swap_co_node_ptr = graph.AddNode(swap_co_op_desc_ptr);
    FE_CHECK(swap_co_node_ptr == nullptr,
             REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCoFus] fusionNode: \
             swap_ci_node_ptr is null, fusion failed."), return FAILED);
    new_nodes.push_back(swap_co_node_ptr);
    // delete edge of prenode and psroinode
    if (ge::GraphUtils::RemoveEdge(node_ptr->GetOutAnchor(0), conv_node_ptr->GetInDataAnchor(weight_index)) !=
        SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCoFus] Remove input edge from fused node:%s.",
                      conv_node_ptr->GetName().c_str());
      return FAILED;
    }

    // add the original edge of prenode to swapci
    if (ge::GraphUtils::AddEdge(node_ptr->GetOutAnchor(0), swap_co_node_ptr->GetInDataAnchor(0)) != SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCoFus] Add edge from fused node:%s to fusion node:%s failed.",
                      psroi_node_ptr->GetName().c_str(), psroi_node_ptr->GetName().c_str());
      return FAILED;
    }

    // add the output of swapco edge to psroi
    if (ge::GraphUtils::AddEdge(swap_co_node_ptr->GetOutDataAnchor(0), conv_node_ptr->GetInDataAnchor(weight_index)) !=
        SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][SwapCoFus] Add edge from node:%s to node:%s failed.",
                      conv_node_ptr->GetName().c_str(), conv_node_ptr->GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

Status PSROIPoolingFusionPass::GetSwapInputTensorDesc(const ge::OpDescPtr &pre_op_desc_ptr,
                                                      ge::GeTensorDesc &input_tensor_desc) const {
  // get output desc of pre node to be input of current node
  int swap_ci_input_size = pre_op_desc_ptr->GetOutputsSize();
  if (swap_ci_input_size < 1) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][GetSwapInTensDesc] The output of %s is zero",
                    pre_op_desc_ptr->GetName().c_str());
    return FAILED;
  }
  input_tensor_desc = pre_op_desc_ptr->GetOutputDesc(0);
  return SUCCESS;
}

Status PSROIPoolingFusionPass::GetSwapCiOutputTensorDesc(const ge::OpDescPtr &next_op_desc_ptr,
                                                         const ge::GeTensorDesc &input_tensor_desc,
                                                         ge::GeTensorDesc &output_tensor_desc) {
  // get input desc of next node to be output of current node
  int swap_ci_out_size = next_op_desc_ptr->GetInputsSize();
  if (swap_ci_out_size < 1) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][GetSwapInTensDesc] The input of %s is zero",
                    next_op_desc_ptr->GetName().c_str());
    return FAILED;
  }
  output_tensor_desc = next_op_desc_ptr->GetInputDesc(0);
  // get new shape after ceiling(output_dim, 16)
  auto input_primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(input_tensor_desc.GetFormat()));
  auto output_primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(output_tensor_desc.GetFormat()));
  ge::GeShape swap_ci_output_shape =
      GetNewShapeFolding(input_primary_format, input_tensor_desc.GetShape(), output_dim_, NCHW_DIM_C);
  // get new shape from NCHW to NC1HWC0
  ge::GeShape new5_hd_shape = GetInOrOutputNewShape(input_primary_format, swap_ci_output_shape, output_primary_format,
                                                    output_tensor_desc.GetDataType());

  output_tensor_desc.SetShape(new5_hd_shape);
  output_tensor_desc.SetOriginFormat(input_primary_format);
  output_tensor_desc.SetOriginShape(swap_ci_output_shape);

  return SUCCESS;
}

Status PSROIPoolingFusionPass::GetSwapCoOutputTensorDesc(const ge::OpDescPtr &next_op_desc_ptr,
                                                         const ge::GeTensorDesc &input_tensor_desc,
                                                         const uint32_t &weight_index,
                                                         ge::GeTensorDesc &output_tensor_desc) {
  // get input desc of next node to be output of current node
  int swap_co_out_size = next_op_desc_ptr->GetInputsSize();
  if (swap_co_out_size < 2) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][GetSwapCoOutTensDesc] The input of %s is zero",
                    next_op_desc_ptr->GetName().c_str());
    return FAILED;
  }
  output_tensor_desc = next_op_desc_ptr->GetInputDesc(weight_index);
  // get new shape after ceiling(output_dim, 16)
  auto input_primary_format = static_cast<ge::Format>(ge::GetPrimaryFormat(input_tensor_desc.GetFormat()));
  ge::GeShape swap_ci_output_shape =
      GetNewShapeFolding(input_primary_format, input_tensor_desc.GetShape(), output_dim_, NCHW_DIM_N);

  output_tensor_desc.SetShape(swap_ci_output_shape);
  output_tensor_desc.SetOriginShape(swap_ci_output_shape);
  output_tensor_desc.SetFormat(input_tensor_desc.GetFormat());

  return SUCCESS;
}

Status PSROIPoolingFusionPass::UpdatePSROIInput(const ge::OpDescPtr &psroi_op_desc_ptr,
                                                const ge::GeTensorDesc &swap_output_tensor_desc) const {
  // shape of psroi, it's shape is NC1HWC0
  ge::GeShape new5_hd_shape = swap_output_tensor_desc.GetShape();
  ge::GeShape swap_output_shape = swap_output_tensor_desc.GetOriginShape();

  ge::GeTensorDesc psroi_input_tensor_desc = psroi_op_desc_ptr->GetInputDesc(0);
  psroi_input_tensor_desc.SetShape(new5_hd_shape);
  psroi_input_tensor_desc.SetOriginShape(swap_output_shape);

  if (psroi_op_desc_ptr->UpdateInputDesc(0, psroi_input_tensor_desc) != GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][UpdPSROIIn] Update input desc of node:%s to fusion node:%s failed.",
                    psroi_op_desc_ptr->GetName().c_str(), psroi_op_desc_ptr->GetName().c_str());
    return FAILED;
  }

  return SUCCESS;
}

Status PSROIPoolingFusionPass::UpdatePSROIOutput(ge::OpDescPtr &psroi_op_desc_ptr) {
  ge::GeTensorDesc psroi_output_tensor_desc = psroi_op_desc_ptr->GetOutputDesc(0);
  ge::GeShape &new_shape = psroi_output_tensor_desc.MutableShape();
  new_shape.SetDim(NCHW_DIM_C, GetMixAliquotsNum(output_dim_, SHAPE_NUMBER_16));

  if (psroi_op_desc_ptr->UpdateOutputDesc(0, psroi_output_tensor_desc) != GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][UpdPSROIOut] Update output desc of node:%s to fusion node:%s failed.",
                    psroi_op_desc_ptr->GetName().c_str(), psroi_op_desc_ptr->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status PSROIPoolingFusionPass::UpdateConv2DWeightAndOut(const ge::OpDescPtr &conv_op_desc_ptr,
                                                        const uint32_t &weight_index,
                                                        const ge::GeTensorDesc &swap_co_output_tensor_desc) const {
  ge::GeShape swap_co_output_shape = swap_co_output_tensor_desc.GetShape();

  // get weight of conv
  ge::GeTensorDesc conv_weight_tensor_desc = conv_op_desc_ptr->GetInputDesc(weight_index);
  // set weight of conv input desc, its shape is NCHW to FRACTAL_Z
  conv_weight_tensor_desc.SetOriginShape(swap_co_output_shape);
  ge::GeShape new_fz_shape =
      GetInOrOutputNewShape(conv_weight_tensor_desc.GetOriginFormat(), swap_co_output_shape,
                            static_cast<ge::Format>(ge::GetPrimaryFormat(conv_weight_tensor_desc.GetFormat())),
                            swap_co_output_tensor_desc.GetDataType());
  conv_weight_tensor_desc.SetShape(new_fz_shape);

  if (conv_op_desc_ptr->UpdateInputDesc(weight_index, conv_weight_tensor_desc) != GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][UpdConv2DWgtOut] Update weight desc of node:%s to fusion node:%s failed.",
                    conv_op_desc_ptr->GetName().c_str(), conv_op_desc_ptr->GetName().c_str());
    return FAILED;
  }

  ge::GeTensorDesc conv_output_tensor_desc = conv_op_desc_ptr->GetOutputDesc(0);
  ge::GeShape conv_output_ori_shape = conv_output_tensor_desc.GetOriginShape();
  // get new ori shape and update. it't C value is value of N of SwapCo
  auto swap_co_out_primary_format =
      static_cast<ge::Format>(ge::GetPrimaryFormat(swap_co_output_tensor_desc.GetFormat()));
  if (swap_co_out_primary_format == ge::FORMAT_NCHW) {
    conv_output_ori_shape.SetDim(NCHW_DIM_C, swap_co_output_shape.GetDim(NCHW_DIM_N));
  } else if (swap_co_out_primary_format == ge::FORMAT_HWCN) {
    conv_output_ori_shape.SetDim(HWCN_DIM_C, swap_co_output_shape.GetDim(HWCN_DIM_N));
  } else if (swap_co_out_primary_format == ge::FORMAT_NHWC) {
    conv_output_ori_shape.SetDim(NHWC_DIM_C, swap_co_output_shape.GetDim(NHWC_DIM_N));
  } else if (swap_co_out_primary_format == ge::FORMAT_CHWN) {
    conv_output_ori_shape.SetDim(CHWN_DIM_C, swap_co_output_shape.GetDim(CHWN_DIM_N));
  }

  // get new shape and update. NCHW to NC1HWC0
  ge::GeShape new5_d_shape =
      GetInOrOutputNewShape(swap_co_out_primary_format, conv_output_ori_shape,
                            static_cast<ge::Format>(GetPrimaryFormat(conv_output_tensor_desc.GetFormat())),
                            conv_output_tensor_desc.GetDataType());

  conv_output_tensor_desc.SetShape(new5_d_shape);
  conv_output_tensor_desc.SetOriginShape(conv_output_ori_shape);

  if (conv_op_desc_ptr->UpdateOutputDesc(0, conv_output_tensor_desc) != GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][PSROIPoolFus][UpdConv2DWgtOut] Update output desc of node:%s to fusion node:%s failed.",
                    conv_op_desc_ptr->GetName().c_str(), conv_op_desc_ptr->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

ge::GeShape PSROIPoolingFusionPass::GetInOrOutputNewShape(const ge::Format &old_format,
                                                          const ge::GeShape &old_shape,
                                                          const ge::Format &new_format,
                                                          const ge::DataType &data_type) const {
  int64_t op_imply_type = EN_IMPL_HW_TBE;
  ge::GeShape new_shape;
  ShapeAndFormat shape_and_format_info = {old_shape, new_shape,     old_format,          new_format,
                                          data_type, op_imply_type, GROUPS_DEFAULT_VALUE};

  Status ret = ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(shape_and_format_info);
  if (ret != SUCCESS) {
    FE_LOGW(
        "Old format is %u, new format is %u, old dimension is %zu and "
        "opImplyType is %ld.",
        old_format, new_format, old_shape.GetDimNum(), op_imply_type);
    return old_shape;
  }
  return new_shape;
}

ge::GeShape PSROIPoolingFusionPass::GetNewShapeFolding(const ge::Format &old_format,
                                                       const ge::GeShape &old_shape,
                                                       const int64_t &output_dim,
                                                       const int32_t &fold_axis) const {
  if (old_format != ge::FORMAT_NCHW) {
    return old_shape;
  }
  if (output_dim == 0) {
    return old_shape;
  }

  int64_t c_dims = old_shape.GetDim(fold_axis);
  int64_t align16_num = GetMixAliquotsNum(output_dim, SHAPE_NUMBER_16);
  int64_t new_c_dims = (output_dim * align16_num != 0) ? c_dims / output_dim * align16_num : 0;
  ge::GeShape new_nchw_shape = old_shape;
  new_nchw_shape.SetDim(fold_axis, new_c_dims);

  return new_nchw_shape;
}

int64_t PSROIPoolingFusionPass::GetMixAliquotsNum(const int64_t &base_num, const int32_t &multiple) const {
  if (multiple == 0) {
    return 0;
  }

  return (base_num + multiple - 1) / multiple * multiple;
}
}  // namespace fe
