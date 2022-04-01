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

#ifndef FUSION_ENGINE_UTILS_ADAPTER_TBE_ADAPTER_TBE_TASK_BUILDER_ADAPTER_H_
#define FUSION_ENGINE_UTILS_ADAPTER_TBE_ADAPTER_TBE_TASK_BUILDER_ADAPTER_H_

#include <map>
#include <mutex>
#include <string>
#include <vector>
#include "adapter/adapter_itf/task_builder_adapter.h"
#include "graph/op_kernel_bin.h"
#include "graph/utils/anchor_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "common/aicore_util_types.h"
#include "common/comm_error_codes.h"
#include "common/l2fusion_struct.h"

namespace fe {
class TbeTaskBuilderAdapter : public TaskBuilderAdapter {
 public:
  TbeTaskBuilderAdapter(const ge::Node &node, TaskBuilderContext &context);
  ~TbeTaskBuilderAdapter() override;

  /*
   * @ingroup fe
   * @brief   Init TaskBuilderAdapter
   * @return  SUCCESS or FAILED
   */
  Status Init() override;

  /*
   * @ingroup fe
   * @brief   Run TaskBuilderAdapter
   * @return  SUCCESS or FAILED
   */
  Status Run(domi::TaskDef &task_def) override;

  Status CheckInputAndOutputSize();

 protected:
  Status InitInput() override;

 private:
  uint32_t block_dim_;
  Status TbeForward(ccHandle_t handle, uint32_t core_dim, const void *args,
                    uint32_t args_size, int32_t input_num, const void *x[], int32_t output_num, const void *y[],
                    int32_t workspace_num, domi::TaskDef &task_def);

  Status SaveTeCoreL2FlowDataForL2Buffer(const ccHandle_t &handle, const rtStream_t stream_id, int32_t input_num,
                                         int32_t output_num, uint64_t cur_ptr, const void *x[], const void *y[],
                                         rtL2Ctrl_t &tel2ctrl, uint32_t l2_args_size, uint32_t workspace_num);
  Status SaveTeCoreL2FlowDataForL2Fusion(const ccHandle_t &handle, int32_t input_num,
                                         int32_t output_num, uint64_t cur_ptr, const void *x[], const void *y[],
                                         rtL2Ctrl_t &tel2ctrl, uint32_t l2_args_size, uint32_t workspace_num);

  std::string GetUniqueGraphIdForNode() const;
  Status HandleAnchorWeight(size_t &weight_index);
  Status CheckAnchorInputValueSkip(size_t input_index) const;

  void SetInputAddrFromDataBase(const int64_t &input_offset);

  Status CheckArrayValue(const void *array[], int32_t num, const string& name) const;

  Status CheckForForward(ccHandle_t handle, rtStream_t &stream, const void *args, const void *x[], const void *y[],
                         int32_t input_num, int32_t output_num) const;

  Status CheckTensorSize(const ge::GeTensorDesc &tensor_desc, uint32_t i, bool is_input,
                         int32_t output_real_calc_flag) const;

  Status DealKernelLaunchForL2Buffer(ccHandle_t &handle, const rtStream_t stream, int32_t input_num,
                                     int32_t output_num,
                                     uint64_t cur_ptr, const void *x[], const void *y[], rtL2Ctrl_t &tel2ctrl,
                                     uint32_t args_size, uint32_t l2_args_size, const std::string &stub_func,
                                     uint32_t core_dim, const void *tmp_buf, int32_t workspace_num,
                                     domi::TaskDef &task_def);

  Status DealKernelLaunchForL2Fusion(ccHandle_t &handle, int32_t input_num, int32_t output_num,
                                     uint64_t cur_ptr, const void *x[], const void *y[], rtL2Ctrl_t &tel2ctrl,
                                     uint32_t args_size, uint32_t l2_args_size, const std::string &stub_func,
                                     uint32_t core_dim, const void *tmp_buf, int32_t workspace_num,
                                     domi::TaskDef &task_def);

  void DealInputOutputL2DataMap(const L2DataMap_t &l2datamap, int32_t data_num, const void *x[], const void *y[],
                                uint64_t &cur_ptr, uint32_t &l2_args_size, bool is_input) const;

  void DealInputOutputL2DataMap(const L2FusionDataMap_t &l2datamap, int32_t data_num, const void *x[], const void *y[],
                                uint64_t &cur_ptr, uint32_t &l2_args_size, bool is_input) const;

  void DisplayRtL2CtrlInfo(const rtL2Ctrl_t &l2ctrl, bool enable_l2) const;

  void DealInputOutputWithDdr(int32_t data_num, uint64_t &cur_ptr, uint32_t &l2_args_size) const;

  void MemCpyForL2IdAndL2Addr(uint64_t &cur_ptr, uint32_t &l2_args_size, int64_t data_in_l2_id,
                              uint64_t data_in_l2_addr) const;
};

}  // namespace fe

#endif  // FUSION_ENGINE_UTILS_ADAPTER_TBE_ADAPTER_TBE_TASK_BUILDER_ADAPTER_H_