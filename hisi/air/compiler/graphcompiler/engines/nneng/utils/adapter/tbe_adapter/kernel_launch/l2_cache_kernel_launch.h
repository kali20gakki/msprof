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

#ifndef FUSION_ENGINE_UTILS_ADAPTER_TBE_ADAPTER_KERNEL_LAUNCH_L2_CACHE_KERNEL_LAUNCH_H_
#define FUSION_ENGINE_UTILS_ADAPTER_TBE_ADAPTER_KERNEL_LAUNCH_L2_CACHE_KERNEL_LAUNCH_H_

#include <memory>
#include <vector>
#include "adapter/tbe_adapter/kernel_launch/tbe_kernel_launch.h"
#include "common/constants_define.h"
#include "graph/debug/ge_attr_define.h"
#include "securec.h"

namespace fe {
class L2CacheKernelLaunch : public TbeKernelLaunch {
 public:
  explicit L2CacheKernelLaunch(int32_t input_num) : TbeKernelLaunch(input_num) {};
  ~L2CacheKernelLaunch() override{};
  size_t GetAppendArgsSizeOf() override;
  size_t GetAppendArgsNum() override;
  Status AddAppendArgs(const ge::Node &node, void *all_args_buff, const uint32_t &args_size) override;

 private:
  Status GenerateReadModes(const ge::Node &node, vector<uint64_t> &read_modes) const;
  bool IsLifeCycleEnd(const ge::Node &node, const ge::GeTensorDescPtr &input_desc, int input_idx) const;
  L2CacheReadMode GenerateReadMode(const ge::Node &node, const ge::GeTensorDescPtr &input_desc, int input_idx,
                                   bool is_life_cycle_end) const;
  L2CacheReadMode GenRmForSpecialInputOps(const ge::NodePtr &src_node, bool is_enable_reuse_mem) const;
};
}  // namespace fe
#endif  // FUSION_ENGINE_UTILS_ADAPTER_TBE_ADAPTER_KERNEL_LAUNCH_L2_CACHE_KERNEL_LAUNCH_H_
