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

#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_handler/l2_fusion_handler.h"
#include <map>
#include <string>
#include <vector>
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "graph/utils/anchor_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_allocation/l2_fusion_allocation.h"
#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_comm/l2_fusion_comm.h"
#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_parser/l2_fusion_parser.h"
#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_rtl2ctrl/l2_fusion_rtl2ctrl.h"

namespace fe {
Status L2FusionHandler::GetL2DataAlloc(uint64_t mem_base, ge::ComputeGraph &graph, TaskL2InfoFEMap_t &l2_info) const {
  FE_LOGD("We use normal l2 buffer here");
  FE_CHECK(GetNormalL2DataAlloc(mem_base, graph, l2_info) != fe::SUCCESS,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][UpdL2FusIn] GetNormalL2DataAlloc failed!"),
           return fe::FAILED);
  return fe::SUCCESS;
}

Status L2FusionHandler::GetNormalL2DataAlloc(uint64_t mem_base, ge::ComputeGraph &graph,
                                             TaskL2InfoFEMap_t &l2_info) const {
  L2FusionComm *comm_algo = L2FusionComm::GetInstance();
  L2FusionParser *parser_algo = L2FusionParser::GetInstance();
  L2FusionAllocation *alloc_algo = L2FusionAllocation::GetInstance();
  L2FusionRtl2ctrl *l2ctrl_algo = L2FusionRtl2ctrl::GetInstance();

  FE_CHECK(comm_algo == nullptr,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetNormalL2DataAlloc] commAlgo is nullptr."), return fe::FAILED);
  FE_CHECK(parser_algo == nullptr,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetNormalL2DataAlloc] parserAlgo is nullptr."), return fe::FAILED);
  FE_CHECK(alloc_algo == nullptr,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetNormalL2DataAlloc] allocAlgo is nullptr."), return fe::FAILED);
  FE_CHECK(l2ctrl_algo == nullptr,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetNormalL2DataAlloc] l2ctrlAlgo is nullptr."), return fe::FAILED);

  k_l2_buffer_t l2;
  k_data_dependent_count_map count_map;
  k_l2_task_datas_map_t datas_map;
  k_l2_data_allocs_t standing_data_alloc;
  k_l2_task_data_allocs_map_t l2_alloc;

  FE_CHECK(comm_algo->GetL2HardwareSet(l2) != fe::SUCCESS,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetNormalL2DataAlloc] GetL2HardwareSet failed!"), return fe::FAILED);
  FE_CHECK(parser_algo->GetDataDependentCountMap(graph, count_map) != fe::SUCCESS,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetNormalL2DataAlloc] GetDataDependentCountMap failed!"),
           return fe::FAILED); /* get dependect map for alloc */

  // we need trans Vistor to vector here, this is not good.
  ge::ComputeGraph::Vistor<ge::NodePtr> nodes = graph.GetDirectNode();
  std::vector<ge::NodePtr> nodes_vector;
  nodes_vector.insert(nodes_vector.end(), nodes.begin(), nodes.end());
  // get data which can be allocated in l2
  FE_CHECK(parser_algo->GetDataFromGraph(nodes_vector, graph, datas_map) != fe::SUCCESS,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetNormalL2DataAlloc] GetDataFromGraph failed!"), return fe::FAILED);
  comm_algo->DisplayParserData(datas_map);

  uint64_t max_page_num = 0;
  FE_CHECK(alloc_algo->AllocateData(datas_map, count_map, l2, standing_data_alloc, l2_alloc,
                                    max_page_num) != fe::SUCCESS,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetNormalL2DataAlloc] Allocate Data failed!"), return fe::FAILED);
  FE_CHECK(l2ctrl_algo->UpdateRtL2Ctrl(l2_alloc, static_cast<uint32_t>(max_page_num), l2.max_data_num) != fe::SUCCESS,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetNormalL2DataAlloc] UpdateRtL2Ctrl failed!"), return fe::FAILED);
  comm_algo->DisplayL2AllocInfo(l2_alloc);
  FE_CHECK(GenerateFinalL2Info(mem_base, graph, l2_alloc, l2_info) != fe::SUCCESS,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetNormalL2DataAlloc] GenerateFinalL2Info failed!"), return fe::FAILED);
  FE_CHECK(DisplayL2Info(l2_info) != fe::SUCCESS,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetNormalL2DataAlloc] DisplayL2Info failed!"), return fe::FAILED);
  return fe::SUCCESS;
}

void L2FusionHandler::GenerateL2Data(uint64_t mem_base, const k_l2_data_allocs_t &src_data,
                                     L2DataMap_t &dst_data) const {
  dst_data.clear();
  for (k_l2_data_allocs_t::const_iterator data_it = src_data.begin(); data_it != src_data.end(); ++data_it) {
    if (CheckUint64AddOverflow(mem_base, data_it->second.data.ddr_addr) != SUCCESS) {
      REPORT_FE_ERROR("[StreamOpt][L2Opt][GenerL2Data] UINT64 %lu and %lu addition can result in overflow!",
                      mem_base, data_it->second.data.ddr_addr);
      return;
    }
    uint64_t ddr_addr = mem_base + data_it->second.data.ddr_addr;
    L2Data_t data_tmp;
    data_tmp.l2Index = data_it->second.data_in_l2_id;
    data_tmp.l2Addr = data_it->second.data_in_l2_addr;
    data_tmp.l2PageNum = data_it->second.l2PageNum;
    dst_data.insert(L2DataPair_t(ddr_addr, data_tmp));
  }
  return;
}

Status L2FusionHandler::GenerateFinalL2Info(uint64_t mem_base, ge::ComputeGraph &graph,
                                            k_l2_task_data_allocs_map_t &l2_alloc, TaskL2InfoFEMap_t &l2) const {
  l2.clear();
  // ComputeGraph::Vistor<ge::NodePtr > nodes = graph.GetDirectNode();
  ge::ComputeGraph::Vistor<ge::NodePtr> nodes = graph.GetDirectNode();
  for (ge::NodePtr node : nodes) {
    if (IsNonValidOp(node)) {
      continue;
    }

    FE_LOGD("now we GenerateFinalL2Info for node:%s", node.get()->GetName().c_str());
    TaskL2Info_t tmp;
    tmp.nodeName = node->GetName();
    k_l2_task_data_allocs_map_t::iterator alloc_iterator = l2_alloc.find(node->GetName());
    int64_t node_id = node->GetOpDesc()->GetId();
    FE_LOGD("nodeId is %ld", node_id);
    if (alloc_iterator != l2_alloc.end()) {
      tmp.l2ctrl = alloc_iterator->second.l2ctrl;
      for (uint32_t i = 0; i < L2_CTRL_DATA_SIZE; ++i) {
        if (tmp.l2ctrl.data[i].L2_data_section_size > 0) {
          FE_UINT64_ADDCHECK(tmp.l2ctrl.data[i].L2_mirror_addr, mem_base);
          tmp.l2ctrl.data[i].L2_mirror_addr += mem_base;
          // load l2 data to ddr if we need dump l2 data.(1 - need load out, 0 - no need)
          tmp.l2ctrl.data[i].L2_load_to_ddr = 0;
        }
      }
      GenerateL2Data(mem_base, alloc_iterator->second.input, tmp.input);
      GenerateL2Data(mem_base, alloc_iterator->second.output, tmp.output);
    } else {
      FE_LOGD("node %s, id = %ld, do not has l2ctrl", node->GetName().c_str(), node_id);
      L2FusionRtl2ctrl *l2_fusion_rtl2ctrl = L2FusionRtl2ctrl::GetInstance();
      FE_CHECK(l2_fusion_rtl2ctrl == nullptr,
               REPORT_FE_ERROR("[StreamOpt][L2Opt][GenerFinalL2Info] l2FusionRtl2ctrl is nullptr."), return fe::FAILED);
      l2_fusion_rtl2ctrl->InitRtl2ctrl(tmp.l2ctrl);
      tmp.input.clear();
      tmp.output.clear();
    }
    l2.insert(TaskL2InfoFEPair_t(node->GetName(), tmp));
  }
  return fe::SUCCESS;
}

Status L2FusionHandler::DisplayL2DataInfo(string title, L2DataMap_t &input, rtL2Ctrl_t &l2ctrl,
                                          L2DataMap_t data) const {
  L2FusionComm *comm_algo = L2FusionComm::GetInstance();
  FE_CHECK(comm_algo == nullptr,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][DisplayL2DataInfo] commAlgo is nullptr."), return fe::FAILED);
  k_l2_buffer_t l2;
  FE_CHECK(comm_algo->GetL2HardwareSet(l2) != fe::SUCCESS,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][DisplayL2DataInfo] GetL2HardwareSet failed!"), return fe::FAILED);
  int64_t page_size = 0;
  if (l2.page_num == 0) {
    FE_LOGW("L2 page num is zero.");
  } else {
    page_size = l2.l2_buffer_size / l2.page_num;
  }
  for (auto it = data.cbegin(); it != data.cend(); ++it) {
    FE_LOGD("%s: data_in_l2_id = %d, if_input = %u, data_size = %7d, l2_page_n = %lu", title.c_str(),
            it->second.l2Index, (uint32_t)(input.count(it->first)),
            l2ctrl.data[it->second.l2Index].L2_data_section_size, it->second.l2PageNum);
    FE_LOGD("l2AddrO = %lu, ddr_addr_key = %lu, ddr_addr = %lu, offset = %2u, l2_load_to_ddr = %u",
            it->second.l2Addr - (l2ctrl.data[it->second.l2Index].L2_page_offset_base) * page_size, it->first,
            l2ctrl.data[it->second.l2Index].L2_mirror_addr,
            (uint32_t)l2ctrl.data[it->second.l2Index].L2_page_offset_base,
            (uint32_t)l2ctrl.data[it->second.l2Index].L2_load_to_ddr);
    FE_LOGD("prev_offset = %2d", (int32_t)l2ctrl.data[it->second.l2Index].prev_L2_page_offset_base);
  }
  return fe::SUCCESS;
}

Status L2FusionHandler::DisplayL2Info(TaskL2InfoFEMap_t &l2) const {
  for (TaskL2InfoFEMap_t::iterator node_it = l2.begin(); node_it != l2.end(); ++node_it) {
    rtL2Ctrl_t l2ctrl = node_it->second.l2ctrl;
    TaskL2Info_t &l2_info = node_it->second;
    FE_LOGD("node key is %s, nodeName = %s, l2ctrl.size = %lu", node_it->first.c_str(), l2_info.nodeName.c_str(),
            l2ctrl.size);
    FE_LOGD("inputNum = %lu, output_num = %lu", l2_info.input.size(), l2_info.output.size());

    FE_CHECK(DisplayL2DataInfo("input", l2_info.input, l2ctrl, l2_info.input) != fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][DisplayL2Info] Display L2 data Info for input failed!"),
             return fe::FAILED);
    FE_CHECK(DisplayL2DataInfo("output", l2_info.input, l2ctrl, l2_info.output) != fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][DisplayL2Info] Display L2 data Info for output failed!"),
             return fe::FAILED);
  }
  return fe::SUCCESS;
}

CCE_DEFINE_SINGLETON(L2FusionHandler)
}  // namespace fe
