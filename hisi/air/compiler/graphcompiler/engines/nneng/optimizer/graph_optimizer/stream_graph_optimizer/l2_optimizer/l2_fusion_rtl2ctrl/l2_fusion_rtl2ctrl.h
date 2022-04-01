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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_STREAM_GRAPH_OPTIMIZER_L2_OPTIMIZER_L2_FUSION_RTL2CTRL_L2_FUSION_RTL2CTRL_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_STREAM_GRAPH_OPTIMIZER_L2_OPTIMIZER_L2_FUSION_RTL2CTRL_L2_FUSION_RTL2CTRL_H_

#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_comm/l2_fusion_comm.h"

namespace fe {

class L2FusionRtl2ctrl {
  CCE_DECLARE_SINGLETON(L2FusionRtl2ctrl)
 public:
  void InitRtl2ctrl(rtL2Ctrl_t &l2ctrl);
  Status UpdateRtL2Ctrl(k_l2_task_data_allocs_map_t &alloc, uint32_t max_page_num, uint32_t max_data_num);

 private:
  Status SetDataL2ctrl(uint32_t max_data_num, k_l2_data_allocs_t &data, rtL2Ctrl_t &l2ctrl) const;
  Status SetOutputDataL2ctrl(uint32_t max_data_num, k_l2_data_allocs_t &standing, k_l2_data_allocs_t &data,
                             rtL2Ctrl_t &out_l2ctrl) const;
};

}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_STREAM_GRAPH_OPTIMIZER_L2_OPTIMIZER_L2_FUSION_RTL2CTRL_L2_FUSION_RTL2CTRL_H_
