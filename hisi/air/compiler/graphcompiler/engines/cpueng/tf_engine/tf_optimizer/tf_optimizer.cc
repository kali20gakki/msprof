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
#include "tf_optimizer.h"
#include "tf_function_builder.h"
#include "util/tf_util.h"
#include "tf_kernel_info/tf_kernel_info.h"
#include "util/constant.h"
#include "config/config_file.h"
#include "ir2tf/ir2tf_parser_factory.h"
#include "error_code/error_code.h"
#include "aicpu_graph_optimizer/graph_optimizer_utils.h"
#include "tf_optimizer_utils.h"

#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/type_utils.h"
#include "common/util/error_manager/error_manager.h"

using domi::tensorflow::NodeDef;
using domi::tensorflow::FunctionDefLibrary;

namespace {
const std::string kPlaceholderOpType = "PlaceHolder";
const std::string kFunctionOp = "FunctionOp";
const std::string kFrameworkOp = "FrameworkOp";
const std::string kEndOpType = "End";
const std::string kPhonyConcatOp = "PhonyConcat";
constexpr uint64_t kDefaultOpFusionMinNumber = 2;
constexpr uint64_t kTfDebugModeOff = 0;
constexpr uint64_t kTfDebugModeOn = 1;
}

namespace aicpu {
OptimizerPtr TfOptimizer::instance_ = nullptr;

OptimizerPtr TfOptimizer::Instance() {
  static std::once_flag flag;
  std::call_once(flag, [&]() {
    instance_.reset(new (std::nothrow) TfOptimizer);
  });
  return instance_;
}

ge::Status TfOptimizer::Initialize() {
  InitTfDebugMode();
  InitOpFusionMinNum();
  Optimizer::InitOpCheckMode();
  return InitializeIr2TfParser();
}

ge::Status TfOptimizer::InitializeIr2TfParser() const {
  ge::shared_ptr<Ir2tfBaseParser> parser = Ir2tfBaseParser::Instance();
  AICPU_IF_BOOL_EXEC(parser == nullptr,
      AICPU_REPORT_INNER_ERROR("Create Ir2tfBaseParser object failed.");
      return ErrorCode::GET_IR2TF_PARSER_FAILED)
  return parser->LoadMappingConfig();
}

void TfOptimizer::InitOpFusionMinNum() {
  // get OpFusionMinNum from config file.
  std::string op_fusion_min_num;
  if (ConfigFile::GetInstance().GetValue(kOpFusionMinNum, op_fusion_min_num)) {
    uint64_t result = kDefaultOpFusionMinNumber;
    if (StringToNum(op_fusion_min_num, result).state != ge::SUCCESS) {
      AICPUE_LOGW("Tran op_fusion_min_num [%s] to integer failed. default value is 2.",
                  op_fusion_min_num.c_str());
      return;
    }
    // if OpFusionMinNum from config file is less than 2, print warning log.
    if (result < kDefaultOpFusionMinNumber) {
      AICPUE_LOGW("OpFusionMinNum is [%s], default value is 2.", op_fusion_min_num.c_str());
      return;
    }
    op_fusion_min_num_ = result;
    return;
  }
  AICPUE_LOGW("Get OpFusionMinNum from config file failed. default value is 2.");
}

void TfOptimizer::InitTfDebugMode() {
  // get TfDebugMode from config file
  std::string tf_debug_mode;
  if (ConfigFile::GetInstance().GetValue(kTfDebugMode, tf_debug_mode)) {
    uint64_t result = kTfDebugModeOff;
    if (StringToNum(tf_debug_mode, result).state != ge::SUCCESS) {
      AICPUE_LOGW("Tran tf_debug_mode [%s] to integer failed. default value is 0.",
                  tf_debug_mode.c_str());
      return;
    }
    if (result == kTfDebugModeOn) {
      AICPUE_LOGI("TfDebugMode is on.");
      tf_debug_mode_ = true;
      return;
    }
    AICPUE_LOGI("TfDebugMode is off.");
    return;
  }
  AICPUE_LOGW("Get tf_debug_mode from config file failed. tf debug mode is off.");
}

bool CheckFftsPlusSubGraph(const ge::ComputeGraph &graph) {
  for (const ge::NodePtr &node : graph.GetDirectNode()) {
    if (node == nullptr) {
      return false;
    }
    ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
    if (op_desc_ptr == nullptr) {
      return false;
    }
    std::string op_type = op_desc_ptr->GetType();
    if (op_type == kPlaceholderOp || op_type == kEndOp || op_type == kPhonyConcatOp) {
      continue;
    }
    bool sgt_flag = false;
    ffts::ThreadSliceMapPtr slice_info_ptr = nullptr;
    (void)GraphOptimizerUtils::CheckIsFftsPlus(op_desc_ptr, slice_info_ptr, sgt_flag);
    if (sgt_flag) {
      return true;
    }
  }
  return false;
}

void TfOptimizer::UpdataOpInfos(ge::ComputeGraph &graph,
                                const std::map<std::string, OpFullInfo> &all_op_info) const {
  for (const ge::NodePtr &node : graph.GetDirectNode()) {
    (void)UpdataOpInfo(*node, all_op_info);
  }
}

ge::Status TfOptimizer::OptimizeFusedGraph(ge::ComputeGraph &graph,
                                           const std::map<std::string, OpFullInfo> &all_op_info) const {
  std::string graph_name = graph.GetName();
  AICPU_CHECK_RES_WITH_LOG(TfVariableGraph::GenerateTfVariableGraph(graph),
      "Call GenerateTfVariableGraph function failed. graph[%s].", graph_name.c_str())
  std::string suffix = "After_Aicpu_Accept_Variable";
  GraphOptimizerUtils::DumpGraph(graph, suffix);
  auto status = graph.TopologicalSorting();
  CHECK_RES_BOOL((status == ge::GRAPH_SUCCESS), ge::FAILED,
      AICPU_REPORT_CALL_ERROR(
          "Call ge::ComputeGraph::TopologicalSorting failed, graph[%s]",
          graph_name.c_str()))

  UpdataOpInfos(graph, all_op_info);

  // step1: make for fusion
  std::unordered_map<std::string, std::vector<ge::NodePtr>> tf_node_cluster_map;
  std::unordered_map<std::string, ge::NodePtr> tf_isolated_node_map;
  if (CheckFftsPlusSubGraph(graph)) {
    // ffts+
    AICPU_CHECK_RES_WITH_LOG(MarkNodeForFusionOfFfts(graph, all_op_info, tf_node_cluster_map, tf_isolated_node_map),
                             "Call TfOptimizer::MarkNodeForFusionOfFfts function failed. graph[%s].",
                             graph_name.c_str())
  } else {
    AICPU_CHECK_RES_WITH_LOG(MarkNodeForFusion(graph, all_op_info, tf_node_cluster_map, tf_isolated_node_map),
                             "Call TfOptimizer::MarkNodeForFusion function failed. graph[%s].", graph_name.c_str())
  }

  // step2: update graph with new fused node
  for (auto iter = tf_node_cluster_map.begin(); iter != tf_node_cluster_map.end(); ++iter) {
    AICPU_CHECK_RES_WITH_LOG(OptimizeNodeCluster(graph, iter->second, tf_isolated_node_map),
        "Call TfOptimizer::OptimizeNodeCluster function failed. op[%s], graph[%s].",
        (iter->first).c_str(), graph_name.c_str())
  }

  // step3: delete old node from graph
  for (auto iter = tf_node_cluster_map.begin(); iter != tf_node_cluster_map.end(); ++iter) {
    for (ge::NodePtr node : iter->second) {
      AICPU_CHECK_RES_WITH_LOG(graph.RemoveNode(node),
          "Call ge::ComputeGraph::ComputeGraphRemove function failed to remove "
          "op[%s]. graph[%s].", node->GetName().c_str(), graph_name.c_str())
    }
  }

  // step4: update isolated node for graph
  for (auto iter = tf_isolated_node_map.begin(); iter != tf_isolated_node_map.end(); ++iter) {
    AICPU_CHECK_RES_WITH_LOG(OptimizeIsolatedNode(iter->second),
        "Call TfOptimizer::OptimizeIsolatedNode function failed. op[%s], graph[%s].",
        (iter->first).c_str(), graph_name.c_str())
  }
  return ge::SUCCESS;
}

bool TfOptimizer::CheckIsFunctionOp(ge::OpDescPtr &op_desc) const {
  AICPU_CHECK_NOTNULL_ERRCODE(op_desc, false)
  std::string op_type = op_desc->GetType();
  if (op_type == kFunctionOp) {
    return true;
  }
  if (op_type == kFrameworkOp) {
    // framework op with function
    ge::Buffer func_def_bytes;
    if (ge::AttrUtils::GetBytes(op_desc, kTfFuncDef, func_def_bytes)) {
      op_desc->SetType(kFunctionOp);
      return true;
    }
    // framework op without function
    return false;
  }
  return false;
}

ge::Status TfOptimizer::CheckOpsFusion(ge::OpDescPtr &op_desc_ptr, const std::map<std::string, OpFullInfo> &all_op_info,
                                       bool &fuse_flag) const {
  std::string op_type = op_desc_ptr->GetType();
  fuse_flag = false;
  // async op flag
  bool op_async_flag = false;
  // tf op flag, FunctionOp/FrameworkOp/tfOp
  bool tf_op_flag = false;
  AICPU_CHECK_RES_WITH_LOG(GetOpFlagAndAsyncFlag(op_desc_ptr, all_op_info, tf_op_flag, op_async_flag),
                           "MarkNodeForFusion GetOpFlagAndAsyncFlag failed, op_type [%s]", op_type.c_str())
  // if op is an async op, can't be fused
  if (!op_async_flag) {
    if (tf_debug_mode_ || tf_op_flag) {
      fuse_flag = true;
    }
    bool aicpu_private = false;
    if (ge::AttrUtils::GetBool(op_desc_ptr, kAicpuPrivate, aicpu_private)) {
      if (aicpu_private) {
        fuse_flag = true;
      }
    }
  }
  return ge::SUCCESS;
}

ge::Status TfOptimizer::SeparateDiffOpsToMap(
    const std::vector<ge::NodePtr> &tf_node_cluster,
    std::unordered_map<std::string, std::vector<ge::NodePtr>> &tf_node_cluster_map,
    std::unordered_map<std::string, ge::NodePtr> &tf_isolated_node_map) const {
  if (static_cast<uint64_t>(tf_node_cluster.size()) >= op_fusion_min_num_) {
    std::vector<ge::NodePtr> tf_node_cluster_tmp;
    tf_node_cluster_tmp.assign(tf_node_cluster.begin(), tf_node_cluster.end());
    if (!tf_node_cluster_tmp.empty()) {
      tf_node_cluster_map[tf_node_cluster_tmp[0]->GetName()] = tf_node_cluster_tmp;
    }
  } else {
    for (auto iter = tf_node_cluster.begin(); iter != tf_node_cluster.end(); ++iter) {
      tf_isolated_node_map[(*iter)->GetName()] = *iter;
    }
  }
  return ge::SUCCESS;
}

ge::Status TfOptimizer::GetOutNoSafeEdgeList(const ge::NodePtr &node, bool &no_safe_flag) const {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  AICPU_CHECK_NOTNULL(op_desc_ptr)
  // traverse all output data anchors of current node
  for (const ge::OutDataAnchorPtr &out_data_anchor : node->GetAllOutDataAnchors()) {
    AICPU_CHECK_NOTNULL(out_data_anchor)
    int out_index = out_data_anchor->GetIdx();
    ge::GeShape out_shape = op_desc_ptr->GetOutputDescPtr(out_index)->GetShape();
    for (const ge::InDataAnchorPtr &peer_in_anchor : out_data_anchor->GetPeerInDataAnchors()) {
      AICPU_CHECK_NOTNULL(peer_in_anchor)
      auto peer_node = peer_in_anchor->GetOwnerNode();
      AICPU_CHECK_NOTNULL(peer_node)
      ge::OpDescPtr peer_op_desc_ptr = peer_node->GetOpDesc();
      AICPU_CHECK_NOTNULL(peer_op_desc_ptr)

      // if op type is PhonyConcat, it can not fuse
      if ((peer_op_desc_ptr->GetType() == "PhonyConcat") || (node->GetOpDesc()->GetType() == "PhonyConcat")) {
        no_safe_flag = true;
        continue;
      }

      int in_index = peer_in_anchor->GetIdx();
      ge::GeShape in_shape = peer_op_desc_ptr->GetInputDescPtr(in_index)->GetShape();
      // if the shape of current op output is different with peer op input , it can not fuse
      if (!IsSameShape(out_shape, in_shape)) {
        no_safe_flag = true;
        continue;
      }
    }
  }
  return ge::SUCCESS;
}

ge::Status TfOptimizer::MarkNodeForFusionOfFfts(
    const ge::ComputeGraph &graph, const std::map<std::string, OpFullInfo> &all_op_info,
    std::unordered_map<std::string, std::vector<ge::NodePtr>> &tf_node_cluster_map,
    std::unordered_map<std::string, ge::NodePtr> &tf_isolated_node_map) const {
  std::vector<ge::NodePtr> tf_node_cluster;
  bool no_safe_flag = false;
  // traverse all topological sorted nodes in graph
  for (const ge::NodePtr &node : graph.GetDirectNode()) {
    AICPU_CHECK_NOTNULL(node)
    ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
    AICPU_CHECK_NOTNULL(op_desc_ptr)

    // if op type is placeholder or end, skip it
    std::string op_type = op_desc_ptr->GetType();
    AICPU_IF_BOOL_EXEC(((op_type == kPlaceholderOpType) || (op_type == kEndOpType) || (op_type == kPhonyConcatOp)),
                       AICPUE_LOGD("Current op type is [%s]. Don't need to fuse.", op_type.c_str());
                       continue)

    bool fuse_flag = false;
    AICPU_CHECK_RES_WITH_LOG(CheckOpsFusion(op_desc_ptr, all_op_info, fuse_flag),
                             "MarkNodeForFusionOfFfts CheckOpsFusion failed, op_type [%s]", op_type.c_str())

    bool sgt_flag = false;
    ffts::ThreadSliceMapPtr slice_info_ptr = nullptr;
    (void)GraphOptimizerUtils::CheckIsFftsPlus(op_desc_ptr, slice_info_ptr, sgt_flag);
    AICPU_CHECK_NOTNULL(slice_info_ptr)

    bool thread_mode = slice_info_ptr->thread_mode;
    // auto threading
    if (thread_mode) {
      // original process
      if (fuse_flag) {
        tf_node_cluster.emplace_back(node);
      } else {
        // if size > op_fusion_min_num_, need to fuse; else, ge IR trans to tf
        AICPU_CHECK_RES_WITH_LOG(SeparateDiffOpsToMap(tf_node_cluster, tf_node_cluster_map, tf_isolated_node_map),
                                 "MarkNodeForFusionOfFfts SeparateDiffOpsToMap failed.")
        tf_node_cluster.clear();
      }
    } else {  // manual threading
      if ((fuse_flag) && (!no_safe_flag)) {
        AICPU_CHECK_RES_WITH_LOG(GetOutNoSafeEdgeList(node, no_safe_flag),
                                 "MarkNodeForFusionOfFfts GetOutNoSafeEdgeList failed, op_type [%s]", op_type.c_str())
        tf_node_cluster.emplace_back(node);
      } else {
        tf_isolated_node_map[node->GetName()] = node;
      }
    }
  }
  if (no_safe_flag) {
    for (auto iter = tf_node_cluster.begin(); iter != tf_node_cluster.end(); ++iter) {
      tf_isolated_node_map[(*iter)->GetName()] = *iter;
    }
    return ge::SUCCESS;
  }

  // if size > op_fusion_min_num_, need to fuse; else, ge IR trans to tf
  AICPU_CHECK_RES_WITH_LOG(SeparateDiffOpsToMap(tf_node_cluster, tf_node_cluster_map, tf_isolated_node_map),
                           "MarkNodeForFusionOfFfts SeparateDiffOpsToMap failed.")
  return ge::SUCCESS;
}

ge::Status TfOptimizer::GetOpFlagAndAsyncFlag(ge::OpDescPtr &op_desc_ptr,
                                              const std::map<std::string, OpFullInfo> &all_op_info,
                                              bool &tf_op_flag, bool &op_async_flag) const {
  std::string op_type = op_desc_ptr->GetType();
  op_async_flag = false;
  tf_op_flag = CheckIsFunctionOp(op_desc_ptr);
  if (tf_op_flag) {
    return ge::SUCCESS;
  }
  if (op_type == kFrameworkOp) {
    auto ret = GetFrameworkOpType(op_desc_ptr, op_type);
    if (ret != ge::SUCCESS) {
      return ret;
    }
  }
  std::string kernel_lib_name = GetKernelLibNameByOpType(op_type, all_op_info);
  if ((kernel_lib_name == kTfKernelInfoChoice)) {
    tf_op_flag = true;
  }
  auto iter = all_op_info.find(op_type);
  // Get the flagasync configuration of the op in the operator information base
  if (iter != all_op_info.end()) {
    op_async_flag = iter->second.flagAsync;
  }
  return ge::SUCCESS;
}

ge::Status TfOptimizer::MarkNodeForFusion(const ge::ComputeGraph &graph, const std::map<std::string, OpFullInfo> &all_op_info,
                                          std::unordered_map<std::string, std::vector<ge::NodePtr>> &tf_node_cluster_map,
                                          std::unordered_map<std::string, ge::NodePtr> &tf_isolated_node_map) const {
  std::vector<ge::NodePtr> tf_node_cluster;
  // traverse all topological sorted nodes in graph
  for (const ge::NodePtr &node : graph.GetDirectNode()) {
    AICPU_CHECK_NOTNULL(node)
    ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
    AICPU_CHECK_NOTNULL(op_desc_ptr)
    // if op type is placeholder or end, skip it
    std::string op_type = op_desc_ptr->GetType();
    AICPU_IF_BOOL_EXEC(((op_type == kPlaceholderOpType) || (op_type == kEndOpType)),
                       AICPUE_LOGD("Current op type is [%s]. Don't need to fuse.", op_type.c_str());
                       continue)

    // async op flag
    bool op_async_flag = false;
    // tf op flag, FunctionOp/FrameworkOp/tfOp
    bool tf_op_flag = false;

    AICPU_CHECK_RES_WITH_LOG(GetOpFlagAndAsyncFlag(op_desc_ptr, all_op_info, tf_op_flag, op_async_flag),
                             "MarkNodeForFusion GetOpFlagAndAsyncFlag failed, op_type [%s]", op_type.c_str())
    // if op is an async op, can't be fused
    if (!op_async_flag) {
      if (tf_debug_mode_ || tf_op_flag) {
        tf_node_cluster.emplace_back(node);
        continue;
      }
      bool aicpu_private = false;
      if (ge::AttrUtils::GetBool(op_desc_ptr, kAicpuPrivate, aicpu_private)) {
        if (aicpu_private) {
          tf_node_cluster.emplace_back(node);
          continue;
        }
      }
    } else {
      tf_isolated_node_map[node->GetName()] = node;
    }

    // if size > op_fusion_min_num_, need to fuse; else, ge IR trans to tf
    if (static_cast<uint64_t>(tf_node_cluster.size()) >= op_fusion_min_num_) {
      std::vector<ge::NodePtr> tmp_tf_node_cluster;
      tmp_tf_node_cluster.assign(tf_node_cluster.begin(), tf_node_cluster.end());
      tf_node_cluster_map[tmp_tf_node_cluster[0]->GetName()] = tmp_tf_node_cluster;
    } else {
      for (auto iter = tf_node_cluster.begin(); iter != tf_node_cluster.end(); ++iter) {
        tf_isolated_node_map[(*iter)->GetName()] = *iter;
      }
    }
    tf_node_cluster.clear();
  }

  // if size > op_fusion_min_num_, need to fuse; else, ge IR trans to tf
  if (static_cast<uint64_t>(tf_node_cluster.size()) >= op_fusion_min_num_) {
    std::vector<ge::NodePtr> tmp_tf_node_cluster;
    tmp_tf_node_cluster.assign(tf_node_cluster.begin(), tf_node_cluster.end());
    if (!tmp_tf_node_cluster.empty()) {
      tf_node_cluster_map[tmp_tf_node_cluster[0]->GetName()] = tmp_tf_node_cluster;
    }
  } else {
    for (auto iter = tf_node_cluster.begin(); iter != tf_node_cluster.end(); ++iter) {
      tf_isolated_node_map[(*iter)->GetName()] = *iter;
    }
  }
  return ge::SUCCESS;
}

ge::Status TfOptimizer::OptimizeNodeCluster(
    ge::ComputeGraph &graph, std::vector<ge::NodePtr> &node_cluster,
    std::unordered_map<std::string, ge::NodePtr> &isolated_node_map) const {
  // if multi nodes in node_cluster, fuse nodes
  return FuseNodesForGraph(graph, node_cluster, isolated_node_map);
}

ge::Status TfOptimizer::CheckSubGraphSupportFuse(
    std::vector<ge::NodePtr> &node_cluster, const SubGraphInfo &sub_graph_info,
    std::unordered_map<std::string, ge::NodePtr> &isolated_node_map) const {
  std::vector<ge::InDataAnchorPtr> in_data_anchors = sub_graph_info.in_data_anchors;
  std::vector<ge::OutDataAnchorPtr> out_data_anchors = sub_graph_info.out_data_anchors;

  for (const auto input_anchor : in_data_anchors) {
    ge::NodePtr in_node;
    ge::OpDescPtr in_op_desc;
    uint32_t input_anchor_idx;
    std::string input_name;
    std::string in_op_type;
    std::set<std::string> refinput_set;

    in_node = input_anchor->GetOwnerNode();
    AICPU_CHECK_NOTNULL(in_node)
    in_op_desc = in_node->GetOpDesc();
    AICPU_CHECK_NOTNULL(in_op_desc)
    input_anchor_idx = static_cast<uint32_t>(input_anchor->GetIdx());
    input_name = in_op_desc->GetInputNameByIndex(input_anchor_idx);
    in_op_type = in_op_desc->GetType();

    (void)Ir2tfParserFactory::Instance().CreateIRParser(in_op_type)->GetRefInputSet(in_op_type, refinput_set);
    auto iter_name = refinput_set.find(input_name);
    if (iter_name != refinput_set.end()) {
      for (auto iter = node_cluster.begin(); iter != node_cluster.end(); ++iter) {
        isolated_node_map[(*iter)->GetName()] = *iter;
      }
      node_cluster.clear();
      return false;
    }
  }

  for (const auto output_anchor : out_data_anchors) {
    ge::NodePtr out_node;
    ge::OpDescPtr out_op_desc;
    uint32_t output_anchor_idx;
    std::string output_name;
    std::string out_op_type;
    std::set<std::string> refoutput_set;

    out_node = output_anchor->GetOwnerNode();
    AICPU_CHECK_NOTNULL(out_node)
    out_op_desc = out_node->GetOpDesc();
    AICPU_CHECK_NOTNULL(out_op_desc)
    output_anchor_idx = static_cast<uint32_t>(output_anchor->GetIdx());
    output_name = out_op_desc->GetOutputNameByIndex(output_anchor_idx);
    out_op_type = out_op_desc->GetType();

    (void)Ir2tfParserFactory::Instance().CreateIRParser(out_op_type)->GetRefOutputSet(out_op_type, refoutput_set);
    auto name_iter = refoutput_set.find(output_name);
    if (name_iter != refoutput_set.end()) {
      for (auto iter = node_cluster.begin(); iter != node_cluster.end(); ++iter) {
        isolated_node_map[(*iter)->GetName()] = *iter;
      }
      node_cluster.clear();
      return false;
    }
  }

  return true;
}

ge::Status TfOptimizer::FuseNodesForGraph(ge::ComputeGraph &graph, std::vector<ge::NodePtr> &node_cluster,
                                          std::unordered_map<std::string, ge::NodePtr> &isolated_node_map) const {
  if (node_cluster.empty()) {
    return ErrorCode::FUSE_NODES_FAILED;
  }
  AICPU_CHECK_NOTNULL(node_cluster[0])

  std::string node_cluster_name = node_cluster[0]->GetName();
  // step1: create empty sub graph
  ge::ComputeGraphPtr sub_graph = std::make_shared<ge::ComputeGraph>("subGraph");
  AICPU_IF_BOOL_EXEC(sub_graph == nullptr,
      AICPU_REPORT_INNER_ERROR("Create sub graph with node cluster[%s] failed.",
          node_cluster_name.c_str());
      return ErrorCode::FUSE_NODES_FAILED)

  SubGraphInfo sub_graph_info;
  // step2: insert nodes to sub graph
  AICPU_CHECK_RES_WITH_LOG(InsertNodesToSubGraph(sub_graph, node_cluster, sub_graph_info),
      "Call TfOptimizer::InsertNodesToSubGraph function failed to insert node "
      "cluster[%s] to sub graph[%s].", node_cluster_name.c_str(),
      sub_graph->GetName().c_str())

  if (!CheckSubGraphSupportFuse(node_cluster, sub_graph_info, isolated_node_map)) {
    AICPUE_LOGEVENT("SubGraph[%s] has ref input or output, it can not be fused", node_cluster_name.c_str());
    return ge::SUCCESS;
  }

  // step3: link nodes in sub graph
  AICPU_CHECK_RES_WITH_LOG(LinkInnerAnchorsForSubGraph(graph, sub_graph_info.new_node_map),
      "Call TfOptimizer::LinkInnerAnchorsForSubGraph function failed, node cluster[%s].",
      node_cluster_name.c_str())

  // step4: create new tf node
  std::unique_ptr<NodeDef> node_def(new(std::nothrow) NodeDef());
  AICPU_IF_BOOL_EXEC(node_def == nullptr,
      AICPU_REPORT_INNER_ERROR("Create tf node def failed. node cluster[%s].",
          node_cluster_name.c_str());
      return ErrorCode::FUSE_NODES_FAILED)

  // step5: create function def (trans sub graph to function def)
  std::unique_ptr<FunctionDefLibrary> function_def_lib(new(std::nothrow) FunctionDefLibrary());
  AICPU_IF_BOOL_EXEC(function_def_lib == nullptr,
      AICPU_REPORT_INNER_ERROR(
          "Create function def library failed. node cluster[%s].",
          node_cluster_name.c_str());
      return ErrorCode::FUSE_NODES_FAILED)

  AICPU_CHECK_RES_WITH_LOG(CollectNodeFuncs(node_cluster, function_def_lib.get()),
      "Call TfOptimizer::CollectNodeFuncs function failed. node cluster[%s].",
      node_cluster_name.c_str())

  AICPU_CHECK_RES_WITH_LOG(
      TfFunctionBuilder::GetInstance().BuildFunctionDef(
          sub_graph, node_cluster_name, function_def_lib.get(), node_def.get(),
          sub_graph_info.in_data_anchors, sub_graph_info.out_data_anchors),
      "Call TfFunctionBuilder::BuildFunctionDef function failed. node cluster[%s].",
      node_cluster_name.c_str())

  // step6: create new ge op desc
  std::string node_def_str;
  AICPU_CHECK_FALSE_EXEC(node_def->SerializeToString(&node_def_str),
      AICPU_REPORT_INNER_ERROR("Serialize nodedef for node[%s] failed.",
          node_cluster_name.c_str());
      return ErrorCode::FUSE_NODES_FAILED)

  std::string func_def_str;
  AICPU_CHECK_FALSE_EXEC(function_def_lib->SerializeToString(&func_def_str),
      AICPU_REPORT_INNER_ERROR(
          "Serialize function def library for node[%s] failed.",
          node_cluster_name.c_str());
      return ErrorCode::FUSE_NODES_FAILED)

  ge::OpDescPtr fused_op_desc = std::make_shared<ge::OpDesc>(node_cluster[0]->GetOpDesc()->GetName(), kFunctionOp);
  AICPU_IF_BOOL_EXEC(fused_op_desc == nullptr,
      AICPU_REPORT_INNER_ERROR("Create fused op desc failed, op[%s].",
          node_cluster_name.c_str());
      return ErrorCode::FUSE_NODES_FAILED)
  if (op_check_mode_) {
    std::vector<std::string> need_check_tf;
    for (auto node : node_cluster) {
      ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
      if (!CheckIsFunctionOp(op_desc_ptr)) {
        need_check_tf.push_back(node->GetOpDesc()->GetType());
      }
    }
    AICPU_CHECK_FALSE_EXEC(ge::AttrUtils::SetListStr(fused_op_desc, "needCheckTf", need_check_tf),
        AICPU_REPORT_INNER_ERROR("Set attr needCheckTf failed, op[%s].",
            node_cluster_name.c_str());
        return ErrorCode::FUSE_NODES_FAILED)
  }

  const uint8_t *func_def_data = reinterpret_cast<const uint8_t*>(func_def_str.data());
  AICPU_CHECK_FALSE_EXEC(
      ge::AttrUtils::SetZeroCopyBytes(fused_op_desc, kTfFuncDef, ge::Buffer::CopyFrom(func_def_data, func_def_str.length())),
      AICPU_REPORT_CALL_ERROR("Call ge::AttrUtils::SetZeroCopyBytes failed"
          " to set tf function def, op[%s].", node_cluster_name.c_str());
      return ErrorCode::FUSE_NODES_FAILED)
  AICPUE_LOGI("Create function def for node cluster[%s] success.", node_cluster_name.c_str());

  const uint8_t *node_def_data = reinterpret_cast<const uint8_t*>(node_def_str.data());
  AICPU_CHECK_FALSE_EXEC(
      ge::AttrUtils::SetZeroCopyBytes(fused_op_desc, kTfNodeDef, ge::Buffer::CopyFrom(node_def_data, node_def_str.length())),
      AICPU_REPORT_CALL_ERROR("Call ge::AttrUtils::SetZeroCopyBytes failed"
          " to set tf node def, op[%s].", node_cluster_name.c_str());
      return ErrorCode::FUSE_NODES_FAILED)
  // set attr async_flag
  AICPU_CHECK_FALSE_EXEC(
      ge::AttrUtils::SetBool(fused_op_desc, kAsyncFlag, false),
      AICPU_REPORT_CALL_ERROR("Set attr kAsyncFlag failed"
          " to set tf node def, op[%s].", node_cluster_name.c_str());
      return ErrorCode::FUSE_NODES_FAILED)

  AICPUE_LOGI("Create node def for node cluster[%s] success.",
              node_cluster_name.c_str());

  // value 3 represent the framework tensorflow
  AICPU_CHECK_FALSE_EXEC(ge::AttrUtils::SetInt(fused_op_desc, kFrameworkType, 3),
      AICPU_REPORT_CALL_ERROR(
          "Call ge::AttrUtils::SetInt failed to set framework type, op[%s].",
          node_cluster_name.c_str());
      return ErrorCode::FUSE_NODES_FAILED)

  // step 7: rebuild output and input desc for op desc of new fused node
  AICPU_CHECK_RES_WITH_LOG(RebuildOutputDesc(sub_graph_info.out_data_anchors, fused_op_desc),
      "Call TfOptimizer::RebuildOutputDesc function failed. op[%s].",
      node_cluster_name.c_str())

  AICPU_CHECK_RES_WITH_LOG(RebuildInputDesc(sub_graph_info.in_data_anchors, fused_op_desc),
      "Call TfOptimizer::RebuildInputDesc function failed. op[%s].",
      node_cluster_name.c_str())
  AICPU_CHECK_RES_WITH_LOG(SaveFusionNodeMappingRelations(node_cluster, sub_graph_info.out_data_anchors, fused_op_desc),
      "Call TfOptimizer::SaveFusionNodeMappingRelations function failed. op[%s].",
      node_cluster_name.c_str())

  // step8: add node and anchor to original graph after node has input and output tensor desc
  ge::NodePtr fused_node = graph.AddNode(fused_op_desc);
  AICPU_CHECK_NOTNULL(fused_node)
  AICPU_CHECK_RES_WITH_LOG(RebuildFusionNode(sub_graph_info, fused_node),
      "Call TfOptimizer::RebuildFusionNode function failed. node cluster[%s].",
      node_cluster_name.c_str())
  AICPU_CHECK_RES_WITH_LOG(CheckAndSetUnknowType(fused_node),
      "Call CheckAndSetUnknowType function failed. node cluster[%s].",
      node_cluster_name.c_str())
  return ge::SUCCESS;
}

ge::Status TfOptimizer::RebuildOutputDesc(const std::vector<ge::OutDataAnchorPtr> &out_data_anchors,
                                          ge::OpDescPtr &fused_op_desc) const {
  AICPU_CHECK_NOTNULL(fused_op_desc);
  std::string op_name = fused_op_desc->GetName();
  std::vector<int64_t> output_list;
  for (size_t i = 0; i < out_data_anchors.size(); ++i) {
    const auto &out_anchor = out_data_anchors[i];
    // out_anchor has been checked when insert node to sub graph
    ge::NodePtr src_node = out_anchor->GetOwnerNode();
    AICPU_CHECK_NOTNULL(src_node);
    ge::GeTensorDesc src_out_tensor_desc = src_node->GetOpDesc()->GetOutputDesc(out_anchor->GetIdx());
    AICPU_CHECK_FALSE_EXEC(fused_op_desc->AddOutputDesc(src_out_tensor_desc) == ge::GRAPH_SUCCESS,
        AICPU_REPORT_INNER_ERROR(
            "Add output[%zu] tensor desc failed, op[%s], op type[%s].",
            i, op_name.c_str(), fused_op_desc->GetType().c_str());
        return ErrorCode::REBUILD_TENSORDESC_FAILED)

    TFDataType tf_data_type = ConvertGeDataType2TfDataType(src_out_tensor_desc.GetDataType());
    if (tf_data_type == TFDataType::DT_INVALID) {
      AICPU_REPORT_INNER_ERROR(
          "Convert output[%zu] ge data type[%d] to tf data type failed,"
          " op[%s], op type[%s].", i, src_out_tensor_desc.GetDataType(),
          op_name.c_str(), fused_op_desc->GetType().c_str());
      return ErrorCode::DATA_TYPE_UNDEFILED;
    }
    output_list.emplace_back(static_cast<int64_t>(tf_data_type));
  }
  AICPU_CHECK_FALSE_EXEC(ge::AttrUtils::SetListInt(fused_op_desc, kTfOutDataType, output_list),
      AICPU_REPORT_INNER_ERROR(
          "Call ge::AttrUtils::SetListInt falied to set attr[%s],"
          " op[%s], op type[%s].", kTfOutDataType.c_str(), op_name.c_str(),
          fused_op_desc->GetType().c_str());
      return ErrorCode::ADD_TF_DATATYPE_FAILED)
  return ge::SUCCESS;
}

ge::Status TfOptimizer::SetAsyncFlag(ge::NodePtr &node) const {
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  std::string op_type = op_desc_ptr->GetType();
  std::string node_name = node->GetName();
  bool async_flag = false;
  if (op_type != kFunctionOp) {
    KernelInfoPtr kernel_info_ptr = TfKernelInfo::Instance();
    AICPU_CHECK_NOTNULL(kernel_info_ptr);
    OpFullInfo op_full_info;
    ge::Status status = kernel_info_ptr->GetOpInfo(op_type, op_full_info);
    AICPU_CHECK_FALSE_EXEC(status == ge::SUCCESS,
        AICPU_REPORT_INNER_ERROR("op type[%s] not support, op[%s].",
            op_type.c_str(), node_name.c_str());
        return status;)
    async_flag = op_full_info.flagAsync;
  }
  AICPU_CHECK_FALSE_EXEC(
    ge::AttrUtils::SetBool(op_desc_ptr, kAsyncFlag, async_flag),
    AICPU_REPORT_CALL_ERROR("Set attr kAsyncFlag failed"
        " op[%s] op_type[%s].", node_name.c_str(), op_type.c_str());
    return ErrorCode::ADD_ATTR_FAILED)
  return ge::SUCCESS;
}

ge::Status TfOptimizer::OptimizeIsolatedNode(ge::NodePtr &node) const {
  AICPU_CHECK_NOTNULL(node)
  AICPU_CHECK_NOTNULL(node->GetOpDesc())
  // if only one node, GE IR -> TF
  ge::Status status = CreateNodeDefForGeNode(node);
  if (status != ge::SUCCESS) {
    AICPU_REPORT_CALL_ERROR(
        "Call TfOptimizer::CreateNodeDefForGeNode failed, op[%s]",
        node->GetName().c_str());
    return status;
  }
  status = SetAsyncFlag(node);
  if (status != ge::SUCCESS) {
    AICPUE_LOGE("OptimizeIsolatedNode SetAsyncFlag failed.");
    return status;
  }
  return CheckAndSetUnknowType(node);
}

ge::Status TfOptimizer::CreateNodeDefForGeNode(ge::NodePtr &node) const {
  std::string node_name = node->GetName();
  ge::OpDescPtr op_desc = node->GetOpDesc();
  // No need to create node def for function op
  if (op_desc->GetType() == kFunctionOp) {
    AICPUE_LOGI("Op[%s] is function op, op type[%s].",
                node_name.c_str(), node->GetType().c_str());
    return ge::SUCCESS;
  }
  if (op_check_mode_) {
    std::vector<std::string> need_check_tf;
    if (!CheckIsFunctionOp(op_desc)) {
      need_check_tf.push_back(op_desc->GetType());
    }
    AICPU_CHECK_FALSE_EXEC(ge::AttrUtils::SetListStr(op_desc, "needCheckTf", need_check_tf),
        AICPU_REPORT_CALL_ERROR(
            "Call ge::SetListStr failed to set attr[needCheckTf], op[%s].",
            node_name.c_str());
        return ErrorCode::FUSE_NODES_FAILED)
  }

  // step1: check tf node def. if not exist, create it
  ge::Buffer node_def_bytes;
  if (ge::AttrUtils::GetBytes(op_desc, kTfNodeDef, node_def_bytes)) {
    AICPUE_LOGI("Node def attr exist in ge op[%s], op type[%s].",
                node_name.c_str(), node->GetType().c_str());
    return ge::SUCCESS;
  }

  // IR -> tf
  NodeDef node_def;
  ge::shared_ptr<Ir2tfBaseParser> parser = Ir2tfParserFactory::Instance().CreateIRParser(op_desc->GetType());
  AICPU_IF_BOOL_EXEC(parser == nullptr,
      AICPU_REPORT_INNER_ERROR("Create ir parser failed, op[%s],"
          " op type[%s].", node->GetName().c_str(), node->GetType().c_str());
      return ErrorCode::GET_IR2TF_PARSER_FAILED)
  AICPU_CHECK_RES_WITH_LOG(parser->ParseNodeDef(*node, &node_def),
      "Call Ir2tfBaseParser::ParseNodeDef function failed. op[%s], op type[%s].",
      node_name.c_str(), node->GetType().c_str())
  AICPUE_LOGI("Create node def for ge op[%s] success, op type[%s].",
              node_name.c_str(), node->GetType().c_str());

  // step2: set tf node def for ge node
  std::string node_def_str;
  AICPU_CHECK_FALSE_EXEC(node_def.SerializeToString(&node_def_str),
      AICPU_REPORT_INNER_ERROR(
          "Serialize nodedef to string failed. op[%s], op type[%s].",
          node_name.c_str(), node->GetType().c_str());
      return ErrorCode::CREATE_NODEDEF_FAILED)
  const uint8_t *node_def_data = reinterpret_cast<const uint8_t*>(node_def_str.data());
  AICPU_CHECK_FALSE_EXEC(
      ge::AttrUtils::SetZeroCopyBytes(op_desc, kTfNodeDef, ge::Buffer::CopyFrom(node_def_data, node_def_str.length())),
      AICPU_REPORT_CALL_ERROR("Call ge::SetZeroCopyBytes for %s, op[%s], op type[%s].", kTfNodeDef.c_str(),
                              node_name.c_str(), node->GetType().c_str());
      return ErrorCode::CREATE_NODEDEF_FAILED)

  // value 3 represent the framework tensorflow
  AICPU_CHECK_FALSE_EXEC(ge::AttrUtils::SetInt(op_desc, kFrameworkType, 3),
      AICPU_REPORT_CALL_ERROR(
          "Call ge::SetInt failed to set attr[%s] to 3, op[%s], op type[%s].",
          kFrameworkType.c_str(), node_name.c_str(), node->GetType().c_str());
      return ErrorCode::CREATE_NODEDEF_FAILED)

  // step3: set input tensor desc for ge node (op_desc)
  std::vector<int64_t> input_type_vec;
  for (const ge::InDataAnchorPtr &in_anchor : node->GetAllInDataAnchors()) {
    ge::GeTensorDesc input_tensor_desc = op_desc->GetInputDesc(in_anchor->GetIdx());
    auto ge_dtype = input_tensor_desc.GetDataType();
    TFDataType tf_data_type = ConvertGeDataType2TfDataType(ge_dtype);
    AICPU_IF_BOOL_EXEC(tf_data_type == TFDataType::DT_INVALID,
        AICPU_REPORT_INNER_ERROR("unsupported input data type[%s], op[%s], op type[%s]",
            ge::TypeUtils::DataTypeToSerialString(ge_dtype).c_str(),
            node_name.c_str(),
            node->GetType().c_str());
        return ErrorCode::DATA_TYPE_UNDEFILED)
    input_type_vec.emplace_back(static_cast<int64_t>(tf_data_type));
  }
  AICPU_CHECK_FALSE_EXEC(ge::AttrUtils::SetListInt(op_desc, kTfInDataType, input_type_vec),
      AICPU_REPORT_CALL_ERROR(
          "Call ge::SetListInt failed to set attr[%s], op[%s], op type[%s].",
          kTfInDataType.c_str(), node_name.c_str(), node->GetType().c_str());
      return ErrorCode::ADD_TF_DATATYPE_FAILED)

  // step4: set output tensor desc for ge node (op_desc)
  std::vector<int64_t> output_type_vec;
  for (const ge::OutDataAnchorPtr &out_anchor : node->GetAllOutDataAnchors()) {
    ge::GeTensorDesc output_tensor_desc = op_desc->GetOutputDesc(out_anchor->GetIdx());
    auto ge_dtype = output_tensor_desc.GetDataType();
    TFDataType tf_data_type = ConvertGeDataType2TfDataType(ge_dtype);
    AICPU_IF_BOOL_EXEC(tf_data_type == TFDataType::DT_INVALID,
        AICPU_REPORT_INNER_ERROR("unsupported output data type[%s], op[%s], op type[%s]",
            ge::TypeUtils::DataTypeToSerialString(ge_dtype).c_str(),
            node_name.c_str(),
            node->GetType().c_str());
        return ErrorCode::DATA_TYPE_UNDEFILED)
    output_type_vec.emplace_back(static_cast<int64_t>(tf_data_type));
  }
  AICPU_CHECK_FALSE_EXEC(ge::AttrUtils::SetListInt(op_desc, kTfOutDataType, output_type_vec),
      AICPU_REPORT_CALL_ERROR("Call ge::AttrUtils::SetListInt failed"
          " to set attr[%s], op[%s], op type[%s].", kTfOutDataType.c_str(),
          node_name.c_str(), node->GetType().c_str());
      return ErrorCode::ADD_TF_DATATYPE_FAILED)
  return ge::SUCCESS;
}

ge::Status TfOptimizer::InsertNodesToSubGraph(ge::ComputeGraphPtr &sub_graph,
                                              std::vector<ge::NodePtr> &node_cluster,
                                              SubGraphInfo &sub_graph_info) const {
  AICPU_CHECK_NOTNULL(sub_graph);
  for (const ge::NodePtr &node : node_cluster) {
    AICPU_CHECK_NOTNULL(node)
    ge::OpDescPtr op_desc = node->GetOpDesc();
    AICPU_CHECK_NOTNULL(op_desc)
    ge::NodePtr new_node = sub_graph->AddNode(op_desc);
    AICPU_CHECK_NOTNULL(new_node)
    sub_graph_info.new_node_map[node->GetName()] = new_node;

    // traverse all input data anchors of current node
    for (const ge::InDataAnchorPtr &in_data_anchor : node->GetAllInDataAnchors()) {
      AICPU_CHECK_NOTNULL(in_data_anchor)
      ge::OutDataAnchorPtr peer_out_anchor = in_data_anchor->GetPeerOutAnchor();
      AICPU_CHECK_NOTNULL(peer_out_anchor)
      auto iter = find(node_cluster.begin(), node_cluster.end(), peer_out_anchor->GetOwnerNode());
      if (iter == node_cluster.end()) {
        sub_graph_info.in_data_anchors.emplace_back(in_data_anchor);
      }
    }

    // traverse all output data anchors of current node
    for (const ge::OutDataAnchorPtr &out_data_anchor : node->GetAllOutDataAnchors()) {
      AICPU_CHECK_NOTNULL(out_data_anchor)
      bool has_node_not_in_sub_graph = false;
      // check whether output anchor link to other anchor not belong to nodes in the sub graph
      for (const ge::InDataAnchorPtr &peer_in_anchor : out_data_anchor->GetPeerInDataAnchors()) {
        AICPU_CHECK_NOTNULL(peer_in_anchor)
        auto iter = find(node_cluster.begin(), node_cluster.end(), peer_in_anchor->GetOwnerNode());
        if (iter == node_cluster.end()) {
          sub_graph_info.out_data_anchor_map[out_data_anchor].emplace_back(peer_in_anchor);
          has_node_not_in_sub_graph = true;
        }
      }
      if (has_node_not_in_sub_graph) {
        sub_graph_info.out_data_anchors.emplace_back(out_data_anchor);
      }
    }

    // check input control anchor of current node
    ge::InControlAnchorPtr in_control_anchor = node->GetInControlAnchor();
    if (in_control_anchor != nullptr) {
      for (const ge::OutControlAnchorPtr &peer_out_anchor : in_control_anchor->GetPeerOutControlAnchors()) {
        AICPU_CHECK_NOTNULL(peer_out_anchor)
        auto iter = find(node_cluster.begin(), node_cluster.end(), peer_out_anchor->GetOwnerNode());
        if (iter == node_cluster.end()) {
          sub_graph_info.in_control_anchor_map[in_control_anchor].emplace_back(peer_out_anchor);
        }
      }
    }

    // check output control anchor of current node
    ge::OutControlAnchorPtr out_control_anchor = node->GetOutControlAnchor();
    if (out_control_anchor != nullptr) {
      for (const ge::InControlAnchorPtr &peer_in_anchor : out_control_anchor->GetPeerInControlAnchors()) {
        AICPU_CHECK_NOTNULL(peer_in_anchor)
        auto iter = find(node_cluster.begin(), node_cluster.end(), peer_in_anchor->GetOwnerNode());
        if (iter == node_cluster.end()) {
          sub_graph_info.out_control_anchor_map[out_control_anchor].emplace_back(peer_in_anchor);
        }
      }
    }
  }
  return ge::SUCCESS;
}

ge::Status TfOptimizer::LinkInnerAnchorsForSubGraph(ge::ComputeGraph &original_graph,
                                                    std::unordered_map<std::string, ge::NodePtr> &new_node_map) const {
  for (auto iter = new_node_map.begin(); iter != new_node_map.end(); ++iter) {
    ge::NodePtr original_node = original_graph.FindNode(iter->first);
    AICPU_CHECK_NOTNULL(original_node)
    ge::NodePtr dst_node = iter->second;
    AICPU_CHECK_NOTNULL(dst_node)

    // traverse all input data anchor of current node
    for (const ge::InDataAnchorPtr &in_data_anchor : original_node->GetAllInDataAnchors()) {
      AICPU_CHECK_NOTNULL(in_data_anchor)
      ge::OutDataAnchorPtr peer_out_anchor = in_data_anchor->GetPeerOutAnchor();
      AICPU_CHECK_NOTNULL(peer_out_anchor)
      auto peer_iter = new_node_map.find(peer_out_anchor->GetOwnerNode()->GetName());
      if (peer_iter == new_node_map.end()) {
        continue;
      }
      ge::NodePtr src_node = peer_iter->second;
      ge::graphStatus status = ge::GraphUtils::AddEdge(src_node->GetOutDataAnchor(peer_out_anchor->GetIdx()),
                                                       dst_node->GetInDataAnchor(in_data_anchor->GetIdx()));
      AICPU_CHECK_FALSE_EXEC(status == ge::GRAPH_SUCCESS,
          AICPU_REPORT_CALL_ERROR("Call ge::GraphUtils::AddEdge failed to link"
              " inner data anchor, src op[%s], src op type[%s],"
              " dst op[%s], dst op type[%s]",
              src_node->GetName().c_str(), src_node->GetType().c_str(),
              dst_node->GetName().c_str(), dst_node->GetType().c_str());
          return ErrorCode::ADD_EDGE_FAILED)
    }

    // check input control anchor of current node
    ge::InControlAnchorPtr in_control_anchor = original_node->GetInControlAnchor();
    if (in_control_anchor != nullptr) {
      for (const ge::OutControlAnchorPtr &peer_out_anchor : in_control_anchor->GetPeerOutControlAnchors()) {
        AICPU_CHECK_NOTNULL(peer_out_anchor)
        auto peer_iter = new_node_map.find(peer_out_anchor->GetOwnerNode()->GetName());
        if (peer_iter == new_node_map.end()) {
          continue;
        }
        ge::NodePtr src_node = peer_iter->second;
        ge::graphStatus status =
            ge::GraphUtils::AddEdge(src_node->GetOutControlAnchor(), dst_node->GetInControlAnchor());
        AICPU_CHECK_FALSE_EXEC(status == ge::GRAPH_SUCCESS,
            AICPU_REPORT_CALL_ERROR("Call ge::GraphUtils::AddEdge failed to link"
                " inner control anchor, src op[%s], src op type[%s],"
                " dst op[%s], dst op type[%s].",
                src_node->GetName().c_str(), src_node->GetType().c_str(),
                dst_node->GetName().c_str(), dst_node->GetType().c_str());
            return ErrorCode::ADD_EDGE_FAILED)
      }
    }
  }
  return ge::SUCCESS;
}

ge::Status TfOptimizer::CollectNodeFuncs(const std::vector<ge::NodePtr> &node_cluster,
                                         FunctionDefLibrary *library) const {
  for (const ge::NodePtr &node : node_cluster) {
    AICPU_CHECK_NOTNULL(node)
    ge::OpDescPtr op_desc = node->GetOpDesc();
    AICPU_CHECK_NOTNULL(op_desc);
    ge::Buffer func_def_bytes;
    if (ge::AttrUtils::GetBytes(op_desc, kTfFuncDef, func_def_bytes)) {
      FunctionDefLibrary func_lib;
      AICPU_CHECK_FALSE_EXEC(func_lib.ParseFromArray(static_cast<void*>(func_def_bytes.GetData()), func_def_bytes.GetSize()),
          AICPU_REPORT_INNER_ERROR(
              "Parse function def using ParseFromArray falied, node cluster[%s].",
              node_cluster[0]->GetName().c_str());
      return ErrorCode::BUILD_FUNCDEF_FAILED)
      library->MergeFrom(func_lib);
    }
  }
  return ge::SUCCESS;
}

ge::Status TfOptimizer::RebuildInputDesc(const std::vector<ge::InDataAnchorPtr> &in_data_anchors,
                                         ge::OpDescPtr &fused_op_desc) const {
  AICPU_CHECK_NOTNULL(fused_op_desc);
  std::string op_name = fused_op_desc->GetName();

  std::vector<int64_t> input_list;
  for (const ge::InDataAnchorPtr &in_anchor : in_data_anchors) {
    ge::NodePtr dst_node = in_anchor->GetOwnerNode();
    AICPU_CHECK_NOTNULL(dst_node);
    ge::GeTensorDesc dst_in_tensor_desc = dst_node->GetOpDesc()->GetInputDesc(in_anchor->GetIdx());
    AICPU_CHECK_FALSE_EXEC(fused_op_desc->AddInputDesc(dst_in_tensor_desc) == ge::GRAPH_SUCCESS,
        AICPU_REPORT_CALL_ERROR("Call ge::OpDesc::AddInputDesc failed to rebuild"
            " input tensor dese. op[%s], op type[%s].",
            op_name.c_str(), fused_op_desc->GetType().c_str());
        return ErrorCode::REBUILD_TENSORDESC_FAILED)

    TFDataType tf_data_type = ConvertGeDataType2TfDataType(dst_in_tensor_desc.GetDataType());
    AICPU_IF_BOOL_EXEC(tf_data_type == TFDataType::DT_INVALID,
      AICPU_REPORT_INNER_ERROR("Invalid ge data type[%d], op[%s], op type[%s].",
          dst_in_tensor_desc.GetDataType(),
          op_name.c_str(), fused_op_desc->GetType().c_str());
      return ErrorCode::DATA_TYPE_UNDEFILED)
    input_list.push_back(static_cast<int64_t>(tf_data_type));
  }

  AICPU_CHECK_FALSE_EXEC(ge::AttrUtils::SetListInt(fused_op_desc, kTfInDataType, input_list),
      AICPU_REPORT_CALL_ERROR("Call ge::AttrUtils::SetListInt failed"
          " to set attr[%s], op[%s], op type[%s].", kTfInDataType.c_str(),
          op_name.c_str(), fused_op_desc->GetType().c_str());
      return ErrorCode::ADD_TF_DATATYPE_FAILED)
  return ge::SUCCESS;
}

ge::Status TfOptimizer::RebuildFusionNode(SubGraphInfo &sub_graph_info,
                                          ge::NodePtr &fused_node) const {
  AICPU_CHECK_NOTNULL(fused_node);
  std::string node_name = fused_node->GetName();

  int32_t src_index = 0;
  // change out_anchor of original node to out_anchor of fusion node
  for (ge::OutDataAnchorPtr &out_anchor : sub_graph_info.out_data_anchors) {
    for (ge::InDataAnchorPtr &peer_in_anchor : sub_graph_info.out_data_anchor_map[out_anchor]) {
      AICPU_CHECK_NOTNULL(peer_in_anchor);
      ge::graphStatus status = peer_in_anchor->Unlink(out_anchor);
      AICPU_CHECK_FALSE_EXEC(status == ge::GRAPH_SUCCESS,
          AICPU_REPORT_CALL_ERROR("Call ge::InDataAnchor::Unlink failed, "
              "op[%s], op type[%s], peer op[%s].", node_name.c_str(),
              fused_node->GetType().c_str(),
              peer_in_anchor->GetOwnerNode()->GetName().c_str());
          return ErrorCode::OPERATE_ANCHOR_FAILED)

      status = ge::GraphUtils::AddEdge(fused_node->GetOutDataAnchor(src_index), peer_in_anchor);
      AICPU_CHECK_FALSE_EXEC(status == ge::GRAPH_SUCCESS,
          AICPU_REPORT_CALL_ERROR("Call ge::GraphUtils::AddEdge failed,"
              " op[%s], op type[%s], peer op[%s].", node_name.c_str(),
              fused_node->GetType().c_str(),
              peer_in_anchor->GetOwnerNode()->GetName().c_str());
          return ErrorCode::ADD_EDGE_FAILED);
    }
    // src_index++ must be here, out_data_anchor can be connected to multi node!
    src_index++;
  }

  src_index = 0;
  // change in_anchor of original node to in_anchor of fusion node
  for (ge::InDataAnchorPtr &in_anchor : sub_graph_info.in_data_anchors) {
    ge::OutDataAnchorPtr peer_out_anchor = in_anchor->GetPeerOutAnchor();
    AICPU_CHECK_NOTNULL(peer_out_anchor);
    ge::graphStatus status = peer_out_anchor->Unlink(in_anchor);
    AICPU_CHECK_FALSE_EXEC(status == ge::GRAPH_SUCCESS,
        AICPU_REPORT_CALL_ERROR("Call ge::OutDataAnchor::Unlink failed,"
            " op[%s], op type[%s], peer op[%s].", node_name.c_str(),
            fused_node->GetType().c_str(),
            peer_out_anchor->GetOwnerNode()->GetName().c_str());
        return ErrorCode::OPERATE_ANCHOR_FAILED)

    status = ge::GraphUtils::AddEdge(peer_out_anchor, fused_node->GetInDataAnchor(src_index));
    AICPU_CHECK_FALSE_EXEC(status == ge::GRAPH_SUCCESS,
        AICPU_REPORT_CALL_ERROR("Call ge::GraphUtils::AddEdge failed,"
            " op[%s], op type[%s], peer op[%s].", node_name.c_str(),
            fused_node->GetType().c_str(),
            peer_out_anchor->GetOwnerNode()->GetName().c_str());
        return ErrorCode::ADD_EDGE_FAILED)
    src_index++;
  }

  // out_control_anchor
  for (auto pair : sub_graph_info.out_control_anchor_map) {
    ge::OutControlAnchorPtr out_anchor = pair.first;
    for (ge::InControlAnchorPtr &peer_in_anchor : pair.second) {
      AICPU_CHECK_NOTNULL(peer_in_anchor);
      ge::graphStatus status = peer_in_anchor->Unlink(out_anchor);
      AICPU_CHECK_FALSE_EXEC(status == ge::GRAPH_SUCCESS,
          AICPU_REPORT_CALL_ERROR("Call ge::InControlAnchorAnchor::Unlink failed,"
              " op[%s], op type[%s], peer op[%s].", node_name.c_str(),
              fused_node->GetType().c_str(),
              peer_in_anchor->GetOwnerNode()->GetName().c_str());
          return ErrorCode::OPERATE_ANCHOR_FAILED)

      status = ge::GraphUtils::AddEdge(fused_node->GetOutControlAnchor(), peer_in_anchor);
      AICPU_CHECK_FALSE_EXEC(status == ge::GRAPH_SUCCESS,
          AICPU_REPORT_CALL_ERROR("Call ge::GraphUtils::AddEdge failed, op[%s],"
              " op type[%s], peer op[%s].", node_name.c_str(),
              fused_node->GetType().c_str(),
              peer_in_anchor->GetOwnerNode()->GetName().c_str());
          return ErrorCode::ADD_EDGE_FAILED)
    }
  }

  // in_control_anchor
  for (auto pair : sub_graph_info.in_control_anchor_map) {
    ge::InControlAnchorPtr in_anchor = pair.first;
    for (ge::OutControlAnchorPtr &peer_out_anchor : pair.second) {
      AICPU_CHECK_NOTNULL(peer_out_anchor);
      ge::graphStatus status = peer_out_anchor->Unlink(in_anchor);
      AICPU_CHECK_FALSE_EXEC(status == ge::GRAPH_SUCCESS,
          AICPU_REPORT_CALL_ERROR("Call ge::OutControlAnchor::Unlink failed,"
              " op[%s], op type[%s], peer op[%s].", node_name.c_str(),
              fused_node->GetType().c_str(),
              peer_out_anchor->GetOwnerNode()->GetName().c_str());
          return ErrorCode::OPERATE_ANCHOR_FAILED)

      status = ge::GraphUtils::AddEdge(peer_out_anchor, fused_node->GetInControlAnchor());
      AICPU_CHECK_FALSE_EXEC(status == ge::GRAPH_SUCCESS,
          AICPU_REPORT_CALL_ERROR("Call ge::GraphUtils::AddEdge failed,"
              " op[%s], op type[%s], peer op[%s].",
               node_name.c_str(), fused_node->GetType().c_str(),
               peer_out_anchor->GetOwnerNode()->GetName().c_str());
          return ErrorCode::ADD_EDGE_FAILED)
    }
  }
  return ge::SUCCESS;
}

ge::Status TfOptimizer::SaveFusionNodeMappingRelations(const std::vector<ge::NodePtr> &node_cluster,
                                                       const std::vector<ge::OutDataAnchorPtr> &out_data_anchors,
                                                       ge::OpDescPtr &mapping_op_desc) const {
  // save name mapping
  std::vector<std::string> original_names_tmp;
  for (const ge::NodePtr &node : node_cluster) {
    AICPU_CHECK_NOTNULL(node);
    (void)original_names_tmp.emplace_back(node->GetName());
  }
  if (original_names_tmp.size() > 0) {
    AICPU_CHECK_FALSE_EXEC(ge::AttrUtils::SetListStr(mapping_op_desc, ge::ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, original_names_tmp),
        AICPU_REPORT_CALL_ERROR("Call ge::AttrUtils::SetListStr failed "
            "to set attr[%s], op[%s], op type[%s].",
            ge::ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES.c_str(),
            mapping_op_desc->GetName().c_str(),
            mapping_op_desc->GetType().c_str());
        return ErrorCode::ADD_TF_DATADESC_FAILED);
  }

  for (size_t i = 0; i < out_data_anchors.size(); ++i) {
    const ge::OutDataAnchorPtr &out_anchor = out_data_anchors[i];
    AICPU_CHECK_NOTNULL(out_anchor);
    ge::NodePtr src_node = out_anchor->GetOwnerNode();
    ge::GeTensorDesc src_out_tensor_desc = src_node->GetOpDesc()->GetOutputDesc(out_anchor->GetIdx());
    ge::GeTensorDescPtr out_tensor = mapping_op_desc->MutableOutputDesc(i);
    AICPU_CHECK_NOTNULL(out_tensor);
    std::string origin_data_type_str = "RESERVED";
    if (src_out_tensor_desc.GetDataType() != ge::DT_UNDEFINED && src_out_tensor_desc.GetDataType() < ge::DT_MAX) {
      origin_data_type_str = ge::TypeUtils::DataTypeToSerialString(src_out_tensor_desc.GetDataType());
    }
    std::string origin_format_str = "RESERVED";
    if (src_out_tensor_desc.GetFormat() != ge::FORMAT_RESERVED) {
      origin_format_str = ge::TypeUtils::FormatToSerialString(src_out_tensor_desc.GetFormat());
    }

    (void)ge::AttrUtils::SetStr(out_tensor, ge::ATTR_NAME_DATA_DUMP_ORIGIN_DATA_TYPE, origin_data_type_str);
    (void)ge::AttrUtils::SetStr(out_tensor, ge::ATTR_NAME_DATA_DUMP_ORIGIN_FORMAT, origin_format_str);
    (void)ge::AttrUtils::SetStr(out_tensor, ge::ATTR_NAME_DATA_DUMP_ORIGIN_NAME, src_node->GetName());
    (void)ge::AttrUtils::SetInt(out_tensor, ge::ATTR_NAME_DATA_DUMP_ORIGIN_OUTPUT_INDEX, out_anchor->GetIdx());
  }
  return ge::SUCCESS;
}

FACTORY_GRAPH_OPTIMIZER_CLASS_KEY(TfOptimizer, "TFOptimizer")
}
