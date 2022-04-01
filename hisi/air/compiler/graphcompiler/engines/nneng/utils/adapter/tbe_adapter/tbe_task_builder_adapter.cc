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

#include "adapter/tbe_adapter/tbe_task_builder_adapter.h"
#include "adapter/tbe_adapter/kernel_launch/l2_cache_kernel_launch.h"
#include "common/comm_log.h"
#include "common/common_utils.h"
#include "common/configuration.h"
#include "common/graph_comm.h"
#include "common/l2_stream_info.h"
#include "common/lxfusion_json_util.h"
#include "common/op_tensor_utils.h"
#include "common/common_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/tuning_utils.h"
#include "runtime/rt_error_codes.h"
#include "securec.h"

namespace fe {
// tiling data size to be reserved when generate task for unknown shape op
static const int RESERVED_TILING_DATA_SIZE = 8;
// max value of weight offset : 30 * 1024 * 1024 * 1024L.
static const int64_t MAX_WEIGHT_OFFSET = 32212254720L;
static const uint32_t DEFAULT_KERNEL_BLOCK_DIM = 1;
static const int ONE_MEM_TYPE_SIZE = 1;
static const std::string OP_PARA_SIZE = "op_para_size";

thread_local rtL2Ctrl_t g_tel2ctrl;

void TbeTaskBuilderAdapter::MemCpyForL2IdAndL2Addr(uint64_t &cur_ptr, uint32_t &l2_args_size, int64_t data_in_l2_id,
                                                   uint64_t data_in_l2_addr) const {
  if (l2_args_size < sizeof(int64_t)) {
    REPORT_CM_ERROR("[GenTask][Memcpy][Node %s type %s] l2_args_size (which is %u) is smalle than size of int64_t.",
                    op_desc_->GetName().c_str(), op_desc_->GetType().c_str(), l2_args_size);
    return;
  }
  errno_t ret = memcpy_s(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(cur_ptr)),
                         l2_args_size, &data_in_l2_id, sizeof(int64_t));
  if (ret != EOK) {
    REPORT_CM_ERROR("[GenTask][Memcpy][Node %s type %s] Failed to memcpy data in l2 id, error num is %d.",
                    op_desc_->GetName().c_str(), op_desc_->GetType().c_str(), ret);
    return;
  }
  l2_args_size = l2_args_size - sizeof(uint64_t);
  cur_ptr = cur_ptr + sizeof(uint64_t);

  if (l2_args_size < sizeof(int64_t)) {
    REPORT_CM_ERROR("[GenTask][Memcpy][Node %s type %s] l2_args_size (which is %u) is smalle than size of int64_t.",
                    op_desc_->GetName().c_str(), op_desc_->GetType().c_str(), l2_args_size);
    return;
  }
  ret = memcpy_s(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(cur_ptr)),
                 l2_args_size, &data_in_l2_addr, sizeof(uint64_t));
  if (ret != EOK) {
    REPORT_CM_ERROR("[GenTask][Memcpy][Node %s type %s] Failed to memcpy data in l2 addr, error num is %d.",
                    op_desc_->GetName().c_str(), op_desc_->GetType().c_str(), ret);
    return;
  }
  l2_args_size = l2_args_size - sizeof(uint64_t);
  cur_ptr = cur_ptr + sizeof(uint64_t);
}

void TbeTaskBuilderAdapter::DealInputOutputWithDdr(int32_t data_num, uint64_t &cur_ptr,
                                                   uint32_t &l2_args_size) const {
  for (int i = 0; i != data_num; ++i) {
    MemCpyForL2IdAndL2Addr(cur_ptr, l2_args_size, -1, 0);
  }
}

void TbeTaskBuilderAdapter::DealInputOutputL2DataMap(const L2DataMap_t &l2datamap, int32_t data_num,
                                                     const void *x[], const void *y[], uint64_t &cur_ptr,
                                                     uint32_t &l2_args_size,
                                                     bool is_input) const {
  for (int i = 0; i < data_num; ++i) {
    L2DataMap_t::const_iterator iter;
    for (iter = l2datamap.begin(); iter != l2datamap.end(); ++iter) {
      const L2Data_t &flowdata = iter->second;
      int64_t data_in_l2_id = flowdata.l2Index;
      auto ddr_key = iter->first;

      if (is_input && static_cast<uint64_t>(reinterpret_cast<uintptr_t>(x[i])) == ddr_key) {
        CM_LOGD("iter->first value is %ld, (uint64_t)(uintptr_t)x[%d] value is %ld", ddr_key, i,
                static_cast<uint64_t>(reinterpret_cast<uintptr_t>(x[i])));
        MemCpyForL2IdAndL2Addr(cur_ptr, l2_args_size, data_in_l2_id, flowdata.l2Addr);
        break;
      }

      if (!is_input && static_cast<uint64_t>(reinterpret_cast<uintptr_t>(y[i])) == ddr_key) {
        CM_LOGD("iter->first value is %ld, (uint64_t)(uintptr_t)y[%d] value is %ld", ddr_key, i,
                static_cast<uint64_t>(reinterpret_cast<uintptr_t>(y[i])));
        MemCpyForL2IdAndL2Addr(cur_ptr, l2_args_size, data_in_l2_id, flowdata.l2Addr);
        break;
      }
    }

    if (iter == l2datamap.end()) {
      string input_or_output = is_input ? "input" : "output";
      CM_LOGD("Can not find anything in l2datamap for the %s %d, set l2_index=-1 and l2_offset=0.",
              input_or_output.c_str(), i);
      MemCpyForL2IdAndL2Addr(cur_ptr, l2_args_size, -1, 0);
    }
  }
}

void TbeTaskBuilderAdapter::DealInputOutputL2DataMap(const L2FusionDataMap_t &l2datamap, int32_t data_num,
                                                     const void *x[], const void *y[],
                                                     uint64_t &cur_ptr, uint32_t &l2_args_size,
                                                     bool is_input) const {
  for (int i = 0; i < data_num; ++i) {
    L2FusionDataMap_t::const_iterator iter;
    for (iter = l2datamap.begin(); iter != l2datamap.end(); ++iter) {
      const L2FusionData_t &flowdata = iter->second;
      int64_t data_in_l2_id = flowdata.l2Index;
      auto ddr_key = iter->first;

      if (is_input && static_cast<uint64_t>(reinterpret_cast<uintptr_t>(x[i])) == ddr_key) {
        CM_LOGD("iter->first value is %ld, (uint64_t)(uintptr_t)x[%d] value is %ld", ddr_key, i,
                static_cast<uint64_t>(reinterpret_cast<uintptr_t>(x[i])));
        MemCpyForL2IdAndL2Addr(cur_ptr, l2_args_size, data_in_l2_id, flowdata.l2Addr);
        break;
      }

      if (!is_input && static_cast<uint64_t>(reinterpret_cast<uintptr_t>(y[i])) == ddr_key) {
        CM_LOGD("iter->first value is %ld, (uint64_t)(uintptr_t)y[%d] value is %ld", ddr_key, i,
                static_cast<uint64_t>(reinterpret_cast<uintptr_t>(y[i])));
        MemCpyForL2IdAndL2Addr(cur_ptr, l2_args_size, data_in_l2_id, flowdata.l2Addr);
        break;
      }
    }

    if (iter == l2datamap.end()) {
      string input_or_output = is_input ? "input" : "output";
      CM_LOGD("Can not find anything in l2datamap for the %s %d, set l2_index=-1 and l2_offset=0.",
              input_or_output.c_str(), i);
      MemCpyForL2IdAndL2Addr(cur_ptr, l2_args_size, -1, 0);
    }
  }
}

Status TbeTaskBuilderAdapter::SaveTeCoreL2FlowDataForL2Buffer(const ccHandle_t &handle, const rtStream_t stream_id,
                                                              int32_t input_num, int32_t output_num, uint64_t cur_ptr,
                                                              const void *x[], const void *y[], rtL2Ctrl_t &tel2ctrl,
                                                              uint32_t l2_args_size, uint32_t workspace_num) {
  TaskL2Info_t *l2_data = nullptr;

  (void)memset_s(&tel2ctrl, sizeof(rtL2Ctrl_t), 0, sizeof(rtL2Ctrl_t));

  if (handle == nullptr) {
    CM_LOGI("handle is nullptr!");
    return FAILED;
  }

  std::string batch_label = "Batch_-1";
  (void)ge::AttrUtils::GetStr(node_.GetOpDesc(), ge::ATTR_NAME_BATCH_LABEL, batch_label);
  Status ret = StreamL2Info::Instance().GetStreamL2Info(stream_id, node_.GetName(), l2_data, batch_label);
  if ((ret == SUCCESS) && (l2_data != nullptr)) {
    CM_LOGI("Node[type=%s,name=%s]: find the l2 data form stream_l2_map.", node_.GetType().c_str(),
            node_.GetName().c_str());
    L2DataMap_t input = l2_data->input;
    L2DataMap_t output = l2_data->output;
    DealInputOutputL2DataMap(input, input_num, x, y, cur_ptr, l2_args_size, true);
    DealInputOutputL2DataMap(output, output_num, x, y, cur_ptr, l2_args_size, false);
    DealInputOutputWithDdr(workspace_num, cur_ptr, l2_args_size);
    tel2ctrl = l2_data->l2ctrl;
    return SUCCESS;
  } else {  // Const/PlaceHolder/PlaceEnd/Data
    CM_LOGW("Node[type=%s,name=%s]: can not find the l2 data from stream_l2_map.", node_.GetType().c_str(),
            node_.GetName().c_str());
    return FAILED;
  }
}

Status TbeTaskBuilderAdapter::SaveTeCoreL2FlowDataForL2Fusion(const ccHandle_t &handle,
                                                              int32_t input_num, int32_t output_num, uint64_t cur_ptr,
                                                              const void *x[], const void *y[], rtL2Ctrl_t &tel2ctrl,
                                                              uint32_t l2_args_size, uint32_t workspace_num) {
  (void)memset_s(&tel2ctrl, sizeof(rtL2Ctrl_t), 0, sizeof(rtL2Ctrl_t));

  if (handle == nullptr) {
    CM_LOGI("handle is nullptr!");
    return FAILED;
  }

  ge::OpDescPtr node_desc = node_.GetOpDesc();
  L2FusionInfoPtr l2_info = GetL2FusionInfoFromJson(node_desc);
  if (l2_info == nullptr) {
    CM_LOGD("Node[type=%s,name=%s]: the l2_fusion_info is nullptr.", node_desc->GetType().c_str(),
            node_desc->GetName().c_str());
    return PARAM_INVALID;
  }

  L2FusionDataMap_t &input = l2_info->input;
  L2FusionDataMap_t &output = l2_info->output;
  DealInputOutputL2DataMap(input, input_num, x, y, cur_ptr, l2_args_size, true);
  DealInputOutputL2DataMap(output, output_num, x, y, cur_ptr, l2_args_size, false);
  DealInputOutputWithDdr(workspace_num, cur_ptr, l2_args_size);

  tel2ctrl = l2_info->l2_info.l2ctrl;
  CM_LOGD("Node[type=%s,name=%s]: SaveL2DataFlow find L2 Alloc and do L2fusion success.", node_desc->GetType().c_str(),
          node_desc->GetName().c_str());
  return SUCCESS;
}

void TbeTaskBuilderAdapter::DisplayRtL2CtrlInfo(const rtL2Ctrl_t &l2ctrl, bool enable_l2) const {
  CM_LOGD("L2ctrl.size = %lu.", l2ctrl.size);
  CM_LOGD("L2 %s.", enable_l2 ? "enable" : "disable");
  for (uint32_t i = 0; i < MAX_L2_DATANUM; i++) {
    if (l2ctrl.data[i].L2_mirror_addr != 0) {
      CM_LOGD("data_index = %u.", i);
      CM_LOGD("L2_data_section_size = %u.", l2ctrl.data[i].L2_data_section_size);
      CM_LOGD("L2_mirror_addr = 0x%lx.", l2ctrl.data[i].L2_mirror_addr);
      CM_LOGD("L2_page_offset_base = %u.", l2ctrl.data[i].L2_page_offset_base);
      CM_LOGD("prev_L2_page_offset_base = %d.", l2ctrl.data[i].prev_L2_page_offset_base);
      CM_LOGD("L2_preload = %u.", l2ctrl.data[i].L2_preload);
      CM_LOGD("L2_load_to_ddr = %u.", l2ctrl.data[i].L2_load_to_ddr);
      CM_LOGD("modified = %u.", l2ctrl.data[i].modified);
      CM_LOGD("priority = %u.", l2ctrl.data[i].priority);
    }
  }
}

Status TbeTaskBuilderAdapter::CheckArrayValue(const void *array[], int32_t num, const string& name) const {
  if (array == nullptr) {
    CM_LOGD("%s is nullptr! Please check.", name.c_str());
  } else {
    for (int i = 0; i < num; i++) {
      if (array[i] == nullptr) {
        REPORT_CM_ERROR("[GenTask][TbeFwd][Check][Node %s type %s] The %s[%d] now is nullptr!",
                        op_desc_->GetName().c_str(), op_desc_->GetType().c_str(), name.c_str(), i);
        return TASK_BUILDER_STATUS_BAD_PARAM;
      }
    }
  }
  return SUCCESS;
}

Status TbeTaskBuilderAdapter::CheckForForward(ccHandle_t handle, rtStream_t &stream, const void *args, const void *x[],
                                              const void *y[], int32_t input_num, int32_t output_num) const {
  CM_CHECK_NOTNULL(args);
  string x_name = "x";
  Status result = CheckArrayValue(x, input_num, x_name);
  if (result != SUCCESS) {
    return result;
  }

  string y_name = "y";
  result = CheckArrayValue(y, output_num, y_name);
  if (result != SUCCESS) {
    return result;
  }

  if (TaskBuilderAdapter::GetHandleStream(handle, &stream) != SUCCESS) {
    return TASK_BUILDER_STATUS_INTERNAL_ERROR;
  }
  if (CheckUint32AddOverflow(static_cast<uint32_t>(input_num), static_cast<uint32_t>(output_num)) != SUCCESS) {
    REPORT_CM_ERROR("[GenTask][TbeFwd][Check] Unsigned Integer %u and %u addition can result in overflow!",
                    static_cast<uint32_t>(input_num), static_cast<uint32_t>(output_num));
    return TASK_BUILDER_STATUS_BAD_PARAM;
  }
  if (CheckUint32MulOverflow((static_cast<uint32_t>(input_num) + static_cast<uint32_t>(output_num)),
                             (static_cast<uint32_t>(sizeof(int64_t) + sizeof(uint64_t)))) != SUCCESS) {
    REPORT_CM_ERROR("[GenTask][TbeFwd][Check] Unsigned Integer %u and %u multiplication can result in overflow!",
                    (static_cast<uint32_t>(input_num) + static_cast<uint32_t>(output_num)),
                    (static_cast<uint32_t>(sizeof(int64_t) + sizeof(uint64_t))));
    return TASK_BUILDER_STATUS_BAD_PARAM;
  }
  return SUCCESS;
}

Status TbeTaskBuilderAdapter::DealKernelLaunchForL2Buffer(ccHandle_t &handle, const rtStream_t stream,
                                                          int32_t input_num,
                                                          int32_t output_num, uint64_t cur_ptr, const void *x[],
                                                          const void *y[], rtL2Ctrl_t &tel2ctrl, uint32_t args_size,
                                                          uint32_t l2_args_size, const std::string &stub_func,
                                                          uint32_t core_dim, const void *tmp_buf, int32_t workspace_num,
                                                          domi::TaskDef &task_def) {
  auto op_name = node_.GetName();
  auto op_type = node_.GetType();
  bool kernelRet = false;
  std::string first_kernel_name;
  Status ret = SaveTeCoreL2FlowDataForL2Buffer(handle, stream, input_num, output_num, cur_ptr, x, y, tel2ctrl,
                                               l2_args_size, workspace_num);
  if (ret == SUCCESS) {
    CM_LOGD("Node[type=%s,name=%s]: L2 alloc infomation get success, core_dim=%d, args_size=%d, l2_args_size=%d.",
            op_type.c_str(), op_name.c_str(), core_dim, args_size, l2_args_size);

    for (uint64_t idx = 0; idx < ((args_size) / sizeof(uint64_t)); ++idx) {
      uint64_t current_address =
          *(reinterpret_cast<uint64_t *>(reinterpret_cast<uintptr_t>(tmp_buf) + idx * sizeof(uint64_t)));
      CM_LOGD("Node[type=%s,name=%s]: do distribute index[%ld], args=[%ld] is ddr address value.", op_type.c_str(),
              op_name.c_str(), idx, current_address);
    }
    uint32_t index_of_offset_base = 2;
    for (uint64_t idx = 0; idx < (l2_args_size / (sizeof(uint64_t))); ++idx) {
      if (idx % index_of_offset_base == 0) {
        uint64_t current_address =
            *(reinterpret_cast<uint64_t *>(reinterpret_cast<uintptr_t>(tmp_buf) + args_size + idx * sizeof(int64_t)));
        CM_LOGD("Node[type=%s,name=%s]: do distribute index[%ld], args=[%ld] is l2 index value.", op_type.c_str(),
                op_name.c_str(), idx, current_address);
      } else {
        uint64_t current_address =
            *(reinterpret_cast<uint64_t *>(reinterpret_cast<uintptr_t>(tmp_buf) + args_size + idx * sizeof(uint64_t)));
        CM_LOGD("Node[type=%s,name=%s]: do distribute index[%ld], args=[%lu] is l2 offset value.", op_type.c_str(),
                op_name.c_str(), idx, current_address);
      }
    }
    if (ge::AttrUtils::GetStr(node_.GetOpDesc(), ATTR_NAME_KERNEL_LIST_FIRST_NAME, first_kernel_name)) {
      kernelRet = TbeKernelLaunch::KernelLaunchWithHandle(core_dim, tmp_buf, args_size + l2_args_size, &tel2ctrl,
                                                          task_def);
    } else {
      kernelRet = TbeKernelLaunch::KernelLaunch(stub_func, core_dim, tmp_buf, args_size + l2_args_size, &tel2ctrl,
                                                task_def);
    }
  } else {
    CM_LOGD("Node[type=%s,name=%s]: can not find L2 alloc infomation and use the ddr address.", op_type.c_str(),
            op_name.c_str());
    if (ge::AttrUtils::GetStr(node_.GetOpDesc(), ATTR_NAME_KERNEL_LIST_FIRST_NAME, first_kernel_name)) {
      kernelRet = TbeKernelLaunch::KernelLaunchWithHandle(core_dim, tmp_buf, args_size, nullptr, task_def);
    } else {
      kernelRet = TbeKernelLaunch::KernelLaunch(stub_func, core_dim, tmp_buf, args_size, nullptr, task_def);
    }
  }
  if (!kernelRet) {
    return TASK_BUILDER_STATUS_RUNTIME_ERROR;
  }
  DisplayRtL2CtrlInfo(tel2ctrl, fe::getFuncState(fe::FuncParamType::FUSION_L2));
  return SUCCESS;
}

Status TbeTaskBuilderAdapter::DealKernelLaunchForL2Fusion(ccHandle_t &handle, int32_t input_num,
                                                          int32_t output_num, uint64_t cur_ptr, const void *x[],
                                                          const void *y[], rtL2Ctrl_t &tel2ctrl, uint32_t args_size,
                                                          uint32_t l2_args_size, const std::string &stub_func,
                                                          uint32_t core_dim, const void *tmp_buf,
                                                          int32_t workspace_num, domi::TaskDef &task_def) {
  auto op_name = node_.GetName();
  auto op_type = node_.GetType();
  bool kernelRet = false;
  Status ret = SaveTeCoreL2FlowDataForL2Fusion(handle, input_num, output_num, cur_ptr, x, y, tel2ctrl,
                                               l2_args_size, workspace_num);
  string first_kernel_name;
  if (ret == SUCCESS) {
    CM_LOGD("Node[type=%s,name=%s]: L2 alloc infomation get success, core_dim=%d, args_size=%d, l2_args_size=%d.",
            op_type.c_str(), op_name.c_str(), core_dim, args_size, l2_args_size);

    for (uint64_t idx = 0; idx < ((args_size) / sizeof(uint64_t)); ++idx) {
      uint64_t address = *(reinterpret_cast<uint64_t *>(reinterpret_cast<uintptr_t>(tmp_buf) + idx * sizeof(uint64_t)));
      CM_LOGD("Node[type=%s,name=%s]: do distribute index[%ld], args[%ld] is ddr address value.", op_type.c_str(),
              op_name.c_str(), idx, address);
    }
    uint32_t index_offset_base = 2;
    for (uint64_t idx = 0; idx < (l2_args_size / (sizeof(uint64_t))); ++idx) {
      if (idx % index_offset_base == 0) {
        uint64_t address =
            *(reinterpret_cast<uint64_t *>(reinterpret_cast<uintptr_t>(tmp_buf) + args_size + idx * sizeof(int64_t)));
        CM_LOGD("Node[type=%s,name=%s]: do distribute index[%ld], args=[%ld] is l2 index value.", op_type.c_str(),
                op_name.c_str(), idx, address);
      } else {
        uint64_t address =
            *(reinterpret_cast<uint64_t *>(reinterpret_cast<uintptr_t>(tmp_buf) + args_size + idx * sizeof(uint64_t)));
        CM_LOGD("Node[type=%s,name=%s]: do distribute index[%ld], args=[%lu] is l2 offset value.", op_type.c_str(),
                op_name.c_str(), idx, address);
      }
    }
    if (ge::AttrUtils::GetStr(node_.GetOpDesc(), ATTR_NAME_KERNEL_LIST_FIRST_NAME, first_kernel_name)) {
      kernelRet = TbeKernelLaunch::KernelLaunchWithHandle(core_dim, tmp_buf, args_size + l2_args_size,
                                                          &tel2ctrl, task_def);
    } else {
      kernelRet = TbeKernelLaunch::KernelLaunch(stub_func, core_dim, tmp_buf, args_size + l2_args_size, &tel2ctrl,
                                                task_def);
  }
  } else {
    CM_LOGD("Node[type=%s,name=%s]: can not find L2 Alloc infomation and use the ddr address(l2_index=-1,l2_offset=0).",
            op_type.c_str(), op_name.c_str());
    DealInputOutputWithDdr(input_num, cur_ptr, l2_args_size);
    DealInputOutputWithDdr(output_num, cur_ptr, l2_args_size);
    DealInputOutputWithDdr(workspace_num, cur_ptr, l2_args_size);
    if (ge::AttrUtils::GetStr(node_.GetOpDesc(), ATTR_NAME_KERNEL_LIST_FIRST_NAME, first_kernel_name)) {
      kernelRet = TbeKernelLaunch::KernelLaunchWithHandle(core_dim, tmp_buf, args_size + l2_args_size, &tel2ctrl,
                                                          task_def);
    } else {
      kernelRet = TbeKernelLaunch::KernelLaunch(stub_func, core_dim, tmp_buf, args_size + l2_args_size, &tel2ctrl,
                                                task_def);
    }
  }
  if (!kernelRet) {
    return TASK_BUILDER_STATUS_RUNTIME_ERROR;
  }
  DisplayRtL2CtrlInfo(tel2ctrl, true);
  return SUCCESS;
}

Status TbeTaskBuilderAdapter::TbeForward(ccHandle_t handle, uint32_t core_dim, const void *args, uint32_t args_size,
                                         int32_t input_num, const void *x[], int32_t output_num, const void *y[],
                                         int32_t workspace_num, domi::TaskDef &task_def) {
  void *tmp_buf = nullptr;
  rtStream_t stream = nullptr;
  Status ret = CheckForForward(handle, stream, args, x, const_cast<const void **>(y), input_num, output_num);
  if (ret != SUCCESS) {
    return ret;
  }

  std::string stub_func;
  if (!ge::AttrUtils::GetStr(op_desc_, ATTR_NAME_KERNEL_LIST_FIRST_NAME, stub_func)) {
    stub_func = GetUniqueGraphIdForNode();
  }
  CM_LOGD("Generate stub func string[%s] of node[%s, %s]",
          stub_func.c_str(), op_desc_->GetName().c_str(), op_desc_->GetType().c_str());

  // l2 buffer or l2 fusion
  bool is_l2_buffer = fe::getFuncState(fe::FuncParamType::FUSION_L2);
  bool lx_fusion_pass = false;
  (void)ge::AttrUtils::GetBool(node_.GetOpDesc(), ATTR_NAME_LX_FUSION_PASS, lx_fusion_pass);
  BufferFusionMode buffer_fusion_mode = Configuration::Instance(AI_CORE_NAME).GetBufferFusionMode();
  std::string build_mode_value = Configuration::Instance(AI_CORE_NAME).GetBuildMode();
  bool is_l2_fusion =
      (buffer_fusion_mode == EN_L2_FUSION && lx_fusion_pass) || build_mode_value == ge::BUILD_MODE_TUNING;

  if (is_l2_fusion || is_l2_buffer) {
    if (CheckUint32AddOverflow(static_cast<uint32_t>(input_num), static_cast<uint32_t>(output_num)) != SUCCESS) {
      REPORT_CM_ERROR(
          "[GenTask][TbeFwd][L2Check][Node %s type %s] Unsigned Integer %u and %u addition can result in overflow!",
          op_desc_->GetName().c_str(), op_desc_->GetType().c_str(),
          static_cast<uint32_t>(input_num), static_cast<uint32_t>(output_num));
      return TASK_BUILDER_STATUS_BAD_PARAM;
    }
    if (CheckUint32AddOverflow(static_cast<uint32_t>(input_num + output_num),
                               static_cast<uint32_t>(workspace_num)) != SUCCESS) {
      REPORT_CM_ERROR(
          "[GenTask][TbeFwd][L2Check][Node %s type %s] Unsigned Integer %u and %u addition can result in overflow!",
          op_desc_->GetName().c_str(), op_desc_->GetType().c_str(), static_cast<uint32_t>(input_num + output_num),
          static_cast<uint32_t>(workspace_num));
      return TASK_BUILDER_STATUS_BAD_PARAM;
    }
    uint32_t l2_args_size =
        static_cast<uint32_t>(input_num + output_num + workspace_num) * (sizeof(int64_t) + sizeof(uint64_t));
    if (rtMallocHost(&tmp_buf, args_size + l2_args_size) != ACL_RT_SUCCESS) {
      REPORT_CM_ERROR("[GenTask][TbeFwd][L2Check][Node %s type %s] Failed to do rt malloc",
                      op_desc_->GetName().c_str(), op_desc_->GetType().c_str());
      return TASK_BUILDER_STATUS_INTERNAL_ERROR;
    }

    CM_CHECK_NOTNULL(tmp_buf);
    if (rtMemcpy(tmp_buf, args_size, args, args_size, RT_MEMCPY_HOST_TO_HOST) != SUCCESS) {
      (void)rtFreeHost(tmp_buf);
      return TASK_BUILDER_STATUS_INTERNAL_ERROR;
    }

    uint64_t cur_ptr = reinterpret_cast<uint64_t>(tmp_buf) + args_size;
    if (is_l2_buffer) {
      ret = DealKernelLaunchForL2Buffer(handle, stream, input_num, output_num, cur_ptr, x, y, g_tel2ctrl, args_size,
                                        l2_args_size, stub_func, core_dim, tmp_buf, workspace_num, task_def);
    } else {
      ret = DealKernelLaunchForL2Fusion(handle, input_num, output_num, cur_ptr, x, y, g_tel2ctrl, args_size,
                                        l2_args_size, stub_func, core_dim, tmp_buf, workspace_num, task_def);
    }

    if (rtFreeHost(tmp_buf) != ACL_RT_SUCCESS) {
      return TASK_BUILDER_STATUS_INTERNAL_ERROR;
    }
    tmp_buf = nullptr;
    return ret;
  }

  AppendArgsMode append_args_mode = Configuration::Instance(AI_CORE_NAME).GetAppendArgsMode();
  if (append_args_mode == AppendArgsMode::L2_CACHE_ARGS) {
    L2CacheKernelLaunch l2_cache_kernel_launch(input_num);
    return l2_cache_kernel_launch.DealKernelLaunch(node_, args, args_size, stub_func, core_dim,
                                                   task_def);
  } else {
    TbeKernelLaunch tbe_kernel_launch(input_num);
    return tbe_kernel_launch.DealKernelLaunch(node_, args, args_size, stub_func, core_dim,
                                              task_def);
  }
}

Status TbeTaskBuilderAdapter::CheckTensorSize(const ge::GeTensorDesc &tensor_desc,
                                              uint32_t i, bool is_input, int32_t output_real_calc_flag) const {
  auto op_type = op_desc_->GetType();
  auto op_name = op_desc_->GetName();
  int64_t tensor_size = 0;
  if (OpTensorUtils::CalcTensorSize(tensor_desc, output_real_calc_flag, tensor_size) != SUCCESS) {
    REPORT_CM_ERROR("[GenTask][CheckTensorSize][Node %s type %s]:op output[%u] tensor size failed to calculate.",
                    op_name.c_str(), op_type.c_str(), i);
    return FAILED;
  }
  int64_t size_output = 0;
  if (ge::TensorUtils::GetSize(tensor_desc, size_output) != ge::GRAPH_SUCCESS) {
    REPORT_CM_ERROR("[GenTask][CheckTensorSize][Node %s type %s]:Get size input[%u] tensor failed!",
                    op_name.c_str(), op_type.c_str(), i);
    return fe::FAILED;
  }
  string input_or_output = is_input ? "Input" : "Output";
  // compare the two size
  if (size_output < tensor_size) {
    std::vector<int64_t> shape_dims = tensor_desc.GetShape().GetDims();
    REPORT_CM_ERROR("[GenTask][CheckTensorSize][Node %s type %s]: %s shape/format/dtype is [%s, %d, %d], Size:[%ld] is \
                    larger than default size:[%ld]",op_name.c_str(), op_type.c_str(),
                    input_or_output.c_str(), IntegerVecToString(shape_dims).c_str(),
                    tensor_desc.GetFormat(), tensor_desc.GetDataType(), tensor_size, size_output);
    return fe::FAILED;
  }
  return SUCCESS;
}

Status TbeTaskBuilderAdapter::CheckInputAndOutputSize() {
  // Get input size
  int32_t output_real_calc_flag = 0;
  for (size_t i = 0; i < op_desc_->GetAllInputsSize(); i++) {
    ge::GeTensorDescPtr tensorDescPtr = op_desc_->MutableInputDesc(i);
    if (tensorDescPtr == nullptr) {
      continue;
    }
    ge::GeTensorDesc tensor_desc = op_desc_->GetInputDesc(i);
    if (CheckTensorSize(tensor_desc, i, true, output_real_calc_flag) != SUCCESS) {
      return FAILED;
    }
  }
  bool ret = ge::AttrUtils::GetInt(op_desc_, ge::ATTR_NAME_GET_TENSOR_ACTUAL_SIZE, output_real_calc_flag);
  CM_LOGD("Get tensor_actual_size : [%d],[%d]", output_real_calc_flag, ret);
  for (size_t i = 0; i < op_desc_->GetAllOutputsDescSize(); i++) {
    ge::GeTensorDescPtr tensorDescPtr = op_desc_->MutableOutputDesc(i);
    if (tensorDescPtr == nullptr) {
      continue;
    }
    ge::GeTensorDesc tensor_desc = op_desc_->GetOutputDesc(i);
    if (CheckTensorSize(tensor_desc, i, false, output_real_calc_flag) != SUCCESS) {
      return FAILED;
    }
  }
  return SUCCESS;
}

TbeTaskBuilderAdapter::TbeTaskBuilderAdapter(const ge::Node &node, TaskBuilderContext &context)
    : TaskBuilderAdapter(node, context),
      block_dim_(DEFAULT_KERNEL_BLOCK_DIM) {}

TbeTaskBuilderAdapter::~TbeTaskBuilderAdapter() {}

std::string TbeTaskBuilderAdapter::GetUniqueGraphIdForNode() const {
  string session_graph_id = "";
  string tid = std::to_string(CommGetTid());
  string timestamp = std::to_string(GetMicroSecondsTime());
  if (ge::AttrUtils::GetStr(op_desc_, ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id) && !session_graph_id.empty()) {
    return tid + "_" + timestamp + "_" + session_graph_id + "_" + op_desc_->GetName();
  } else {
    return tid + "_" + timestamp + "_" + op_desc_->GetName();
  }
}

Status TbeTaskBuilderAdapter::Init() {
  auto op_type = op_desc_->GetType();
  auto op_name = op_desc_->GetName();
  CM_LOGD("Init begin, node name:%s, node type:%s.", node_.GetName().c_str(), node_.GetType().c_str());

  // Common initialization
  Status status = TaskBuilderAdapter::Init();
  if (status != SUCCESS) {
    REPORT_CM_ERROR("[GenTask][Init][Node %s type %s]:TaskBuilderAdapter::Init failed.",
                    op_name.c_str(), op_type.c_str());
    return status;
  }

  // Get block dim
  int32_t block_dim = -1;
  if (ge::AttrUtils::GetInt(op_desc_, ge::TVM_ATTR_NAME_BLOCKDIM, block_dim)) {
    block_dim_ = static_cast<uint32_t>(block_dim);
  }
  CM_LOGD("Tbe blockdim: %u", block_dim_);

  // Get magic
  string bin_magic;
  (void)ge::AttrUtils::GetStr(op_desc_, ge::TVM_ATTR_NAME_MAGIC, bin_magic);
  bool illegal_bin_magic = (bin_magic != "RT_DEV_BINARY_MAGIC_ELF" &&
                            bin_magic != "RT_DEV_BINARY_MAGIC_ELF_AIVEC" &&
                            bin_magic != "RT_DEV_BINARY_MAGIC_ELF_AICUBE" &&
                            bin_magic != "FFTS_BINARY_MAGIC_ELF_MIX_AIC" &&
                            bin_magic != "FFTS_BINARY_MAGIC_ELF_MIX_AIV");
  if (illegal_bin_magic) {
    REPORT_CM_ERROR("[GenTask][Init][Node %s type %s]:Unsupported binary magic %s, only support \
                    RT_DEV_BINARY_MAGIC_(ELF/ELF_AIVEC/ELF_AICUBE.",
                    op_name.c_str(), op_type.c_str(), bin_magic.c_str());
    return PARAM_INVALID;
  }

  // Check Size
  if (!OpTensorUtils::IsFeSupportedDynamicOp(*op_desc_, false)) {
    Status ret = CheckInputAndOutputSize();
    if (ret != SUCCESS) {
      return fe::FAILED;
    }
  }

  return SUCCESS;
}

void TbeTaskBuilderAdapter::SetInputAddrFromDataBase(const int64_t &input_offset) {
  // if the size of mem_type_to_data_mem_base less than one, directly set mem base from context
  if (context_.mem_type_to_data_mem_base.size() <= ONE_MEM_TYPE_SIZE) {
    input_addrs_.push_back(context_.dataMemBase + input_offset);
  } else {
    // if the size of mem_type_to_data_mem_base more than one, set mem base according to ATTR_NAME_TENSOR_MEM_TYPE
    int64_t tensor_memtype = -1;
    (void)ge::AttrUtils::GetInt(op_desc_, ge::ATTR_NAME_TENSOR_MEM_TYPE, tensor_memtype);
    CM_LOGD("Parse mem_type from node[%s] is %ld.", op_desc_->GetName().c_str(), tensor_memtype);
    auto it = context_.mem_type_to_data_mem_base.find(tensor_memtype);
    if (it != context_.mem_type_to_data_mem_base.end()) {
      input_addrs_.push_back(it->second + input_offset);
    } else {
      CM_LOGW("Cannot find tensor_mem_type[%ld] from node[%s] in data_mem_base.", tensor_memtype,
              op_desc_->GetName().c_str());
    }
  }
}

Status TbeTaskBuilderAdapter::HandleAnchorWeight(size_t &weight_index) {
  auto op_type = op_desc_->GetType();
  auto op_name = op_desc_->GetName();
  vector<ge::ConstGeTensorPtr> weights = ge::OpDescUtils::GetWeights(node_);

  if (weight_index >= weights.size()) {
    REPORT_CM_ERROR("[GenTask][HandleAnchor][Node %s type %s]: weightIndex must be less than weights.size(), \
                    weightIndex[%zu], weights.size()[%zu].", op_name.c_str(), op_type.c_str(),
                    weight_index, weights.size());
    return FAILED;
  }
  ge::ConstGeTensorPtr weight = weights[weight_index];
  int64_t weight_offset = 0;
  if (ge::TensorUtils::GetDataOffset(weight->GetTensorDesc(), weight_offset) != ge::GRAPH_SUCCESS) {
    REPORT_CM_ERROR("[GenTask][HandleAnchor][Node %s type %s]: Get weight offset failed.",
                    op_name.c_str(), op_type.c_str());
    return FAILED;
  }

  if (ge::TensorUtils::GetWeightSize(weight) > 0) {
    // max value of weight offset : 30 * 1024 * 1024 * 1024L.
    if (weight_offset < MAX_WEIGHT_OFFSET) {
      input_addrs_.push_back(context_.weightMemBase + weight_offset);
    } else {
      CM_LOGI("Print Input Addr is offset of Op name: %s, Op Type: %s, wight offset is too big.",
              op_desc_->GetName().c_str(), op_desc_->GetType().c_str());
      input_addrs_.push_back(reinterpret_cast<void *>(reinterpret_cast<intptr_t>(weight_offset)));
    }
  }
  CM_LOGI("weightOffset : %lu.", weight_offset);
  weight_index++;
  return SUCCESS;
}

Status TbeTaskBuilderAdapter::CheckAnchorInputValueSkip(size_t input_index) const {
  size_t input_size = op_desc_->GetAllInputsSize();
  if (input_index >= input_size) {
    CM_LOGI("Illegal input index. (input_index=%zu, input_size=%zu)", input_index, input_size);
    return FAILED;
  }
  ge::GeTensorDescPtr tensor_desc_ptr = op_desc_->MutableInputDesc(input_index);
  if (tensor_desc_ptr == nullptr) {
    return FAILED;
  }

  bool value_skip = false;
  (void)ge::AttrUtils::GetBool(tensor_desc_ptr, "valueSkip", value_skip);
  if (value_skip) {
    CM_LOGI("Input(%zu) valueSkip is true.", input_index);
    return SUCCESS;
  }
  return FAILED;
}

Status TbeTaskBuilderAdapter::InitInput() {
  auto op_type = op_desc_->GetType();
  auto op_name = op_desc_->GetName();
  vector<bool> input_is_addr_var;
  (void)ge::AttrUtils::GetListBool(op_desc_, ATTR_NAME_INPUT_IS_VAR, input_is_addr_var);

  // if input_offsets is empty, set 0 to vector
  vector<int64_t> input_offsets = op_desc_->GetInputOffset();
  if (input_offsets.empty()) {
    vector<int64_t> input_offset_zero(op_desc_->GetInputsSize(), 0);
    input_offsets.swap(input_offset_zero);
    CM_LOGD("Node[type=%s,name=%s]: input_offset_zero:%zu", op_type.c_str(), op_name.c_str(), input_offsets.size());
  }

  size_t input_index = 0;
  size_t anchor_index = 0;
  size_t weight_index = 0;
  for (auto const &anchor : node_.GetAllInDataAnchors()) {
    if (ge::AnchorUtils::GetStatus(anchor) == ge::ANCHOR_SUSPEND) {
      CM_LOGD("Node[type=%s,name=%s]: the anchor %zu status is suspend.", op_type.c_str(), op_name.c_str(),
              anchor_index);
      anchor_index++;
      continue;
    }

    if (CheckAnchorInputValueSkip(input_index) == SUCCESS) {
      input_addrs_.push_back(nullptr);
      input_index++;
      anchor_index++;
      continue;
    }

    if (ge::AnchorUtils::GetStatus(anchor) != ge::ANCHOR_DATA) {
      Status ret = HandleAnchorWeight(weight_index);
      if (ret != SUCCESS) {
        return ret;
      }
      input_index++;
      anchor_index++;
      continue;
    }

    if (input_index >= input_offsets.size()) {
      REPORT_CM_ERROR(
          "[GenTask][InitInput] [Node %s type %s]: inputIndex must be less than size of offset, index[%zu], size[%zu].",
          op_name.c_str(), op_type.c_str(), input_index, input_offsets.size());
      return FAILED;
    }

    int64_t input_offset = input_offsets[input_index];
    CM_LOGD("Node[type=%s,name=%s]: input_index=%zu, input_offset=%lu.", op_type.c_str(), op_name.c_str(), input_index,
            input_offset);
    bool is_addr_var = input_index < input_is_addr_var.size() && input_is_addr_var[input_index];
    if (is_addr_var) {
      input_addrs_.push_back(reinterpret_cast<void *>(reinterpret_cast<intptr_t>(input_offset)));
    } else {
      SetInputAddrFromDataBase(input_offset);
    }

    CM_LOGD("Node[type=%s,name=%s]: Init input.", op_type.c_str(), op_name.c_str());
    input_index++;
    anchor_index++;
  }

  return SUCCESS;
}

Status TbeTaskBuilderAdapter::Run(domi::TaskDef &task_def) {
  auto op_type = op_desc_->GetType();
  auto op_name = op_desc_->GetName();
  CM_LOGD("Node[type=%s,name=%s]: start to run.", op_type.c_str(), op_name.c_str());
  if (input_addrs_.empty()) {
    CM_LOGD("Node[type=%s,name=%s]: inputAddrs_ is empty!", op_type.c_str(), op_name.c_str());
  }
  if (output_addrs_.empty()) {
    CM_LOGD("Node[type=%s,name=%s]: outputAddrs_ is empty!", op_type.c_str(), op_name.c_str());
  }
  if (input_l1_addrs_.empty()) {
    CM_LOGD("Node[type=%s,name=%s]: inputL1Addrs_ is empty!", op_type.c_str(), op_name.c_str());
  }

  vector<void *> device_addrs;
  device_addrs.insert(device_addrs.cend(), input_addrs_.cbegin(), input_addrs_.cend());
  device_addrs.insert(device_addrs.cend(), output_addrs_.cbegin(), output_addrs_.cend());
  device_addrs.insert(device_addrs.cend(), workspace_addrs_.cbegin(), workspace_addrs_.cend());
  device_addrs.insert(device_addrs.cend(), input_l1_addrs_.cbegin(), input_l1_addrs_.cend());

  bool is_support_unknown_shape = false;
  (void)ge::AttrUtils::GetBool(node_.GetOpDesc(), ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, is_support_unknown_shape);
  int64_t is_unknown_shape_value = 0;
  (void)ge::AttrUtils::GetInt(node_.GetOpDesc(), ATTR_NAME_IS_UNKNOWN_SHAPE_OP, is_unknown_shape_value);
  CM_LOGD("Node[type=%s,name=%s]: is_unknown_shape flag is %ld.", op_type.c_str(), op_name.c_str(),
          is_unknown_shape_value);
  bool unknown_shape_flag =
      (is_support_unknown_shape && OpTensorUtils::IsFeSupportedDynamicOp(*node_.GetOpDesc(), false)) ||
      is_unknown_shape_value == IS_UNKNOWN_SHAPE_VALUE;

  if (unknown_shape_flag) {
    CM_LOGD("Node[type=%s,name=%s] is not support dynamic shape", op_type.c_str(), op_name.c_str());
    int64_t temp_op_para_size = 0;
    (void)ge::AttrUtils::GetInt(op_desc_, OP_PARA_SIZE, temp_op_para_size);
    if (temp_op_para_size) {
      char tiling_data_ptr[RESERVED_TILING_DATA_SIZE] = {0x00};
      device_addrs.push_back(tiling_data_ptr);
    }
  }

  size_t input_num = ge::OpDescUtils::GetNonConstInputsSize(node_) + ge::OpDescUtils::GetWeights(node_).size();
  size_t output_num = output_addrs_.size();
  CM_LOGD("Node[type=%s,name=%s]: start to call TbeForward, input_num=%zu, output_num=%zu, workspace_num=%zu, \
          device_address.size=%zu.", op_type.c_str(), op_name.c_str(), input_num, output_num, workspace_addrs_.size(),
          device_addrs.size());

  Status ret = TbeForward(context_.handle, block_dim_, device_addrs.data(), sizeof(void *) * device_addrs.size(),
                          static_cast<int32_t>(input_num), const_cast<const void **>(input_addrs_.data()),
                          static_cast<int32_t>(output_num), const_cast<const void **>(output_addrs_.data()),
                          static_cast<int32_t>(workspace_addrs_.size()), task_def);
  if (ret != SUCCESS) {
    REPORT_CM_ERROR("[GenTask][Run][Node %s type %s] TbeForward failed, ret[0x%X]",
                    op_name.c_str(), op_type.c_str(), ret);
    return FAILED;
  }

  CM_LOGD("Node[type=%s,name=%s]: end to run.", op_type.c_str(), op_name.c_str());
  return SUCCESS;
}

}  // namespace fe
