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

#ifndef GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_HCCL_TASK_INFO_H_
#define GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_HCCL_TASK_INFO_H_

#include "common/opskernel/ge_task_info.h"
#include "common/opskernel/ops_kernel_info_store.h"
#include "graph/load/model_manager/task_info/task_info.h"
#include "graph/manager/util/hcom_util.h"

namespace ge {
class HcclTaskInfo : public TaskInfo {
 public:
  HcclTaskInfo() = default;

  ~HcclTaskInfo() override;

  Status Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) override;

  Status Distribute() override;

  uint32_t GetTaskID() const override { return id_; }

  Status CalculateArgs(const domi::TaskDef &task_def, DavinciModel *const davinci_model) override;

  Status UpdateArgs() override;

  Status Release() override;

 private:
  void SetIoAddrs(const OpDescPtr &op_desc);

  Status SetAddrs(const std::shared_ptr<OpDesc> &op_desc, std::vector<GETaskKernelHcclInfo> &kernel_hccl_infos);

  void TransToGETaskInfo(GETaskInfo &ge_task) const;

  void GetPrivateDefByTaskDef(const OpDescPtr &op_desc, const domi::TaskDef &task);

  Status CreateStream(const int64_t stream_num, const int64_t main_stream_id);

  Status SetFollowStream(const ConstOpDescPtr &op_desc);

  void CreateKernelHcclInfo(const ConstOpDescPtr &op_desc);

  Status SetWorkspace(const std::shared_ptr<OpDesc> &op_desc, std::vector<GETaskKernelHcclInfo> &kernel_hccl_infos);

  DavinciModel *davinci_model_{nullptr};
  uint32_t id_{0U};
  std::vector<rtStream_t> hccl_stream_list_;
  OpsKernelInfoStore *ops_kernel_store_{nullptr};
  void *private_def_{nullptr};
  uint32_t private_def_len_{0U};
  static std::mutex hccl_follow_stream_mutex_;
  std::vector<GETaskKernelHcclInfo> kernel_hccl_infos_;
  std::vector<void *> io_addrs_;
  void *args_{nullptr};
  uint32_t args_offset_{0U};
};
}  // namespace ge
#endif  // GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_HCCL_TASK_INFO_H_
