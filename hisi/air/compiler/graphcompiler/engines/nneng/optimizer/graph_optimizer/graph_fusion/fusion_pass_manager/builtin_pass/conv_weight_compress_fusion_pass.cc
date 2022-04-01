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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/conv_weight_compress_fusion_pass.h"
#include <algorithm>
#include <vector>
#include "common/configuration.h"
#include "common/fe_inner_attr_define.h"
#include "common/fe_utils.h"
#include "common/unknown_shape_util.h"
#include "common/util/op_info_util.h"
#include "common/util/platform_info.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/shape_format_transfer/transfer_shape_according_to_format.h"

/**
 *            weight                weight -> weight_compress
 *            /                          \   /
 *          conv        ->              switch
 *           /                          /    \
 *         relu                      Conv   ConvCompress
 *                                      \   /
 *                                      Merge
 *                                        /
 *                                       relu
 */
namespace fe {
static const string PATTERN_CONV = "conv_pattern";
static const std::map<std::string, std::string> CONV_COMPRESS_OP_TYPE_MAP {
    {CONV2D, CONV2D_COMPRESS}, {FCOP, FC_COMPRESS}, {MATMULV2OP, MATMULV2_COMPRESS}};
static const std::string HOST_OP_TYPE = "WeightCompressHost";
static const std::string SWITCH_OP_TYPE = "Switch";
static const std::string MERGE_OP_TYPE = "Merge";
static const std::string TENSOR_NAME_FILTER_COMPRESS = "filter_compress";
static const std::string TENSOR_NAME_COMPRESS_INDEX = "compress_index";

static const std::vector<std::string> HOST_OP_TYPE_VEC{"GroupPadding",
                                                       "ConvBnFilterHost",
                                                       "ConvScaleFilterHost",
                                                       "Concatv2HostCpuOp",
                                                       "RequantHostCpuOp",
                                                       "QuantWeightRollBack",
                                                       "GatherV2",
                                                       "GatherV2D",
                                                       "SwapCo",
                                                       "ReverseV2D",
                                                       "ConcatV2",
                                                       "TransData",
                                                       "Cast",
                                                       "Reshape",
                                                       "Maximum",
                                                       "Add",
                                                       "Mul",
                                                       "Sub",
                                                       "AscendWeightQuant"};

const uint64_t kRecursiveLoopMax = 10000000;
uint64_t kFeRecursiveCnt = 0;
const float kDefaultCompressRatioThreshold = 0.8;
vector<FusionPattern *> ConvWeightCompressFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;
  FusionPattern *pattern = new (std::nothrow) FusionPattern("ConvWeightCompressFusion");
  FE_CHECK(pattern == nullptr, REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][DefPtn] Fail to new an object."),
           return patterns);

  // conv2d / FC / MatMulV2
  pattern->AddOpDesc(PATTERN_CONV, {CONV2D, FCOP, MATMULV2OP}).SetOutput(PATTERN_CONV);
  patterns.push_back(pattern);

  return patterns;
}

Status ConvWeightCompressFusionPass::Fusion(ge::ComputeGraph &graph, Mapping &mapping,
                                            vector<ge::NodePtr> &fusion_nodes) {
  ge::NodePtr conv_node = GetNodeFromMapping(PATTERN_CONV, mapping);
  FE_CHECK(conv_node == nullptr, REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][Fusion] Conv node is nullptr."),
           return FAILED);

  std::string conv_name = conv_node->GetName();
  if (conv_node->GetOpDesc()->GetInputsSize() < 2) {
    REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][Fusion] The size of input desc of node[%s] is less than 2.",
                    conv_name.c_str());
    return FAILED;
  }

  // avoid the function loss of conv op
  ge::DataType weight_data_type = conv_node->GetOpDesc()->GetInputDescPtr(0)->GetDataType();
  if (weight_data_type != ge::DT_INT8 && weight_data_type != ge::DT_UINT8) {
    FE_LOGD("The weight data type of node[%s] is not int8 or uint8.", conv_name.c_str());
    return NOT_CHANGED;
  }

  // check whether this op is supported by ai core
  ge::OpDescPtr conv_op_desc = conv_node->GetOpDesc();
  if (!CheckOpSupported(conv_op_desc)) {
    FE_LOGI("Op[%s, %s] is not supported by AI Core.", conv_op_desc->GetName().c_str(),
            conv_op_desc->GetType().c_str());
    return NOT_CHANGED;
  }

  // if is unknown shape op, do not do weight compress.
  if (IsFeSupportedDynamicOp(*conv_node->GetOpDesc(), true)) {
    FE_LOGD("The node[%s] is unknown shape op, does not need to be compressed.", conv_name.c_str());
    return NOT_CHANGED;
  }

  // check whether do compress this conv node
  if (!IsCompressWeight(conv_node)) {
    FE_LOGD("Node[%s] does not need to be compressed.", conv_name.c_str());
    return NOT_CHANGED;
  }

  // check all the node between conv and weight node can be fold
  if (!CheckWeightConstFoldNode(conv_node)) {
    FE_LOGD(
        "There is some node between Node[%s, %s] and weight node"
        "which can not be fold.",
        conv_name.c_str(), conv_node->GetType().c_str());
    return NOT_CHANGED;
  }

  FE_LOGD("Begin to do weight compress for node[%s, %s].", conv_node->GetName().c_str(), conv_node->GetType().c_str());

  ge::OpDescPtr conv_compress_op_desc = ge::AttrUtils::CopyOpDesc(conv_node->GetOpDesc());

  // modify op type of conv node
  auto iter = CONV_COMPRESS_OP_TYPE_MAP.find(conv_node->GetType());
  if (iter == CONV_COMPRESS_OP_TYPE_MAP.end()) {
    FE_LOGD("Can not find conv compress op type by op type[%s].", conv_node->GetType().c_str());
    return NOT_CHANGED;
  }
  conv_compress_op_desc->SetType(iter->second);

  if (CreateConvCompressOpDesc(conv_op_desc, conv_compress_op_desc) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][Fusion] Fail to create compress type op of node[%s, %s].",
                    conv_op_desc->GetName().c_str(), conv_op_desc->GetType().c_str());
    return FAILED;
  }
  // check whether this op is supported by ai core
  if (!CheckOpSupported(conv_compress_op_desc)) {
    FE_LOGI("Op[%s, %s] is not supported by AI Core.", conv_compress_op_desc->GetName().c_str(),
            conv_compress_op_desc->GetType().c_str());
    return NOT_CHANGED;
  }

  // add node to graph
  ge::NodePtr conv_compress_node = graph.AddNode(conv_compress_op_desc);
  FE_CHECK(conv_compress_node == nullptr,
           REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][Fusion] Fail to add op[%s, %s] to graph.",
                           conv_compress_op_desc->GetName().c_str(), conv_compress_op_desc->GetType().c_str()),
           return FAILED);

  // add host op
  ge::NodePtr host_node = CreateHostNode(conv_op_desc, graph);
  FE_CHECK(host_node == nullptr, REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][Fusion] Fail to add weight compress host \
           node to graph."),
           return FAILED);
  // add switch op
  ge::NodePtr switch_node = CreateSwitchNode(conv_op_desc, graph);
  FE_CHECK(switch_node == nullptr, REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][Fusion] Fail to add switch node to \
           graph."),
           return FAILED);
  // add merge op
  ge::NodePtr merge_node = CreateMergeNode(conv_op_desc, graph);
  FE_CHECK(merge_node == nullptr, REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][Fusion] Fail to add merge node to \
           graph."),
           return FAILED);

  if (RelinkNodeEdges(conv_node, conv_compress_node, host_node, switch_node, merge_node) != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][Fusion] Fail to link edges around node[%s, %s].",
                    conv_node->GetName().c_str(), conv_node->GetType().c_str());
    return FAILED;
  }
  SetHostNodeCompressRatio(host_node);
  FE_LOGD("Finish converting to compress type of node[%s].", conv_compress_node->GetName().c_str());
  return SUCCESS;
}

void ConvWeightCompressFusionPass::SetHostNodeCompressRatio(ge::NodePtr &node_ptr) const {
  std::string soc_version = Configuration::Instance(AI_CORE_NAME).GetSocVersion();
  PlatformInfo platform_info;
  OptionalInfo optional_info;
  if (PlatformInfoManager::Instance().GetPlatformInfo(soc_version, platform_info, optional_info) != SUCCESS) {
    FE_LOGW("Fail to get platform info by soc version[%s].", soc_version.c_str());
    return;
  }
  int32_t core_num = static_cast<int32_t>(platform_info.soc_info.ai_core_cnt);
  float compress_ratio = kDefaultCompressRatioThreshold;
  auto compress_ratios = Configuration::Instance(AI_CORE_NAME).GetCompressRatio();
  if (compress_ratios.find(core_num) != compress_ratios.end()) {
    compress_ratio = compress_ratios[core_num];
  }

  ge::OpDescPtr op_desc = node_ptr->GetOpDesc();
  (void)ge::AttrUtils::SetFloat(op_desc, "compress_ratio_threshold", compress_ratio);
  return;
}

Status RelinkControlEdges(ge::NodePtr conv_node, ge::NodePtr conv_compress_node) {
  // connect in control anchor
  if (conv_node->GetInControlAnchor() != nullptr) {
    if (!conv_node->GetInControlAnchor()->GetPeerOutControlAnchors().empty() &&
        conv_compress_node->GetInControlAnchor() != nullptr) {
      for (const ge::OutControlAnchorPtr &out_ctrl_anchor_ptr :
           conv_node->GetInControlAnchor()->GetPeerOutControlAnchors()) {
        if (ge::GraphUtils::AddEdge(out_ctrl_anchor_ptr, conv_compress_node->GetInControlAnchor()) !=
            ge::GRAPH_SUCCESS) {
          REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][RelkCtrlEdge] Fail to add input control edge for node[%s].",
                          conv_compress_node->GetName().c_str());
          return FAILED;
        }
      }
    }
  }
  // connect out control anchor
  if (conv_node->GetOutControlAnchor() != nullptr) {
    if (!conv_node->GetOutControlAnchor()->GetPeerInControlAnchors().empty() &&
        conv_compress_node->GetOutControlAnchor() != nullptr) {
      for (const ge::InControlAnchorPtr &in_ctrl_anchor_ptr :
           conv_node->GetOutControlAnchor()->GetPeerInControlAnchors()) {
        if (ge::GraphUtils::AddEdge(conv_compress_node->GetOutControlAnchor(), in_ctrl_anchor_ptr) !=
            ge::GRAPH_SUCCESS) {
          REPORT_FE_ERROR(
              "[GraphOpt][ConvWgtCmpsFus][RelkCtrlEdge] Fail to add output control edge for fusion node[%s].",
              conv_compress_node->GetName().c_str());
          return FAILED;
        }
      }
    }
  }
  return SUCCESS;
}

Status RelinkDataEdgesOfMergeNode(ge::NodePtr conv_node, ge::NodePtr conv_compress_node, ge::NodePtr merge_node) {
  auto conv_out_peer_data_anchors = conv_node->GetOutDataAnchor(0)->GetPeerInDataAnchors();
  conv_node->GetOutDataAnchor(0)->UnlinkAll();

  // link the input anchor of merge node
  if (ge::GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0), merge_node->GetInDataAnchor(0)) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOpt][ConvWgtCmpsFus][RelkDataEdge] Fail to add edge between conv node[%s] and merge node[%s].",
        conv_node->GetName().c_str(), merge_node->GetName().c_str());
    return FAILED;
  }
  if (ge::GraphUtils::AddEdge(conv_compress_node->GetOutDataAnchor(0), merge_node->GetInDataAnchor(1)) !=
      ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOpt][ConvWgtCmpsFus][RelkDataEdge] Fail to add edge between conv compress node[%s] and merge node[%s].",
        conv_compress_node->GetName().c_str(), merge_node->GetName().c_str());
    return FAILED;
  }

  // link the output anchor of merge node
  for (const auto &peer_in_data_anchor : conv_out_peer_data_anchors) {
    if (ge::GraphUtils::AddEdge(merge_node->GetOutDataAnchor(0), peer_in_data_anchor) != ge::GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][RelkDataEdge] Fail to add edge for output anchor of merge node[%s].",
                      merge_node->GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

Status ConvWeightCompressFusionPass::RelinkNodeEdges(ge::NodePtr conv_node, ge::NodePtr conv_compress_node,
                                                     ge::NodePtr host_node, ge::NodePtr switch_node,
                                                     ge::NodePtr merge_node) const {
  // unlink the edge of conv's weight input
  ge::OutDataAnchorPtr conv_weight_output_anchor = conv_node->GetInDataAnchor(1)->GetPeerOutAnchor();
  if (ge::GraphUtils::RemoveEdge(conv_weight_output_anchor, conv_node->GetInDataAnchor(1)) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][RelkNdEdge] Fail to remove edge of conv node[%s]'s 1st input anchor.",
                    conv_node->GetName().c_str());
    return FAILED;
  }
  // link the input of host node
  if (ge::GraphUtils::AddEdge(conv_weight_output_anchor, host_node->GetInDataAnchor(0)) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][RelkNdEdge] Fail to add edge for host node[%s]'s 1st indata anchor.",
                    host_node->GetName().c_str());
    return FAILED;
  }

  // link the input of switch node with weight and host node
  if (ge::GraphUtils::AddEdge(conv_weight_output_anchor, switch_node->GetInDataAnchor(0)) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][RelkNdEdge] Fail to add edge for switch node[%s]'s 1st indata anchor.",
                    switch_node->GetName().c_str());
    return FAILED;
  }
  if (ge::GraphUtils::AddEdge(host_node->GetOutDataAnchor(0), switch_node->GetInDataAnchor(1)) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][RelkNdEdge] Fail to add edge between host node[%s] and switch node[%s]",
                    host_node->GetName().c_str(), switch_node->GetName().c_str());
    return FAILED;
  }

  // link the output of switch node with two conv node
  if (ge::GraphUtils::AddEdge(switch_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1)) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOpt][ConvWgtCmpsFus][RelkNdEdge] Fail to add edge between switch node[%s] and conv node[%s].",
        switch_node->GetName().c_str(), conv_node->GetName().c_str());
    return FAILED;
  }
  if (ge::GraphUtils::AddEdge(switch_node->GetOutDataAnchor(1), conv_compress_node->GetInDataAnchor(1)) !=
      ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOpt][ConvWgtCmpsFus][RelkNdEdge] Fail to add edge between switch node[%s] and conv/fc \
        compress node[%s].", switch_node->GetName().c_str(), conv_compress_node->GetName().c_str());
    return FAILED;
  }

  // link the feature map input for conv compress
  if (ge::GraphUtils::AddEdge(conv_node->GetInDataAnchor(0)->GetPeerOutAnchor(),
                              conv_compress_node->GetInDataAnchor(0)) != ge::GRAPH_SUCCESS) {
    REPORT_FE_ERROR(
        "[GraphOpt][ConvWgtCmpsFus][RelkNdEdge] Fail to add edge between switch node[%s] and conv/fc \
        compress node[%s].", switch_node->GetName().c_str(), conv_compress_node->GetName().c_str());
    return FAILED;
  }

  // link the anchor after filter input for conv compress node
  if (conv_node->GetAllInDataAnchorsSize() > 2) {
    for (uint32_t i = 2; i < conv_node->GetAllInDataAnchorsSize(); i++) {
      ge::InDataAnchorPtr in_data_anchor_ptr = conv_node->GetInDataAnchor(i);
      if (in_data_anchor_ptr != nullptr && in_data_anchor_ptr->GetPeerOutAnchor() != nullptr) {
        uint32_t index = i + 1;
        if (ge::GraphUtils::AddEdge(in_data_anchor_ptr->GetPeerOutAnchor(),
                                    conv_compress_node->GetInDataAnchor(index)) != ge::GRAPH_SUCCESS) {
          REPORT_FE_ERROR(
              "[GraphOpt][ConvWgtCmpsFus][RelkNdEdge] Fail to add edge for the input[%u] of conv/fc \
              compress node[%s] .", index, conv_compress_node->GetName().c_str());
          return FAILED;
        }
      }
    }
  }

  if (RelinkDataEdgesOfMergeNode(conv_node, conv_compress_node, merge_node) != SUCCESS) {
    return FAILED;
  }

  return RelinkControlEdges(conv_node, conv_compress_node);
}

Status ConvWeightCompressFusionPass::CreateConvCompressOpDesc(ge::OpDescPtr conv_op_desc,
                                                              ge::OpDescPtr &conv_compress_op_desc) const {
  conv_compress_op_desc->SetName(conv_op_desc->GetName() + "_Compress");
  // remove all tensor desc except input
  for (uint32_t i = conv_compress_op_desc->GetAllInputsSize() - 1; i > 0; i--) {
    if (!ge::OpDescUtils::ClearInputDesc(conv_compress_op_desc, i)) {
      REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][CrtConvCprsOpDesc] Fail to clear input[%u] of node[%s].", i,
                      conv_compress_op_desc->GetName().c_str());
      return FAILED;
    }
  }

  // input name shall be updated
  std::map<string, uint32_t> new_conv_desc_input_name;
  new_conv_desc_input_name.emplace(conv_op_desc->GetInputNameByIndex(0), 0);
  if (!conv_compress_op_desc->UpdateInputName(new_conv_desc_input_name)) {
    REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][CrtConvCprsOpDesc] Fail to update the input name of node[%s].",
                    conv_compress_op_desc->GetName().c_str());
    return FAILED;
  }

  conv_compress_op_desc->AddInputDesc(TENSOR_NAME_FILTER_COMPRESS,
                                      conv_op_desc->GetInputDesc(TENSOR_INDEX_FILTER_COMPRESS));
  new_conv_desc_input_name.emplace(TENSOR_NAME_FILTER_COMPRESS, TENSOR_INDEX_FILTER_COMPRESS);

  // copy the weight tensor desc
  ge::GeTensorDesc compress_index_tensor_desc = conv_op_desc->GetInputDesc(TENSOR_INDEX_FILTER_COMPRESS);
  // set the format to ND and datatype to int8
  compress_index_tensor_desc.SetOriginFormat(ge::FORMAT_ND);
  compress_index_tensor_desc.SetFormat(ge::FORMAT_ND);
  compress_index_tensor_desc.SetOriginDataType(ge::DT_INT8);
  compress_index_tensor_desc.SetDataType(ge::DT_INT8);
  // clear the shape
  std::vector<int64_t> empty_dims;
  ge::GeShape index_shape(empty_dims);
  compress_index_tensor_desc.SetShape(index_shape);
  compress_index_tensor_desc.SetOriginShape(index_shape);
  conv_compress_op_desc->AddInputDesc(TENSOR_NAME_COMPRESS_INDEX, compress_index_tensor_desc);
  new_conv_desc_input_name.emplace(TENSOR_NAME_COMPRESS_INDEX, TENSOR_INDEX_COMPRESS_INDEX);
  if (conv_op_desc->GetAllInputsSize() > TENSOR_INDEX_COMPRESS_INDEX) {
    for (uint32_t i = TENSOR_INDEX_COMPRESS_INDEX; i < conv_op_desc->GetAllInputsSize(); i++) {
      conv_compress_op_desc->AddInputDesc(conv_op_desc->GetInputNameByIndex(i), conv_op_desc->GetInputDesc(i));
      new_conv_desc_input_name.emplace(conv_op_desc->GetInputNameByIndex(i), i + 1);
    }
  }
  // update input name mapping
  if (!conv_compress_op_desc->UpdateInputName(new_conv_desc_input_name)) {
    REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][CrtConvCprsOpDesc] Fail to update the input name of node[%s].",
                    conv_compress_op_desc->GetName().c_str());
    return FAILED;
  }

  // update is input const
  vector<bool> is_input_const_vec = conv_compress_op_desc->GetIsInputConst();
  vector<bool> new_is_input_const_vec;
  for (uint32_t i = 0; i < is_input_const_vec.size(); i++) {
    new_is_input_const_vec.push_back(is_input_const_vec[i]);
    if (i == TENSOR_INDEX_FILTER_COMPRESS) {
      new_is_input_const_vec.push_back(true);
    }
  }
  conv_compress_op_desc->SetIsInputConst(new_is_input_const_vec);

  // add _weight_compress
  if (!ge::AttrUtils::SetBool(conv_compress_op_desc, ATTR_NAME_WEIGHT_COMPRESS, true)) {
    FE_LOGD("Fail to set _weight_compress attr on node[%s].", conv_compress_op_desc->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

ge::NodePtr ConvWeightCompressFusionPass::CreateHostNode(const ge::OpDescPtr &conv_op_desc,
                                                         ge::ComputeGraph &graph) const {
  // add host node
  std::string op_name = conv_op_desc->GetName() + "_" + HOST_OP_TYPE;
  ge::OpDescPtr host_op_desc = nullptr;
  FE_MAKE_SHARED(host_op_desc = std::make_shared<ge::OpDesc>(op_name, HOST_OP_TYPE), return nullptr);
  FE_CHECK(host_op_desc == nullptr, REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][CrtHostNd] Fail to make shared \
           of host op desc."),
           return nullptr);
  ge::GeTensorDesc tensor_desc = conv_op_desc->GetInputDesc(1);

  ge::GeShape new_shape;
  ge::GeShape origin_shape = tensor_desc.GetOriginShape();
  ge::Format origin_format = tensor_desc.GetOriginFormat();
  ge::DataType data_type = tensor_desc.GetDataType();
  ge::Format new_format = ge::FORMAT_FRACTAL_Z;
  int64_t op_impl_type = static_cast<int64_t>(EN_IMPL_HW_TBE);
  ShapeAndFormat output_shape_and_format_info = {origin_shape, new_shape,    origin_format,       new_format,
                                                 data_type,    op_impl_type, GROUPS_DEFAULT_VALUE};
  (void)ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(output_shape_and_format_info);
  tensor_desc.SetFormat(ge::FORMAT_FRACTAL_Z);
  tensor_desc.SetShape(new_shape);
  host_op_desc->AddInputDesc("weight", tensor_desc);

  ge::GeTensorDesc output_desc;
  output_desc.SetDataType(ge::DT_BOOL);
  output_desc.SetOriginDataType(ge::DT_BOOL);
  output_desc.SetFormat(ge::FORMAT_ND);
  output_desc.SetOriginFormat(ge::FORMAT_ND);
  host_op_desc->AddOutputDesc("iscompress", output_desc);

  ge::NodePtr host_node = graph.AddNode(host_op_desc);
  return host_node;
}

ge::NodePtr ConvWeightCompressFusionPass::CreateSwitchNode(const ge::OpDescPtr &conv_op_desc,
                                                           ge::ComputeGraph &graph) const {
  // add switch node
  std::string op_name = conv_op_desc->GetName() + "_" + SWITCH_OP_TYPE;
  ge::OpDescPtr switch_op_desc = nullptr;
  FE_MAKE_SHARED(switch_op_desc = std::make_shared<ge::OpDesc>(op_name, SWITCH_OP_TYPE), return nullptr);
  FE_CHECK(switch_op_desc == nullptr, REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][CrtSwtNd] Fail to make shared of \
           switch op desc."),
           return nullptr);
  switch_op_desc->AddInputDesc("data", conv_op_desc->GetInputDesc(1));

  ge::GeTensorDesc input_desc;
  input_desc.SetDataType(ge::DT_BOOL);
  input_desc.SetOriginDataType(ge::DT_BOOL);
  input_desc.SetFormat(ge::FORMAT_ND);
  input_desc.SetOriginFormat(ge::FORMAT_ND);
  switch_op_desc->AddInputDesc("pred", input_desc);

  switch_op_desc->AddOutputDesc("output_false", conv_op_desc->GetInputDesc(1));
  switch_op_desc->AddOutputDesc("output_true", conv_op_desc->GetInputDesc(1));

  (void)ge::AttrUtils::SetInt(switch_op_desc, FORMAT_AGNOSTIC, 1);
  std::vector<int64_t> input_vec = {1};
  (void)ge::AttrUtils::SetListInt(switch_op_desc, INPUT_FORMAT_AGNOSTIC_EXCEPTION, input_vec);

  ge::NodePtr switch_node = graph.AddNode(switch_op_desc);
  return switch_node;
}

ge::NodePtr ConvWeightCompressFusionPass::CreateMergeNode(const ge::OpDescPtr &conv_op_desc,
                                                          ge::ComputeGraph &graph) const {
  // add merge node
  std::string op_name = conv_op_desc->GetName() + "_" + MERGE_OP_TYPE;
  ge::OpDescPtr merge_op_desc = nullptr;
  FE_MAKE_SHARED(merge_op_desc = std::make_shared<ge::OpDesc>(op_name, MERGE_OP_TYPE), return nullptr);
  FE_CHECK(merge_op_desc == nullptr,
           REPORT_FE_ERROR("[GraphOpt][ConvWgtCmpsFus][CrtMrgNd] Fail to make shared of host op desc."),
           return nullptr);

  merge_op_desc->AddInputDesc("x1", conv_op_desc->GetOutputDesc(0));
  merge_op_desc->AddInputDesc("x2", conv_op_desc->GetOutputDesc(0));
  merge_op_desc->AddOutputDesc("y", conv_op_desc->GetOutputDesc(0));

  ge::GeTensorDesc output_desc;
  output_desc.SetDataType(ge::DT_INT32);
  output_desc.SetOriginDataType(ge::DT_INT32);
  output_desc.SetFormat(ge::FORMAT_ND);
  output_desc.SetOriginFormat(ge::FORMAT_ND);
  merge_op_desc->AddOutputDesc("value_index", output_desc);

  (void)ge::AttrUtils::SetInt(merge_op_desc, FORMAT_AGNOSTIC, 1);

  ge::NodePtr merge_node = graph.AddNode(merge_op_desc);
  return merge_node;
}

bool ConvWeightCompressFusionPass::IsCompressWeight(ge::NodePtr node_ptr) const {
  int64_t groups_val = GROUPS_DEFAULT_VALUE;
  if (ge::AttrUtils::GetInt(node_ptr->GetOpDesc(), ATTR_NAME_GROUPS, groups_val)) {
    if (groups_val != GROUPS_DEFAULT_VALUE) {
      FE_LOGI("The groups value of node[%s, %s] is [%ld] which is not supported.", node_ptr->GetName().c_str(),
              node_ptr->GetType().c_str(), groups_val);
      return false;
    }
  }

  bool enable_compress = Configuration::Instance(AI_CORE_NAME).GetEnableCompressWeight();
  if (!enable_compress) {
    bool is_compress = false;
    // if the node does not contain is_compress_weight, return false
    if (!ge::AttrUtils::GetBool(node_ptr->GetOpDesc(), ge::ATTR_NAME_COMPRESS_WEIGHT, is_compress)) {
      FE_LOGD("The node[%s] dose not have is_compress_weight attr.", node_ptr->GetName().c_str());
      return false;
    }
    return is_compress;
  } else {
    return true;
  }
}

bool ConvWeightCompressFusionPass::CheckWeightConstFoldNode(ge::NodePtr conv_node_ptr) const {
  ge::InDataAnchorPtr in_data_anchor_ptr = conv_node_ptr->GetInDataAnchor(1);
  if (in_data_anchor_ptr == nullptr || in_data_anchor_ptr->GetPeerOutAnchor() == nullptr ||
      in_data_anchor_ptr->GetPeerOutAnchor()->GetOwnerNode() == nullptr) {
    return false;
  }
  ge::NodePtr node_ptr = in_data_anchor_ptr->GetPeerOutAnchor()->GetOwnerNode();
  kFeRecursiveCnt = 0;
  return CheckConstFoldNode(node_ptr);
}

bool ConvWeightCompressFusionPass::CheckConstFoldNode(ge::NodePtr node_ptr) const {
  if (kFeRecursiveCnt == kRecursiveLoopMax) {
    FE_LOGD("Recursive calls reach max num(%lu).", kRecursiveLoopMax);
    return false;
  }
  kFeRecursiveCnt++;

  string op_type;
  if (ge::NodeUtils::GetConstOpType(node_ptr, op_type)) {
    return true;
  }

  auto iter = std::find(HOST_OP_TYPE_VEC.begin(), HOST_OP_TYPE_VEC.end(), node_ptr->GetType());
  if (iter == HOST_OP_TYPE_VEC.end()) {
    FE_LOGD("Node[%s, %s] can not be fold.", node_ptr->GetType().c_str(), node_ptr->GetType().c_str());
    return false;
  }
  if (node_ptr->GetInNodes().empty()) {
    return true;
  }
  for (const ge::NodePtr &node : node_ptr->GetInNodes()) {
    if (!CheckConstFoldNode(node)) {
      return false;
    }
  }
  return true;
}
}  // namespace fe
