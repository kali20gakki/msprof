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

#ifndef GE_SINGLE_OP_SINGLE_OP_MANAGER_H_
#define GE_SINGLE_OP_SINGLE_OP_MANAGER_H_

#include <mutex>
#include <unordered_map>
#include <string>
#include "common/plugin/op_tiling_manager.h"
#include "single_op/single_op_model.h"
#include "single_op/stream_resource.h"

namespace ge {
class SingleOpManager {
 public:
  ~SingleOpManager();

  static SingleOpManager &GetInstance() {
    static SingleOpManager instance;
    return instance;
  }

  Status GetOpFromModel(const std::string &model_name, const ModelData &model_data,
                        void *const stream, SingleOp **const single_op, const uint64_t model_id);

  Status GetDynamicOpFromModel(const std::string &model_name, const ModelData &model_data,
                               void *const stream, DynamicSingleOp **const single_op, const uint64_t model_id);

  Status DeleteSingleOp(const uint64_t op_id);

  Status DeleteDynamicSingleOp(const uint64_t op_id);

  StreamResource *GetResource(const uintptr_t resource_id, const rtStream_t stream);

  Status ReleaseResource(const void *const stream);

  void RegisterTilingFunc();

 private:
  static Status GetResourceId(const rtStream_t stream, uintptr_t &resource_id);

  std::mutex mutex_;
  bool tiling_func_registered_ = false;
  std::unordered_map<uintptr_t, std::unique_ptr<StreamResource>> stream_resources_;
  OpTilingManager op_tiling_manager_;
};
}  // namespace ge

#endif  // GE_SINGLE_OP_SINGLE_OP_MANAGER_H_
