/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#ifndef GE_GRAPH_LOAD_GRAPH_LOADER_H_
#define GE_GRAPH_LOAD_GRAPH_LOADER_H_

#include <memory>
#include <string>
#include <vector>

#include "framework/common/debug/log.h"
#include "framework/common/fmk_types.h"
#include "framework/common/ge_types.h"
#include "graph/compute_graph.h"
#include "graph/manager/graph_manager_utils.h"
#include "graph/model.h"
#include "runtime/mem.h"

namespace ge {
class GraphLoader {
 public:
  GraphLoader() = default;

  virtual ~GraphLoader() = default;

  GraphLoader(const GraphLoader &in) = delete;

  GraphLoader &operator=(const GraphLoader &in) = delete;

  static Status UnloadModel(const uint32_t model_id);

  static Status CommandHandle(const Command &command);

  static Status LoadDataFromFile(const std::string &path, const int32_t priority, ModelData &model_data);

  static Status LoadModelFromData(uint32_t &model_id, const ModelData &model_data,
                                  const uintptr_t mem_ptr, const size_t mem_size,
                                  const uintptr_t weight_ptr, const size_t weight_size);

  static Status LoadModelWithQ(uint32_t &model_id, const ModelData &model_data,
                               const std::vector<uint32_t> &input_queue_ids,
                               const std::vector<uint32_t> &output_queue_ids);

  static Status LoadModelWithQ(uint32_t &model_id, const GeRootModelPtr &root_model,
                               const std::vector<uint32_t> &input_queue_ids,
                               const std::vector<uint32_t> &output_queue_ids,
                               const bool need_update_session_id = true);

  static Status LoadModelWithoutQ(uint32_t &model_id, const GeRootModelPtr &root_model);

  static Status ExecuteModel(const uint32_t model_id, const rtStream_t stream, const bool async_mode,
                             const InputData &input_data, const std::vector<GeTensorDesc> &input_desc,
                             OutputData &output_data, std::vector<GeTensorDesc> &output_desc);

  static Status LoadModelOnline(uint32_t &model_id, const GeRootModelPtr &ge_root_model,
                                const GraphNodePtr &graph_node, const uint32_t device_id,
                                const error_message::Context &error_context,
                                const int64_t die_id = std::numeric_limits<int64_t>::max());

  static Status MultiLoadModelOnline(const GeRootModelPtr &ge_root_model, const GraphNodePtr &graph_node,
                                     const std::vector<NamedAttrs> &deploy_info);
};
}  // namespace ge
#endif  // GE_GRAPH_LOAD_GRAPH_LOADER_H_
