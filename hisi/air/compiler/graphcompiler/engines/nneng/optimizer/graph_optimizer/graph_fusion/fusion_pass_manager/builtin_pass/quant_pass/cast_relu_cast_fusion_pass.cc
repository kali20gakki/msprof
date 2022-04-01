/**
 * Copyright 2021-2022 Huawei Technologies Co., Ltd
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

#include "cast_relu_cast_fusion_pass.h"
#include "common/fe_utils.h"
#include "common/fe_log.h"
#include "common/op_info_common.h"
#include "common/util/op_info_util.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/fusion_pass_registry.h"

using namespace ge;
namespace fe {
static const char *kOpTypeCast = "Cast";
static const char *kOpTypeRelu = "Relu";

static const char *kPatternCast0 = "Cast0";
static const char *kPatternCast1 = "Cast1";
static const char *kPatternRelu = "Relu";

vector<FusionPattern *> CastReluCastFusionPass::DefinePatterns() {
  vector<FusionPattern *> patterns;

  FusionPattern *pattern = new(std::nothrow) FusionPattern("CastCastFusionPass");
  FE_CHECK(pattern == nullptr, REPORT_FE_ERROR(" Fail to create a new pattern object."),
           return patterns);
  FE_LOGD("Start to do node fusion pass.");
  pattern->AddOpDesc(kPatternCast0, {kOpTypeCast})
      .AddOpDesc(kPatternRelu, {kOpTypeRelu})
      .AddOpDesc(kPatternCast1, {kOpTypeCast})
      .SetInputs(kPatternRelu, {kPatternCast0})
      .SetInputs(kPatternCast1, {kPatternRelu})
      .SetOutput(kPatternCast1);

  patterns.push_back(pattern);

  return patterns;
}

Status CastReluCastFusionPass::Fusion(ge::ComputeGraph &graph, Mapping &mapping,
                                      vector<ge::NodePtr> &new_nodes) {
  ge::NodePtr cast_Node0 = GetNodeFromMapping(kPatternCast0, mapping);
  FE_CHECK(cast_Node0 == nullptr, REPORT_FE_ERROR("cast_Node0 is null,fusion failed."),
           return NOT_CHANGED);
  ge::OpDescPtr cast_desc0 = cast_Node0->GetOpDesc();
  FE_CHECK(cast_desc0 == nullptr, REPORT_FE_ERROR("cast_Node0's Desc is null,fusion failed."),
           return NOT_CHANGED);

  ge::NodePtr relu_Node = GetNodeFromMapping(kPatternRelu, mapping);
  FE_CHECK(relu_Node == nullptr, REPORT_FE_ERROR("relu_Node is null,fusion failed."),
           return NOT_CHANGED);
  ge::OpDescPtr relu_desc = relu_Node->GetOpDesc();
  FE_CHECK(cast_desc0 == nullptr, REPORT_FE_ERROR("relu_Node's Desc is null,fusion failed."),
           return NOT_CHANGED);

  auto relu_input = relu_desc->MutableInputDesc(0);
  FE_CHECK_NOTNULL(relu_input);
  auto relu_input_desc_dtype = relu_input->GetDataType();

  auto relu_output = relu_desc->MutableOutputDesc(0);
  FE_CHECK_NOTNULL(relu_output);
  auto relu_output_desc_dtype = relu_output->GetDataType();
  if (relu_input_desc_dtype != DT_FLOAT || relu_output_desc_dtype != DT_FLOAT) {
    FE_LOGD("Relu node [%s]'s input dtype or output dtype is unsuitable", relu_desc->GetName().c_str());
    return NOT_CHANGED;
  }

  ge::NodePtr cast_Node1 = GetNodeFromMapping(kPatternCast1, mapping);
  FE_CHECK(cast_Node1 == nullptr, REPORT_FE_ERROR("cast_Node1 is null,fusion failed."),
           return NOT_CHANGED);
  ge::OpDescPtr cast_desc1 = cast_Node1->GetOpDesc();
  FE_CHECK(cast_desc0 == nullptr, REPORT_FE_ERROR("cast_Node1's Desc is null,fusion failed."),
           return NOT_CHANGED);

  auto cast0_input = cast_desc0->MutableInputDesc(0);
  FE_CHECK_NOTNULL(cast0_input);
  DataType cast0_in_d_type = cast0_input->GetDataType();
  auto cast1_output = cast_desc1->MutableOutputDesc(0);
  FE_CHECK_NOTNULL(cast1_output);
  DataType cast1_out_d_type = cast1_output->GetDataType();
  if (cast0_in_d_type != cast1_out_d_type) {
    FE_LOGD("Cast Node0 [%s] input data type is not equal to Cast Node1 [%s] output data type ",
            cast_Node0->GetName().c_str(), cast_Node1->GetName().c_str());
    return NOT_CHANGED;
  }

  auto cast0_out_data_anchor = cast_Node0->GetOutDataAnchor(0);
  FE_CHECK_NOTNULL(cast0_out_data_anchor);
  if (cast0_out_data_anchor->GetPeerInDataAnchors().size() > 1) {
    FE_LOGD("The first output anchor of Cast node[%s] has more than one peer in anchor.",
            cast_Node0->GetName().c_str());
    return NOT_CHANGED;
  }

  auto relu_out_data_anchor = relu_Node->GetOutDataAnchor(0);
  FE_CHECK_NOTNULL(relu_out_data_anchor);
  if (relu_out_data_anchor->GetPeerInDataAnchors().size() > 1) {
    for (auto node : relu_Node->GetOutAllNodes()) {
      if (node->GetType() != "Cast") {
        FE_LOGD("The  output anchor of Relu node has not Cast node,name is [%s] Type is [%s].",
                node->GetName().c_str(), node->GetType().c_str());
        return NOT_CHANGED;
      }
      auto node_desc = node->GetOpDesc();
      FE_CHECK_NOTNULL(node_desc);
      auto in_dtype = node_desc->MutableInputDesc(0)->GetDataType();
      auto out_dtype = node_desc->MutableOutputDesc(0)->GetDataType();
      if (in_dtype != DT_FLOAT || out_dtype != DT_FLOAT16) {
        FE_LOGD("The Cast node [%s]'s indatatype is not equal to DT_FLOAT or outdatatype is not equal to DT_FLOAT16.",
                node->GetName().c_str());
        return NOT_CHANGED;
      }
    }
  }

  ge::ComputeGraphPtr graphPtr = relu_Node->GetOwnerComputeGraph();
  FE_CHECK_NOTNULL(graphPtr);
  if (GraphUtils::IsolateNode(cast_Node0, {0}) != GRAPH_SUCCESS) {
    REPORT_FE_ERROR("Isolate op:%s(%s) failed", cast_Node0->GetName().c_str(), cast_Node0->GetType().c_str());
    return FAILED;
  }
  if (GraphUtils::RemoveNodeWithoutRelink(graphPtr, cast_Node0) != GRAPH_SUCCESS) {
    REPORT_FE_ERROR("[Remove][Node] %s, type:%s without relink in graph:%s failed",
                    cast_Node0->GetName().c_str(), cast_Node0->GetType().c_str(), graph.GetName().c_str());
    return FAILED;
  }
  for (auto inAnchor : relu_out_data_anchor->GetPeerInDataAnchors()) {
    auto node = inAnchor->GetOwnerNode();
    FE_CHECK_NOTNULL(node);
    if (GraphUtils::IsolateNode(node, {0}) != GRAPH_SUCCESS) {
      REPORT_FE_ERROR("Isolate op:%s(%s) failed", node->GetName().c_str(), node->GetType().c_str());
      return FAILED;
    }
    if (GraphUtils::RemoveNodeWithoutRelink(graphPtr, node) != GRAPH_SUCCESS) {
      REPORT_FE_ERROR("[Remove][Node] %s, type:%s without relink in graph:%s failed",
                      node->GetName().c_str(), node->GetType().c_str(), graph.GetName().c_str());
      return FAILED;
    }
  }
  relu_desc->MutableInputDesc(0)->SetDataType(cast0_in_d_type);
  relu_desc->MutableOutputDesc(0)->SetDataType(cast1_out_d_type);
  return SUCCESS;
}

REGISTER_PASS("CastReluCastFusionPass", SECOND_ROUND_BUILT_IN_GRAPH_PASS, CastReluCastFusionPass);
}