/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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
#include "kernel_builder.h"
#include <stdint.h>
#include "engine/base_engine.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/type_utils.h"
#include "securec.h"
#include "util/constant.h"
#include "util/log.h"
#include "util/util.h"

using namespace std;
using namespace ge;
using namespace ::aicpu::FWKAdapter;

namespace {
const string kAllShape = "_AllShape";

// need fix when update libgraph.so in blue zone
const std::string AICPU_ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE = "_tensor_no_tiling_mem_type";

const std::string kResourceList = "_resource_list";
const std::string kResourceType = "resource_type";
const std::string kQueueName = "queue_name";
const std::string kQueueDepth = "queue_depth";
const std::string kQueueId = "queue_id";
const std::string kQueueIdIdx = "queue_id_idx";
const int64_t kDefaultQueueDepth = 100;
}  // namespace

namespace aicpu {
Status KernelBuilder::GetWorkspaceInfo(const OpDescPtr &op_desc_ptr,
                                       uint8_t *data_mem_base,
                                       uint64_t &workspace_bytes_size) const {
  // Get the workspace's mem base and mem size
  AICPU_CHECK_NOTNULL_ERRCODE(op_desc_ptr, INPUT_PARAM_NULL)

  uintptr_t data_mem_base_addr = reinterpret_cast<uintptr_t>(data_mem_base);
  int64_t workspace_mem_base = static_cast<int64_t>(data_mem_base_addr);

  // Get workspace total size, the data source is from calcOpRunningParam
  vector<int64_t> workspace_bytes = op_desc_ptr->GetWorkspaceBytes();
  AICPUE_LOGI("The workspace_bytes vector's size is [%lu], op[%s].",
              workspace_bytes.size(), op_desc_ptr->GetName().c_str());
  CHECK_EQUAL(workspace_bytes.size(), 1, INPUT_PARAM_VALID,
      "workspace bytes vector's size[%zu] should be 1, op[%s].",
      workspace_bytes.size(), op_desc_ptr->GetName().c_str())
  CHECK_RES_BOOL((workspace_bytes[0] >= 0), INPUT_PARAM_VALID,
      AICPU_REPORT_INNER_ERROR("Invalied workspace bytes[%ld], op[%s]",
          workspace_bytes[0], op_desc_ptr->GetName().c_str()))
  workspace_bytes_size = static_cast<uint64_t>(workspace_bytes[0]);
  AICPUE_LOGI("The workspace byte size is [%lu], op[%s]", workspace_bytes_size,
              op_desc_ptr->GetName().c_str());
  // Check workspace mem
  CHECK_INT64_ADD_OVERFLOW(workspace_mem_base, workspace_bytes_size, DATA_OVERFLOW,
      "Overflow when offset workspace bytes[%lu] base on workspace addr[%ld], op[%s]",
      workspace_bytes_size, workspace_mem_base, op_desc_ptr->GetName().c_str())

  return SUCCESS;
}

ge::Status KernelBuilder::MakeExtInfoForOpName(
    const ge::OpDescPtr &op_desc_ptr, std::vector<char> &task_ext_info) const {
  AICPU_CHECK_NOTNULL_ERRCODE(op_desc_ptr, INPUT_PARAM_NULL)
  // WARNING: OP NAME MUST BE THE FIRST EXTEND INFO FOR RUNTIME!!!
  // make extend info for op name
  string op_name = op_desc_ptr->GetName();
  // calc ExtInfo length
  uint64_t extend_info_len = task_ext_info.size();
  // ext info: op name, value: op_name.length()
  extend_info_len += (kExtInfoHeadSize + op_name.length());

  uint64_t ext_info_offset = task_ext_info.size();
  task_ext_info.resize(extend_info_len, 0);
  char *ext_info_buf = task_ext_info.data();
  AICPU_CHECK_NOTNULL_ERRCODE(ext_info_buf, INPUT_PARAM_NULL)
  // create ExtInfo struct
  AICPU_CHECK_NOTNULL(ext_info_buf)
  ExtInfo *ext_info = reinterpret_cast<ExtInfo *>(
      reinterpret_cast<uintptr_t>(ext_info_buf) + ext_info_offset);
  ext_info->infoType = FWK_ADPT_EXT_OP_NAME;
  ext_info->infoLen = op_name.length();
  ext_info_offset += kExtInfoHeadSize;
  char *op_name_buf = reinterpret_cast<char *>(
      reinterpret_cast<uintptr_t>(ext_info_buf) + ext_info_offset);
  errno_t cpy_ret =
      memcpy_s(op_name_buf, ext_info->infoLen, op_name.c_str(), op_name.length());
  if (cpy_ret != EOK) {
    AICPUE_LOGW(
        "Generate extend info for op[%s] failed, ext info len[%u], ret[%d].",
        op_name.c_str(), ext_info->infoLen, cpy_ret);
  }
  AICPUE_LOGI("Make ext_info for op[%s], ext info len[%u].", op_name.c_str(),
              ext_info->infoLen);
  return SUCCESS;
}

void KernelBuilder::GetInOutPutsShape(const ge::OpDescPtr &op_desc_ptr,
                                      std::vector<std::vector<int64_t>> &inputs_shape,
                                      std::vector<std::vector<int64_t>> &outputs_shape) const {
  size_t input_size = op_desc_ptr->GetAllInputsSize();
  size_t output_size = op_desc_ptr->GetAllOutputsDescSize();
  for (size_t i = 0; i < input_size; i++) {
    auto input_desc = op_desc_ptr->GetInputDesc(i);
    auto input_shape = input_desc.GetShape();
    auto input_dims = input_shape.GetDims();
    inputs_shape.push_back(input_dims);
  }
  for (size_t i = 0; i < output_size; i++) {
    auto output_desc = op_desc_ptr->GetOutputDesc(i);
    auto output_shape = output_desc.GetShape();
    auto output_dims = output_shape.GetDims();
    outputs_shape.push_back(output_dims);
  }
}

void KernelBuilder::CalcBaseExtInfoLen(const ge::OpDescPtr &op_desc_ptr,
                                       const bool op_async_flag,
                                       uint64_t &extend_info_len) const {
  // ext info 1: unknown shape_type, value: int32
  extend_info_len += (kExtInfoHeadSize + sizeof(int32_t));
  // ext info 2: bitmap, value: uint64
  extend_info_len += (kExtInfoHeadSize + sizeof(uint64_t));
  // ext info 3: topic type, value: enum
  extend_info_len += (kExtInfoHeadSize + sizeof(FWKExtTopicType));
  // ext info 4: input ShapeAndType, value: input_num * sizeof(ShapeAndType)
  // get input and output total num. no need to check overflow
  size_t input_num = op_desc_ptr->GetInputsSize();
  size_t output_num = op_desc_ptr->GetOutputsSize();
  extend_info_len += kExtInfoHeadSize;
  extend_info_len += (input_num * sizeof(ShapeAndType));
  // ext info 5: output ShapeAndType, value: output_num * sizeof(ShapeAndType)
  extend_info_len += kExtInfoHeadSize;
  extend_info_len += (output_num * sizeof(ShapeAndType));
  // ext info 6: Wait
  if (op_async_flag) {
    extend_info_len += (kExtInfoHeadSize + sizeof(AsyncWait));
  }
  return;
}

Status KernelBuilder::MakeNoTilingExtInfo(const OpDescPtr &op_desc_ptr, vector<char> &task_ext_info) const {
  size_t input_size = op_desc_ptr->GetAllInputsSize();
  vector<uint32_t> input_index;
  for (size_t i = 0; i < input_size; i++) {
    auto tiling_mem_type = false;
    auto input_desc = op_desc_ptr->GetInputDesc(i);
    (void)AttrUtils::GetBool(input_desc, AICPU_ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, tiling_mem_type);
    AICPUE_LOGD("Op[%s], input[%zu] mem type[%d].", op_desc_ptr->GetName().c_str(), i, tiling_mem_type);
    if (tiling_mem_type) {
      input_index.emplace_back(static_cast<uint32_t>(i));
    }
  }
  size_t output_size = op_desc_ptr->GetAllOutputsDescSize();
  vector<uint32_t> output_index;
  for (size_t i = 0; i < output_size; i++) {
    auto tiling_mem_type = false;
    auto output_desc = op_desc_ptr->GetOutputDesc(i);
    (void)AttrUtils::GetBool(output_desc, AICPU_ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, tiling_mem_type);
    AICPUE_LOGD("Op[%s], output[%zu] mem type[%d].", op_desc_ptr->GetName().c_str(), i, tiling_mem_type);
    if (tiling_mem_type) {
      output_index.emplace_back(static_cast<uint32_t>(i));
    }
  }
  uint64_t extend_info_len = 0;
  if (!input_index.empty()) {
    extend_info_len += (kExtInfoHeadSize + input_index.size() * sizeof(uint32_t));
  }
  if (!output_index.empty()) {
    extend_info_len += (kExtInfoHeadSize + output_index.size() * sizeof(uint32_t));
  }
  if (extend_info_len != 0) {
    uint64_t ext_info_offset = task_ext_info.size();
    task_ext_info.resize(extend_info_len + ext_info_offset, 0);
    char *ext_info_buf = task_ext_info.data();
    if (!input_index.empty()) {
      ExtInfo *ext_info = reinterpret_cast<ExtInfo *>(reinterpret_cast<uintptr_t>(ext_info_buf) + ext_info_offset);
      ext_info->infoType = FWK_ADPT_EXT_UNKNOWN_SHAPE_INPUT_INDEX;
      ext_info->infoLen = input_index.size() * sizeof(uint32_t);
      ext_info_offset += kExtInfoHeadSize;
      uint32_t *input_index_ptr =
          reinterpret_cast<uint32_t *>(reinterpret_cast<uintptr_t>(ext_info_buf) + ext_info_offset);
      auto cpy_ret = memcpy_s(input_index_ptr, ext_info->infoLen, input_index.data(), ext_info->infoLen);
      if (cpy_ret != EOK) {
        AICPUE_LOGE("Generate extend info for op[%s] failed, ext info type[%d], ext info len[%u], memcpy_s ret[%d].",
                    op_desc_ptr->GetName().c_str(), ext_info->infoType, ext_info->infoLen, cpy_ret);
        return RT_MEMCPY_ERROR;
      }
      ext_info_offset += ext_info->infoLen;
    }
    if (!output_index.empty()) {
      ExtInfo *ext_info = reinterpret_cast<ExtInfo *>(reinterpret_cast<uintptr_t>(ext_info_buf) + ext_info_offset);
      ext_info->infoType = FWK_ADPT_EXT_UNKNOWN_SHAPE_OUTPUT_INDEX;
      ext_info->infoLen = output_index.size() * sizeof(uint32_t);
      ext_info_offset += kExtInfoHeadSize;
      uint32_t *output_index_ptr =
          reinterpret_cast<uint32_t *>(reinterpret_cast<uintptr_t>(ext_info_buf) + ext_info_offset);
      auto cpy_ret = memcpy_s(output_index_ptr, ext_info->infoLen, output_index.data(), ext_info->infoLen);
      if (cpy_ret != EOK) {
        AICPUE_LOGE("Generate extend info for op[%s] failed, ext info type[%d], ext info len[%u], memcpy_s ret[%d].",
                    op_desc_ptr->GetName().c_str(), ext_info->infoType, ext_info->infoLen, cpy_ret);
        return RT_MEMCPY_ERROR;
      }
    }
  }
  return SUCCESS;
}

Status KernelBuilder::GetFftsPlusInAddrOffset(const ge::OpDescPtr &op_desc_ptr,
                                              FftsPlusInfo &ffts_info) const {
  const uint32_t index = ffts_info.slice_instance_index;
  if (index >= ffts_info.thread_input_shape.size()) {
    AICPU_REPORT_INNER_ERROR("Node[%s] index[%u], thread_input_shape size[%zu]",
                             op_desc_ptr->GetName().c_str(), index,
                             ffts_info.thread_input_shape.size());
    return ge::GE_GRAPH_INIT_FAILED;
  }
  const size_t input_size = op_desc_ptr->GetAllInputsSize();
  AICPUE_LOGD("input size[%zu]", input_size);
  if (input_size != ffts_info.thread_input_shape[index].size()) {
    AICPU_REPORT_INNER_ERROR(
        "Node[%s] tensor slice input size is not equal to op_desc, "
        "index[%u], input_tensor_slice size[%zu], input_tensor_size[%zu]",
        op_desc_ptr->GetName().c_str(), index,
        ffts_info.thread_input_shape[index].size(), input_size);
    return ge::GE_GRAPH_INIT_FAILED;
  }
  for (size_t i = 0; i < input_size; i++) {
    auto input_desc = op_desc_ptr->GetInputDesc(i);
    auto data_type = input_desc.GetDataType();
    int32_t data_size = GetDataTypeSize(data_type);
    AICPU_CHECK_GREATER_THAN_ZERO(
        data_size, DATA_TYPE_UNDEFILED, "Invalid data type[%s].",
        ge::TypeUtils::DataTypeToSerialString(data_type).c_str())
    int64_t temp_size = 1;
    for (size_t j = 0; j < ffts_info.thread_input_shape[index][i].size(); j++) {
      CHECK_INT64_MUL_OVERFLOW(temp_size, ffts_info.thread_input_shape[index][i][j],
          ErrorCode::DATA_OVERFLOW,
          "Mul Operator for int64 is data overflow, temp_size is[%ld],"
          " thread_input_shape is [%ld]", temp_size,
          ffts_info.thread_input_shape[index][i][j])
      temp_size *= ffts_info.thread_input_shape[index][i][j];
    }
    CHECK_INT64_MUL_OVERFLOW(temp_size, data_size,
        ErrorCode::DATA_OVERFLOW,
        "Mul Operator for int64 is data overflow, temp_size is[%ld],"
        " data_size is [%d]", temp_size, data_size)
    ffts_info.input_addr_offset.push_back(temp_size * data_size);
  }

  return ge::SUCCESS;
}

Status KernelBuilder::GetFftsPlusOutAddrOffset(const ge::OpDescPtr &op_desc_ptr,
                                               FftsPlusInfo &ffts_info) const {
  const uint32_t index = ffts_info.slice_instance_index;
  if (index >= ffts_info.thread_output_shape.size()) {
    AICPU_REPORT_INNER_ERROR("Node[%s] index[%u], thread_output_shape[%zu], ",
                             op_desc_ptr->GetName().c_str(), index,
                             ffts_info.thread_output_shape.size());
    return ge::GE_GRAPH_INIT_FAILED;
  }
  const size_t output_size = op_desc_ptr->GetAllOutputsDescSize();
  AICPUE_LOGD("output size[%zu]", output_size);
  if (output_size != ffts_info.thread_output_shape[index].size()) {
    AICPU_REPORT_INNER_ERROR(
        "Node[%s] tensor slice output size is not equal to op_desc, "
        "index[%u], output_tensor_slice size[%zu], output_tensor_size[%zu]",
        op_desc_ptr->GetName().c_str(), index,
        ffts_info.thread_output_shape[index].size(), output_size);
    return ge::GE_GRAPH_INIT_FAILED;
  }
  for (size_t i = 0; i < output_size; i++) {
    auto output_desc = op_desc_ptr->GetOutputDesc(i);
    auto data_type = output_desc.GetDataType();
    int32_t data_size = GetDataTypeSize(data_type);
    AICPU_CHECK_GREATER_THAN_ZERO(
        data_size, DATA_TYPE_UNDEFILED, "Invalid data type[%s].",
        ge::TypeUtils::DataTypeToSerialString(data_type).c_str())
    int64_t temp_size = 1;
    for (size_t j = 0; j < ffts_info.thread_output_shape[index][i].size(); j++) {
      CHECK_INT64_MUL_OVERFLOW(temp_size, ffts_info.thread_output_shape[index][i][j],
          ErrorCode::DATA_OVERFLOW,
          "Mul Operator for int64 is data overflow, temp_size is[%ld],"
          " thread_output_shape is [%ld]", temp_size,
          ffts_info.thread_output_shape[index][i][j])
      temp_size *= ffts_info.thread_output_shape[index][i][j];
    }
    CHECK_INT64_MUL_OVERFLOW(temp_size, data_size,
        ErrorCode::DATA_OVERFLOW,
        "Mul Operator for int64 is data overflow, temp_size is[%ld],"
        " data_size is [%d]", temp_size, data_size)
    ffts_info.output_addr_offset.push_back(temp_size * data_size);
  }

  return ge::SUCCESS;
}

Status KernelBuilder::GetFftsPlusInOutAddrOffset(const ge::OpDescPtr &op_desc_ptr,
                                                 FftsPlusInfo &ffts_info) const {
  if (!ffts_info.auto_static_split) {
    return SUCCESS;
  }
  Status state = GetFftsPlusInAddrOffset(op_desc_ptr, ffts_info);
  if (state != SUCCESS) {
    return state;
  }

  return GetFftsPlusOutAddrOffset(op_desc_ptr, ffts_info);
}

// Make and init task extend info
Status KernelBuilder::MakeBaseExtInfo(const ge::OpDescPtr &op_desc_ptr,
                                      std::vector<char> &task_ext_info,
                                      const FftsPlusInfo &ffts_info) const {
  // set attr _all_shape
  CHECK_RES_BOOL(
      AttrUtils::SetBool(op_desc_ptr, kAllShape, true), INVOKE_GRAPH_ITF_FAILED,
      AICPU_REPORT_CALL_ERROR(
          "Call ge::AttrUtils::SetBool failed to set attr[%s], op[%s].",
          kAllShape.c_str(), op_desc_ptr->GetName().c_str()))
  bool op_async_flag = false;
  if (ge::AttrUtils::HasAttr(op_desc_ptr, kAsyncFlag)) {
    if (!ge::AttrUtils::GetBool(op_desc_ptr, kAsyncFlag, op_async_flag)) {
      AICPU_REPORT_CALL_ERROR("Call ge::AttrUtils::GetBool failed to get attr[%s], op[%s].", kAsyncFlag.c_str(),
                              op_desc_ptr->GetName().c_str());
      return GET_ATTR_FAILED;
    }
  }
  // calc ExtInfo length
  uint64_t extend_info_len = task_ext_info.size();
  (void)CalcBaseExtInfoLen(op_desc_ptr, op_async_flag, extend_info_len);

  // get input and output total num. no need to check overflow
  size_t input_num = op_desc_ptr->GetInputsSize();
  size_t output_num = op_desc_ptr->GetOutputsSize();

  uint64_t ext_info_offset = task_ext_info.size();
  task_ext_info.resize(extend_info_len, 0);
  char *ext_info_buf = task_ext_info.data();
  AICPU_CHECK_NOTNULL_ERRCODE(ext_info_buf, INPUT_PARAM_NULL)
  // initialize extend info
  // init ext info 1: unknown shape_type
  int32_t unknow_shape_type = 0;
  if (!AttrUtils::GetInt(op_desc_ptr, ATTR_NAME_UNKNOWN_SHAPE_TYPE,
                         unknow_shape_type)) {
    AICPU_REPORT_CALL_ERROR(
        "Call ge::AttrUtils::GetInt failed to get attr[%s], op[%s].",
        ATTR_NAME_UNKNOWN_SHAPE_TYPE.c_str(), op_desc_ptr->GetName().c_str());
    return GET_ATTR_FAILED;
  }
  ExtInfo *ext_info = reinterpret_cast<ExtInfo *>(ext_info_buf + ext_info_offset);
  ext_info->infoType = FWK_ADPT_EXT_SHAPE_TYPE;
  ext_info->infoLen = sizeof(int32_t);
  ext_info_offset += kExtInfoHeadSize;
  int32_t *shape_type = reinterpret_cast<int32_t *>(ext_info_buf + ext_info_offset);
  *shape_type = unknow_shape_type;
  ext_info_offset += ext_info->infoLen;
  // init ext info 2: BitMap
  ext_info = reinterpret_cast<ExtInfo *>(ext_info_buf + ext_info_offset);
  ext_info->infoType = FWK_ADPT_EXT_BITMAP;
  ext_info->infoLen = sizeof(uint64_t);
  ext_info_offset += kExtInfoHeadSize;
  uint64_t *bit_map = reinterpret_cast<uint64_t *>(ext_info_buf + ext_info_offset);
  *bit_map = 0;
  ext_info_offset += ext_info->infoLen;
  std::vector<uint32_t> inputs_type;
  std::vector<uint32_t> outputs_type;
  std::vector<std::vector<int64_t>> inputs_shape;
  std::vector<std::vector<int64_t>> outputs_shape;
  GetInOutPutsDataType(op_desc_ptr, inputs_type, outputs_type);
  if (ffts_info.valid) {
    inputs_shape = ffts_info.thread_input_shape[ffts_info.slice_instance_index];
    outputs_shape = ffts_info.thread_output_shape[ffts_info.slice_instance_index];
  } else {
    GetInOutPutsShape(op_desc_ptr, inputs_shape, outputs_shape);
  }

  // init ext info 3: topicType
  int32_t type_value = 0;
  if (!AttrUtils::GetInt(op_desc_ptr, kTopicType, type_value)) {
    AICPUE_LOGI("Can not get attr[topic_type], op[%s], use default value[DEVICE_ONLY].",
                op_desc_ptr->GetName().c_str());
  }
  ext_info = reinterpret_cast<ExtInfo *>(ext_info_buf + ext_info_offset);
  ext_info->infoType = FWK_ADPT_EXT_TOPIC_TYPE;
  ext_info->infoLen = sizeof(FWKExtTopicType);
  ext_info_offset += kExtInfoHeadSize;
  FWKExtTopicType *topic_type = reinterpret_cast<FWKExtTopicType *>(ext_info_buf + ext_info_offset);
  *topic_type = FWKExtTopicType(type_value);
  ext_info_offset += ext_info->infoLen;
  // init ext info 4: input ShapeAndType
  ext_info = reinterpret_cast<ExtInfo *>(ext_info_buf + ext_info_offset);
  ext_info->infoType = FWK_ADPT_EXT_INPUT_SHAPE;
  ext_info->infoLen = (input_num * sizeof(ShapeAndType));
  ext_info_offset += kExtInfoHeadSize;
  ShapeAndType *inputs =
      reinterpret_cast<ShapeAndType *>(ext_info_buf + ext_info_offset);
  for (size_t index = 0; index < inputs_type.size(); ++index) {
    inputs[index].type = inputs_type[index];
  }
  for (size_t index = 0; index < inputs_shape.size(); ++index) {
    size_t dim_length = inputs_shape[index].size();
    if (dim_length > FWKAdapter::kMaxShapeDims) {
      AICPU_REPORT_INNER_ERROR(
          "op[%s] set Input ExtInfo Shape failed because dimLen [%zu] > [%u]",
          op_desc_ptr->GetName().c_str(), dim_length,
          FWKAdapter::kMaxShapeDims);
      return INVOKE_GRAPH_ITF_FAILED;
    }
    for (size_t i = 0; i < dim_length; ++i) {
      inputs[index].dims[i] = inputs_shape[index][i];
    }
    if (dim_length < FWKAdapter::kMaxShapeDims) {
      inputs[index].dims[dim_length] = INT64_MIN;
    }
  }

  ext_info_offset += ext_info->infoLen;
  // init ext info 5: output ShapeAndType
  ext_info = reinterpret_cast<ExtInfo *>(ext_info_buf + ext_info_offset);
  ext_info->infoType = FWK_ADPT_EXT_OUTPUT_SHAPE;
  ext_info->infoLen = (output_num * sizeof(ShapeAndType));
  ext_info_offset += kExtInfoHeadSize;
  ShapeAndType *outputs =
      reinterpret_cast<ShapeAndType *>(ext_info_buf + ext_info_offset);
  for (size_t index = 0; index < outputs_type.size(); ++index) {
    outputs[index].type = outputs_type[index];
  }
  if (unknow_shape_type != ge::DEPEND_COMPUTE) {
    for (size_t index = 0; index < outputs_shape.size(); ++index) {
      size_t dim_length = outputs_shape[index].size();
      if (dim_length > FWKAdapter::kMaxShapeDims) {
        AICPU_REPORT_INNER_ERROR(
            "op[%s] set Output ExtInfo Shape failed because dimLen[%zu] > [%u]",
            op_desc_ptr->GetName().c_str(), dim_length,
            FWKAdapter::kMaxShapeDims);
        return INVOKE_GRAPH_ITF_FAILED;
      }
      for (size_t i = 0; i < dim_length; ++i) {
        outputs[index].dims[i] = outputs_shape[index][i];
      }
      if (dim_length < FWKAdapter::kMaxShapeDims) {
        outputs[index].dims[dim_length] = INT64_MIN;
      }
    }
  }

  ext_info_offset += ext_info->infoLen;
  AICPUE_LOGI(
      "Make ext_info for unknown shape success: op[%s], op_type[%s], "
      "input_num[%zu], output_num[%zu], extend length[%lu].",
      op_desc_ptr->GetName().c_str(), op_desc_ptr->GetType().c_str(), input_num,
      output_num, extend_info_len);
  // init ext info 6 : Async Wait
  if (op_async_flag) {
    ext_info = reinterpret_cast<ExtInfo *>(ext_info_buf + ext_info_offset);
    ext_info->infoType = FWK_ADPT_EXT_ASYNCWAIT;
    ext_info->infoLen = sizeof(AsyncWait);
    ext_info_offset += kExtInfoHeadSize;
    AsyncWait *wait_info = reinterpret_cast<AsyncWait *>(ext_info_buf + ext_info_offset);
    wait_info->waitType = FWK_ADPT_WAIT_TYPE_NULL;
    wait_info->waitId = 0;
    wait_info->timeOut = 0;
    wait_info->reserved = 0;
    ext_info_offset += ext_info->infoLen;
    AICPUE_LOGI("Make ext_info for wait info success : op[%s], op_type[%s].", op_desc_ptr->GetName().c_str(),
                op_desc_ptr->GetType().c_str());
  }
  return SUCCESS;
}

FWKAdapter::FWKExtTopicType KernelBuilder::GetOpNodeTopicType(const ge::OpDescPtr &op_desc_ptr) const {
  int32_t type;
  if (!AttrUtils::GetInt(op_desc_ptr, kTopicType, type)) {
    AICPUE_LOGI("op[%s] Can not get attr of topic_type", op_desc_ptr->GetName().c_str());
    type = FWK_ADPT_TOPIC_DEVICE_ONLY;
  }
  return FWKExtTopicType(type);
}

Status KernelBuilder::SetAttrQueueResource(
    const std::string &node_name,
    std::shared_ptr<ge::OpDesc> &op_desc_ptr,
    std::vector<ge::GeAttrValue::NAMED_ATTRS> &resource_list) const {
  ge::GeAttrValue::NAMED_ATTRS resource_queue;
  AICPU_CHECK_FALSE_EXEC(
      ge::AttrUtils::SetStr(resource_queue, kResourceType, kResourceQueue),
      AICPU_REPORT_INNER_ERROR("Set attr[%s] failed, op[%s]",
                                  kResourceType.c_str(), node_name.c_str());
      return ErrorCode::ADD_ATTR_FAILED)

  std::string queue_name;
  AICPU_CHECK_FALSE_EXEC(
      ge::AttrUtils::GetStr(op_desc_ptr, kQueueName, queue_name),
      AICPU_REPORT_INNER_ERROR("Get attr[%s] failed, op[%s]",
                                  kQueueName.c_str(), node_name.c_str());
      return ErrorCode::GET_ATTR_FAILED)
  AICPU_CHECK_FALSE_EXEC(
      ge::AttrUtils::SetStr(resource_queue, kQueueName, queue_name),
      AICPU_REPORT_INNER_ERROR("Set attr[%s] failed, op[%s]",
                                  kQueueName.c_str(), node_name.c_str());
      return ErrorCode::ADD_ATTR_FAILED)

  int64_t queue_depth = 0;
  (void)ge::AttrUtils::GetInt(op_desc_ptr, kQueueDepth, queue_depth);
  if (queue_depth <= 0) {
      queue_depth = kDefaultQueueDepth;
  }
  AICPU_CHECK_FALSE_EXEC(
      ge::AttrUtils::SetInt(resource_queue, kQueueDepth, queue_depth),
      AICPU_REPORT_INNER_ERROR("Set attr[%s] failed, op[%s]",
                                  kQueueDepth.c_str(), node_name.c_str());
      return ErrorCode::ADD_ATTR_FAILED)

  const int64_t input_index = op_desc_ptr->GetInputIndexByName(kQueueId);
  AICPU_CHECK_FALSE_EXEC(
      ge::AttrUtils::SetInt(resource_queue, kQueueIdIdx, input_index),
      AICPU_REPORT_INNER_ERROR("Set attr[%s] failed, op[%s]",
                                  kQueueIdIdx.c_str(), node_name.c_str());
      return ErrorCode::ADD_ATTR_FAILED)
  resource_list.push_back(resource_queue);
  return ge::SUCCESS;
}

Status KernelBuilder::SetAttrResource(
    const std::string &node_name,
    std::shared_ptr<ge::OpDesc> &op_desc_ptr) const {
  std::string resourceValue;
  (void)ge::AttrUtils::GetStr(op_desc_ptr, kResource, resourceValue);
  AICPUE_LOGI("Op[%s] get attr resource is %s", node_name.c_str(),
              resourceValue.c_str());
  if (resourceValue.empty()) {
    return ge::SUCCESS;
  }

  std::vector<std::string> resouceList = SplitString(resourceValue, ",");
  std::vector<ge::GeAttrValue::NAMED_ATTRS> resource_list;
  for (std::string const& resource: resouceList) {
    if (resource == kResourceQueue) {
      AICPU_CHECK_RES_WITH_LOG(
          SetAttrQueueResource(node_name, op_desc_ptr, resource_list),
          "Call SetAttrQueueResource function failed, op[%s].",
          node_name.c_str());
    } else if (resource == kResourceChannel) {
      ge::GeAttrValue::NAMED_ATTRS resource_channel;
      AICPU_CHECK_FALSE_EXEC(
          ge::AttrUtils::SetStr(resource_channel, kResourceType,
                                kResourceChannel),
          AICPU_REPORT_INNER_ERROR("Set attr[%s] failed, op[%s]",
                                  kResourceType.c_str(), node_name.c_str());
          return ErrorCode::ADD_ATTR_FAILED)
      resource_list.push_back(resource_channel);
    } else if (resource == kResourceVdecChannel) {
      ge::GeAttrValue::NAMED_ATTRS resource_channel;
      AICPU_CHECK_FALSE_EXEC(
          ge::AttrUtils::SetStr(resource_channel, kResourceType,
                                kResourceVdecChannel),
          AICPU_REPORT_INNER_ERROR("Set attr[%s] failed, op[%s]",
                                  kResourceType.c_str(), node_name.c_str());
          return ErrorCode::ADD_ATTR_FAILED)
      resource_list.push_back(resource_channel);
    }
  }

  if (!resource_list.empty()) {
    AICPUE_LOGI("Op[%s] resource list size is %zu", node_name.c_str(),
                resource_list.size());
    AICPU_CHECK_FALSE_EXEC(
        ge::AttrUtils::SetListNamedAttrs(*op_desc_ptr, kResourceList,
                                         resource_list),
        AICPU_REPORT_INNER_ERROR("Set attr[%s] failed, op[%s]",
                                 kResourceList.c_str(), node_name.c_str());
        return ErrorCode::ADD_ATTR_FAILED)
  }
  return ge::SUCCESS;
}
}  // namespace aicpu
