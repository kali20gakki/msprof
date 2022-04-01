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
#ifndef AICPU_GRAPH_OPTIMIZER_UTILS_H_
#define AICPU_GRAPH_OPTIMIZER_UTILS_H_

#include "external/ge/ge_api_error_codes.h"
#include "graph/compute_graph.h"
#include "aicpu_ops_kernel_info_store/op_struct.h"
#include "common/sgt_slice_type.h"

namespace aicpu {
class GraphOptimizerUtils {
 public:
  /**
   * Verify PlaceHolder node and End node
   * @param graph Compute graph
   * @return status whether this operation success
   */
  static ge::Status VerifyPldAndEndNode(const ge::ComputeGraph &graph);

  /**
   * dump graph to visual
   * @param graph Compute graph
   * @param suffix, dump option
   */
  static void DumpGraph(ge::ComputeGraph &graph, std::string &suffix);

  /**
   * Check if ffts plus is supported
   * @param op_desc_ptr op desc ptr
   * @param slice_info_ptr slice info ptr
   * @param sgt_flag ffts support flag
   * @return status whether this operation success
   */
  static ge::Status CheckIsFftsPlus(const ge::OpDescPtr &op_desc_ptr,
                                    ffts::ThreadSliceMapPtr &slice_info_ptr,
                                    bool &sgt_flag);
};

class CacheGraph {
 public:
  /**
   * Generate the graph after inserte CacheUpdate node
   * @param graph Compute graph
   * @return status whether this operation success
   */
  static ge::Status GenerateNoCacheGraph(ge::ComputeGraph &graph);

 private:
  /**
   * Create and insert CacheUpdate op to graph
   * @param src_anchor Src anchor ptr
   * @param dst_anchor Dst anchor ptr
   * @param graph Compute graph
   * @param slice_info_ptr slice info ptr
   * @param sgt_flag ffts+ support flag
   * @return status whether this operation success
   */
  static ge::Status CreateAndInsertCacheUpdate(
      const ge::OutDataAnchorPtr &src_anchor,
      const ge::InDataAnchorPtr &dst_anchor, ge::ComputeGraph &graph,
      const ffts::ThreadSliceMapPtr &slice_info_ptr,
      const bool &sgt_flag);
};

class AutoCastGraph {
 public:
  /**
   * Generate the graph after insert Cast node
   * @param graph Compute graph
   * @param all_op_info store op information
   * @return status whether this operation success
   */
  static ge::Status GenerateAutoCastGraph(ge::ComputeGraph &graph, const std::map<std::string, OpFullInfo> &all_op_info);

 private:
  /**
   * Insert Cast op for input
   * @param dst_anchor Dst anchor ptr
   * @param graph Compute graph
   * @param dst_type Dst type
   * @param slice_info_ptr slice info ptr
   * @param sgt_flag ffts+ support flag
   * @return status whether this operation success
   */
  static ge::Status InsertCastForInput(
      const ge::InDataAnchorPtr &dst_anchor, ge::ComputeGraph &graph,
      ge::DataType dst_type,
      const ffts::ThreadSliceMapPtr &slice_info_ptr,
      const bool &sgt_flag);

  /**
   * Insert Cast op for output
   * @param src_anchor Src anchor ptr
   * @param graph Compute graph
   * @param src_type Src type
   * @param dst_type Dst type
   * @return status whether this operation success
   */
  static ge::Status InsertCastForOutput(
      const ge::OutDataAnchorPtr &src_anchor, ge::ComputeGraph &graph, ge::DataType src_type, ge::DataType dst_type);

  /**
   * Get framework op original type
   * @param op_desc_ptr OpDesc ptr
   * @param op_type Op type
   * @return status whether this operation success
   */
  static ge::Status GetFrameworkOpType(ge::OpDescPtr &op_desc_ptr, std::string &op_type);
  
};
}  // namespace aicpu

#endif
