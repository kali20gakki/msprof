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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANS_NODE_IMPLEMENTATION_TRANS_NODE_TRANSDATA_GENERATOR_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANS_NODE_IMPLEMENTATION_TRANS_NODE_TRANSDATA_GENERATOR_H_

#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_base_generator.h"

#include "common/fe_inner_error_codes.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph_optimizer/range_format_transfer/transfer_range_according_to_format.h"
#include "graph_optimizer/shape_format_transfer/transfer_shape_according_to_format.h"

namespace fe {

using trans_data_map = std::unordered_map<uint64_t, ge::Format>;

/** @brief class of generating Trans-node Transdata. Provide function of setting
 * unique attrs and tensors for Transdata op.
 * @version 1.0 */
class TransNodeTransdataGenerator : public TransNodeBaseGenerator {
 public:
  TransNodeTransdataGenerator(FEOpsKernelInfoStorePtr fe_ops_store_ptr, TransInfoPtr trans_info_ptr);

  ~TransNodeTransdataGenerator() override;

  TransNodeTransdataGenerator(const TransNodeTransdataGenerator &) = delete;

  TransNodeTransdataGenerator &operator=(const TransNodeTransdataGenerator &) = delete;

  Status AddTransNode(ge::ComputeGraph &fused_graph, TransInfoPtr trans_info_ptr) override;

  virtual Status AddOpAndNode(ge::ComputeGraph &fused_graph, const ge::GeShape &shape,
                              const ge::Format &out_primary_format,
                              const int32_t &out_sub_format, const ge::DataType &dtype);

  void SetAttr(const TransInfoPtr &trans_info_ptr, const ge::Format &primary_format, const int32_t &sub_format,
               ge::OpDescPtr &op_desc_ptr) const;

  Status AddAndSetTensor(const ge::GeShape &shape, const ge::Format &primary_format, const int32_t &sub_format,
                         const ge::DataType &dtype, ge::OpDescPtr &op_desc_ptr);

  Status GetShapeOfTransdata(const ge::OpDescPtr op_desc_ptr, ge::GeShape &new_in_shape, ge::GeShape &new_out_shape,
                             vector<std::pair<int64_t, int64_t>> &new_in_range,
                             vector<std::pair<int64_t, int64_t>> &new_out_range, const ge::Format &output_format,
                             const ge::DataType &output_dtype);
};

}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANS_NODE_IMPLEMENTATION_TRANS_NODE_TRANSDATA_GENERATOR_H_