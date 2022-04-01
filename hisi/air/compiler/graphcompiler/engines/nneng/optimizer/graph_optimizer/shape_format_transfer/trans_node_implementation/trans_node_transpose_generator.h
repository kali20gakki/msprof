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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANS_NODE_IMPLEMENTATION_TRANS_NODE_TRANSPOSE_GENERATOR_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANS_NODE_IMPLEMENTATION_TRANS_NODE_TRANSPOSE_GENERATOR_H_

#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_base_generator.h"

#include "common/fe_inner_error_codes.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph_optimizer/shape_format_transfer/transfer_shape_according_to_format.h"

namespace fe {
using trans_pose_map = std::unordered_map<uint64_t, ge::Format>;

const std::map<ge::Format, uint32_t> FORMAT_INDEX_MAP = {
    {ge::FORMAT_NCHW, 0}, {ge::FORMAT_NHWC, 1}, {ge::FORMAT_HWCN, 2}, {ge::FORMAT_CHWN, 3}};
/* First row:  NCHW->NCHW, NCHW->NHWC, NCHW->HWCN, NCHW->CHWN;
 * Second row: NHWC->NCHW, NHCW->NHWC, NHWC->HWCN, NHWC->CHWN;
 * Third row:  HWCN->NCHW, HWCN->NHWC, HWNC->HWCN, HWNC->CHWN;
 * Fourth row: CHWN->NCHW, CHWN->NHWC, CHWN->HWCN, CHWN->CHWN;
 */
const std::vector<std::vector<std::vector<int64_t>>> PERM_VALUE_VECTOR = {
    {{}, {0, 2, 3, 1}, {2, 3, 1, 0}, {1, 2, 3, 0}},
    {{0, 3, 1, 2}, {}, {1, 2, 3, 0}, {3, 1, 2, 0}},
    {{3, 2, 0, 1}, {3, 0, 1, 2}, {}, {3, 0, 1, 2}},
    {{3, 0, 1, 2}, {3, 1, 2, 0}, {1, 2, 0, 3}, {}}};

/** @brief class of generating Trans-node Transpose. Provide function of setting
* unique attrs and tensors for Transpose op.
* @version 1.0 */
class TransNodeTransposeGenerator : public TransNodeBaseGenerator {
 public:
  TransNodeTransposeGenerator(FEOpsKernelInfoStorePtr fe_ops_store_ptr, TransInfoPtr trans_info_ptr);

  ~TransNodeTransposeGenerator() override;

  TransNodeTransposeGenerator(const TransNodeTransposeGenerator &) = delete;

  TransNodeTransposeGenerator &operator=(const TransNodeTransposeGenerator &) = delete;

  Status AddTransNode(ge::ComputeGraph &fused_graph, TransInfoPtr trans_info_ptr) override;

 private:
  void GetNewShape(ge::OpDescPtr &op_desc_ptr, ge::Format format, ge::DataType dtype, int64_t imply_type,
                   ge::GeShape &newshape);

  Status AddOpAndNode(ge::ComputeGraph &fused_graph, const ge::Format &primary_format, const int32_t &sub_format,
                      const ge::DataType &dtype);
};

}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANS_NODE_IMPLEMENTATION_TRANS_NODE_TRANSPOSE_GENERATOR_H_