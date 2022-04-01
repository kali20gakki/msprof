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

#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_optimizer.h"
#include <mutex>
#include "common/fe_utils.h"
#include "common/graph_comm.h"
#include "common/l2_stream_info.h"
#include "common/lxfusion_json_util.h"
#include "common/util/op_info_util.h"
#include "graph/tuning_utils.h"
#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_handler/l2_fusion_handler.h"
#include "common/resource_def.h"

namespace fe {
std::mutex g_stream_l2_map_mutex;
static TaskL2InfoFEMap_t g_l2_info;

L2Optimizer::L2Optimizer(const std::string &engine_name) : engine_name_(engine_name) {}

L2Optimizer::~L2Optimizer() {}

Status L2Optimizer::UpdateInputForL2Fusion(const ge::ComputeGraph &stream_graph) const {
  for (auto &node : stream_graph.GetDirectNode()) {
    FE_LOGD("update input for node:%s.", node->GetName().c_str());
    for (uint8_t i = 0; i < node->GetAllInDataAnchors().size(); ++i) {
      auto in_anchor = node->GetInDataAnchor(i);
      FE_CHECK_NOTNULL(in_anchor);
      auto peer_out_anchor = in_anchor->GetPeerOutAnchor();
      if (peer_out_anchor == nullptr) {
        FE_LOGD("peer_out_anchor is empty");
        continue;
      }
      auto input_node = peer_out_anchor->GetOwnerNode();
      FE_CHECK_NOTNULL(input_node);
      ge::OpDescPtr input_node_desc = input_node->GetOpDesc();
      L2FusionInfoPtr input_l2_info = GetL2FusionInfoFromJson(input_node_desc);
      // input node does not has l2_info
      if (input_l2_info == nullptr) {
        FE_LOGD("node:%s does not has l2_info", input_node->GetName().c_str());
        continue;
      }
      // Because the OPTUNE will splits graph, the node with _data_anchor_index_for_lxfusion attributes is set as the
      // first node. We need to get peer index from opdesc when attr anchorIndex is valid.
      int idx = peer_out_anchor->GetIdx();
      bool is_optune = ge::AttrUtils::GetInt(input_node->GetOpDesc(), "_data_anchor_index_for_lxfusion", idx);
      // update l2info'input with input_l2_info's output
      auto iter = input_l2_info->output.find(idx);
      // input node this output does not alloc l2 address.
      if (iter == input_l2_info->output.end()) {
        FE_LOGD("L2 info : output[%d] of node[%s] is not found.", idx, input_node->GetName().c_str());
        continue;
      }
      // node may alloc l2 input,but not have l2 output
      ge::OpDescPtr node_desc = node->GetOpDesc();
      L2FusionInfoPtr l2_info = GetL2FusionInfoFromJson(node_desc);
      if (l2_info == nullptr) {
        FE_LOGD("node:%s does not has output l2_info", node->GetName().c_str());
        FE_MAKE_SHARED(l2_info = std::make_shared<TaskL2FusionInfo_t>(), return FAILED);
        FE_CHECK_NOTNULL(l2_info);
        SetL2FusionInfoToNode(node_desc, l2_info);
      }

      L2FusionData_t &output = iter->second;
      if (l2_info->input.find(in_anchor->GetIdx()) != l2_info->input.end()) {
        REPORT_FE_ERROR("[StreamOpt][L2Opt][UpdL2FusIn] node:%s l2_info already has input which index is %d.",
                        node->GetName().c_str(), in_anchor->GetIdx());
        return FAILED;
      }
      l2_info->input[in_anchor->GetIdx()] = output;
      // update l2info l2ctrl
      l2_info->l2_info.l2ctrl.data[output.l2Index] = input_l2_info->l2_info.l2ctrl.data[output.l2Index];
      l2_info->l2_info.l2ctrl.size = input_l2_info->l2_info.l2ctrl.size;
      l2_info->l2_info.l2ctrl.data[output.l2Index].prev_L2_page_offset_base =
          l2_info->l2_info.l2ctrl.data[output.l2Index].L2_page_offset_base;
      // Because the OPTUNE will splits graph, the node with _data_anchor_index_for_lxfusion attributes is set as the
      // first node. Therefore, we need to reset the information about the first node generated after the optune splits
      // the graph, the L2_preload of this node must be 1, the prev_L2_page_offset_base of this node must be -1.
      // These information indicates that the node is the first node.
      if (is_optune) {
        l2_info->l2_info.l2ctrl.data[output.l2Index].L2_preload = 1;
        l2_info->l2_info.l2ctrl.data[output.l2Index].prev_L2_page_offset_base = -1;
      }
      FE_LOGD("node:%s input which index is %d set l2_info success.", node->GetName().c_str(), in_anchor->GetIdx());
    }
    ge::OpDescPtr node_desc = node->GetOpDesc();
    L2FusionInfoPtr l2_info = GetL2FusionInfoFromJson(node_desc);
    FE_LOGD("Set all l2fusion info to node:[%s].", node_desc->GetName().c_str());
    SetL2FusionInfoToNode(node_desc, l2_info);
  }
  return SUCCESS;
}
Status L2Optimizer::UpdateDDRForL2Fusion(const ge::ComputeGraph &stream_graph, uint64_t mem_base) const {
  for (auto &node : stream_graph.GetDirectNode()) {
    FE_LOGD("update ddr address for node:%s.", node->GetName().c_str());
    ge::OpDescPtr node_desc = node->GetOpDesc();
    L2FusionInfoPtr l2_info = GetL2FusionInfoFromJson(node_desc);
    if (l2_info == nullptr) {
      FE_LOGD("node:%s does not has l2_info", node->GetName().c_str());
      continue;
    }

    vector<uint32_t> processed_l2_index;
    vector<int64_t> input_vector = node->GetOpDesc()->GetInputOffset();
    vector<int64_t> output_vector = node->GetOpDesc()->GetOutputOffset();
    // update input ddr value
    for (uint32_t i = 0; i < input_vector.size(); ++i) {
      auto iter = l2_info->input.find(i);
      if (iter == l2_info->input.end()) {
        continue;
      }
      L2FusionData_t &input = iter->second;
      l2_info->input[mem_base + input_vector[i]] = input;
      l2_info->l2_info.l2ctrl.data[input.l2Index].L2_mirror_addr = mem_base + input_vector[i];
      processed_l2_index.push_back(input.l2Index);
      l2_info->input.erase(i);
      FE_LOGD("node:%s input which index is %u set ddr address:0x%lx success.", node->GetName().c_str(), i,
              mem_base + input_vector[i]);
    }

    // update output ddr value
    for (size_t i = 0; i < output_vector.size(); ++i) {
      auto iter = l2_info->output.find(i);
      if (iter == l2_info->output.end()) {
        continue;
      }
      L2FusionData_t &output = iter->second;
      l2_info->output[mem_base + output_vector[i]] = output;
      l2_info->l2_info.l2ctrl.data[output.l2Index].L2_mirror_addr = mem_base + output_vector[i];
      processed_l2_index.push_back(output.l2Index);
      l2_info->output.erase(i);
      FE_LOGD("node:%s output which index is %zu set ddr address:0x%lx success.", node->GetName().c_str(), i,
              mem_base + output_vector[i]);
    }

    for (uint32_t i = 0; i < L2_MAXDATANUM; ++i) {
      if (find(processed_l2_index.begin(), processed_l2_index.end(), i) == processed_l2_index.end() &&
          !l2_info->l2_info.node_name[i].empty()) {
        for (auto &node_ptr : stream_graph.GetDirectNode()) {
          if (node_ptr->GetName() == l2_info->l2_info.node_name[i]) {
            vector<int64_t> output = node_ptr->GetOpDesc()->GetOutputOffset();
            uint8_t output_index = l2_info->l2_info.output_index[i];
            l2_info->l2_info.l2ctrl.data[i].L2_mirror_addr = mem_base + output[output_index];
            FE_LOGD("node:%s output which index is %u set ddr address:0x%lx success.", node_ptr->GetName().c_str(), i,
                    mem_base + output[output_index]);
          }
        }
      }
    }
  }
  return SUCCESS;
}
/*
 *  @ingroup fe
 *  @brief   allocate L2 address
 *  @param   [in] (single) stream graph
 *  @param   [in] mem_base
 *  @param   [in] stream_id
 *  @return  SUCCESS or FAILED
 */
Status L2Optimizer::GetL2DataAlloc(ge::ComputeGraph &stream_graph, uint64_t mem_base,
                                   const rtStream_t &streamId) const {
  L2FusionHandler *handler_algo = L2FusionHandler::GetInstance();
  FE_CHECK(handler_algo == nullptr,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetL2DataAlloc] handlerAlgo is nullptr."), return fe::FAILED);

  Configuration &configure = Configuration::Instance(engine_name_);
  std::string build_mode_value = configure.GetGeContextOptionValue(ge::BUILD_MODE);
  // l2 buffer
  if ((CheckL2BufferFusionStrategy(stream_graph) &&
       configure.GetAppendArgsMode() == AppendArgsMode::L2_BUFFER_ARGS &&
       build_mode_value != ge::BUILD_MODE_TUNING)) {
    FE_LOGD("L2 buffer enable. build mode is %s, graph_name: %s",
            build_mode_value.c_str(), stream_graph.GetName().c_str());
    std::lock_guard<std::mutex> lock_guard(g_stream_l2_map_mutex);
    FE_CHECK(handler_algo->GetL2DataAlloc(mem_base, stream_graph, g_l2_info) != fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][GetL2DataAlloc] Allocate L2 Buffer Address failed!"),
             return fe::FAILED);
    FE_LOGD("Allocate L2 Buffer Address for stream graph successfully.");

    FE_CHECK(!setFuncState(fe::FuncParamType::FUSION_L2, true),
             REPORT_FE_ERROR("[StreamOpt][L2Opt][GetL2DataAlloc] Set Func State true failed!"),
             return fe::FAILED);

    FE_LOGD("Set function state successfully.");
    std::string batch_label = "Batch_-1";
    (void)ge::AttrUtils::GetStr(stream_graph, ge::ATTR_NAME_BATCH_LABEL, batch_label);
    FE_CHECK(StreamL2Info::Instance().SetStreamL2Info(streamId, g_l2_info, batch_label) != fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][UpdL2FusIn] Set Stream L2 Map failed!"), return fe::FAILED);
    FE_LOGD("Set stream L2 map successfully.");
  }
  if (build_mode_value == ge::BUILD_MODE_TUNING ||
      (configure.GetBufferFusionMode() == EN_L2_FUSION && CheckL2FusionFusionStrategy(stream_graph))) {
    FE_CHECK(!setFuncState(fe::FuncParamType::FUSION_L2, false),
             REPORT_FE_ERROR("[StreamOpt][L2Opt][GetL2DataAlloc] Set Func State false failed!"),
             return fe::FAILED);
    // update Input
    FE_CHECK(UpdateInputForL2Fusion(stream_graph) != fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][GetL2DataAlloc] Updata input for L2Fusion failed!"),
             return fe::FAILED);
    // update ddr
    FE_CHECK(UpdateDDRForL2Fusion(stream_graph, mem_base) != fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][GetL2DataAlloc] UpdataInputForL2Fusion failed!"),
             return fe::FAILED);
    for (auto node : stream_graph.GetDirectNode()) {
      (void)ge::AttrUtils::SetBool(node->GetOpDesc(), ATTR_NAME_LX_FUSION_PASS, true);
    }
  }
  return fe::SUCCESS;
}

}  // namespace fe
