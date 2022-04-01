/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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
#ifndef GE_GRAPH_FUZZ_COMPILE_BIN_SELECTOR_H_
#define GE_GRAPH_FUZZ_COMPILE_BIN_SELECTOR_H_

#include <mutex>
#include <unordered_map>

#include "common/tbe_handle_store/bin_register_utils.h"
#include "framework/common/ge_inner_error_codes.h"
#include "graph/bin_cache/node_bin_selector.h"
#include "graph/bin_cache/node_bin_selector_factory.h"
#include "graph/bin_cache/node_compile_cache_module.h"
#include "graph/bin_cache/op_binary_cache.h"
#include "opskernel_manager/ops_kernel_manager.h"

namespace ge {

namespace hybrid {
using NodeInOutShape = struct {
  std::unordered_map<size_t, ge::GeShape> in_ori_shapes;
  std::unordered_map<size_t, ge::GeShape> in_shapes;
  std::unordered_map<size_t, ge::GeShape> out_ori_shapes;
  std::unordered_map<size_t, ge::GeShape> out_shapes;
};

class FuzzCompileBinSelector : public NodeBinSelector {
 public:
  FuzzCompileBinSelector() = default;

  explicit FuzzCompileBinSelector(NodeCompileCacheModule *nccm) : nccm_(nccm){};

  Status Initialize() override;

  NodeCompileCacheItem *SelectBin(const NodePtr &node, const GEThreadLocalContext *ge_context,
                                  std::vector<domi::TaskDef> &task_defs) override;

 private:
  Status DoCompileOp(const NodePtr &node_ptr) const;
  Status DoRegisterBin(const OpDesc &op_desc, const domi::TaskDef &task_def, KernelLaunchBinType &bin_type,
                       void *&handle) const;

  Status DoGenerateTask(const NodePtr &node_ptr, std::vector<domi::TaskDef> &task_defs);

  NodeCompileCacheModule *nccm_ = nullptr;
  std::mutex gen_task_mutex_;
  OpsKernelInfoStorePtr aicore_kernel_store_;
};

REGISTER_BIN_SELECTOR(kOneNodeMultipleBinsMode, FuzzCompileBinSelector, OpBinaryCache::GetInstance().GetNodeCcm());
}  // namespace hybrid
}  // namespace ge

#endif