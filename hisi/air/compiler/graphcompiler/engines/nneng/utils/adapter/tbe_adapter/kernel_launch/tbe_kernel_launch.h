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

#ifndef FUSION_ENGINE_UTILS_ADAPTER_TBE_ADAPTER_KERNEL_LAUNCH_TBE_KERNEL_LAUNCH_H_
#define FUSION_ENGINE_UTILS_ADAPTER_TBE_ADAPTER_KERNEL_LAUNCH_TBE_KERNEL_LAUNCH_H_

#include <securec.h>
#include <memory>
#include <string>
#include <vector>
#include "proto/task.pb.h"
#include "common/resource_def.h"
#include "graph/anchor.h"
#include "graph/node.h"
#include "runtime/base.h"
#include "register/graph_optimizer/graph_optimize_register_error_codes.h"

namespace fe {
class TbeKernelLaunch {
 public:
  explicit TbeKernelLaunch(int32_t input_num) : input_num_(input_num) {};
  virtual ~TbeKernelLaunch() {};
  Status DealKernelLaunch(const ge::Node &node, const void *args, const uint32_t &args_size,
                          const std::string &stub_func, const uint32_t &core_dim, domi::TaskDef &task_def);

  virtual size_t GetAppendArgsSizeOf();
  virtual size_t GetAppendArgsNum();
  virtual Status AddAppendArgs(const ge::Node &node, void *all_args_buff, const uint32_t &args_size);

  static bool KernelLaunch(const std::string &stub_func, uint32_t block_dim, const void *args, uint32_t args_size,
                           const rtSmDesc_t *sm_desc, domi::TaskDef &task_def);
  static bool KernelLaunchWithHandle(uint32_t block_dim, const void *args, uint32_t args_size,
                                     const rtSmDesc_t *sm_desc, domi::TaskDef &task_def);
 protected:
  int32_t input_num_;

 private:
  void PrintAllArgs(const string &op_name, const string &op_type, const void *all_args_buff, uint32_t args_size);
};
}  // namespace fe
#endif  // FUSION_ENGINE_UTILS_ADAPTER_TBE_ADAPTER_KERNEL_LAUNCH_TBE_KERNEL_LAUNCH_H_