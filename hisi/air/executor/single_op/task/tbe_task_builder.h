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

#ifndef GE_SINGLE_OP_TASK_TBE_TASK_BUILDER_H_
#define GE_SINGLE_OP_TASK_TBE_TASK_BUILDER_H_

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>

#include "graph/op_desc.h"
#include "single_op/single_op.h"
#include "single_op/single_op_model.h"

namespace ge {
class TbeTaskBuilder {
 public:
  TbeTaskBuilder(const std::string &model_name, const NodePtr &node, const domi::TaskDef &task_def);
  virtual ~TbeTaskBuilder() = default;

  Status BuildTask(TbeOpTask &task, const SingleOpModelParam &param);

 protected:
  virtual std::string GetKeyForOpParamSize() const;
  virtual std::string GetKeyForTvmMetaData() const;
  virtual TBEKernelPtr GetTbeKernel(const OpDescPtr &op_desc) const;
  virtual void GetKernelName(const OpDescPtr &op_desc, std::string &kernel_name) const;
  virtual Status InitKernelArgs(void *const args_addr, const size_t arg_size, const SingleOpModelParam &param);

 private:
  Status InitTilingInfo(TbeOpTask &task);
  Status SetKernelArgs(TbeOpTask &task, const SingleOpModelParam &param, const OpDescPtr &op_desc);
  Status UpdateTilingArgs(TbeOpTask &task, const size_t index, const size_t tiling_arg_index) const;
  size_t CalArgsSize(TbeOpTask &task, const OpDescPtr &op_desc, size_t io_size) const;
  Status RegisterKernel(TbeOpTask &task, const SingleOpModelParam &param);
  Status RegisterKernelWithHandle(const SingleOpModelParam &param);

  Status DoRegisterKernel(const ge::OpKernelBin &tbe_kernel, const char_t *const bin_file_key,
                          void **const bin_handle, const SingleOpModelParam &param);

  Status DoRegisterBinary(const OpKernelBin &kernel_bin, void **const bin_handle,
                          const SingleOpModelParam &param) const;
  Status DoRegisterMeta(void *const bin_handle) const;
  Status GetMagic(uint32_t &magic) const;

  static Status DoRegisterFunction(void *const bin_handle, const char_t *const stub_name,
                                   const char_t *const kernel_name);

  const NodePtr node_;
  const OpDescPtr op_desc_;
  const domi::TaskDef &task_def_;
  const domi::KernelDef &kernel_def_;
  const domi::KernelDefWithHandle &kernel_def_with_handle_;
  const std::string model_name_;
  std::string stub_name_;
  void *handle_ = nullptr;
};

class AtomicAddrCleanTaskBuilder : public TbeTaskBuilder {
 public:
  AtomicAddrCleanTaskBuilder(const std::string &model_name, const NodePtr &node, const domi::TaskDef &task_def)
      : TbeTaskBuilder(model_name, node, task_def) {}
  ~AtomicAddrCleanTaskBuilder() override = default;

 protected:
  std::string GetKeyForOpParamSize() const override;
  std::string GetKeyForTvmMetaData() const override;
  TBEKernelPtr GetTbeKernel(const OpDescPtr &op_desc) const override;
  void GetKernelName(const OpDescPtr &op_desc, std::string &kernel_name) const override;
  Status InitKernelArgs(void *const args_addr, const size_t arg_size,
                        const SingleOpModelParam &param) override;
};
}  // namespace ge

#endif  // GE_SINGLE_OP_TASK_TBE_TASK_BUILDER_H_
