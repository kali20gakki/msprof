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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/depthwise_insert_transdata_fusion_pass.h"
#include <atomic>
#include <map>
#include <string>
#include <vector>
#include "common/fe_log.h"
#include "common/op_info_common.h"
#include "external/graph/types.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/fusion_pass_registry.h"
#include "graph_optimizer/shape_format_transfer/transfer_shape_according_to_format.h"

/**
 *  Input     filter                    Input        filter
 *    |         |                         |            |
 *    |         |                         |         TransData
 *    |         |                         |       (nchw -> hwcn)
 *    |      TransData           ->       |            |
 *    |   (nchw -> fractal_z)             |         TransData
 *    |         |                         |      (hwcn -> fractal_z)
 *     \        |                          \           |
 *      \       |                           \          |
 *     DepthwiseConv2D                     DepthwiseConv2D
 *
 */
namespace fe {
static constexpr char const *DEPTHWISEINSERTTRANSDATA_PASS_NAME = "DepthwiseInsertTransDataFusionPass";
static const string PATTERN_DEPTHWISE = "Pattern_Depthwise";
static const string OP_TYPE_DEPTHWISE = "DepthwiseConv2D";
static const string OP_TYPE_TRANSDATA = "TransData";
static const std::vector<ge::Format> INPUT_FORMATS = {ge::FORMAT_NCHW, ge::FORMAT_NHWC};

vector<FusionPattern *> DepthwiseInsertTransDataFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;

  FusionPattern *pattern = new (std::nothrow) FusionPattern("DepthwiseInsertTransDataFusionPass");
  FE_CHECK(pattern == nullptr, REPORT_FE_ERROR("[GraphOpt][DepwiseInsTrsDataFus][DefPtn] Fail to create a new pattern \
           object."),
           return patterns);
  FE_LOGD("Start to do Depthwise Insert TransData node fusion pass.");
  pattern->AddOpDesc(PATTERN_DEPTHWISE, {OP_TYPE_DEPTHWISE}).SetOutput(PATTERN_DEPTHWISE);

  patterns.push_back(pattern);

  return patterns;
}

Status DepthwiseInsertTransDataFusionPass::Fusion(ge::ComputeGraph &graph, Mapping &mapping,
                                                  vector<ge::NodePtr> &new_nodes) {
  FE_LOGD("Start to do Depthwise Insert TransData node fusion pass.");
  ge::NodePtr depthwise_node = GetNodeFromMapping(PATTERN_DEPTHWISE, mapping);
  FE_CHECK_NOTNULL(depthwise_node);
  auto in_nodes = depthwise_node->GetInDataNodes();
  if (in_nodes.size() < 2) {
    FE_LOGW("Depthwise node:[%s] input nodes size is less than 2.", depthwise_node->GetName().c_str());
    return NOT_CHANGED;
  }
  ge::NodePtr trans_node = in_nodes.at(1);
  FE_CHECK_NOTNULL(trans_node);
  if (VerifyFusionPattern(trans_node) != SUCCESS) {
    FE_LOGD("Depthwise node:[%s] input filter node is not expected transdata node.", depthwise_node->GetName().c_str());
    return NOT_CHANGED;
  }
  ge::OpDescPtr trans_desc = trans_node->GetOpDesc();
  ge::Format src_format = static_cast<ge::Format>(ge::GetPrimaryFormat(trans_desc->GetInputDescPtr(0)->GetFormat()));
  ge::OpDescPtr new_trans_desc = ge::AttrUtils::CopyOpDesc(trans_desc);
  static atomic<uint64_t> name_id(0);
  name_id.fetch_add(1, std::memory_order_relaxed);
  new_trans_desc->SetName("trans_TransData_" + std::to_string(name_id) + "_" + depthwise_node->GetName());
  int64_t op_imply_type = EN_IMPL_HW_TBE;
  ge::GeShape new_shape;
  int64_t group = GROUPS_DEFAULT_VALUE;
  ShapeAndFormat shape_and_format_info = {
      trans_desc->GetInputDescPtr(0)->GetShape(),    new_shape,     src_format, ge::FORMAT_HWCN,
      trans_desc->GetInputDescPtr(0)->GetDataType(), op_imply_type, group};

  Status ret = ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(shape_and_format_info);
  if (ret != SUCCESS) {
    FE_LOGW("Depthwise node[%s] get new shape failed.", depthwise_node->GetName().c_str());
    return NOT_CHANGED;
  }
  (void)ge::AttrUtils::SetStr(trans_desc, ATTR_NAME_DST_FORMAT, ge::TypeUtils::FormatToSerialString(ge::FORMAT_HWCN));
  (void)ge::AttrUtils::SetStr(new_trans_desc, ATTR_NAME_SRC_FORMAT,
                              ge::TypeUtils::FormatToSerialString(ge::FORMAT_HWCN));
  trans_desc->MutableOutputDesc(0)->SetFormat(ge::FORMAT_HWCN);
  trans_desc->MutableOutputDesc(0)->SetShape(new_shape);
  new_trans_desc->MutableInputDesc(0)->SetFormat(ge::FORMAT_HWCN);
  new_trans_desc->MutableInputDesc(0)->SetShape(new_shape);

  ge::NodePtr new_trans_node = graph.AddNode(new_trans_desc);
  FE_CHECK_NOTNULL(new_trans_node);
  // link edge
  (void)ge::GraphUtils::RemoveEdge(trans_node->GetOutDataAnchor(0), depthwise_node->GetInDataAnchor(1));
  (void)ge::GraphUtils::AddEdge(trans_node->GetOutDataAnchor(0), new_trans_node->GetInDataAnchor(0));
  (void)ge::GraphUtils::AddEdge(new_trans_node->GetOutDataAnchor(0), depthwise_node->GetInDataAnchor(1));
  return SUCCESS;
}

Status DepthwiseInsertTransDataFusionPass::VerifyFusionPattern(const ge::NodePtr &trans_node) const {
  if (trans_node->GetType() != OP_TYPE_TRANSDATA) {
    FE_LOGD("Depthwise node input filter node[%s] is not transdata node.", trans_node->GetName().c_str());
    return NOT_CHANGED;
  }
  ge::OpDescPtr trans_desc = trans_node->GetOpDesc();
  ge::Format src_format = static_cast<ge::Format>(ge::GetPrimaryFormat(trans_desc->GetInputDescPtr(0)->GetFormat()));
  if (std::find(INPUT_FORMATS.begin(), INPUT_FORMATS.end(), src_format) == INPUT_FORMATS.end()) {
    FE_LOGD("TransData node[%s] src format[%s] is not support.", trans_desc->GetName().c_str(),
            ge::TypeUtils::FormatToSerialString(src_format).c_str());
    return NOT_CHANGED;
  }
  if (ge::GetPrimaryFormat(trans_desc->GetOutputDescPtr(0)->GetFormat()) != ge::FORMAT_FRACTAL_Z) {
    return NOT_CHANGED;
  }
  return SUCCESS;
}

REGISTER_PASS(DEPTHWISEINSERTTRANSDATA_PASS_NAME, SECOND_ROUND_BUILT_IN_GRAPH_PASS, DepthwiseInsertTransDataFusionPass);
}  // namespace fe
