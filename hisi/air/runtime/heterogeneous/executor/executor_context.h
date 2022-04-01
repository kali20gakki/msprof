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

#ifndef AIR_RUNTIME_DEPLOY_EXECUTOR_EXECUTOR_CONTEXT_H_
#define AIR_RUNTIME_DEPLOY_EXECUTOR_EXECUTOR_CONTEXT_H_

#include <vector>
#include <map>
#include <memory>
#include "ge/ge_api_error_codes.h"
#include "common/model/ge_model.h"
#include "graph/model.h"
#include "framework/common/types.h"
#include "framework/common/helper/om_file_helper.h"
#include "common/model/ge_root_model.h"
#include "executor/incremental_model_parser.h"
#include "executor/dynamic_model_executor.h"
#include "proto/deployer.pb.h"

namespace ge {
class ExecutorContext {
 public:
  ExecutorContext() = default;
  virtual ~ExecutorContext() = default;

  class ModelHandle {
   public:
    explicit ModelHandle(uint64_t model_size);
    virtual ~ModelHandle();
    GE_DELETE_ASSIGN_AND_COPY(ModelHandle);

    Status ParsePartialModel(uint64_t offset, const void *model_buffer, uint64_t buffer_size);
    Status LoadModel(const std::vector<uint32_t> &input_queues,
                     const std::vector<uint32_t> &output_queues);
    Status UnloadModel();

   protected:
    virtual Status DoLoadModel(const std::shared_ptr<GeRootModel> &root_model,
                               const std::vector<uint32_t> &input_queues,
                               const std::vector<uint32_t> &output_queues);

    virtual Status DoUnloadModel(const uint32_t model_id) const;
   private:
    IncrementalModelParser model_parser_;
    uint32_t inner_model_id_ = UINT32_MAX;
    std::unique_ptr<DynamicModelExecutor> dynamic_model_executor_;
    bool is_dynamic_ = false;
    bool loaded_ = false;
  };

  Status AddModel(uint32_t root_model_id, uint32_t model_id, uint64_t model_size);

  Status GetModel(uint32_t root_model_id, uint32_t model_id, ModelHandle **handle);

  Status GetModel(uint32_t root_model_id, std::map<uint32_t, std::unique_ptr<ModelHandle>> *&submodel_map);

  Status InitVarManager(const deployer::MultiVarManagerInfo &multi_var_manager);

  Status FillSharedContent(const deployer::SharedContentDescription &shared_content_desc);

 protected:
  virtual std::unique_ptr<ModelHandle> CreateModelHandle(uint64_t model_size) const;
 private:
  // single-threaded access
  std::map<uint32_t, std::map<uint32_t, std::unique_ptr<ModelHandle>>> models_;
};
}  // namespace ge

#endif  // AIR_RUNTIME_DEPLOY_EXECUTOR_EXECUTOR_CONTEXT_H_
