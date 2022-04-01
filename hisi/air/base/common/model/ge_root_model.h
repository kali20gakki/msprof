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
#ifndef GE_MODEL_GE_ROOT_MODEL_H_
#define GE_MODEL_GE_ROOT_MODEL_H_

#include <map>
#include "graph/compute_graph.h"
#include "common/model/ge_model.h"
#include "common/model/model_relation.h"
#include "framework/pne/pne_model.h"

namespace ge {
 class GeRootModel : public std::enable_shared_from_this<GeRootModel>, public PneModel {
 public:
  GeRootModel() = default;
  explicit GeRootModel(const ComputeGraphPtr &root_graph) : PneModel(root_graph) {};
  ~GeRootModel() = default;

  void SetSubgraphInstanceNameToModel(const std::string &instance_name, const GeModelPtr &ge_model);
  const std::map<std::string, GeModelPtr> &GetSubgraphInstanceNameToModel() const {
    return subgraph_instance_name_to_model_;
  };

  void SetModelId(uint32_t model_id) override {
    const std::lock_guard<std::mutex> lock(model_ids_mutex_);
    PneModel::SetModelId(model_id);
    // cached for removement
    model_ids_.emplace_back(model_id);
  }

  void SetIsSpecificStream(const bool is_specific_stream) { is_specific_stream_ = is_specific_stream; }

  bool IsSpecificStream() const { return is_specific_stream_; }

  std::vector<uint32_t> GetAllModelId() const { return model_ids_; }

  void ClearAllModelId() { model_ids_.clear(); }

  Status CheckIsUnknownShape(bool &is_dynamic_shape) const;

  void SetTrainFlag(const bool flag) { train_flag_ = flag; }

  Status SerializeModel(ModelBufferData &model_buff) override;

  Status UnSerializeModel(const ModelBufferData &model_buff) override;

 private:
  std::map<std::string, GeModelPtr> subgraph_instance_name_to_model_;
  // In multithread online secenario, same graph can owns different davinci_model for for concurrency
  std::vector<uint32_t> model_ids_;
  std::mutex model_ids_mutex_;
  bool train_flag_ = false;
  bool is_specific_stream_ = false;
};
using GeRootModelPtr = std::shared_ptr<ge::GeRootModel>;
}  // namespace ge
#endif  // GE_MODEL_GE_ROOT_MODEL_H_
