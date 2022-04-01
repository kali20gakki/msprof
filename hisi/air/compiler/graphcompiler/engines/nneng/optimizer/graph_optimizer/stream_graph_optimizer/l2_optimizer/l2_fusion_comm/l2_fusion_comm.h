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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_STREAM_GRAPH_OPTIMIZER_L2_OPTIMIZER_L2_FUSION_COMM_L2_FUSION_COMM_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_STREAM_GRAPH_OPTIMIZER_L2_OPTIMIZER_L2_FUSION_COMM_L2_FUSION_COMM_H_

#include <map>
#include <string>
#include <utility>
#include <vector>
#include "common/fe_inner_attr_define.h"
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/util/op_info_util.h"
#include "graph/compute_graph.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_comm/singleton.h"

namespace fe {

const uint32_t L2_CTRL_REMAP_SIZE = 64;
const uint32_t L2_CTRL_DATA_SIZE = 8;

// data_type: input-0, output-1, filter-2
#define DATA_OVERALL_ID(node_id, datatype, data_id) (((node_id) << 16) | ((datatype) << 8) | (data_id))
const uint32_t INPUT_DATA = 0;
const uint32_t OUTPUT_DATA = 1;

#define UNUSED(x) (void)(x)

using CceIdArray = std::vector<uint32_t>;

using k_l2_buffer_t = struct TagKernelL2Buffer {
  uint64_t l2_buffer_size;
  uint64_t page_num;
  uint64_t max_data_num;  // runtime allows at most 8 data segments in l2 buffer
};

// data info for l2 buffer
using k_l2_data_t = struct TagFusionL2Data {
  uint64_t id;
  uint64_t ddr_addr;
  int64_t data_size;
  int32_t occupy_data_id;
  CceIdArray occupy_data_ids;
};

using k_l2_datas_t = std::map<uint64_t, k_l2_data_t>;

using k_l2_task_datas_t = struct TagKernelL2TaskDatas {
  uint32_t node_id;
  string node_name;
  uint32_t node_task_num;

  k_l2_datas_t input;
  k_l2_datas_t output;
  k_l2_datas_t filter;
};

// the order is important, so use vector instead of map
using k_l2_task_datas_map_t = std::vector<k_l2_task_datas_t> ;

// data allocation info in l2 buffer
using k_l2_data_alloc_t = struct TagKernelL2DataAlloc {
  k_l2_data_t data;    // init data info
  uint64_t l2PageNum;  // available l2 buffer size
  int32_t data_in_l2_id;
  uint64_t data_in_l2_addr;  // l2 addr

  int8_t pre_L2_page_offset_base;  // input
  uint8_t L2_page_offset_base;
  uint8_t modified;       // 1 - data will be modified by kernel, 0 - no modified
  uint8_t L2_preload;     // 1 - preload from mirror_dd_r, 0-no preload
  uint8_t L2_load_to_ddr; /**< 1 - need load out, 0 - no need */
  uint8_t priority;       // data priority

  int32_t occupy_data_id;
};

using k_l2_data_allocs_t = std::map<uint64_t, k_l2_data_alloc_t>;
using k_l2_task_data_allocs_t = struct TagKernelL2TaskDataAllocs {
  ~TagKernelL2TaskDataAllocs() { clear(); }
  void clear() {
    standing_data.clear();
    input.clear();
    output.clear();
    filter_preload.clear();
    input_preload.clear();
  }
  uint32_t node_id;
  string node_name;

  rtL2Ctrl_t l2ctrl;
  k_l2_data_allocs_t standing_data;
  k_l2_data_allocs_t input;
  k_l2_data_allocs_t output;
  k_l2_data_allocs_t converge;
  k_l2_data_allocs_t filter_preload;
  k_l2_data_allocs_t input_preload;
};

using k_l2_task_data_allocs_map_t = std::map<string, k_l2_task_data_allocs_t>;  // the key is node_name;
using k_data_dependent_count_map = std::map<uint64_t, int32_t>;

using k_l2_data_pair_t = std::pair<uint64_t, k_l2_data_t>;
using k_l2_data_alloc_pair_t = std::pair<uint64_t, k_l2_data_alloc_t>;
using k_l2_task_data_allocs_pair_t = std::pair<string, k_l2_task_data_allocs_t>;
using NodeIdMap_t = std::map<uint32_t, uint32_t>;
using NodeIdPair_t = std::pair<uint32_t, uint32_t>;

class L2FusionComm {
  CCE_DECLARE_SINGLETON(L2FusionComm)

 public:
  Status GetL2HardwareSet(k_l2_buffer_t &l2) const;
  Status GetGraphDataSize(ge::NodePtr node, uint32_t data_id, uint32_t data_type, int64_t &data_size) const;
  void DisplayParserData(const k_l2_task_datas_map_t &data) const;
  void DisplayL2AllocInfo(const k_l2_task_data_allocs_map_t &alloc) const;

 private:
  Status GetGraphDataSize(ge::OpDescPtr opdef, ge::GeTensorDesc &tensor, uint32_t data_type, int64_t &data_size) const;
};
// tools function
bool IsNonValidOp(ge::NodePtr node);
bool IsNonValidOpOrData(ge::NodePtr node);
bool IsConstInput(const ge::ConstNodePtr &node, size_t index);
bool IsConstInput(const ge::Node &node, const size_t index);

Status CalcTensorSize(ge::GeTensorDesc &tensor_desc, int64_t &tensor_size);
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_STREAM_GRAPH_OPTIMIZER_L2_OPTIMIZER_L2_FUSION_COMM_L2_FUSION_COMM_H_
