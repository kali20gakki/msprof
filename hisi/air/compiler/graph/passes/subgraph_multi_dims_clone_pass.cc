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

#include "graph/passes/subgraph_multi_dims_clone_pass.h"

#include "common/formats/utils/formats_trans_utils.h"
#include "common/plugin/ge_util.h"
#include "common/local_context.h"
#include "graph/preprocess/multi_batch_options.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/type_utils.h"
#include "register/op_registry.h"
#include "common/omg_util.h"
#include "ir_build/option_utils.h"

namespace ge {
namespace {
constexpr uint8_t kCaseArgIndex = 1U;

const std::string kSubgraphMultiDimsCaseNode = "subgraph_multi_dims_shape_case";
const std::string kSubgraphMultiDimsGetShapeNode = "subgraph_multi_dims_get_shape";
const std::string kSubgraphMultiDimsConstNode = "subgraph_multi_dims_const";
const std::string kSubgraphMultiDimsMapIndexNode = "subgraph_multi_dims_mapindex";
const std::string kSubgraphMultiDimsNodePostfix = "subgraph_multi_dims_branch_";
}  // namespace

Status SubgraphMultiDimsClonePass::Run(ComputeGraphPtr graph) {
  GELOGD("SubgraphMultiDimsClonePass start.");
  GE_CHECK_NOTNULL(graph);

  if (graph->GetParentNode() != nullptr) {
    return SUCCESS;
  }

  for (const auto &subgraph : graph->GetAllSubgraphs()) {
    const auto &parent_node = subgraph->GetParentNode();
    if (parent_node == nullptr) {
      GELOGW("invalid parent node for subgraph[%s].", subgraph->GetName().c_str());
      continue;
    }
    GELOGD("Start multi dims clone for subgraph[%s].", subgraph->GetName().c_str());
    bool is_dyn_dims = false;
    (void)AttrUtils::GetBool(subgraph, ATTR_NAME_SUBGRAPH_IS_MULTI_DIMS, is_dyn_dims);
    if (!is_dyn_dims) {
      GELOGD("No need proc multi batch for subgraph[%s].", subgraph->GetName().c_str());
      continue;
    }

    GE_CHK_STATUS_RET(CollectIoNodes(subgraph), "CollectIoNodes for graph:%s failed.", subgraph->GetName().c_str());
    GE_CHK_STATUS_RET(MergeDataDynDims(), "Merge data dynamic dims failed.");

    std::string session_graph_id;
    (void)AttrUtils::GetStr(subgraph, ATTR_NAME_SESSION_GRAPH_ID, session_graph_id);
    ComputeGraphPtr branch = MakeShared<ComputeGraph>(subgraph->GetName());
    GE_CHECK_NOTNULL(branch);
    (void)AttrUtils::SetStr(branch, ATTR_NAME_SESSION_GRAPH_ID, session_graph_id);
    (void)AttrUtils::SetBool(branch, ATTR_NAME_SUBGRAPH_IS_MULTI_DIMS, true);

    subgraph->InValid();
    subgraph->Swap(*branch);
    (void)branch->DelAttr(ATTR_NAME_SUBGRAPH_IS_MULTI_DIMS);

    GE_CHK_STATUS_RET(CreateOriGraph(subgraph),
                      "[Create][OriGraph] for graph:%s failed.", subgraph->GetName().c_str());
    GE_CHK_STATUS_RET(CreateSubgraphs(graph, subgraph, branch),
                      "[Create][Subgraphs] for graph:%s failed.", subgraph->GetName().c_str());

    subgraph->SetParentNode(parent_node);
    subgraph->SetParentGraph(graph);
    if (subgraph->TopologicalSorting() != GRAPH_SUCCESS) {
      REPORT_INNER_ERROR("E19999", "Topological sort failed for subgraph[%s]", subgraph->GetName().c_str());
      GELOGE(FAILED, "Topological sort failed for subgraph[%s]", subgraph->GetName().c_str());
      return FAILED;
    }
  }

  GELOGD("SubgraphMultiDimsClonePass end.");
  return SUCCESS;
}

Status SubgraphMultiDimsClonePass::MergeDataDynDims() {
  for (const auto &node : all_data_nodes_) {
    const auto &op_desc = node->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    std::vector<int64_t> dims;
    bool has_attr = AttrUtils::GetListInt(op_desc, ATTR_NAME_OP_MULTI_DIMS_INPUT_DIMS, dims);
    if (!has_attr || dims.empty()) {
      continue;
    }
    const auto &tensor_desc = op_desc->GetInputDescPtr(0U);
    GE_CHECK_NOTNULL(tensor_desc);
    const auto &shape = tensor_desc->GetShape();
    if (shape.GetDimNum() == 0U) {
      GELOGE(PARAM_INVALID, "Invalid input shape for data node[%s]", node->GetName().c_str());
      return FAILED;
    }
    for (size_t i = 0U; i < dims.size(); i++) {
      size_t index = i / shape.GetDimNum();
      if (index >= merged_multi_dims_.size()) {
        merged_multi_dims_.push_back({dims[i]});
      } else {
        merged_multi_dims_[index].push_back(dims[i]);
      }
    }
  }
  return SUCCESS;
}

Status SubgraphMultiDimsClonePass::CreateOriGraph(const ComputeGraphPtr &subgraph) {
  GELOGD("CreateOriGraph start for subgraph[%s].", subgraph->GetName().c_str());

  GE_CHK_STATUS_RET(CreateGetShapeNode(subgraph),
                    "[Create][GetShapeNode] for graph:%s failed", subgraph->GetName().c_str());
  GE_CHK_STATUS_RET(CreateIndexConstNode(subgraph),
                    "[Create][IndexConstNode] failed, graph:%s", subgraph->GetName().c_str());
  GE_CHK_STATUS_RET(CreateMapIndexNode(subgraph),
                    "[Create][GetShapeNode] for graph:%s failed", subgraph->GetName().c_str());
  GE_CHK_STATUS_RET(CreateCaseNode(subgraph),
                    "[Create][CaseNode] for graph:%s failed", subgraph->GetName().c_str());
  GE_CHK_STATUS_RET(CreateInputNode(subgraph),
                    "[Create][InputNode] for graph:%s failed", subgraph->GetName().c_str());
  GE_CHK_STATUS_RET(CreateConstNode(subgraph),
                    "[Create][ConstNode] for graph:%s failed", subgraph->GetName().c_str());
  GE_CHK_STATUS_RET(CreateOutputNode(subgraph),
                    "[Create][OutputNode] for graph:%s failed", subgraph->GetName().c_str());

  GELOGD("CreateOriGraph end for subgraph[%s].", subgraph->GetName().c_str());
  return SUCCESS;
}

Status SubgraphMultiDimsClonePass::CollectIoNodes(const ComputeGraphPtr &subgraph) {
  all_data_nodes_.clear();
  all_const_nodes_.clear();
  for (const auto &node : subgraph->GetDirectNode()) {
    if (node->GetType() == DATA) {
      all_data_nodes_.emplace_back(node);
    } else if (node->GetType() == CONSTANT) {
      all_const_nodes_.emplace_back(node);
    } else {
      // do nothing
    }
  }
  output_node_ = subgraph->FindFirstNodeMatchType(NETOUTPUT);

  if (all_data_nodes_.empty()) {
    REPORT_INNER_ERROR("E19999", "Data node num is 0 or output node num != 1, graph:%s, check invalid",
                       subgraph->GetName().c_str());
    GELOGE(FAILED, "[Check][Param] Data node num is 0 or output node num != 1, graph:%s", subgraph->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status SubgraphMultiDimsClonePass::CreateGetShapeNode(const ComputeGraphPtr &subgraph) {
  GELOGD("CreateGetShapeNode start for subgraph[%s].", subgraph->GetName().c_str());

  const OpDescPtr op_desc = MakeShared<OpDesc>(kSubgraphMultiDimsGetShapeNode, "GetShape");
  GE_CHECK_NOTNULL(op_desc);

  size_t all_dims_num = 0U;
  size_t input_cnt = 0U;
  for (const auto &node : all_data_nodes_) {
    const auto &data_desc = node->GetOpDesc();
    GE_CHECK_NOTNULL(data_desc);
    const auto &input_desc = data_desc->GetInputDesc(0U);
    if (input_desc.GetShape().IsUnknownShape()) {
      all_dims_num += input_desc.GetShape().GetDimNum();
      GE_CHK_STATUS_RET(op_desc->AddInputDesc(input_desc), "Add input desc fail");
      input_cnt++;
    }
  }

  GeTensorDesc tensor_desc(GeShape({static_cast<int64_t>(all_dims_num)}), FORMAT_ND, DT_INT32);
  tensor_desc.SetOriginShape(GeShape({static_cast<int64_t>(all_dims_num)}));
  GE_CHK_STATUS_RET(op_desc->AddOutputDesc(tensor_desc), "Add output desc fail");

  (void)AttrUtils::SetBool(op_desc, ATTR_INSERT_BY_MBATCH, true);

  get_shape_node_ = subgraph->AddNode(op_desc);
  GE_CHECK_NOTNULL(get_shape_node_);

  GELOGD("CreateGetShapeNode end for subgraph[%s].", subgraph->GetName().c_str());
  return SUCCESS;
}

Status SubgraphMultiDimsClonePass::CreateMapIndexNode(const ComputeGraphPtr &subgraph) {
  GELOGD("CreateMapIndexNode start for subgraph[%s].", subgraph->GetName().c_str());

  OpDescBuilder op_builder(kSubgraphMultiDimsMapIndexNode, "MapIndex");
  const auto &get_shape_op_desc = get_shape_node_->GetOpDesc();
  GE_CHECK_NOTNULL(get_shape_op_desc);
  const auto &const_op_desc = const_node_->GetOpDesc();
  GE_CHECK_NOTNULL(const_op_desc);
  op_builder.AddInput("x", get_shape_op_desc->GetOutputDesc(0U))
            .AddInput("data_seq", const_op_desc->GetOutputDesc(0U))
            .AddOutput("y", GeTensorDesc(GeShape(), FORMAT_ND, DT_INT32));

  const OpDescPtr op_desc = op_builder.Build();
  GE_CHECK_NOTNULL(op_desc);

  (void)AttrUtils::SetBool(op_desc, ATTR_INSERT_BY_MBATCH, true);

  map_index_node_ = subgraph->AddNode(op_desc);
  GE_CHECK_NOTNULL(map_index_node_);

  if (GraphUtils::AddEdge(get_shape_node_->GetOutDataAnchor(0), map_index_node_->GetInDataAnchor(0)) !=
      GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Add edge between op:%s(%s)(index:0) and op:%s(%s)(index:0) failed",
                      get_shape_node_->GetName().c_str(), get_shape_node_->GetType().c_str(),
                      map_index_node_->GetName().c_str(), map_index_node_->GetType().c_str());
    GELOGE(FAILED, "[Add][Edge] between op:%s(%s)(index:0) and op:%s(%s)(index:0) failed",
           get_shape_node_->GetName().c_str(), get_shape_node_->GetType().c_str(),
           map_index_node_->GetName().c_str(), map_index_node_->GetType().c_str());
    return FAILED;
  }

  if (GraphUtils::AddEdge(const_node_->GetOutDataAnchor(0), map_index_node_->GetInDataAnchor(1)) != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Add edge between op:%s(%s)(index:0) and op:%s(%s)(index:1) failed",
                      const_node_->GetName().c_str(), const_node_->GetType().c_str(),
                      map_index_node_->GetName().c_str(), map_index_node_->GetType().c_str());
    GELOGE(FAILED, "[Add][Edge] between node:%s to MapIndex:%s", const_node_->GetName().c_str(),
           map_index_node_->GetName().c_str());
    return FAILED;
  }

  GELOGD("CreateMapIndexNode end for subgraph[%s].", subgraph->GetName().c_str());
  return SUCCESS;
}

Status SubgraphMultiDimsClonePass::CreateCaseNode(const ComputeGraphPtr &subgraph) {
  GELOGD("CreateCaseNode start for subgraph[%s].", subgraph->GetName().c_str());

  const size_t input_num = all_data_nodes_.size() + all_const_nodes_.size();
  const size_t output_num = output_node_->GetAllInDataAnchorsSize();
  OpDescBuilder op_builder(kSubgraphMultiDimsCaseNode, CASE);
  op_builder.AddInput("branch_index").AddDynamicInput("input", input_num).AddDynamicOutput("output", output_num);
  const OpDescPtr op_desc = op_builder.Build();
  GE_CHECK_NOTNULL(op_desc);

  op_desc->RegisterSubgraphIrName("branches", kDynamic);
  case_node_ = subgraph->AddNode(op_desc);
  GE_CHECK_NOTNULL(case_node_);

  if (GraphUtils::AddEdge(map_index_node_->GetOutDataAnchor(0), case_node_->GetInDataAnchor(0)) != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Add edge between op:%s(%s)(index:0) and op:%s(%s)(index:0) failed",
                      map_index_node_->GetName().c_str(), map_index_node_->GetType().c_str(),
                      case_node_->GetName().c_str(), case_node_->GetType().c_str());
    GELOGE(FAILED, "[Add][Edge] between op:%s(%s)(index:0) and op:%s(%s)(index:0) failed",
           map_index_node_->GetName().c_str(), map_index_node_->GetType().c_str(),
           case_node_->GetName().c_str(), case_node_->GetType().c_str());
    return FAILED;
  }

  // Current max batch num is 100
  const size_t batch_num = merged_multi_dims_.size();
  (void)AttrUtils::SetInt(op_desc, ATTR_NAME_BATCH_NUM, batch_num);
  for (size_t i = 0U; i < batch_num; i++) {
    const std::string &attr_name = ATTR_NAME_PRED_VALUE + "_" + std::to_string(i);
    (void)AttrUtils::SetListInt(op_desc, attr_name, merged_multi_dims_[i]);
  }

  (void)AttrUtils::SetBool(op_desc, ATTR_INSERT_BY_MBATCH, true);

  GELOGD("CreateCaseNode end for subgraph[%s].", subgraph->GetName().c_str());
  return SUCCESS;
}

Status SubgraphMultiDimsClonePass::CreateSubgraphs(const ComputeGraphPtr &root_graph, const ComputeGraphPtr &subgraph,
                                                   const ComputeGraphPtr &branch) {
  GELOGD("CreateSubgraphs start for subgraph[%s].", subgraph->GetName().c_str());

  const auto &case_desc = case_node_->GetOpDesc();
  GE_CHECK_NOTNULL(case_desc);
  for (size_t i = 0U; i < merged_multi_dims_.size(); ++i) {
    std::vector<NodePtr> input_nodes;
    std::vector<NodePtr> output_nodes;
    const std::string postfix = "_" + kSubgraphMultiDimsNodePostfix + std::to_string(i);
    ComputeGraphPtr new_subgraph = (i == 0U) ? branch :
        GraphUtils::CloneGraph(branch, postfix, input_nodes, output_nodes);
    GE_IF_BOOL_EXEC(new_subgraph == nullptr,
                    REPORT_CALL_ERROR("E19999", "Clone graph from graph:%s failed", branch->GetName().c_str());
                    GELOGE(FAILED, "[Clone][Graph] from graph:%s failed", branch->GetName().c_str());
                        return FAILED);
    new_subgraph->SetName("Subgraph_Multi_Dims_Branch_" + std::to_string(i));
    new_subgraph->SetParentNode(case_node_);
    new_subgraph->SetParentGraph(subgraph);
    (void)root_graph->AddSubgraph(new_subgraph->GetName(), new_subgraph);

    const std::string key_name = "branches_" + std::to_string(i);
    (void)case_desc->AddSubgraphName(key_name);
    (void)case_desc->SetSubgraphInstanceName(i, new_subgraph->GetName());

    GELOGD("The %s has %zu input, %zu output.",
           new_subgraph->GetName().c_str(), input_nodes.size(), output_nodes.size());
    for (const auto &data : input_nodes) {
      GE_CHK_STATUS_RET(UpdateSubgraphData(data, i),
                        "[Update][SubgraphData] in subgraph:%s failed, node:%s, index:%zu",
                        new_subgraph->GetName().c_str(), data->GetName().c_str(), i);
    }
  }

  // Origninal graph take as first subgraph, update node name.
  for (const auto &n : branch->GetDirectNode()) {
    const auto &op_desc = n->GetOpDesc();
    GE_CHECK_NOTNULL(op_desc);
    op_desc->SetName(n->GetName() + "_" + kSubgraphMultiDimsNodePostfix + "0");
    if (n->GetType() == DATA) {
      GE_CHK_STATUS_RET(UpdateSubgraphData(n, 0U),
                        "[Update][SubgraphData] in first subgraph:%s failed, node:%s, index:0",
                        branch->GetName().c_str(), n->GetName().c_str());
    }
  }

  GELOGD("CreateSubgraphs end for subgraph[%s].", subgraph->GetName().c_str());
  return SUCCESS;
}

Status SubgraphMultiDimsClonePass::CreateIndexConstNode(const ComputeGraphPtr &subgraph) {
  GELOGD("CreateIndexConstNode start for subgraph[%s].", subgraph->GetName().c_str());

  const OpDescPtr const_desc = MakeShared<OpDesc>(kSubgraphMultiDimsConstNode, CONSTANT);
  if (const_desc == nullptr) {
    REPORT_CALL_ERROR("E19999", "New OpDesc failed");
    GELOGE(OUT_OF_MEMORY, "[New][OpDesc] failed");
    return FAILED;
  }

  const int64_t count = static_cast<int64_t>(merged_multi_dims_.size() * merged_multi_dims_[0U].size());
  std::unique_ptr<int32_t[]> addr(MakeUnique<int32_t[]>(count));
  GE_CHECK_NOTNULL(addr);

  size_t i = 0U;
  for (auto &shape : merged_multi_dims_) {
    for (int64_t dim : shape) {
      addr[i++] = static_cast<int32_t>(dim);
    }
  }

  GeTensorDesc const_tensor(GeShape({count}), FORMAT_ND, DT_INT32);
  GeTensor tensor(const_tensor);
  (void)tensor.SetData(reinterpret_cast<uint8_t *>(addr.get()), count * sizeof(int32_t));
  if (!AttrUtils::SetTensor(const_desc, ATTR_NAME_WEIGHTS, tensor)) {
    REPORT_CALL_ERROR("E19999", "Set Attr:%s to op:%s(%s) failed", ATTR_NAME_WEIGHTS.c_str(),
                      const_desc->GetName().c_str(), const_desc->GetType().c_str());
    GELOGE(OUT_OF_MEMORY, "[Set][Attr] %s to op:%s(%s) failed", ATTR_NAME_WEIGHTS.c_str(),
           const_desc->GetName().c_str(), const_desc->GetType().c_str());
    return FAILED;
  }

  if (const_desc->AddOutputDesc(const_tensor) != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Add ouput desc to op:%s(%s) failed",
                      const_desc->GetName().c_str(), const_desc->GetType().c_str());
    GELOGE(OUT_OF_MEMORY, "[Add][OutputDesc] to op:%s(%s) failed",
           const_desc->GetName().c_str(), const_desc->GetType().c_str());
    return FAILED;
  }

  const_node_ = subgraph->AddNode(const_desc);
  if (const_node_ == nullptr) {
    REPORT_CALL_ERROR("E19999", "Add node:%s(%s) to graph:%s failed",
                      const_desc->GetName().c_str(), const_desc->GetType().c_str(), subgraph->GetName().c_str());
    GELOGE(OUT_OF_MEMORY, "[Add][Node] %s(%s) to graph:%s failed",
           const_desc->GetName().c_str(), const_desc->GetType().c_str(), subgraph->GetName().c_str());
    return OUT_OF_MEMORY;
  }

  GELOGD("CreateIndexConstNode end for subgraph[%s].", subgraph->GetName().c_str());
  return SUCCESS;
}

Status SubgraphMultiDimsClonePass::CreateInputNode(const ComputeGraphPtr &subgraph) {
  GELOGD("CreateInputNode start for subgraph[%s].", subgraph->GetName().c_str());

  std::vector<NodePtr> all_data_nodes;
  size_t case_input_index = kCaseArgIndex;
  size_t get_shape_input_index = 0U;
  for (size_t i = 0U; i < all_data_nodes_.size(); ++i, ++case_input_index) {
    const auto &node = all_data_nodes_[i];
    const OpDescPtr data_desc = OpDescUtils::CopyOpDesc(node->GetOpDesc());
    GE_CHECK_NOTNULL(data_desc);

    if (GraphUtils::CopyTensorAttrs(data_desc, node) != GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Copy tensor attr from op:%s(%s) failed",
                        node->GetName().c_str(), node->GetType().c_str());
      GELOGE(OUT_OF_MEMORY, "[Copy][TensorAttrs] from op:%s(%s) failed",
             node->GetName().c_str(), node->GetType().c_str());
      return FAILED;
    }

    data_desc->SetName(node->GetName());
    const NodePtr &data = subgraph->AddNode(data_desc);
    GE_CHECK_NOTNULL(data);

    // add edge between data and case
    if (GraphUtils::AddEdge(data->GetOutDataAnchor(0), case_node_->GetInDataAnchor(case_input_index)) !=
        GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Add edge between op:%s(%s)(index:0) and op:%s(%s)(index:%zu) failed",
                        data->GetName().c_str(), data->GetType().c_str(),
                        case_node_->GetName().c_str(), case_node_->GetType().c_str(), case_input_index);
      GELOGE(FAILED, "[Add][Edge] between op:%s(%s)(index:0) and op:%s(%s)(index:%zu) failed",
             data->GetName().c_str(), data->GetType().c_str(),
             case_node_->GetName().c_str(), case_node_->GetType().c_str(), case_input_index);
      return FAILED;
    }

    // add edge between data and getShape
    const auto &input_tensor = data_desc->GetInputDescPtr(0U);
    GE_CHECK_NOTNULL(input_tensor);
    if (input_tensor->GetShape().IsUnknownShape()) {
      if (GraphUtils::AddEdge(data->GetOutDataAnchor(0), get_shape_node_->GetInDataAnchor(get_shape_input_index)) !=
          GRAPH_SUCCESS) {
        REPORT_CALL_ERROR("E19999", "Add edge between op:%s(%s)(index:0) and op:%s(%s)(index:%zu) failed",
                          data->GetName().c_str(), data->GetType().c_str(), get_shape_node_->GetName().c_str(),
                          get_shape_node_->GetType().c_str(), get_shape_input_index);
        GELOGE(FAILED, "[Add][Edge] between op:%s(%s)(index:0) and op:%s(%s)(index:%zu) failed",
               data->GetName().c_str(), data->GetType().c_str(),
               get_shape_node_->GetName().c_str(), get_shape_node_->GetType().c_str(), get_shape_input_index);
        return FAILED;
      }
      get_shape_input_index++;
    }
    all_data_nodes.emplace_back(data);
  }
  all_data_nodes_.swap(all_data_nodes);

  GELOGD("CreateInputNode end for subgraph[%s].", subgraph->GetName().c_str());
  return SUCCESS;
}

Status SubgraphMultiDimsClonePass::CreateConstNode(const ComputeGraphPtr &subgraph) {
  GELOGD("CreateConstNode start for subgraph[%s].", subgraph->GetName().c_str());
  std::vector<NodePtr> all_const_nodes;
  const size_t arg_index = kCaseArgIndex + all_data_nodes_.size();
  size_t data_index = all_data_nodes_.size();

  for (size_t i = 0U; i < all_const_nodes_.size(); ++i) {
    const auto &node = all_const_nodes_[i];
    const OpDescPtr const_desc = OpDescUtils::CopyOpDesc(node->GetOpDesc());
    GE_CHECK_NOTNULL(const_desc);

    const_desc->SetName(node->GetName());
    if (GraphUtils::CopyTensorAttrs(const_desc, node) != GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Copy tensor attr from op:%s(%s) failed",
                        node->GetName().c_str(), node->GetType().c_str());
      GELOGE(OUT_OF_MEMORY, "[Copy][TensorAttrs] from op:%s(%s) failed",
             node->GetName().c_str(), node->GetType().c_str());
      return FAILED;
    }

    const NodePtr &const_node = subgraph->AddNode(const_desc);
    GE_CHECK_NOTNULL(const_node);

    if (GraphUtils::AddEdge(const_node->GetOutDataAnchor(0),
                            case_node_->GetInDataAnchor(arg_index + i)) != GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Add edge between op:%s(%s)(index:0) and op:%s(%s)(index:%zu) failed",
                        const_node->GetName().c_str(), const_node->GetType().c_str(),
                        case_node_->GetName().c_str(), case_node_->GetType().c_str(), arg_index + i);
      GELOGE(FAILED, "[Add][Edge] between op:%s(%s)(index:0) and op:%s(%s)(index:%zu) failed",
             const_node->GetName().c_str(), const_node->GetType().c_str(),
             case_node_->GetName().c_str(), case_node_->GetType().c_str(), arg_index + i);
      return FAILED;
    }

    const OpDescPtr &old_desc = node->GetOpDesc();
    if (old_desc == nullptr) {
      continue;
    }
    old_desc->SetType(DATA);
    (void)old_desc->DelAttr(ATTR_NAME_WEIGHTS);  // Delete weight.

    // Const no InputDesc, Data need InputDesc.
    (void)old_desc->AddInputDesc(old_desc->GetOutputDesc(0U));
    (void)AttrUtils::SetInt(old_desc, ATTR_NAME_PARENT_NODE_INDEX, data_index);
    (void)NodeUtils::AppendInputAnchor(const_node, 1U);
    GELOGI("Change const node[%s] to data, parent index[%zu]", const_node->GetName().c_str(), data_index);
    data_index++;
  }
  all_const_nodes_.swap(all_const_nodes);
  GELOGD("CreateConstNode end for subgraph[%s].", subgraph->GetName().c_str());
  return SUCCESS;
}

Status SubgraphMultiDimsClonePass::CreateOutputNode(const ComputeGraphPtr &subgraph) {
  GELOGD("CreateOutputNode start for subgraph[%s].", subgraph->GetName().c_str());

  const OpDescPtr output_desc = OpDescUtils::CopyOpDesc(output_node_->GetOpDesc());
  GE_CHECK_NOTNULL(output_desc);

  if (GraphUtils::CopyTensorAttrs(output_desc, output_node_) != GRAPH_SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Copy tensor attr from op:%s(%s) failed",
                      output_node_->GetName().c_str(), output_node_->GetType().c_str());
    GELOGE(OUT_OF_MEMORY, "[Copy][TensorAttrs] from op:%s(%s) failed",
           output_node_->GetName().c_str(), output_node_->GetType().c_str());
    return FAILED;
  }

  for (size_t i = 0U; i < output_desc->GetAllInputsSize(); i++) {
    const auto &tensor_desc = output_desc->MutableInputDesc(i);
    (void)AttrUtils::SetInt(tensor_desc, ATTR_NAME_PARENT_NODE_INDEX, i);
  }

  output_desc->SetName(kSubgraphMultiDimsNodePostfix + output_node_->GetName());
  const NodePtr &node = subgraph->AddNode(output_desc);
  GE_CHECK_NOTNULL(node);

  for (size_t i = 0U; i < case_node_->GetAllOutDataAnchorsSize(); ++i) {
    if (GraphUtils::AddEdge(case_node_->GetOutDataAnchor(i), node->GetInDataAnchor(i)) != GRAPH_SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Add edge between op:%s(%s)(index:%zu) and op:%s(%s)(index:%zu) failed",
                        case_node_->GetName().c_str(), case_node_->GetType().c_str(), i,
                        node->GetName().c_str(), node->GetType().c_str(), i);
      GELOGE(FAILED, "[Add][Edge] between op:%s(%s)(index:%zu) and op:%s(%s)(index:%zu) failed",
             case_node_->GetName().c_str(), case_node_->GetType().c_str(), i,
             node->GetName().c_str(), node->GetType().c_str(), i);
      return FAILED;
    }
  }
  output_node_ = node;

  GELOGD("CreateOutputNode end for subgraph[%s].", subgraph->GetName().c_str());
  return SUCCESS;
}

Status SubgraphMultiDimsClonePass::UpdateSubgraphData(const NodePtr &data, size_t grade_index) const {
  auto data_desc = data->GetOpDesc();
  GE_CHECK_NOTNULL(data_desc);
  int32_t parent_index = -1;
  bool has_attr = AttrUtils::GetInt(data_desc, ATTR_NAME_PARENT_NODE_INDEX, parent_index);
  if ((!has_attr) || (parent_index < 0)) {
    REPORT_CALL_ERROR("E19999", "Subgraph data[%s] has no parent_index.", data->GetName().c_str());
    GELOGE(PARAM_INVALID, "Subgraph data[%s] has no parent_index.", data->GetName().c_str());
    return PARAM_INVALID;
  }
  // case node's first input is mapIndex, so data index plus 1
  (void)AttrUtils::SetInt(data_desc, ATTR_NAME_PARENT_NODE_INDEX, parent_index + 1);

  auto input_desc = data_desc->MutableInputDesc(0U);
  GE_CHECK_NOTNULL(input_desc);
  (void)AttrUtils::SetListInt(data->GetOpDesc(), ATTR_MBATCH_ORIGIN_INPUT_DIMS, input_desc->GetShape().GetDims());

  if (!input_desc->GetShape().IsUnknownShape()) {
    return SUCCESS;
  }
  std::vector<int64_t> dims;
  has_attr = AttrUtils::GetListInt(data_desc, ATTR_NAME_OP_MULTI_DIMS_INPUT_DIMS, dims);
  if ((!has_attr) || dims.empty()) {
    REPORT_CALL_ERROR("E19999", "Dynamic shape data node[%s] has no dyn_dims attr.", data->GetName().c_str());
    GELOGE(PARAM_INVALID, "Dynamic shape data node[%s] has no dyn_dims attr.", data->GetName().c_str());
    return PARAM_INVALID;
  }
  std::vector<int64_t> actual_shape;
  for (size_t i = 0U; i < input_desc->GetShape().GetDimNum(); i++) {
    const size_t index = input_desc->GetShape().GetDimNum() * grade_index + i;
    actual_shape.push_back(dims.at(index));
  }
  input_desc->SetShape(GeShape(actual_shape));
  input_desc->SetOriginShape(GeShape(actual_shape));
  GELOGD("Update data[%s] shape[%s] by grade[%zu]",
         data->GetName().c_str(), GeShape(actual_shape).ToString().c_str(), grade_index);
  return SUCCESS;
}
}  // namespace ge
