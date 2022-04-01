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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_STREAM_GRAPH_OPTIMIZER_L2_OPTIMIZER_L2_FUSION_ALLOCATION_L2_FUSION_ALLOCATION_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_STREAM_GRAPH_OPTIMIZER_L2_OPTIMIZER_L2_FUSION_ALLOCATION_L2_FUSION_ALLOCATION_H_

#include "common/math_util.h"
#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_comm/l2_fusion_comm.h"

namespace fe {

class L2FusionAllocation {
  CCE_DECLARE_SINGLETON(L2FusionAllocation)

 public:
  Status AllocateData(k_l2_task_datas_map_t &data, k_data_dependent_count_map &count_map, k_l2_buffer_t &l2,
                      k_l2_data_allocs_t &standing_data_alloc, k_l2_task_data_allocs_map_t &alloc,
                      uint64_t &max_page_num);

 private:
  Status AllocateStandingData(uint32_t &data_in_l2_id, uint8_t priority, int64_t page_size,
                              k_data_dependent_count_map &count_map, k_l2_data_allocs_t &standing_data_alloc,
                              k_l2_datas_t &output, int32_t &page_num_left) const;
  Status AllocateStandingDataSpecial(uint32_t &data_in_l2_id, uint8_t priority, int64_t page_size,
                                     k_data_dependent_count_map &count_map, k_l2_data_allocs_t &standing_data_alloc,
                                     k_l2_datas_t &output, int32_t &page_num_left) const;
  Status AllocateInputData(k_l2_data_allocs_t &standing, k_l2_datas_t &input, k_data_dependent_count_map &count_map,
                           k_l2_data_allocs_t &input_alloc) const;
  Status AllocateOutputData(uint32_t &data_in_l2_id, uint8_t priority, int64_t page_size, uint32_t max_page_num,
                            int32_t &data_num_left, int32_t &page_num_left, k_l2_datas_t &output,
                            k_l2_data_allocs_t &standing_allocs,
                            k_l2_data_allocs_t &output_allocs) const;

  Status UpdateStandingDataSizeWithOutputSet(int64_t page_size, k_l2_data_allocs_t &standing_allocs,
                                             k_l2_datas_t &output,
                                             int32_t page_num_left) const;

  Status AllocateStandingDataByTaskNum(uint32_t node_task_num, uint32_t &data_in_l2_id, uint64_t page_size,
                                       k_data_dependent_count_map &count_map, k_l2_data_allocs_t &standing_data_alloc,
                                       k_l2_datas_t &output, int32_t &page_num_left);
};

}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_STREAM_GRAPH_OPTIMIZER_L2_OPTIMIZER_L2_FUSION_ALLOCATION_L2_FUSION_ALLOCATION_H_
