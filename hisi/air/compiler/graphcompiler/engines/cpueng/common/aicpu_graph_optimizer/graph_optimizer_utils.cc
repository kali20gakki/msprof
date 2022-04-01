/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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
#include "graph_optimizer_utils.h"

#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/type_utils.h"
#include "internal_ops.h"
#include "util/log.h"
#include "elewise_calculation_ops.h"
#include "util/util.h"
#include "util/constant.h"

using namespace ge;
using namespace std;

namespace {
const string kPlaceHolderOpType = "PlaceHolder";
const string kEndOpType = "End";
}  // namespace

namespace aicpu {
ge::Status GraphOptimizerUtils::VerifyPldAndEndNode(const ComputeGraph &graph) {
  for (const NodePtr &curr_node : graph.GetDirectNode()) {
    AICPU_CHECK_NOTNULL(curr_node)
    OpDescPtr curr_op_desc_ptr = curr_node->GetOpDesc();
    AICPU_CHECK_NOTNULL(curr_op_desc_ptr)
    string op_type = curr_op_desc_ptr->GetType();
    // if place hold
    if (op_type == kPlaceHolderOpType) {
      uint32_t only_one_input = 0;
      GeTensorDesc pld_in_tensor_desc = curr_op_desc_ptr->GetInputDesc(only_one_input);
      // our format
      ge::Format pld_format = pld_in_tensor_desc.GetFormat();
      // client format
      ge::Format pld_original_format = pld_in_tensor_desc.GetOriginFormat();
      // verify_succ = 1 if verify succ
      bool verify_succ =
          ((pld_format == pld_original_format) || (pld_format == ge::FORMAT_ND));
      CHECK_RES_BOOL(verify_succ, FAILED,
          AICPU_REPORT_INNER_ERROR(
              "Invalied format[%s], should be [FORMAT_ND] or same as original"
              " format[%s]. op[%s], op type[%s]",
              ge::TypeUtils::FormatToSerialString(pld_format).c_str(),
              ge::TypeUtils::FormatToSerialString(pld_original_format).c_str(),
              curr_node->GetName().c_str(), curr_node->GetType().c_str()))
    } else if (op_type == kEndOpType) {
      uint32_t only_one_output = 0;
      GeTensorDesc end_out_tensor_desc =
          curr_op_desc_ptr->GetOutputDesc(only_one_output);
      ge::Format end_format = end_out_tensor_desc.GetFormat();
      ge::Format end_original_format = end_out_tensor_desc.GetOriginFormat();
      // verify_succ = 1 if verify succ
      bool verify_succ =
          ((end_format == end_original_format) || (end_format == ge::FORMAT_ND));
      CHECK_RES_BOOL(verify_succ, FAILED,
          AICPU_REPORT_INNER_ERROR(
              "Invalied format[%s], should be [FORMAT_ND] or same as original"
              " format[%s]. op[%s], op type[%s]",
              ge::TypeUtils::FormatToSerialString(end_format).c_str(),
              ge::TypeUtils::FormatToSerialString(end_original_format).c_str(),
              curr_node->GetName().c_str(), curr_node->GetType().c_str()))
    }
  }
  return SUCCESS;
}

void GraphOptimizerUtils::DumpGraph(ComputeGraph &graph, string &suffix) {
  ComputeGraphPtr graph_ptr = make_shared<ComputeGraph>(graph);
  GraphUtils::DumpGEGraph(graph_ptr, suffix);
  GraphUtils::DumpGEGraphToOnnx(graph, suffix);
}

ge::Status GraphOptimizerUtils::CheckIsFftsPlus(const ge::OpDescPtr &op_desc_ptr,
                                                ffts::ThreadSliceMapPtr &slice_info_ptr, bool &sgt_flag) {
  AICPU_CHECK_NOTNULL(op_desc_ptr)
  // get ffts+ support info
  uint32_t thread_scopeId = 0;
  ge::AttrUtils::GetInt(op_desc_ptr, kAttrNameThreadScopeId, thread_scopeId);
  if (thread_scopeId == 0) {
    return ge::FAILED;
  }
  sgt_flag = true;
  AICPUE_LOGD("start to do ffts+, sgt_flag is [%d], scope_id[%u]", sgt_flag, thread_scopeId);

  // get sgt info
  slice_info_ptr = op_desc_ptr->TryGetExtAttr(kAttrNameSgtStruct, slice_info_ptr);
  if (slice_info_ptr == nullptr) {
    AICPU_REPORT_CALL_ERROR("The Node[%s][%s] has no attr _sgt_struct_info.", op_desc_ptr->GetType().c_str(),
                            op_desc_ptr->GetName().c_str());
    return INVOKE_GRAPH_ITF_FAILED;
  }
  return ge::SUCCESS;
}

ge::Status CacheGraph::CreateAndInsertCacheUpdate(const OutDataAnchorPtr &src_anchor, const InDataAnchorPtr &dst_anchor,
                                                  ComputeGraph &graph, const ffts::ThreadSliceMapPtr &slice_info_ptr,
                                                  const bool &sgt_flag) {
  NodePtr src_node = src_anchor->GetOwnerNode();
  AICPU_CHECK_NOTNULL(src_node)
  OpDescPtr src_op = src_node->GetOpDesc();
  AICPU_CHECK_NOTNULL(src_op)
  int src_anchor_idx = AnchorUtils::GetIdx(src_anchor);
  CHECK_RES_BOOL(src_anchor_idx >= 0, FAILED,
      AICPU_REPORT_INNER_ERROR("Invalid output anchor index[%d] of op[%s]", src_anchor_idx, src_op->GetName().c_str()))
  GeTensorDesc src_tensor_desc = src_op->GetOutputDesc(src_anchor_idx);

  NodePtr dst_node = dst_anchor->GetOwnerNode();
  AICPU_CHECK_NOTNULL(dst_node)
  OpDescPtr dst_op = dst_node->GetOpDesc();
  AICPU_CHECK_NOTNULL(dst_op)
  int dst_anchor_idx = AnchorUtils::GetIdx(dst_anchor);
  CHECK_RES_BOOL(
      dst_anchor_idx >= 0, FAILED,
      AICPU_REPORT_INNER_ERROR("Invalid input anchor index[%d] of op[%s]", dst_anchor_idx, dst_op->GetName().c_str()))
  GeTensorDesc dst_tensor_desc = dst_op->GetInputDesc(dst_anchor_idx);
  // create CacheUpdate op
  auto cache_update_op = op::CacheUpdate();
  OpDescPtr cache_update_op_desc = OpDescUtils::GetOpDescFromOperator(cache_update_op);
  AICPU_CHECK_NOTNULL(cache_update_op_desc)
  (void)AttrUtils::SetBool(cache_update_op_desc, ge::ATTR_NAME_REFERENCE, true);
  cache_update_op_desc->SetType("CacheUpdate");
  std::string op_name = src_op->GetName() + "_" + cache_update_op_desc->GetName();
  cache_update_op_desc->SetName(op_name);
  AICPU_CHECK_RES_WITH_LOG(cache_update_op_desc->UpdateInputDesc(0, src_tensor_desc),
                           "Call UpdateInputDesc function failed to update input[0] desc, op[CacheUpdate].")
  AICPU_CHECK_RES_WITH_LOG(
      cache_update_op_desc->UpdateOutputDesc(0, dst_tensor_desc),
      "Call UpdateInputDesc function failed to update output[0] desc, op[CacheUpdate].")

  if ((sgt_flag) && (slice_info_ptr != nullptr)) {
    (void)cache_update_op_desc->SetExtAttr(kAttrNameSgtStruct, slice_info_ptr);
    (void)ge::AttrUtils::SetInt(cache_update_op_desc, kAttrNameThreadScopeId, slice_info_ptr->thread_scope_id);
  }
  NodePtr cache_update_node = graph.AddNode(cache_update_op_desc);
  AICPU_CHECK_NOTNULL(cache_update_node)

  // insert CacheUpdate op
  AICPU_CHECK_RES_WITH_LOG(GraphUtils::RemoveEdge(src_anchor, dst_anchor),
      "Call GraphUtils::RemoveEdge failed to remove edge between op[%s] and "
      "op[%s].", src_op->GetName().c_str(), dst_op->GetName().c_str())
  InDataAnchorPtr input_anchor = cache_update_node->GetInDataAnchor(0);
  AICPU_CHECK_NOTNULL(input_anchor)
  OutDataAnchorPtr output_anchor = cache_update_node->GetOutDataAnchor(0);
  AICPU_CHECK_NOTNULL(output_anchor)
  AICPU_CHECK_RES_WITH_LOG(GraphUtils::AddEdge(src_anchor, input_anchor),
      "Call GraphUtils::AddEdge failed to add edge between op[%s] and CacheUpdate.",
      src_op->GetName().c_str())
  AICPU_CHECK_RES_WITH_LOG(GraphUtils::AddEdge(output_anchor, dst_anchor),
      "Call GraphUtils::AddEdge failed to add edge CacheUpdate and op[%s].",
      dst_op->GetName().c_str())
  return ge::SUCCESS;
}

Status CacheGraph::GenerateNoCacheGraph(ComputeGraph &graph) {
  for (const NodePtr &curr_node : graph.GetDirectNode()) {
    AICPU_CHECK_NOTNULL(curr_node)
    OpDescPtr curr_op_desc_ptr = curr_node->GetOpDesc();
    AICPU_CHECK_NOTNULL(curr_op_desc_ptr)
    string op_type = curr_op_desc_ptr->GetType();
    // if op type is variable, replace it with assign and variable
    if (op_type == kPlaceHolderOpType) {
      string pld_front_node_engine;
      CHECK_RES_BOOL(
          AttrUtils::GetStr(curr_op_desc_ptr, ge::ATTR_NAME_PLD_FRONT_NODE_ENGINE_NAME, pld_front_node_engine),
          ErrorCode::GET_ATTR_FAILED,
          AICPU_REPORT_CALL_ERROR("Call ge::AttrUtils::GetStr faield to get attr[%s], op[%s].",
                                  ge::ATTR_NAME_PLD_FRONT_NODE_ENGINE_NAME.c_str(),
                                  curr_op_desc_ptr->GetName().c_str()));

      // if front name engine is aicore enginge or vector engine, insert cache
      // update op
      if (pld_front_node_engine == "AIcoreEngine" ||
          pld_front_node_engine == "VectorEngine") {
        // placeholder op just have one output edge
        OutDataAnchorPtr src_anchor = curr_node->GetOutDataAnchor(0);
        AICPU_CHECK_NOTNULL(src_anchor)
        auto nodes_and_anchors = curr_node->GetOutDataNodesAndAnchors();
        AICPU_IF_BOOL_EXEC(nodes_and_anchors.empty(),
            AICPUE_LOGI("no output data adge, op[%s]",
                curr_node->GetName().c_str());
            continue)
        InDataAnchorPtr dst_anchor = nodes_and_anchors.at(0).second;
        AICPU_CHECK_NOTNULL(dst_anchor)
        AICPU_CHECK_NOTNULL(nodes_and_anchors.at(0).first)

        bool sgt_flag = false;
        ffts::ThreadSliceMapPtr slice_info_ptr = nullptr;
        (void)GraphOptimizerUtils::CheckIsFftsPlus(curr_op_desc_ptr, slice_info_ptr, sgt_flag);

        AICPU_CHECK_RES_WITH_LOG(
            CreateAndInsertCacheUpdate(src_anchor, dst_anchor, graph, slice_info_ptr, sgt_flag),
            "Call CreateAndInsertCacheUpdatef failed to insert CacheUpdate op"
            " between op[%s] and op[%s].", curr_node->GetName().c_str(),
            nodes_and_anchors.at(0).first->GetName().c_str())
      }
    }
    if (op_type == kEndOpType) {
      string end_rear_node_engine;
      CHECK_RES_BOOL(AttrUtils::GetStr(curr_op_desc_ptr, ge::ATTR_NAME_END_REAR_NODE_ENGINE_NAME, end_rear_node_engine),
                     ErrorCode::GET_ATTR_FAILED,
                     AICPU_REPORT_CALL_ERROR("Call ge::AttrUtils::GetStr failed to get attr[%s], op[%s].",
                                             ge::ATTR_NAME_END_REAR_NODE_ENGINE_NAME.c_str(),
                                             curr_op_desc_ptr->GetName().c_str()));

      string parent_op_type;
      // get attr parentOpType
      CHECK_RES_BOOL(
          AttrUtils::GetStr(curr_op_desc_ptr, "parentOpType", parent_op_type),
          ErrorCode::GET_ATTR_FAILED,
          AICPU_REPORT_CALL_ERROR(
              "Call ge::AttrUtils::GetStr failed to get attr[parentOpType], op[%s].",
              curr_op_desc_ptr->GetName().c_str()));

      // if rear name engine is aicore enginge or parent op is End, insert cache
      // update op
      if (end_rear_node_engine == "AIcoreEngine" || parent_op_type == "NetOutput") {
        auto nodes_and_anchors = curr_node->GetInDataNodesAndAnchors();
        AICPU_IF_BOOL_EXEC(nodes_and_anchors.empty(),
            AICPUE_LOGI("no in data adge, op[%s]",
                curr_node->GetName().c_str());
            continue)
        OutDataAnchorPtr src_anchor = nodes_and_anchors.at(0).second;
        AICPU_CHECK_NOTNULL(src_anchor)
        InDataAnchorPtr dst_anchor = curr_node->GetInDataAnchor(0);
        AICPU_CHECK_NOTNULL(dst_anchor)
        AICPU_CHECK_NOTNULL(nodes_and_anchors.at(0).first)
        AICPU_CHECK_RES_WITH_LOG(
            CreateAndInsertCacheUpdate(src_anchor, dst_anchor, graph, nullptr, false),
            "Call CreateAndInsertCacheUpdate failed to insert CacheUpdate op"
            " between op[%s] and op[%s].",
            nodes_and_anchors.at(0).first->GetName().c_str(),
            curr_node->GetName().c_str())
      }
    }
  }
  return SUCCESS;
}

ge::Status AutoCastGraph::GenerateAutoCastGraph(ge::ComputeGraph &graph,
                                                const std::map<std::string, OpFullInfo> &all_op_info) {
  for (ge::NodePtr &node : graph.GetDirectNode()) {
    AICPU_CHECK_NOTNULL(node)
    ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
    AICPU_CHECK_NOTNULL(op_desc_ptr)
    std::string op_type = op_desc_ptr->GetType();
    AICPU_IF_BOOL_EXEC(
        ((op_type == kPlaceHolderOpType) || (op_type == kEndOpType) || (op_type == kFunctionOp)),
        AICPUE_LOGI("Current op type is [%s]. Don't need to AutoCast.",
                    op_type.c_str());
        continue)
    // if op type is framework_op, get original op
    AICPU_IF_BOOL_EXEC(
        (op_type == kFrameworkOp),
        AICPU_CHECK_RES(GetFrameworkOpType(op_desc_ptr, op_type)))
    std::map<std::string, OpFullInfo>::const_iterator iter = all_op_info.find(op_type);
    if (iter == all_op_info.end()) {
      continue;
    }

    bool sgt_flag = false;
    ffts::ThreadSliceMapPtr slice_info_ptr = nullptr;
    (void)GraphOptimizerUtils::CheckIsFftsPlus(op_desc_ptr, slice_info_ptr, sgt_flag);

    OpFullInfo op_full_info = iter->second;
    auto cast_src_type = op_full_info.castSrcType;
    auto cast_dst_type = op_full_info.castDstType;
    // auto cast for input
    size_t input_num = op_desc_ptr->GetInputsSize();
    for (size_t i = 0; i < input_num; ++i) {
      // get src_to_dst_type map
      string input_real_name = "input" + to_string(i);
      vector<DataType> src_data_type;
      GetDataType(cast_src_type, input_real_name, src_data_type);
      vector<DataType> dst_data_type;
      GetDataType(cast_dst_type, input_real_name, dst_data_type);
      AICPU_IF_BOOL_EXEC(
        (src_data_type.size() != dst_data_type.size()),
        AICPU_REPORT_INNER_ERROR("Src data type size[%lu] is not equal to dst data type size[%lu], please check!",
                                 src_data_type.size(), dst_data_type.size());
        return ErrorCode::SRC_DST_SIZE_ERROR)
      auto cast_size = src_data_type.size();
      map<DataType, DataType> src_to_dst_type;
      for (size_t j = 0; j < cast_size; ++j) {
        src_to_dst_type[src_data_type[j]] = dst_data_type[j];
      }
      // insert cast op if rules are set
      auto input_tensor_desc = op_desc_ptr->GetInputDesc(i);
      auto input_type = input_tensor_desc.GetDataType();
      map<DataType, DataType>::const_iterator iter1 = src_to_dst_type.find(input_type);
      if (iter1 != src_to_dst_type.end()) {
        DataType dst_type = iter1->second;
        InDataAnchorPtr dst_anchor = node->GetInDataAnchor(i);
        AICPU_CHECK_NOTNULL(dst_anchor)
        AICPU_CHECK_RES_WITH_LOG(
            InsertCastForInput(dst_anchor, graph, dst_type, slice_info_ptr, sgt_flag),
            "Insert Cast op for input[%zu] of op[%s] failed.", i, node->GetName().c_str())
      }
    }
    // auto cast for output
    size_t output_num = op_desc_ptr->GetOutputsSize();
    for (size_t i = 0; i < output_num; ++i) {
      // get dst_to_src_type map
      string output_real_name = "output" + to_string(i);
      vector<DataType> src_data_type;
      GetDataType(cast_src_type, output_real_name, src_data_type);
      vector<DataType> dst_data_type;
      GetDataType(cast_dst_type, output_real_name, dst_data_type);
      auto cast_size = src_data_type.size() < dst_data_type.size() ? src_data_type.size() : dst_data_type.size();
      map<DataType, DataType> dst_to_src_type;
      for (size_t j = 0; j < cast_size; ++j) {
        dst_to_src_type[dst_data_type[j]] = src_data_type[j];
      }
      // insert cast op if rules are set
      auto output_tensor_desc = op_desc_ptr->GetOutputDesc(i);
      auto output_type = output_tensor_desc.GetDataType();
      map<DataType, DataType>::const_iterator iter2 = dst_to_src_type.find(output_type);
      if (iter2 != dst_to_src_type.end()) {
        DataType src_type = iter2->second;
        OutDataAnchorPtr src_anchor = node->GetOutDataAnchor(i);
        AICPU_CHECK_NOTNULL(src_anchor)
        AICPU_CHECK_RES_WITH_LOG(
            InsertCastForOutput(src_anchor, graph, src_type, output_type),
            "Insert Cast op for output[%zu] of op[%s] failed.", i, node->GetName().c_str())
      }
    }
  }
  return ge::SUCCESS;
}

ge::Status AutoCastGraph::InsertCastForInput(const ge::InDataAnchorPtr &dst_anchor, ge::ComputeGraph &graph,
                                             ge::DataType dst_type,
                                             const ffts::ThreadSliceMapPtr &slice_info_ptr, const bool &sgt_flag) {
  // get src op desc
  OutDataAnchorPtr src_anchor = dst_anchor->GetPeerOutAnchor();
  AICPU_CHECK_NOTNULL(src_anchor)
  NodePtr src_node = src_anchor->GetOwnerNode();
  AICPU_CHECK_NOTNULL(src_node)
  OpDescPtr src_op = src_node->GetOpDesc();
  AICPU_CHECK_NOTNULL(src_op)
  int src_anchor_idx = AnchorUtils::GetIdx(src_anchor);
  CHECK_RES_BOOL(src_anchor_idx >= 0, FAILED,
      AICPU_REPORT_INNER_ERROR("Invalid output anchor index[%d] of op[%s]", src_anchor_idx, src_op->GetName().c_str()))
  GeTensorDesc src_tensor_desc = src_op->GetOutputDesc(src_anchor_idx);
  // get dst op desc
  NodePtr dst_node = dst_anchor->GetOwnerNode();
  AICPU_CHECK_NOTNULL(dst_node)
  OpDescPtr dst_op = dst_node->GetOpDesc();
  AICPU_CHECK_NOTNULL(dst_op)
  int dst_anchor_idx = AnchorUtils::GetIdx(dst_anchor);
  CHECK_RES_BOOL(
      dst_anchor_idx >= 0, FAILED,
      AICPU_REPORT_INNER_ERROR("Invalid input anchor index[%d] of op[%s]",
          dst_anchor_idx, dst_op->GetName().c_str()))
  GeTensorDesc dst_tensor_desc = dst_op->GetInputDesc(dst_anchor_idx);
  // update input desc of dst node
  dst_tensor_desc.SetDataType(dst_type);
  AICPU_CHECK_RES_WITH_LOG(dst_op->UpdateInputDesc(dst_anchor_idx, dst_tensor_desc),
                           "Dst op[%s] update input[%d] desc failed.", dst_op->GetName().c_str(), dst_anchor_idx)
  // create Cast op
  auto cast_op = op::Cast();
  OpDescPtr cast_op_desc = OpDescUtils::GetOpDescFromOperator(cast_op);
  AICPU_CHECK_NOTNULL(cast_op_desc)
  (void)ge::AttrUtils::SetInt(cast_op_desc, "dst_type", dst_type);
  cast_op_desc->SetType("Cast");
  std::string op_name = src_op->GetName() + "_" + cast_op_desc->GetName();
  cast_op_desc->SetName(op_name);
  cast_op_desc->SetOpEngineName(dst_op->GetOpEngineName());
  cast_op_desc->SetOpKernelLibName(dst_op->GetOpKernelLibName());
  AICPU_CHECK_RES_WITH_LOG(cast_op_desc->UpdateInputDesc(0, src_tensor_desc),
                           "Cast update input 0 desc failed.")
  AICPU_CHECK_RES_WITH_LOG(cast_op_desc->UpdateOutputDesc(0, dst_tensor_desc), "Cast update output 0 desc failed.")
  if ((sgt_flag) && (slice_info_ptr != nullptr)) {
    (void)cast_op_desc->SetExtAttr(kAttrNameSgtStruct, slice_info_ptr);
    (void)ge::AttrUtils::SetInt(cast_op_desc, kAttrNameThreadScopeId, slice_info_ptr->thread_scope_id);
  }
  // insert Cast op
  NodePtr cast_node = graph.AddNode(cast_op_desc);
  AICPU_CHECK_NOTNULL(cast_node)
  AICPU_CHECK_RES_WITH_LOG(GraphUtils::RemoveEdge(src_anchor, dst_anchor),
                           "Remove edge between op[%s] and op[%s] failed.",
                           src_op->GetName().c_str(), dst_op->GetName().c_str())
  InDataAnchorPtr input_anchor = cast_node->GetInDataAnchor(0);
  AICPU_CHECK_NOTNULL(input_anchor)
  OutDataAnchorPtr output_anchor = cast_node->GetOutDataAnchor(0);
  AICPU_CHECK_NOTNULL(output_anchor)
  AICPU_CHECK_RES_WITH_LOG(GraphUtils::AddEdge(src_anchor, input_anchor),
                           "Add edge between op[%s] and Cast failed.",
                           src_op->GetName().c_str())
  AICPU_CHECK_RES_WITH_LOG(GraphUtils::AddEdge(output_anchor, dst_anchor),
                           "Add edge between Cast and op[%s] failed.",
                           dst_op->GetName().c_str())
  AICPUE_LOGI("Insert Cast op between op[%s] and op[%s] success.", src_op->GetName().c_str(),
              dst_op->GetName().c_str());
  return ge::SUCCESS;
}

ge::Status AutoCastGraph::InsertCastForOutput(
    const ge::OutDataAnchorPtr &src_anchor, ge::ComputeGraph &graph, ge::DataType src_type, ge::DataType dst_type) {
  // get src op desc
  NodePtr src_node = src_anchor->GetOwnerNode();
  AICPU_CHECK_NOTNULL(src_node)
  OpDescPtr src_op = src_node->GetOpDesc();
  AICPU_CHECK_NOTNULL(src_op)
  int src_anchor_idx = AnchorUtils::GetIdx(src_anchor);
  CHECK_RES_BOOL(
      src_anchor_idx >= 0, FAILED,
      AICPU_REPORT_INNER_ERROR("Invalid output anchor index[%d] of op[%s]",
          src_anchor_idx, src_op->GetName().c_str()))
  GeTensorDesc src_tensor_desc = src_op->GetOutputDesc(src_anchor_idx);
  // update output desc of src node
  src_tensor_desc.SetDataType(src_type);
  AICPU_CHECK_RES_WITH_LOG(
      src_op->UpdateOutputDesc(src_anchor_idx, src_tensor_desc),
      "Src op[%s] update output[%d] desc failed.", src_op->GetName().c_str(), src_anchor_idx)
  // create Cast op
  auto cast_op = op::Cast();
  OpDescPtr cast_op_desc = OpDescUtils::GetOpDescFromOperator(cast_op);
  AICPU_CHECK_NOTNULL(cast_op_desc)
  (void)ge::AttrUtils::SetInt(cast_op_desc, "dst_type", dst_type);
  cast_op_desc->SetType("Cast");
  std::string op_name = src_op->GetName() + "_" + cast_op_desc->GetName();
  cast_op_desc->SetName(op_name);
  cast_op_desc->SetOpEngineName(src_op->GetOpEngineName());
  cast_op_desc->SetOpKernelLibName(src_op->GetOpKernelLibName());
  AICPU_CHECK_RES_WITH_LOG(cast_op_desc->UpdateInputDesc(0, src_tensor_desc),
                           "Cast update input 0 desc failed.")
  src_tensor_desc.SetDataType(dst_type);
  AICPU_CHECK_RES_WITH_LOG(
      cast_op_desc->UpdateOutputDesc(0, src_tensor_desc),
      "Cast update output 0 desc failed.")
  // insert Cast op
  NodePtr cast_node = graph.AddNode(cast_op_desc);
  AICPU_CHECK_NOTNULL(cast_node)
  InDataAnchorPtr input_anchor = cast_node->GetInDataAnchor(0);
  AICPU_CHECK_NOTNULL(input_anchor)
  OutDataAnchorPtr output_anchor = cast_node->GetOutDataAnchor(0);
  AICPU_CHECK_NOTNULL(output_anchor)
  for (auto dst_anchor: src_anchor->GetPeerInDataAnchors()) {
    AICPU_CHECK_NOTNULL(dst_anchor)
    AICPU_CHECK_NOTNULL(dst_anchor->GetOwnerNode())
    AICPU_CHECK_RES_WITH_LOG(GraphUtils::RemoveEdge(src_anchor, dst_anchor),
                             "Remove edge between op[%s] and op[%s] failed.",
                             src_op->GetName().c_str(), dst_anchor->GetOwnerNode()->GetName().c_str())
    AICPU_CHECK_RES_WITH_LOG(GraphUtils::AddEdge(output_anchor, dst_anchor),
                             "Add edge between op[%s] and op[%s] failed.",
                             cast_node->GetName().c_str(), dst_anchor->GetOwnerNode()->GetName().c_str())
  }
  AICPU_CHECK_RES_WITH_LOG(GraphUtils::AddEdge(src_anchor, input_anchor),
                           "Add edge between op[%s] and Cast failed.",
                           src_op->GetName().c_str())
  AICPUE_LOGI("Insert Cast op for output[%d] of op[%s] success.", src_anchor_idx, src_op->GetName().c_str());
  return ge::SUCCESS;
}

ge::Status AutoCastGraph::GetFrameworkOpType(OpDescPtr &op_desc_ptr, string &op_type) {
  // op_desc_ptr already check not null
  string original_type;
  CHECK_RES_BOOL(AttrUtils::GetStr(op_desc_ptr, kOriginalType, original_type),
      ErrorCode::GET_ATTR_FAILED,
      AICPU_REPORT_CALL_ERROR(
          "Call ge::AttrUtils::GetStr failed to get attr[%s], op[%s].",
          kOriginalType.c_str(), op_desc_ptr->GetName().c_str()))
  if (original_type.empty()) {
    AICPU_REPORT_INNER_ERROR("Attr[%s] is empty, op[%s].", kOriginalType.c_str(),
        op_desc_ptr->GetName().c_str());
    return ErrorCode::STR_IS_EMPTY;
  }
  op_desc_ptr->SetType(original_type);
  op_type = original_type;
  return ge::SUCCESS;
}
}  // namespace aicpu
