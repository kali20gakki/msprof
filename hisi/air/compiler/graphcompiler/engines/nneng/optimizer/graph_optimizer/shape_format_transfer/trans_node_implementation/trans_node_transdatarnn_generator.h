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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANS_NODE_IMPLEMENTATION_TRANS_NODE_TRANSDATARNN_GENERATOR_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANS_NODE_IMPLEMENTATION_TRANS_NODE_TRANSDATARNN_GENERATOR_H_

#include "graph_optimizer/shape_format_transfer/trans_node_implementation/trans_node_transdata_generator.h"

#include "common/fe_inner_error_codes.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph_optimizer/range_format_transfer/transfer_range_according_to_format.h"
#include "graph_optimizer/shape_format_transfer/transfer_shape_according_to_format.h"

namespace fe {

/** @brief class of generating Trans-node TransdataRNN. Provide function of setting
 * unique attrs and tensors for TransdataRNN op.
 * @version 1.0 */
class TransNodeTransdataRNNGenerator : public TransNodeTransdataGenerator {
 public:
  TransNodeTransdataRNNGenerator(FEOpsKernelInfoStorePtr fe_ops_store_ptr, TransInfoPtr trans_info_ptr);

  ~TransNodeTransdataRNNGenerator() override;

  TransNodeTransdataRNNGenerator(const TransNodeTransdataRNNGenerator &) = delete;

  TransNodeTransdataRNNGenerator &operator=(const TransNodeTransdataRNNGenerator &) = delete;

  Status AddTransNode(ge::ComputeGraph &fused_graph, TransInfoPtr trans_info_ptr) override;

  Status AddOpAndNode(ge::ComputeGraph &fused_graph, const ge::GeShape &shape, const ge::Format &primary_format,
                      const int32_t  &sub_format, const ge::DataType &dtype) override;
};

}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_SHAPE_FORMAT_TRANSFER_TRANS_NODE_IMPLEMENTATION_TRANS_NODE_TRANSDATARNN_GENERATOR_H_
