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

#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_allocation/l2_fusion_allocation.h"
#include <cmath>
#include "common/fe_inner_error_codes.h"
#include "common/fe_utils.h"

namespace fe {

Status L2FusionAllocation::AllocateStandingDataByTaskNum(uint32_t node_task_num, uint32_t &data_in_l2_id,
                                                         uint64_t page_size, k_data_dependent_count_map &count_map,
                                                         k_l2_data_allocs_t &standing_data_alloc,
                                                         k_l2_datas_t &output,
                                                         int32_t &page_num_left) {
  FE_LOGD("nodeTaskNum is %d", node_task_num);
  if (node_task_num > 1) {
    // kernel with multi tasks (fc & spatial transfprmer) should deal with the
    // standing specially,
    FE_CHECK(AllocateStandingDataSpecial(data_in_l2_id, 0, page_size, count_map, standing_data_alloc, output,
                                         page_num_left) != fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][AllocStandData] AllocateStandingDataSpecial failed!"),
             return fe::FAILED);
  } else {
    FE_CHECK(AllocateStandingData(data_in_l2_id, 0, page_size, count_map, standing_data_alloc, output, page_num_left) !=
                 fe::SUCCESS,
             FE_LOGW("AllocateStandingData failed!"), return fe::FAILED);
  }
  FE_CHECK(page_num_left < 0,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][AllocStandData] Page Number left less than zero!"), return fe::FAILED);
  return fe::SUCCESS;
}

Status L2FusionAllocation::AllocateData(k_l2_task_datas_map_t &data, k_data_dependent_count_map &count_map,
                                        k_l2_buffer_t &l2, k_l2_data_allocs_t &standing_data_alloc,
                                        k_l2_task_data_allocs_map_t &alloc,
                                        uint64_t &max_page_num) {
  if (data.empty()) {
    FE_LOGW("The data need to be allocated is empty!");
  }
  FE_CHECK(l2.page_num == 0, REPORT_FE_ERROR("[StreamOpt][L2Opt][AllocData] l2 page Num is zero!"), return fe::FAILED);
  uint64_t page_size = l2.l2_buffer_size / l2.page_num;
  k_l2_data_allocs_t all_output_alloc;

  alloc.clear();
  FE_LOGD("AllocateData data size is %zu ", data.size());
  for (k_l2_task_datas_map_t::iterator it = data.begin(); it != data.end(); ++it) {
    FE_LOGD("Now we alloc L2 for node %s !", it->node_name.c_str());
    int32_t page_num_left = static_cast<int32_t>(l2.page_num);
    int32_t data_numleft = static_cast<int32_t>(l2.max_data_num);
    k_l2_datas_t input = it->input;
    k_l2_datas_t &output = it->output;
    uint32_t data_in_l2_id = 0;
    FE_LOGD("pageNumLeft is %d, l2.page_num is %lu", page_num_left, l2.page_num);

    FE_CHECK(AllocateStandingDataByTaskNum(it->node_task_num, data_in_l2_id, page_size, count_map, standing_data_alloc,
                                           output, page_num_left) != fe::SUCCESS,
             FE_LOGW("AllocateStandingDataByTaskNum failed!"), return fe::FAILED);

    /* update alloc info */
    k_l2_task_data_allocs_t tmp_alloc;
    tmp_alloc.node_id = it->node_id;
    tmp_alloc.node_name = it->node_name;
    tmp_alloc.standing_data = standing_data_alloc;
    FE_CHECK(AllocateInputData(standing_data_alloc, input, count_map, tmp_alloc.input) != fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][AllocData] AllocateInputData for node:%s  failed!",
             it->node_name.c_str()), return fe::FAILED);
    data_numleft = data_numleft - standing_data_alloc.size();
    FE_CHECK(AllocateOutputData(data_in_l2_id, 1, page_size, l2.page_num, data_numleft, page_num_left, output,
                                standing_data_alloc, tmp_alloc.output) != fe::SUCCESS,
             FE_LOGW("Allocate OutputData not success!"), return fe::FAILED);

    alloc.insert(k_l2_task_data_allocs_pair_t(tmp_alloc.node_name, tmp_alloc));

    /* update max page num info */
    FE_LOGD(
        "After Alloc page_num_left value is %d, max_page_num is %lu,"
        "l2.page_num %lu",
        page_num_left, max_page_num, l2.page_num);
    if (l2.page_num - page_num_left > max_page_num) {
      max_page_num = l2.page_num - page_num_left;
    }
  }
  return fe::SUCCESS;
}

Status EraseStandingDataAllocCountMapZero(k_l2_data_allocs_t &standing_data_alloc,
                                          k_data_dependent_count_map &count_map) {
  for (auto it = standing_data_alloc.cbegin(); it != standing_data_alloc.cend();) {
    auto iter = count_map.find(it->first);
    if (iter != count_map.end() && iter->second == 0) {
      FE_LOGD("countMap[%lu] is 0, need to erase it.", it->first);
      it = standing_data_alloc.erase(it);
    } else {
      FE_LOGD("countMap[%lu] is %d, no need to erase it.", it->first, count_map[it->first]);
      ++it;
    }
  }
  return fe::SUCCESS;
}

Status L2FusionAllocation::AllocateStandingData(uint32_t &data_in_l2_id, uint8_t priority, int64_t page_size,
                                                k_data_dependent_count_map &count_map,
                                                k_l2_data_allocs_t &standing_data_alloc, k_l2_datas_t &output,
                                                int32_t &page_num_left) const {
  FE_LOGD("standingDataAlloc size is %zu", standing_data_alloc.size());
  FE_CHECK(EraseStandingDataAllocCountMapZero(standing_data_alloc, count_map) != fe::SUCCESS,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][AllocStandData] Erase standing Data Allocation for CountMap \
                           zero failed!"), return fe::FAILED);
  uint32_t ret = UpdateStandingDataSizeWithOutputSet(page_size, standing_data_alloc, output, page_num_left);
  FE_CHECK(ret != fe::SUCCESS, FE_LOGD("UpdateStandingDataSizeWithOutputSet directly return."), return ret);
  uint8_t cur_page_offset_base = 0;
  for (k_l2_data_allocs_t::iterator it = standing_data_alloc.begin(); it != standing_data_alloc.end(); ++it) {
    k_l2_data_alloc_t &cur_l2_data = it->second;
    cur_l2_data.data_in_l2_id = static_cast<int32_t>(data_in_l2_id++);
    cur_l2_data.pre_L2_page_offset_base = static_cast<int8_t>(cur_l2_data.L2_page_offset_base);
    cur_l2_data.L2_page_offset_base = cur_page_offset_base;
    cur_l2_data.priority = priority;

    FE_INT64_MULCHECK(static_cast<int64_t>(cur_l2_data.L2_page_offset_base), static_cast<int64_t>(page_size));
    cur_l2_data.data_in_l2_addr = static_cast<uint64_t>(static_cast<int64_t>(cur_l2_data.L2_page_offset_base) *
                                                        static_cast<int64_t>(page_size));

    FE_UINT8_ADDCHECK(cur_page_offset_base, static_cast<uint8_t>(cur_l2_data.l2PageNum));
    cur_page_offset_base += static_cast<uint8_t>(cur_l2_data.l2PageNum);
  }
  for (auto it = standing_data_alloc.cbegin(); it != standing_data_alloc.cend(); ++it) {
    page_num_left -= it->second.l2PageNum;
    FE_CHECK((page_num_left < 0),
             REPORT_FE_ERROR("[StreamOpt][L2Opt][AllocStandData] Left page number %d, less than zero", page_num_left),
             return fe::FAILED);
  }
  return fe::SUCCESS;
}

Status L2FusionAllocation::AllocateStandingDataSpecial(uint32_t &data_in_l2_id, uint8_t priority, int64_t page_size,
                                                       k_data_dependent_count_map &count_map,
                                                       k_l2_data_allocs_t &standing_data_alloc, k_l2_datas_t &output,
                                                       int32_t &page_num_left) const {
  UNUSED(output);
  FE_CHECK(EraseStandingDataAllocCountMapZero(standing_data_alloc, count_map) != fe::SUCCESS,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][AllocStandDataSpec] Erase standing Data Allocation for CountMap \
                           zero failed!"), return fe::FAILED);
  int32_t max_page_num = static_cast<int32_t>(page_num_left);
  for (k_l2_data_allocs_t::iterator it = standing_data_alloc.begin(); it != standing_data_alloc.end(); ++it) {
    k_l2_data_alloc_t &cur_l2_data = it->second;
    cur_l2_data.data_in_l2_id = static_cast<int32_t>(data_in_l2_id++);
    cur_l2_data.pre_L2_page_offset_base = static_cast<int8_t>(cur_l2_data.L2_page_offset_base);
    cur_l2_data.L2_page_offset_base = cur_l2_data.pre_L2_page_offset_base;
    cur_l2_data.priority = priority;

    FE_INT64_MULCHECK(static_cast<int64_t>(cur_l2_data.L2_page_offset_base), static_cast<int64_t>(page_size));
    cur_l2_data.data_in_l2_addr = static_cast<uint64_t>(static_cast<int64_t>(cur_l2_data.L2_page_offset_base) *
                                                        static_cast<int64_t>(page_size));

    FE_INT32_SUBCHECK(max_page_num, static_cast<int32_t>(cur_l2_data.L2_page_offset_base));
    int32_t tmp = max_page_num - static_cast<int32_t>(cur_l2_data.L2_page_offset_base);
    FE_INT32_SUBCHECK(tmp, static_cast<int32_t>(cur_l2_data.l2PageNum));

    page_num_left =
        MIN(page_num_left, max_page_num - static_cast<int32_t>(cur_l2_data.L2_page_offset_base) -
                               static_cast<int32_t>(cur_l2_data.l2PageNum));
    FE_CHECK((page_num_left < static_cast<int32_t>(0)),
             REPORT_FE_ERROR("[StreamOpt][L2Opt][AllocStandDataSpec] Left page number is %d, less than zero.",
                             page_num_left),
             return fe::FAILED);
  }
  return fe::SUCCESS;
}

Status L2FusionAllocation::AllocateInputData(k_l2_data_allocs_t &standing, k_l2_datas_t &input,
                                             k_data_dependent_count_map &count_map,
                                             k_l2_data_allocs_t &input_alloc) const {
  k_l2_datas_t input_left;
  input_alloc.clear();
  for (auto it = input.cbegin(); it != input.cend(); ++it) {
    FE_CHECK(count_map.find(it->first) == count_map.end(),
             REPORT_FE_ERROR("[StreamOpt][L2Opt][AllocInData] Input can not be find in count map.input id is:%zu",
             it->first), return fe::FAILED);
    FE_CHECK(--count_map[it->first] < 0,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][AllocInData] Count map value %d is less than zero, id is %zu",
             count_map[it->first], it->first),
             return fe::FAILED);
    k_l2_data_allocs_t::const_iterator standing_alloc = standing.find(it->first);
    FE_LOGD("standing size is %zu", standing.size());
    if (standing_alloc != standing.end()) {
      input_alloc.insert(k_l2_data_alloc_pair_t(it->first, standing_alloc->second));
    } else {
      input_left.insert(k_l2_data_pair_t(it->first, it->second));
    }
  }
  input = input_left;
  return fe::SUCCESS;
}

Status InitDataAlloc(uint32_t &data_in_l2_id, const uint8_t priority, const int32_t page, const uint32_t max_page_num,
                     int32_t &page_num_left, const int64_t page_size, const k_l2_data_t &data,
                     k_l2_data_alloc_t &tmp_l2_data) {
  tmp_l2_data.data_in_l2_id = static_cast<int32_t>(data_in_l2_id++);
  tmp_l2_data.data = data;
  tmp_l2_data.l2PageNum = static_cast<uint64_t>(page);
  tmp_l2_data.pre_L2_page_offset_base = -1;
  if ((static_cast<int64_t>(max_page_num) - static_cast<int64_t>(page_num_left) < 0) ||
      (static_cast<int64_t>(max_page_num) - static_cast<int64_t>(page_num_left) > UINT8_MAX)) {
    REPORT_FE_ERROR("[StreamOpt][L2Opt][InitDataAlloc] L2_page_offset_base init failed, max_page_num %d, page_num_left \
                    %d.", max_page_num, page_num_left);
    return FAILED;
  }
  tmp_l2_data.L2_page_offset_base = static_cast<uint8_t>(static_cast<uint16_t>(static_cast<int64_t>(max_page_num) -
                                                                               static_cast<int64_t>(page_num_left)));
  tmp_l2_data.data_in_l2_addr = static_cast<uint64_t>(tmp_l2_data.L2_page_offset_base) *
                                static_cast<uint64_t>(page_size);
  tmp_l2_data.modified = 1;
  tmp_l2_data.L2_preload = 0;
  tmp_l2_data.priority = priority;
  tmp_l2_data.L2_load_to_ddr = 0;
  tmp_l2_data.occupy_data_id = -1;
  return fe::SUCCESS;
}

Status L2FusionAllocation::AllocateOutputData(uint32_t &data_in_l2_id, uint8_t priority, int64_t page_size,
                                              uint32_t max_page_num, int32_t &data_num_left, int32_t &page_num_left,
                                              k_l2_datas_t &output, k_l2_data_allocs_t &standing_allocs,
                                              k_l2_data_allocs_t &output_allocs) const {
  // directly return
  FE_CHECK(page_num_left < 0, FE_LOGD("Left page number is less than zero, directly return."), return fe::SUCCESS);
  FE_CHECK(output.size() == 0, FE_LOGD("Output size is zero, directly return."), return fe::SUCCESS);

  // error checking
  FE_CHECK(page_size == 0, REPORT_FE_ERROR("[StreamOpt][L2Opt][AllocOutData] Page size is zero."), return fe::FAILED);
  for (auto it = output.cbegin(); it != output.cend(); ++it) {
    FE_LOGD("output id is %lu", it->first);
    k_l2_data_allocs_t::iterator standing_alloc = standing_allocs.find(it->first);
    if (standing_alloc == standing_allocs.end()) {
      const double const_double = 1.0;
      int32_t page = static_cast<int32_t>(std::ceil(const_double * it->second.data_size / page_size));
      FE_LOGD("page is %d,page_size is %ld,page_num_left is %d.", page, page_size, page_num_left);
      // the data num and page num both has left space
      bool is_has_left_space = (data_num_left > 0) && (page <= page_num_left);
      FE_LOGD("isHasLeftSpace is %d", is_has_left_space);
      if (is_has_left_space) {
        k_l2_data_alloc_t tmp;
        FE_CHECK(InitDataAlloc(data_in_l2_id, priority, page, max_page_num, page_num_left, page_size, it->second,
                               tmp) != fe::SUCCESS,
                 REPORT_FE_ERROR("[StreamOpt][L2Opt][AllocOutData] InitDataAlloc failed!"), return fe::FAILED);
        output_allocs.insert(k_l2_data_alloc_pair_t(it->first, tmp));
        standing_allocs.insert(k_l2_data_alloc_pair_t(it->first, tmp));
        FE_INT32_SUBCHECK(page_num_left, page);
        page_num_left = page_num_left - page;
        data_num_left--;
      }
    } else {
      FE_LOGD("standingAlloc id is :%lu. it id is %lu", standing_alloc->first, it->first);
      // the output is already allocted
      // converge data, make sure standing space is enough for output data
      FE_CHECK(standing_alloc->second.data.data_size < it->second.data_size,
               FE_LOGW("Standing data size %ld, less than output size %ld.", standing_alloc->second.data.data_size,
                       it->second.data_size),
               return fe::FAILED);
      output_allocs.insert(k_l2_data_alloc_pair_t(it->first, standing_alloc->second));
    }
  }
  return fe::SUCCESS;
}

Status L2FusionAllocation::UpdateStandingDataSizeWithOutputSet(int64_t page_size, k_l2_data_allocs_t &standing_allocs,
                                                               k_l2_datas_t &output, int32_t page_num_left) const {
  FE_CHECK(page_size == 0,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][UpdStandDataSize] pageSize is zero."), return fe::FAILED);
  for (k_l2_data_allocs_t::iterator it = standing_allocs.begin(); it != standing_allocs.end(); ++it) {
    k_l2_datas_t::iterator data_iterator = output.find(it->first);

    // output batch
    if (data_iterator != output.end() && (data_iterator->second.data_size > it->second.data.data_size)) {
      const double const_double = 1.0;
      int32_t page = static_cast<int32_t>(std::ceil(const_double * data_iterator->second.data_size / page_size));
      FE_CHECK(page > page_num_left, FE_LOGW("Page %d is bigger than left page number %d", page, page_num_left),
               return fe::FAILED);
      it->second.data.data_size = data_iterator->second.data_size;
      it->second.l2PageNum = static_cast<uint64_t>(page);
    }

    page_num_left = page_num_left - it->second.l2PageNum;
  }
  return fe::SUCCESS;
}
CCE_DEFINE_SINGLETON(L2FusionAllocation);

}  // namespace fe
