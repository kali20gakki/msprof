/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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

#include "fftsplus_graph_optimizer.h"
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include "optimizer/cache_optimizer/cache_manager.h"
#include "inc/ffts_utils.h"
#include "engine/engine_manager.h"
#include "ge/ge_api_types.h"
#include "inc/graph/debug/ge_attr_define.h"
#include "inc/graph/utils/graph_utils.h"
#include "graph/ge_context.h"
#include "graph/tuning_utils.h"

namespace ffts {
const int64_t kSizeAlignedLimit = 32;

void to_json(nlohmann::json& json_value, const DimRange &struct_value)
{
  json_value = nlohmann::json{{"lower", struct_value.lower},
                              {"higher", struct_value.higher}};
}

void from_json(const nlohmann::json &j, OpCut &d)
{
  j.at("splitCutIndex").get_to(d.split_cut_idx);
  j.at("reduceCutIndex").get_to(d.reduce_cut_idx);
  j.at("cutId").get_to(d.cut_id);
}

void from_json(const nlohmann::json &json_value, DimRange &struct_value)
{
  json_value.at("lower").get_to(struct_value.lower);
  json_value.at("higher").get_to(struct_value.higher);
}
void to_json(nlohmann::json &j, const OpCut &d)
{
  j = nlohmann::json { { "splitCutIndex", d.split_cut_idx },
                       { "reduceCutIndex", d.reduce_cut_idx },
                       { "cutId", d.cut_id } };
}
void to_json(nlohmann::json &json_value, const ThreadSliceMap &struct_value)
{
  json_value = nlohmann::json{{"thread_scopeId", struct_value.thread_scope_id},
                              {"is_first_node_in_topo_order", struct_value.is_first_node_in_topo_order},
                              {"threadMode", struct_value.thread_mode},
                              {"node_num_in_thread_scope", struct_value.node_num_in_thread_scope},
                              {"slice_instance_num", struct_value.slice_instance_num},
                              {"parallel_window_size", struct_value.parallel_window_size},
                              {"thread_id", struct_value.thread_id},
                              {"dependencies", struct_value.dependencies},
                              {"input_tensor_slice", struct_value.input_tensor_slice},
                              {"output_tensor_slice", struct_value.output_tensor_slice},
                              { "ori_input_tensor_slice", struct_value.ori_input_tensor_slice},
                              { "ori_output_tensor_slice", struct_value.ori_output_tensor_slice},
                              {"cutType", struct_value.cut_type},
                              {"is_input_node_of_thread_scope", struct_value.is_input_node_of_thread_scope},
                              {"is_output_node_of_thread_scope", struct_value.is_output_node_of_thread_scope},
                              {"oriInputTensorShape", struct_value.ori_input_tensor_shape},
                              {"oriOutputTensorShape", struct_value.ori_output_tensor_shape},
                              {"original_node", struct_value.original_node}
  };
}


void from_json(const nlohmann::json &json_value, ThreadSliceMap &struct_value)
{
  json_value.at("is_input_node_of_thread_scope").get_to(struct_value.is_input_node_of_thread_scope);
  json_value.at("is_output_node_of_thread_scope").get_to(struct_value.is_output_node_of_thread_scope);
  json_value.at("oriInputTensorShape").get_to(struct_value.ori_input_tensor_shape);
  json_value.at("oriOutputTensorShape").get_to(struct_value.ori_output_tensor_shape);
  json_value.at("original_node").get_to(struct_value.original_node);
  json_value.at("threadMode").get_to(struct_value.thread_mode);
  json_value.at("thread_scopeId").get_to(struct_value.thread_scope_id);
  json_value.at("node_num_in_thread_scope").get_to(struct_value.node_num_in_thread_scope);
  json_value.at("slice_instance_num").get_to(struct_value.slice_instance_num);
  json_value.at("parallel_window_size").get_to(struct_value.parallel_window_size);
  json_value.at("thread_id").get_to(struct_value.thread_id);
  json_value.at("dependencies").get_to(struct_value.dependencies);
  json_value.at("cutType").get_to(struct_value.cut_type);
  json_value.at("input_tensor_slice").get_to(struct_value.input_tensor_slice);
  json_value.at("output_tensor_slice").get_to(struct_value.output_tensor_slice);
}

void FFTSPlusGraphOptimizer::JudgeThreadTensorAlignedAndAlarm(ge::NodePtr &node,
                                                              vector<vector<vector<DimRange>>> &tensor_slice,
                                                              const bool &is_input) const {
  if (!node) {
    return;
  }
  for (size_t i = 0; i < tensor_slice.size(); i++) {
    for (size_t j = 0; j < tensor_slice[i].size(); j++) {
      const auto &tensor_desc = is_input ? node->GetOpDesc()->MutableInputDesc(j) :
                                node->GetOpDesc()->MutableOutputDesc(j);
      if (!tensor_desc) {
        continue;
      }
      ge::DataType tensor_type = tensor_desc->GetDataType();
      auto data_size = ge::GetSizeByDataType(tensor_type);
      int64_t size = data_size;
      for (size_t k = 0; k < tensor_slice[i][j].size(); k++) {
        size *= (tensor_slice[i][j][k].higher - tensor_slice[i][j][k].lower);
      }

      if (size % kSizeAlignedLimit != 0) {
        FFTS_LOGW("slice op: %s,%s is not 32 aligned, %zu-%zu, %s", node->GetOpDesc()->GetName().c_str(),
                  node->GetOpDesc()->GetType().c_str(), i, j, is_input ? "input" : "output");
      }
    }
  }
}

void FFTSPlusGraphOptimizer::GetSliceInfo(ge::ComputeGraph &graph) const {
  for (auto &node : graph.GetDirectNode()) {
    ThreadSliceMapPtr slice_info_ptr = GetSliceInfoFromJson(node->GetOpDesc());
    if (slice_info_ptr) {
      (void)node->GetOpDesc()->SetExtAttr(kAttrSgtStructInfo, slice_info_ptr);
      FFTS_LOGD("GetSliceInfo: %u of op: %s success.", slice_info_ptr->thread_scope_id,
                node->GetOpDesc()->GetName().c_str());
      (void)ge::AttrUtils::SetInt(node->GetOpDesc(), kThreadScopeId, slice_info_ptr->thread_scope_id);
      (void)ge::AttrUtils::SetBool(node->GetOpDesc(), kTypeFFTSPlus, true);
      (void)ge::AttrUtils::SetInt(node->GetOpDesc(), kThreadMode, slice_info_ptr->thread_mode);
      (void)JudgeThreadTensorAlignedAndAlarm(node, slice_info_ptr->input_tensor_slice, true);
      (void)JudgeThreadTensorAlignedAndAlarm(node, slice_info_ptr->output_tensor_slice, false);
    }
  }
}

ThreadSliceMapPtr FFTSPlusGraphOptimizer::GetSliceInfoFromJson(const ge::OpDescPtr &op_desc_ptr) const {
  std::string str_slice_info;
  ThreadSliceMapPtr slice_info_ptr = nullptr;

  (void)ge::AttrUtils::GetStr(op_desc_ptr, kAttrSgtJsonInfo, str_slice_info);
  FFTS_LOGD("Slice info: %s of op: %s success.", str_slice_info.c_str(), op_desc_ptr->GetName().c_str());
  if (str_slice_info.empty()) {
    FFTS_LOGW("Node:%s, slice info is empty.", op_desc_ptr->GetName().c_str());
  } else {
    try {
      nlohmann::json slice_info_json = nlohmann::json::parse(str_slice_info);
      if (slice_info_json.is_null()) {
        FFTS_LOGW("Get l2 info: %s is empty.", str_slice_info.c_str());
      } else {
        FFTS_MAKE_SHARED(slice_info_ptr = std::make_shared<ThreadSliceMap>(), return nullptr);
        slice_info_json.get_to(*slice_info_ptr);
      }
    } catch (const nlohmann::json::exception &e) {
      FFTS_LOGE("Parse json str failed, %s", e.what());
      return nullptr;
    }
  }
  return slice_info_ptr;
}

FFTSPlusGraphOptimizer::FFTSPlusGraphOptimizer()
    : graph_optimizer_attr_({kFFTSPlusCoreName, ge::ENGINE}),
      init_flag_(false) {}

FFTSPlusGraphOptimizer::~FFTSPlusGraphOptimizer() {}


Status FFTSPlusGraphOptimizer::Initialize(const std::map<std::string, std::string>& options,
                                          ge::OptimizeUtility *const optimize_utility) {
  FFTS_LOGI("Begin to init FFTSPlusGraphOptimizer.");
  // if graph optimizer has been initialized, return success
  if (GetPlatformFFTSMode() != FFTSMode::FFTS_MODE_FFTS_PLUS) {
    FFTS_LOGW("FFTSPlusGraphOptimizer ffts_plus flag is 0");
    return SUCCESS;
  }
  if (init_flag_) {
    FFTS_LOGW("FFTSPlusGraphOptimizer has been initialized.");
    return SUCCESS;
  }

  Status ret = InitLibPath();
  if (ret != SUCCESS) {
    FFTS_LOGW("Failed to get the so path.");
    return FAILED;
  }
  const string FFTS_OPTIMIZER_FUNC_NAME = "FFTSOptimize";
  const std::string LX_FUSION_PLUGIN = "libgraph_tuner.so";

  string plugin_path = lib_path_ + LX_FUSION_PLUGIN;
  FFTS_MAKE_SHARED(lx_fusion_plugin_ffts_plus_ = std::make_shared<PluginManager>(plugin_path), return FAILED);
  FFTS_CHECK(lx_fusion_plugin_ffts_plus_ == nullptr, REPORT_FFTS_ERROR("[GraphOpt][Init][InitLxFusPlg] Create lx \
             fusion plugin manager ptr failed."),
  return FAILED);
  if (lx_fusion_plugin_ffts_plus_->OpenPlugin(plugin_path) != SUCCESS) {
    FFTS_LOGE("Failed to open %s.", plugin_path.c_str());
    return FAILED;
  }
  ret = lx_fusion_plugin_ffts_plus_->GetFunction<tune::Status, ge::ComputeGraph &, bool>(
      FFTS_OPTIMIZER_FUNC_NAME, FFTSOptimizer_);
  if (ret != SUCCESS) {
    FFTS_LOGW("Failed to get the function %s in the plugin %s.", FFTS_OPTIMIZER_FUNC_NAME.c_str(), plugin_path.c_str());
    return FAILED;
  }
  init_flag_ = true;
  FFTS_LOGI("Initialize success.");
  return SUCCESS;
}

Status FFTSPlusGraphOptimizer::Finalize() {
  if (!init_flag_) {
    FFTS_LOGW("FFTSPlusGraphOptimizer finalize is not allowed, initialize first is necessary.");
    return SUCCESS;
  }

  (void)lx_fusion_plugin_ffts_plus_->CloseHandle();
  init_flag_ = false;
  FFTS_LOGD("Finalized success.");
  return SUCCESS;
}


Status FFTSPlusGraphOptimizer::OptimizeOriginalGraph(ge::ComputeGraph& graph) {
  if (!init_flag_) {
    REPORT_FFTS_ERROR("[GraphOpt][init] FEGraphOptimizer has not been initialized.");
    return FAILED;
  }

  DumpGraphAndOnnx(graph, "OptimizeOriginalGraph_FeTopoSortingAfter");
  DumpSubGraphAndOnnx(graph, "OptimizeOriginalGraph_FeTopoSortingAfter_Subgraph");

  FFTS_LOGI("Optimize original graph[%s] success, node_size:%zu.", graph.GetName().c_str(), graph.GetAllNodesSize());
  return SUCCESS;
}


Status FFTSPlusGraphOptimizer::OptimizeFusedGraph(ge::ComputeGraph& graph) {
  FFTS_LOGD("Begin to optimizing fused graph in engine[%s].", graph_optimizer_attr_.engineName.c_str());

  if (!init_flag_) {
    FFTS_LOGW("OptimizeFusedGraph is not allowed, initialize firstly.");
    return FAILED;
  }
  FFTS_TIMECOST_START(OptimizeFusedGraph);

  // do sgt slice
  FFTS_CHECK(FFTSOptimizer_ == nullptr, REPORT_FFTS_ERROR("[SubGraphOpt][BufFusProc] FFTSOptimizerFunc is nullptr."),
  return FAILED);
  Status ret = FFTSOptimizer_(graph, true);
  if (ret != tune::SUCCESS) {
    REPORT_FFTS_ERROR("[SubGraphOpt][BufFusProc][fuc] FFTSOptimizerFunc failed.");
    return FAILED;
  }

  GetSliceInfo(graph);

  CacheManager::SetPersistWeightForGraph(graph);
  const auto &parent_node = graph.GetParentNode();
  bool is_control_graph =
       (parent_node != nullptr && (CONTROL_OP_V2_TYPE.count(parent_node->GetType())));
  if (is_control_graph) {
    FFTS_LOGD("graph %s 's parent node is control Op %s, skipped gen FunctionOp.",
              graph.GetName().c_str(), parent_node->GetName().c_str());
    return SUCCESS;
  }

  if (TransSubGraphToFunctionOp(graph) != SUCCESS) {
    FFTS_LOGE("TransSubGraphToFunctionOp failed, graph %s.", graph.GetName().c_str());
    return FAILED;
  }
  FFTS_TIMECOST_END(OptimizeFusedGraph, "FFTSPlusGraphOptimizer::OptimizeFusedGraph");
  return SUCCESS;
}

Status FFTSPlusGraphOptimizer::GetAttributes(ge::GraphOptimizerAttribute& attrs) const {
  attrs = graph_optimizer_attr_;
  return SUCCESS;
}

Status FFTSPlusGraphOptimizer::OptimizeWholeGraph(ge::ComputeGraph& graph) {
  Status ret = CacheOptionJudge(graph);
  if (ret != SUCCESS) {
    return ret;
  }
  return SUCCESS;
}

ge::OpDescPtr FFTSPlusGraphOptimizer::CreateFunctionOp(vector<ge::NodePtr> &node_vec) const {
  if (node_vec.empty()) {
    return nullptr;
  }
  ge::NodePtr first_node = node_vec[0];
  FFTS_CHECK(first_node == nullptr, FFTS_LOGE("CreateFusionOp nullptr Pointer."), return nullptr);
  ge::OpDescPtr function_opdef = nullptr;
  FFTS_MAKE_SHARED(function_opdef = std::make_shared<ge::OpDesc>(ge::OpDesc()), return nullptr);
  std::string fusion_node_name;

  for (ge::NodePtr &node : node_vec) {
    FFTS_CHECK(node == nullptr, REPORT_FFTS_ERROR("[FFTSSubGraphOpt][CrtFuncOp] node is nullptr."), return nullptr);
    fusion_node_name += node->GetOpDesc()->GetName();
      if (fusion_node_name.size() > kMaxOpNmaLen) {
      fusion_node_name = first_node->GetOpDesc()->GetName() + "_ffts_fusion";
      break;
    }
  }
  function_opdef->SetName(fusion_node_name);
  function_opdef->SetType("PartitionedCall");

  // copy session graph id
  string session_graph_id;
  if (ge::AttrUtils::GetStr(first_node->GetOpDesc(), ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id)) {
    if (!ge::AttrUtils::SetStr(function_opdef, ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id)) {
      FFTS_LOGE("Op[%s]: fail to set the attribute %s.", fusion_node_name.c_str(),
                ge::ATTR_NAME_SESSION_GRAPH_ID.c_str());
      return nullptr;
    }
  }

  function_opdef->SetOpEngineName(kFFTSPlusCoreName);

  return function_opdef;
}

Status EstablishConnection(const ge::NodePtr &node,
                           const std::unordered_set<std::string> &node_name_set,
                           ge::CompleteGraphBuilder &builder) {
  const auto &src_node_name = node->GetName();
  for (const auto &out_data_anchor : node->GetAllOutDataAnchors()) {
    FFTS_CHECK_NOTNULL(out_data_anchor);
    auto src_idx = out_data_anchor->GetIdx();
    for (auto &in_data_anchor : out_data_anchor->GetPeerInDataAnchors()) {
      FFTS_CHECK_NOTNULL(in_data_anchor);
      auto dst_node = in_data_anchor->GetOwnerNode();
      FFTS_CHECK_NOTNULL(dst_node);
      if (!node_name_set.count(dst_node->GetName())) {
        FFTS_LOGD("Node[name:%s] is out op, skip.", dst_node->GetName().c_str());
        continue;
      }
      builder.AddDataLink(src_node_name, src_idx, dst_node->GetName(), in_data_anchor->GetIdx());
    }
  }
  auto out_ctrl_anchor =  node->GetOutControlAnchor();
  FFTS_CHECK_NOTNULL(out_ctrl_anchor);
  for (auto &in_ctrl_anchor : out_ctrl_anchor->GetPeerInControlAnchors()) {
    FFTS_CHECK_NOTNULL(in_ctrl_anchor);
    auto dst_node = in_ctrl_anchor->GetOwnerNode();
    FFTS_CHECK_NOTNULL(dst_node);
    if (!node_name_set.count(dst_node->GetName())) {
      FFTS_LOGD("Dst Node[name:%s] linked by control edge is out op, skip.", dst_node->GetName().c_str());
      continue;
    }
    builder.AddControlLink(src_node_name, dst_node->GetName());
  }
  return SUCCESS;
}

static void SetReuseInputAttr(ge::OpDescPtr &function_op_desc)
{
  for (size_t i = 0; i < function_op_desc->GetAllOutputsDescSize(); i++) {
    auto out_desc = function_op_desc->MutableOutputDesc(i);
    if (out_desc == nullptr) {
      continue;
    }
    ge::TensorUtils::SetReuseInput(*out_desc.get(), false);
  }
}

Status FFTSPlusGraphOptimizer::CreateFunctionOpSubGraph(ge::NodePtr &function_node,
                                                        std::vector<ge::NodePtr> &node_vec,
                                                        vector<fe::FusionDataFlow> &input_edge_list,
                                                        vector<fe::FusionDataFlow> &output_edge_list) {
  if (node_vec.empty()) {
    return FAILED;
  }
  auto graph = node_vec[0]->GetOwnerComputeGraph();
  FFTS_LOGD("ai-core sub graph name is %s.", graph->GetName().c_str());
  auto func_graph = function_node->GetOwnerComputeGraph();
  FFTS_LOGD("func owner graph name is %s.", func_graph->GetName().c_str());
  ge::ComputeGraphPtr src_graph = graph->TryGetExtAttr("part_src_graph", ge::ComputeGraphPtr());
  FFTS_CHECK_NOTNULL(src_graph);
  FFTS_LOGD("src graph name is %s", src_graph->GetName().c_str());
  auto root_graph = ge::GraphUtils::FindRootGraph(src_graph);
  FFTS_CHECK_NOTNULL(root_graph);
  FFTS_LOGD("root graph name is %s", root_graph->GetName().c_str());
  std::unordered_set<std::string> node_name_set;
  for (auto &node : node_vec) {
    node_name_set.emplace(node->GetName());
  }

  string sgt_graph_name = func_graph->GetName() + "_sgt_graph_" + std::to_string(sgt_graph_index_++);
  FFTS_LOGD("function_node: %s, sgt graph name is %s", function_node->GetName().c_str(), sgt_graph_name.c_str());
  ge::CompleteGraphBuilder builder(sgt_graph_name, false);

  for (auto &node : node_vec) {
    /* Adds a node to the builder. Currently, only the input parameter op_desc
       is supported, and the connection information will be lost. Therefore,
       the AddDataLink interface needs to be invoked to re-establish the
       connection. */
    builder.AddNode(node->GetOpDesc());

    FFTS_LOGD("Begin re-establish connection of node[name:%s]. out data size is %d.",
              node->GetName().c_str(), node->GetAllOutDataAnchorsSize());
    (void)EstablishConnection(node, node_name_set, builder);
  }

  FFTS_LOGD("input_edge_list size is %zu", input_edge_list.size());
  std::map<uint32_t, uint32_t> input_mapping;
  uint32_t index = 0;
  for (auto &input_edge : input_edge_list) {
    FFTS_CHECK(input_edge.edge.first == nullptr, FFTS_LOGW("input_edge.first is null"), return FAILED);
    FFTS_CHECK(input_edge.edge.second == nullptr, FFTS_LOGW("input_edge.second is null"), return FAILED);
    std::uint32_t src_idx = std::static_pointer_cast<ge::DataAnchor>(input_edge.edge.first)->GetIdx();
    std::uint32_t fusion_idx = std::static_pointer_cast<ge::DataAnchor>(input_edge.edge.second)->GetIdx();
    ge::NodePtr src_node = std::static_pointer_cast<ge::DataAnchor>(input_edge.edge.first)->GetOwnerNode();
    ge::NodePtr fusion_node = std::static_pointer_cast<ge::DataAnchor>(input_edge.edge.second)->GetOwnerNode();
    FFTS_LOGD("srcIdx is %d, fusion_idx is %d. src_node is : %s, fusion_node is : %s.", src_idx, fusion_idx,
              src_node->GetName().c_str(), fusion_node->GetName().c_str());
    // add relation: input index of new graph to input index of original graph.
    std::vector<std::string> ffts_node_names{fusion_node->GetName()};
    std::vector<uint32_t> dst_indexes{fusion_idx};
    builder.SetInput(index, ffts_node_names, dst_indexes);
    input_mapping.emplace(index, index);
    index++;
  }
  builder.SetInputMapping(input_mapping);

  FFTS_LOGD("fusionOpOutputAnchorsMap size is %zu", output_edge_list.size());
  std::map<uint32_t, uint32_t> output_mapping;
  std::vector<ge::AnchorPtr> sgt_output;
  uint32_t output_index = 0;
  for (auto &output_edge : output_edge_list) {
    if (std::find(sgt_output.begin(), sgt_output.end(), output_edge.edge.first) == sgt_output.end()) {
      sgt_output.push_back(output_edge.edge.first);
      FFTS_CHECK(output_edge.edge.first == nullptr, FFTS_LOGW("output_edge.first is null"), return FAILED);
      FFTS_CHECK(output_edge.edge.second == nullptr, FFTS_LOGW("iterVec->second is null"), return FAILED);
      std::uint32_t src_idx = std::static_pointer_cast<ge::DataAnchor>(output_edge.edge.first)->GetIdx();
      std::uint32_t fusion_idx = std::static_pointer_cast<ge::DataAnchor>(output_edge.edge.second)->GetIdx();
      ge::NodePtr src_node = std::static_pointer_cast<ge::DataAnchor>(output_edge.edge.first)->GetOwnerNode();
      ge::NodePtr fusion_node = std::static_pointer_cast<ge::DataAnchor>(output_edge.edge.second)->GetOwnerNode();
      FFTS_LOGD("out src_idx is %d, out fusion_idx is %d. src_node is : %s, fusion_node is : %s", src_idx, fusion_idx,
                src_node->GetName().c_str(), fusion_node->GetName().c_str());
      // add relation: output index of new graph to output index of original graph.
      std::vector<std::string> input_node_names{src_node->GetName()};
      builder.AddOutput(src_node->GetName(), src_idx);
      output_mapping.emplace(output_index, output_index);
      output_index++;
    }
  }

  builder.SetOutputMapping(output_mapping);
  builder.SetParentNode(function_node);

  ge::graphStatus status = ge::GRAPH_SUCCESS;
  string err_msg;
  auto sgt_graph = builder.Build(status, err_msg);
  if (status != ge::GRAPH_SUCCESS) {
    FFTS_LOGE("Build graph not success. status is : %d, err_msg is : %s.", status, err_msg.c_str());
    return FAILED;
  }

  sgt_graph->SetParentGraph(graph);
  sgt_graph->SetParentNode(function_node);
  ge::OpDescPtr function_op_desc = function_node->GetOpDesc();
  function_op_desc->AddSubgraphName(sgt_graph->GetName());
  function_op_desc->SetSubgraphInstanceName(0, sgt_graph->GetName());
  (void)ge::AttrUtils::SetGraph(function_op_desc, "_sgt_sub_graph", sgt_graph);
  (void)ge::AttrUtils::SetStr(function_op_desc, "_ffts_plus_sub_graph", sgt_graph->GetName().c_str());
  (void)ge::AttrUtils::SetBool(function_op_desc, "_sgt_function_op", true);
  (void)ge::AttrUtils::SetStr(function_op_desc, kAttrCompositeEngineName, kFFTSPlusCoreName);
  (void)ge::AttrUtils::SetStr(function_op_desc, kAttrCompositeKernelLibName, kFFTSPlusCoreName);
  SetReuseInputAttr(function_op_desc);
  if (root_graph->AddSubgraph(sgt_graph->GetName(), sgt_graph) != ge::GRAPH_SUCCESS) {
    FFTS_LOGE("Add sub graph not success. status is : %d, root graph: %s.", status, root_graph->GetName().c_str());
    return FAILED;
  }
  FFTS_LOGD("Add sub graph success. root graph: %s, subgraph: %s.",
            root_graph->GetName().c_str(), sgt_graph->GetName().c_str());

  for (auto &node : node_vec) {
    std::vector<int> io_map = {0};
    if (node->GetAllOutDataAnchors().size() == 0) {
      io_map = {};
    } else if (node->GetAllInDataAnchors().size() == 0) {
      io_map = {-1};
    }
    if (ge::GraphUtils::IsolateNode(node, io_map) != ge::GRAPH_SUCCESS) {
      FFTS_LOGE("Isolate Node %s not success.", node->GetName().c_str());
      return FAILED;
    }
    if (ge::GraphUtils::RemoveNodeWithoutRelink(graph, node) != ge::GRAPH_SUCCESS) {
      FFTS_LOGE("Remove node: %s without relink failed", node->GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

Status FFTSPlusGraphOptimizer::TransSingleSubGraph(ge::ComputeGraph &graph, std::vector<ge::NodePtr> &node_vec) {
  vector<fe::FusionDataFlow> input_edge_list;
  vector<fe::FusionDataFlow> output_edge_list;
  vector<fe::FusionDataFlow> input_ctrl_edge_list;
  vector<fe::FusionDataFlow> output_ctrl_edge_list;

  FFTS_LOGD("TransSingleSubGraph enter");

  if (graph_comm_ptr_->GetFusionNodeEdgeList(node_vec, input_edge_list, output_edge_list) != SUCCESS) {
    FFTS_LOGE("GetFusionNodeEdgeList failed.");
    return FAILED;
  }

  if (graph_comm_ptr_->GetFusionNodeCtrlEdgeList(node_vec, input_ctrl_edge_list, output_ctrl_edge_list) != SUCCESS) {
    FFTS_LOGE("GetFusionNodeCtrlEdgeList failed.");
    return FAILED;
  }

  ge::OpDescPtr function_op = CreateFunctionOp(node_vec);
  if (function_op == nullptr) {
    FFTS_LOGE("CreateFunctionOp failed.");
    return FAILED;
  }

  if (AddFunctionNodeInputDesc(function_op, input_edge_list) != SUCCESS) {
    FFTS_LOGE("Failed to AddFusionNodeInputDesc.");
    return FAILED;
  }

  if (AddFunctionNodeOutputDesc(function_op, output_edge_list) != SUCCESS) {
    FFTS_LOGE("Failed to AddFusionNodeOutputDesc.");
    return FAILED;
  }

  ge::NodePtr function_node = graph.AddNode(function_op);
  FFTS_CHECK_NOTNULL(function_node);

  // Merge Same scope node
  if (graph_comm_ptr_->MergeFusionNodeEdgeList(function_node, node_vec, input_edge_list, output_edge_list) !=
      SUCCESS) {
    FFTS_LOGE("MergeFusionNodeEdgeList failed!");
    return FAILED;
  }

  if (graph_comm_ptr_->MergeFusionNodeCtrlEdgeList(function_node, node_vec, input_ctrl_edge_list,
                                                   output_ctrl_edge_list) != SUCCESS) {
    FFTS_LOGE("MergeFusionNodeCtrlEdgeList failed!");
    return FAILED;
  }

  return CreateFunctionOpSubGraph(function_node, node_vec, input_edge_list, output_edge_list);
}


Status FFTSPlusGraphOptimizer::GetSubGraphNodes(const ge::ComputeGraph &graph, SubGraphNodeMap &node_map) const {
  for (auto const &node : graph.GetDirectNode()) {
    FFTS_CHECK(node == nullptr, FFTS_LOGE("node is nullptr."), return FAILED);
    auto op_desc_ptr = node->GetOpDesc();

    uint32_t thread_scope_id = 0;
    (void)ge::AttrUtils::GetInt(op_desc_ptr, kThreadScopeId, thread_scope_id);
    if (thread_scope_id == 0) {
      FFTS_LOGD("op %s is not belong to any subgraph.", node->GetName().c_str());
      continue;
    }

    SubGraphNodeMap::iterator nodelist_it = node_map.find(thread_scope_id);
    if (nodelist_it == node_map.end()) {
      std::vector<ge::NodePtr> node_list_new;
      node_list_new.push_back(node);
      node_map.insert(std::pair<uint32_t, std::vector<ge::NodePtr>>(thread_scope_id, node_list_new));
      FFTS_LOGD("add %u. op %s.", thread_scope_id, node->GetName().c_str());
    } else {
      nodelist_it->second.push_back(node);
      FFTS_LOGD("op %s push back %u.", node->GetName().c_str(), thread_scope_id);
    }
  }
  return SUCCESS;
}

Status FFTSPlusGraphOptimizer::AddFunctionNodeInputDesc(ge::OpDescPtr fus_op,
                                                        vector<fe::FusionDataFlow> &fus_input_edge_list) {
  int32_t fusion_dst_index = -1;
  graph_comm_ptr_->ClearFusionSrc();
  vector<bool> fusion_is_input_const_vector;
  uint32_t tensor_desc_index = 0;

  for (fe::FusionDataFlow &dataflow : fus_input_edge_list) {
    auto inedge = dataflow.edge;
    std::pair<string, ge::AnchorPtr> &node_dstindex_pair = dataflow.node_dataindex_pair;
    int64_t src_op_id = inedge.first->GetOwnerNode()->GetOpDesc()->GetId();

    // only support data edge
    fusion_dst_index = fusion_dst_index + 1;
    vector<int64_t> input_vector;
    input_vector = fus_op->GetInputOffset();
    if (static_cast<uint32_t>(fusion_dst_index) < input_vector.size()) {
      return FAILED;
    }

    ge::DataAnchorPtr in_edge_dst_data_anchor_ptr = std::static_pointer_cast<ge::DataAnchor>(inedge.second);
    FFTS_CHECK(in_edge_dst_data_anchor_ptr == nullptr, FFTS_LOGE("inEdgeDstDataAnchorPtr is nullptr."), return FAILED);
    ge::OpDescPtr in_edge_dst_op_desc_ptr = in_edge_dst_data_anchor_ptr->GetOwnerNode()->GetOpDesc();
    vector<bool> is_input_const_vector = in_edge_dst_op_desc_ptr->GetIsInputConst();
    uint32_t dst_anchor_index = static_cast<uint32_t>(in_edge_dst_data_anchor_ptr->GetIdx());
    if (dst_anchor_index < in_edge_dst_op_desc_ptr->GetInputsSize()) {
      if (fus_op->AddInputDesc(*(in_edge_dst_op_desc_ptr->GetInputDescPtr(in_edge_dst_data_anchor_ptr->GetIdx()))) !=
          ge::GRAPH_SUCCESS) {
        FFTS_LOGE("Add input desc failed.");
        return FAILED;
      }
      FFTS_LOGD("fusOp name %s, %d", fus_op->GetName().c_str(), in_edge_dst_data_anchor_ptr->GetIdx());

      if (dst_anchor_index < is_input_const_vector.size()) {
        fusion_is_input_const_vector.push_back(is_input_const_vector[in_edge_dst_data_anchor_ptr->GetIdx()]);
      } else {
        fusion_is_input_const_vector.push_back(false);
      }
      tensor_desc_index++;
    } else {
      int32_t input_desc_size = in_edge_dst_op_desc_ptr->GetInputsSize();
      int32_t is_input_const_size = is_input_const_vector.size();
      int32_t DstIndex = in_edge_dst_data_anchor_ptr->GetIdx();
      FFTS_LOGE("AddFusionNodeInput input_desc_size %u, is_input_const_size %u, input DstIndex %u.",
                (uint32_t)input_desc_size, (uint32_t)is_input_const_size, (uint32_t)DstIndex);
      return FAILED;
    }
    fus_op->SetIsInputConst(fusion_is_input_const_vector);
    graph_comm_ptr_->AddFusionInputSrc(src_op_id, inedge.first, fusion_dst_index, node_dstindex_pair);
  }

  return SUCCESS;
}

Status FFTSPlusGraphOptimizer::AddFunctionNodeOutputDesc(ge::OpDescPtr fus_op,
                                                         vector<fe::FusionDataFlow> &fus_output_edge_list) {
  graph_comm_ptr_->ClearFusionSrc();
  graph_comm_ptr_->ClearFusionDst();

  int32_t fusion_src_index = 0;
  for (fe::FusionDataFlow &dataflow : fus_output_edge_list) {
    auto outedge = dataflow.edge;
    std::pair<string, ge::AnchorPtr> &node_srcindex_pair = dataflow.node_dataindex_pair;
    int64_t dst_op_id = outedge.second->GetOwnerNode()->GetOpDesc()->GetId();
    ge::DataAnchorPtr out_edge_dst_data_anchor_ptr = std::static_pointer_cast<ge::DataAnchor>(outedge.second);
    FFTS_CHECK(out_edge_dst_data_anchor_ptr == nullptr, FFTS_LOGE("outEdgeDstDataAnchorPtr is nullptr."),
               return FAILED);
    if (graph_comm_ptr_->IsFusionDstExist(dst_op_id, outedge.second)) {
      FFTS_LOGI("MergeFusionNodeOutputEdgeList Dstid %u, DstIndex %u.", (uint32_t)dst_op_id,
                (uint32_t)out_edge_dst_data_anchor_ptr->GetIdx());
      continue;
    }
    graph_comm_ptr_->SaveFusionDst(dst_op_id, outedge.second);

    int32_t fusion_src_exist_index;
    int32_t fusion_dst_exist_index;

    ge::DataAnchorPtr out_edge_src_data_anchor_ptr = std::static_pointer_cast<ge::DataAnchor>(outedge.first);
    FFTS_CHECK(out_edge_src_data_anchor_ptr == nullptr, FFTS_LOGE("outEdgeSrcDataAnchorPtr is nullptr."),
               return FAILED);
    ge::OpDescPtr out_edge_src_op_desc_ptr = out_edge_src_data_anchor_ptr->GetOwnerNode()->GetOpDesc();

    auto src_op_id = outedge.first->GetOwnerNode()->GetOpDesc()->GetId();
    if (!graph_comm_ptr_->GetFusionSrc(src_op_id, outedge.first, fusion_src_exist_index, fusion_dst_exist_index)) {
      FFTS_CHECK(out_edge_src_op_desc_ptr == nullptr, FFTS_LOGE("outEdgeSrcOpDescPtr is nullptr."), return FAILED);
      if (static_cast<uint32_t>(out_edge_src_data_anchor_ptr->GetIdx()) < out_edge_src_op_desc_ptr->GetOutputsSize()) {
        FFTS_LOGI_IF(fus_op->AddOutputDesc(*(out_edge_src_op_desc_ptr->GetOutputDescPtr(
            out_edge_src_data_anchor_ptr->GetIdx()))) != ge::GRAPH_SUCCESS, "AddOutputDesc not success");
        graph_comm_ptr_->AddFusionOutputSrc(src_op_id, outedge.first, fusion_src_index, node_srcindex_pair);
      } else {
        int32_t output_desc_size = out_edge_src_op_desc_ptr->GetOutputsSize();
        int32_t SrcIndex = out_edge_src_data_anchor_ptr->GetIdx();
        FFTS_LOGE("MergeFusionNodeOutputEdgeList output_desc_size %u, SrcIndex %u.", (uint32_t)output_desc_size,
                  (uint32_t)SrcIndex);
        return FAILED;
      }
      fusion_src_index++;
    }
  }

  return SUCCESS;
}

Status FFTSPlusGraphOptimizer::TransSubGraphToFunctionOp(ge::ComputeGraph& graph) {
  SubGraphNodeMap node_map;
  if (GetSubGraphNodes(graph, node_map) != SUCCESS) {
    FFTS_LOGE("Get SubGraph Nodes failed, graph %s.", graph.GetName().c_str());
    return FAILED;
  }
  FFTS_LOGD("TransSubGraphToFunctionOp subgraph %s, %p.", graph.GetName().c_str(), &graph);
  std::shared_ptr<fe::GraphComm> graph_comm_ptr = nullptr;
  FFTS_MAKE_SHARED(graph_comm_ptr = std::make_shared<fe::GraphComm>(kFFTSPlusCoreName),
                 return FAILED);
  FFTS_CHECK(graph_comm_ptr == nullptr, FFTS_LOGE("graphCommPtr is nullptr."), return FAILED);
  graph_comm_ptr_ = graph_comm_ptr;
  for (auto &it : node_map) {
    std::vector<ge::NodePtr> node_vec = it.second;
    if (TransSingleSubGraph(graph, node_vec) != SUCCESS) {
      FFTS_LOGE("Get SubGraph Nodes failed, graph %s.", graph.GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

Status FFTSPlusGraphOptimizer::OptimizeGraphBeforeBuild(ge::ComputeGraph &graph) {
  FFTS_LOGD("Begin to OptimizeGraphBeforeBuild [%s].", graph_optimizer_attr_.engineName.c_str());
  for (auto const &node : graph.GetDirectNode()) {
    auto op_desc_ptr = node->GetOpDesc();
    if (!ge::AttrUtils::SetBool(op_desc_ptr, kNoMemReuse, true)) {
      FFTS_LOGE("Set not mem reuse attr of node %s failed.", op_desc_ptr->GetName().c_str());
      return FAILED;
    }
    if (!ge::AttrUtils::SetBool(op_desc_ptr, kNoStreamSplit, true)) {
      FFTS_LOGE("Set not stream split attr of node %s failed.", op_desc_ptr->GetName().c_str());
      return FAILED;
    }
  }

  return SUCCESS;
}

Status FFTSPlusGraphOptimizer::InitLibPath() {
  Dl_info dl_info;
  EngineManager &(*instance_ptr)(const std::string &) = &EngineManager::Instance;
  if (dladdr(reinterpret_cast<void *>(instance_ptr), &dl_info) == 0) {
    REPORT_FFTS_ERROR("[GraphOpt][Init][InitLibPath] Failed to get so file path.");
    return FAILED;
  } else {
    std::string so_path = dl_info.dli_fname;
    FFTS_LOGD("Libfe so file path is %s.", so_path.c_str());

    if (so_path.empty()) {
      REPORT_FFTS_ERROR("[GraphOpt][Init][InitLibPath] So file path is empty.");
      return FAILED;
    }

    lib_path_ = RealPath(so_path);
    int32_t pos = lib_path_.rfind('/');
    if (pos < 0) {
      REPORT_FFTS_ERROR("[GraphOpt][Init][InitLibPath] The path of current so file dose not contain /.");
      return FAILED;
    }

    lib_path_ = lib_path_.substr(0, pos + 1);
  }
  FFTS_LOGD("The real path of libfe is: %s", lib_path_.c_str());
  return SUCCESS;
}

Status FFTSPlusGraphOptimizer::CacheOptionJudge(ge::ComputeGraph& graph) const {
  Status ret = CacheManager::SetCacheOperation(graph);
  if (ret != SUCCESS) {
    REPORT_FFTS_ERROR("[WholeGraphOpt][CacheOptionJudge][SetCacheOperate] Failed to set cache operation on graph [%s].",
                      graph.GetName().c_str());
    return ret;
  }

  for (auto &node : graph.GetAllNodes()) {
    CacheManager::HandleCachePersist(node);
  }
  return SUCCESS;
}
}  // namespace ffts
