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

#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_parser/l2_fusion_parser.h"
#include <string>
#include <vector>
#include "common/fe_type_utils.h"
#include "common/fe_utils.h"
#include "common/op_info_common.h"
#include "common/util/op_info_util.h"
#include "graph/debug/ge_attr_define.h"
#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_comm/l2_fusion_comm.h"

namespace fe {

Status L2FusionParser::GetDataDependentCountMap(const ge::ComputeGraph &graph,
                                                k_data_dependent_count_map &data_dependent_count_map) const {
  data_dependent_count_map.clear();

  for (auto node : graph.GetDirectNode()) {
    // jump over CONSTANT and other non-valid node
    if (IsNonValidOp(node)) {
      continue;
    }
    int64_t src_id = node->GetOpDesc()->GetId();
    for (auto out_anchor : node->GetAllOutDataAnchors()) {
      FE_CHECK(out_anchor == nullptr,
               REPORT_FE_ERROR("[StreamOpt][L2Opt][GetDataDepdCountMap] node %s out anchor is null",
               node->GetName().c_str()), return FAILED);
      size_t peer_anchor_num = out_anchor->GetPeerAnchors().size();
      if (peer_anchor_num == 0) {
        continue;
      }
      if (peer_anchor_num > 1) {
        set<ge::NodePtr> peer_anchor_nodes;
        for (auto peer_anchor : out_anchor->GetPeerAnchors()) {
          peer_anchor_nodes.insert(peer_anchor->GetOwnerNode());
        }
        peer_anchor_num = peer_anchor_nodes.size();
      }
      uint64_t tmp_id = DATA_OVERALL_ID(static_cast<uint64_t>(src_id), 1, static_cast<uint32_t>(out_anchor->GetIdx()));
      if (data_dependent_count_map.find(tmp_id) != data_dependent_count_map.end()) {
        FE_LOGD("There is already output tensors, node %s, index %d", node->GetName().c_str(), out_anchor->GetIdx());
      }
      data_dependent_count_map[tmp_id] = peer_anchor_num;
      FE_LOGD("node %s: src_id %ld, out_anchor_index %d; id is %lu, peer_anchor_num is %zu",
              node->GetName().c_str(), src_id, out_anchor->GetIdx(), tmp_id, peer_anchor_num);
    }
  }

  return fe::SUCCESS;
}

bool L2FusionParser::HasAtomicNode(const ge::NodePtr &nodePtr) const {
  for (auto &input_node : nodePtr->GetInControlNodes()) {
    if (input_node->GetType() == ATOMIC_CLEAN_OP_TYPE) {
      return true;
    }
  }
  return false;
}

Status L2FusionParser::GetDataFromGraph(std::vector<ge::NodePtr> &nodes,
                                        ge::ComputeGraph &graph,
                                        k_l2_task_datas_map_t &datas_map) {
  datas_map.clear();

  for (auto &node : nodes) {
    if (NoNeedAllocInputsAndOutputs(node)) {
      continue;
    }

    k_l2_task_datas_t datas;
    datas.node_id = node->GetOpDesc()->GetId();
    datas.node_name = node->GetName();
    FE_CHECK(GetDataFromNode(node, graph, datas) != fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][GetDataFromGph] GetDataFromNode failed!"), return fe::FAILED);
    datas_map.push_back(datas);
  }
  return fe::SUCCESS;
}

Status L2FusionParser::GetDataFromNode(ge::NodePtr node, ge::ComputeGraph &graph, k_l2_task_datas_t &datas) {
  FE_CHECK(node == nullptr, REPORT_FE_ERROR("[StreamOpt][L2Opt][GetDataFromGph] node is nullptr."),
           return fe::FAILED);
  FE_CHECK(ModifyNodeTaskNum(node, datas.node_task_num) != fe::SUCCESS,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetDataFromGph] ModifyNodeTaskNum failed!"),
           return fe::FAILED);
  // the node is not in L2 white list
  FE_CHECK(GetInputDataFromNode(node, graph, (datas.input)) != fe::SUCCESS,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetDataFromGph] GetInputDataFromNode failed!"),
           return fe::FAILED);
  FE_CHECK(GetOutputDataFromNode(node, graph, (datas.output)) != fe::SUCCESS,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetDataFromGph] GetOutputDataFromNode failed!"),
           return fe::FAILED);
  return fe::SUCCESS;
}

Status L2FusionParser::GetInputDataFromNode(ge::NodePtr node, ge::ComputeGraph &graph, k_l2_datas_t &datas) const {
  L2FusionComm *comm_algo = L2FusionComm::GetInstance();
  FE_CHECK(comm_algo == nullptr,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetInDataFromInNd] commAlgo is nullptr."),
           return fe::FAILED);
  ge::OpDescPtr opdef = node->GetOpDesc();
  FE_CHECK(opdef == nullptr,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetInDataFromInNd] opdef is nullptr."),
           return fe::FAILED);

  uint32_t input_size = opdef->GetAllInputsSize();
  uint32_t null_input_size = 0;
  for (uint32_t i = 0; i < input_size; ++i) {
    ge::InDataAnchorPtr in_anchor = node->GetInDataAnchor(i);
    FE_CHECK(in_anchor == nullptr,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][GetInDataFromInNd] node %s in anchor is null",
             node->GetName().c_str()), return fe::FAILED);
    ge::OutDataAnchorPtr out_anchor = in_anchor->GetPeerOutAnchor();
    if (out_anchor == nullptr) {
      null_input_size++;
    }
  }

  // get input
  vector<int64_t> op_input;
  op_input = opdef->GetInputOffset();

  for (uint32_t i = 0; i < input_size; ++i) {
    if (IsConstInput(node, static_cast<size_t>(i))) {
      FE_LOGD("Input is const, do not processed.");
      FE_LOGD("node name is %s, index is %d", node->GetName().c_str(), i);
      continue;
    }
    ge::InDataAnchorPtr in_anchor = node->GetInDataAnchor(i);
    FE_CHECK(in_anchor == nullptr,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][GetInDataFromInNd] node %s in anchor is null",
             node->GetName().c_str()), return fe::FAILED);
    ge::OutDataAnchorPtr ou_anchor = in_anchor->GetPeerOutAnchor();
    // no out anchor, do continue
    if (ou_anchor == nullptr) {
      continue;
    }

    const ge::GeTensorDesc tensor = opdef->GetInputDesc(i);
    uint64_t unit_id = GetDataUnitDataId(node, i, INPUT_DATA, tensor, graph);
    k_l2_data_t tmp;
    tmp.id = unit_id;
    tmp.ddr_addr = (static_cast<uint32_t>(op_input.size()) > i) ? op_input[i] : 0;
    FE_CHECK(comm_algo->GetGraphDataSize(node, i, INPUT_DATA, tmp.data_size) != fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][GetInDataFromInNd] GetGraphDataSize failed!"), return fe::FAILED);
    tmp.occupy_data_ids.clear();
    datas.insert(k_l2_data_pair_t(unit_id, tmp));
  }
  return fe::SUCCESS;
}

Status L2FusionParser::GetOutputDataFromNode(ge::NodePtr node, ge::ComputeGraph &graph, k_l2_datas_t &datas) {
  if (NoNeedAllocOutputs(node)) {
    return SUCCESS;
  }

  L2FusionComm *comm_algo = L2FusionComm::GetInstance();
  FE_CHECK(comm_algo == nullptr,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetOutDataFromInNd] commAlgo is nullptr."), return fe::FAILED);
  ge::OpDescPtr op_desc_ptr = node->GetOpDesc();
  FE_CHECK(op_desc_ptr == nullptr,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetOutDataFromInNd] op_desc_ptr is nullptr."), return fe::FAILED);

  vector<int64_t> output_offset = op_desc_ptr->GetOutputOffset();
  size_t output_size = op_desc_ptr->GetOutputsSize();
  for (size_t i = 0; i < output_size; ++i) {
    auto output_desc = op_desc_ptr->GetOutputDesc(i);
    if (NoNeedAllocOutput(output_desc)) {
      FE_LOGI("Node[type=%s,name=%s]: does not alloc l2 for the output %s.", op_desc_ptr->GetType().c_str(),
              op_desc_ptr->GetName().c_str(), op_desc_ptr->GetOutputNameByIndex(i).c_str());
      continue;
    }
    uint64_t unit_id = GetDataUnitDataId(node, i, OUTPUT_DATA, output_desc, graph);
    k_l2_data_t tmp_data;
    tmp_data.id = unit_id;
    tmp_data.ddr_addr = (output_offset.size() > i) ? output_offset[i] : 0;  // hyz,output
    FE_CHECK(comm_algo->GetGraphDataSize(node, i, OUTPUT_DATA, tmp_data.data_size) != fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][GetOutDataFromInNd] GetGraphDataSize failed!"), return fe::FAILED);
    tmp_data.occupy_data_ids.clear();
    datas.insert(k_l2_data_pair_t(unit_id, tmp_data));
  }
  return SUCCESS;
}

bool L2FusionParser::IsNotSupportOp(const ge::NodePtr &node) const {
  ge::OpDescPtr op_desc = node->GetOpDesc();
  if (CheckVirtualOp(op_desc)) {
    FE_LOGD("Node %s is virtual node, don't alloc l2 buffer.", node->GetName().c_str());
    return true;
  }

  if (HasAtomicNode(node)) {
    FE_LOGD("Node %s has the atomic input node, don't alloc l2 buffer.", node->GetName().c_str());
    return true;
  }
  return false;
}

bool L2FusionParser::NoNeedAllocInputsAndOutputs(const ge::NodePtr &node) const {
  // jump over CONSTANT and other non-valid node
  if (IsNonValidOpOrData(node)) {
    return true;
  }
  return IsNotSupportOp(node);
}

bool L2FusionParser::NoNeedAllocOutputs(const ge::NodePtr &node) const {
  if (node->GetOutDataNodes().size() == 0) {
    FE_LOGD("Node[%s] has no out data node, don't alloc l2 buffer for its outputs.", node->GetName().c_str());
    return true;
  }
  for (const auto &anchor : node->GetAllOutDataAnchors()) {
    if (anchor != nullptr) {
      for (const auto &dst_anchor : anchor->GetPeerInDataAnchors()) {
        FE_CHECK(dst_anchor == nullptr,
                 REPORT_FE_ERROR("[StreamOpt][L2Opt][NoNeedAllocOut] node %s peer in anchor is null",
                 node->GetName().c_str()),
                 return FAILED);
        auto peer_node = dst_anchor->GetOwnerNode();
        if (peer_node->GetType() == fe::OP_TYPE_END) {
          FE_LOGD("Node %s is the last node, don't alloc l2 buffer for its outputs.", node->GetName().c_str());
          return true;
        }
      }
    }
  }

  for (const auto &next_node : node->GetOutNodes()) {
    if (IsNotSupportOp(next_node)) {
      FE_LOGD("Next node %s don't alloc l2 buffer, so don't alloc l2 buffer for node:%s outputs.",
              next_node->GetName().c_str(), node->GetName().c_str());
      return true;
    }
  }
  return false;
}

bool L2FusionParser::NoNeedAllocOutput(const ge::GeTensorDesc &tensor_desc) const {
  return IsMemoryEmpty(tensor_desc);
}

int64_t L2FusionParser::GetDataUnitDataId(ge::NodePtr node, uint32_t data_id, uint32_t data_type,
                                          const ge::GeTensorDesc &tensor,
                                          const ge::ComputeGraph &graph __attribute__((__unused__))) const {
  int64_t node_id = node->GetOpDesc()->GetId();
  // output data
  if (data_type == OUTPUT_DATA) {
    bool is_out_put_tensor = false;
    FE_CHECK(ge::TensorUtils::GetOutputTensor(tensor, is_out_put_tensor) != ge::GRAPH_SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][NoNeedAllocOut] GetOutputTensor failed!"), return fe::FAILED);
    if (!is_out_put_tensor) {  // the data is not graph's output
      return DATA_OVERALL_ID(static_cast<uint64_t>(node_id), OUTPUT_DATA, data_id);
    }
  }

  // input data, only src in graph can be allocted
  if (data_type == INPUT_DATA) {
    ge::InDataAnchorPtr in_anchor = node->GetInDataAnchor(data_id);
    FE_CHECK(in_anchor == nullptr,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][NoNeedAllocOut] node %s in anchor is null", node->GetName().c_str()),
             return fe::FAILED);
    ge::OutDataAnchorPtr ou_anchor = in_anchor->GetPeerOutAnchor();
    if (ou_anchor != nullptr) {
      int64_t src_id = 0;
      ge::OpDescPtr op = ou_anchor->GetOwnerNode()->GetOpDesc();
      src_id = op->GetId();
      return DATA_OVERALL_ID(static_cast<uint64_t>(src_id), OUTPUT_DATA,
                             static_cast<uint32_t>(ou_anchor->GetIdx()));  // find the src data id
    }
  }

  // left data without src node
  return -1;
}

Status L2FusionParser::ModifyNodeTaskNum(ge::NodePtr node, uint32_t &task_num) const {
  ge::OpDescPtr op_desc = node->GetOpDesc();
  FE_CHECK(op_desc == nullptr,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][MdfNdTaskNum] node:%s opdesc is nullptr.", node->GetName().c_str()),
           return FAILED);
  task_num = 1;
  if (op_desc->GetType() == "Scale") {
    task_num = 2;
  }
  return SUCCESS;
}

CCE_DEFINE_SINGLETON(L2FusionParser);

}  // namespace fe
