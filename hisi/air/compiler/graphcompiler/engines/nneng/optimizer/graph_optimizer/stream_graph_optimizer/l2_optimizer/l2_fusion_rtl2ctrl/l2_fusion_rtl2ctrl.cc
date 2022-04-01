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

#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_rtl2ctrl/l2_fusion_rtl2ctrl.h"
#include <vector>

namespace fe {
void L2FusionRtl2ctrl::InitRtl2ctrl(rtL2Ctrl_t &l2ctrl) {
  l2ctrl.size = 0;

  l2ctrl.l2_in_main = 0;

  for (uint32_t i = 0; i < L2_CTRL_REMAP_SIZE; ++i) {
    l2ctrl.remap[i] = 0;
  }

  for (uint32_t i = 0; i < L2_CTRL_DATA_SIZE; ++i) {
    l2ctrl.data[i].L2_mirror_addr = 0;
    l2ctrl.data[i].L2_data_section_size = 0;
    l2ctrl.data[i].L2_preload = 0;
    l2ctrl.data[i].modified = 0;
    l2ctrl.data[i].priority = 0;
    l2ctrl.data[i].prev_L2_page_offset_base = -1;
    l2ctrl.data[i].L2_page_offset_base = 0;
    l2ctrl.data[i].L2_load_to_ddr = 0;
  }
  return;
}

Status UpdateInputDatal2ctrlId(k_l2_data_allocs_t &input, k_l2_data_allocs_t &standing_data) {
  for (k_l2_data_allocs_t::iterator it = input.begin(); it != input.end(); ++it) {
    FE_CHECK(standing_data.count(it->first) == 0,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][UpdInDataL2CtrlId] Standing Data value is zero."), return fe::FAILED);
    it->second.data_in_l2_id = standing_data[it->first].data_in_l2_id;
  }
  return fe::SUCCESS;
}

Status L2FusionRtl2ctrl::SetDataL2ctrl(uint32_t max_data_num, k_l2_data_allocs_t &data, rtL2Ctrl_t &l2ctrl) const {
  for (auto it = data.cbegin(); it != data.cend(); ++it) {
    uint32_t data_id = it->second.data_in_l2_id;
    FE_CHECK(data_id >= max_data_num,
             FE_LOGD("Data Id is %d, larger than or equal max data number, which is %d", data_id, max_data_num),
             return fe::SUCCESS);
    l2ctrl.data[data_id].L2_mirror_addr = it->second.data.ddr_addr;
    l2ctrl.data[data_id].L2_data_section_size = it->second.data.data_size;
    l2ctrl.data[data_id].L2_preload = it->second.L2_preload;
    l2ctrl.data[data_id].modified = it->second.modified;
    l2ctrl.data[data_id].priority = it->second.priority;
    l2ctrl.data[data_id].prev_L2_page_offset_base = it->second.pre_L2_page_offset_base;
    l2ctrl.data[data_id].L2_page_offset_base = it->second.L2_page_offset_base;
    l2ctrl.data[data_id].L2_load_to_ddr = it->second.L2_load_to_ddr;
  }
  return fe::SUCCESS;
}

Status L2FusionRtl2ctrl::SetOutputDataL2ctrl(uint32_t max_data_num, k_l2_data_allocs_t &standing,
                                             k_l2_data_allocs_t &data, rtL2Ctrl_t &out_l2ctrl) const {
  for (k_l2_data_allocs_t::iterator it = data.begin(); it != data.end(); ++it) {
    if (standing.find(it->first) == standing.end() && it->second.occupy_data_id < 0) {
      uint32_t data_id = it->second.data_in_l2_id;
      FE_CHECK(data_id >= max_data_num,
               FE_LOGD("Data Id is %d, larger than or equal max data number: %d", data_id, max_data_num),
               return fe::SUCCESS);
      out_l2ctrl.data[data_id].L2_mirror_addr = it->second.data.ddr_addr;
      out_l2ctrl.data[data_id].L2_data_section_size = it->second.data.data_size;
      out_l2ctrl.data[data_id].L2_preload = it->second.L2_preload;
      out_l2ctrl.data[data_id].modified = it->second.modified;
      out_l2ctrl.data[data_id].priority = it->second.priority;
      out_l2ctrl.data[data_id].prev_L2_page_offset_base = it->second.pre_L2_page_offset_base;
      out_l2ctrl.data[data_id].L2_page_offset_base = it->second.L2_page_offset_base;
      out_l2ctrl.data[data_id].L2_load_to_ddr = it->second.L2_load_to_ddr;
    } else {
      uint32_t data_id = it->second.data_in_l2_id;
      FE_CHECK(data_id >= max_data_num,
               FE_LOGD("Data Id is %d, larger than or euqal max data number: %d", data_id, max_data_num),
               return fe::SUCCESS);
      out_l2ctrl.data[data_id].L2_mirror_addr = it->second.data.ddr_addr;
    }
  }
  return fe::SUCCESS;
}

// create the l2ctrl in l2
Status L2FusionRtl2ctrl::UpdateRtL2Ctrl(k_l2_task_data_allocs_map_t &alloc, uint32_t max_page_num,
                                        uint32_t max_data_num) {
  std::vector<uint32_t> data_num;
  for (k_l2_task_data_allocs_map_t::iterator it = alloc.begin(); it != alloc.end(); ++it) {
    uint32_t data_id = 0;
    InitRtl2ctrl(it->second.l2ctrl);
    it->second.l2ctrl.size = max_page_num;

    FE_CHECK(SetDataL2ctrl(max_data_num, it->second.standing_data, it->second.l2ctrl) != fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][UpdRtL2Ctrl] SetDataL2ctrl failed!"), return fe::FAILED);

    FE_CHECK(SetOutputDataL2ctrl(max_data_num, it->second.standing_data, it->second.output, it->second.l2ctrl) !=
                 fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][UpdRtL2Ctrl] SetOutputDataL2ctrl failed!"), return fe::FAILED);

    FE_CHECK(SetOutputDataL2ctrl(max_data_num, it->second.standing_data, it->second.converge, it->second.l2ctrl) !=
                 fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][UpdRtL2Ctrl] SetOutputDataL2ctrl failed!"), return fe::FAILED);

    FE_CHECK(SetDataL2ctrl(max_data_num, it->second.filter_preload, it->second.l2ctrl) != fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][UpdRtL2Ctrl] SetDataL2ctrl failed!"), return fe::FAILED);

    FE_CHECK(SetDataL2ctrl(max_data_num, it->second.input_preload, it->second.l2ctrl) != fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][UpdRtL2Ctrl] SetDataL2ctrl failed!"), return fe::FAILED);

    FE_CHECK(UpdateInputDatal2ctrlId(it->second.input, it->second.standing_data) != fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][UpdRtL2Ctrl] UpdateInputDatal2ctrlId failed!"), return fe::FAILED);
    data_num.push_back(data_id);
  }

  return fe::SUCCESS;
}

CCE_DEFINE_SINGLETON(L2FusionRtl2ctrl)
}  // namespace fe
