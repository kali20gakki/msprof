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

#ifndef OPENSOURCEGE_AIR_TESTS_ST_STUBS_UTILS_TASKDEF_BUILDER_H_
#define OPENSOURCEGE_AIR_TESTS_ST_STUBS_UTILS_TASKDEF_BUILDER_H_

#include "proto/task.pb.h"
#include "graph/node.h"
#include "graph/op_desc.h"
#include "aicpu/common/aicpu_task_struct.h"
#include "hybrid/node_executor/aicpu/aicpu_ext_info.h"
#include "framework/common/taskdown_common.h"

namespace ge {
class ExtInfoBuilder {
 public:
  explicit ExtInfoBuilder(std::vector<uint8_t> &buffer) : buffer_(buffer) {
  }
  template<typename T>
  ExtInfoBuilder &AddExtInfo(int32_t type, const T &value) {
    AddExtInfoHeader(type, sizeof(value));
    AddExtInfoValue(value);
    return *this;
  }

  ExtInfoBuilder &AddExtInfoHeader(int32_t type, size_t value_size) {
    auto current_size = buffer_.size();
    auto header_size = sizeof(hybrid::AicpuExtInfo);
    buffer_.resize(current_size + header_size);
    auto *ext_info = reinterpret_cast<hybrid::AicpuExtInfo *>(buffer_.data() + current_size);
    ext_info->infoType = type;
    ext_info->infoLen = value_size;
    return *this;
  }

  template<typename T>
  ExtInfoBuilder &AddExtInfoValue(const T &value) {
    auto current_size = buffer_.size();
    auto ext_info_size = sizeof(value);
    buffer_.resize(current_size + ext_info_size);
    auto *dst_addr = buffer_.data() + current_size;
    memcpy_s(dst_addr, sizeof(value), &value, sizeof(value));
    return *this;
  }

  ExtInfoBuilder &AddUnknownShapeType(int32_t type) {
    return AddExtInfo(aicpu::FWKAdapter::FWK_ADPT_EXT_SHAPE_TYPE, type);
  }

  ExtInfoBuilder &AddShapeAndType(size_t num, int32_t type) {
    AddExtInfoHeader(type, sizeof(hybrid::AicpuShapeAndType) * num);
    hybrid::AicpuShapeAndType shape_and_type;
    shape_and_type.type = 0;
    for (size_t i = 0; i < num; ++i) {
      AddExtInfoValue(shape_and_type);
    }
    return *this;
  }

 private:
  std::vector<uint8_t> &buffer_;
};

class AicpuTaskDefBuilder {
 public:
  struct AicpuTaskStruct {
    aicpu::AicpuParamHead head;
    uint64_t io_addrp[6];
  }__attribute__((packed));

  AicpuTaskDefBuilder(const Node &node) : node_(node) {
  }

  domi::TaskDef BuildHostCpuTask(int unknown_shape_type) {
    auto task_def = BuildAicpuTask(unknown_shape_type);
    task_def.mutable_kernel()->mutable_context()->set_kernel_type(8);  // HOST_CPU
    node_.GetOpDesc()->SetOpKernelLibName("DNN_VM_HOST_CPU_OP_STORE");
    return task_def;
  }

  domi::TaskDef BuildTfTask(int unknown_shape_type) {
    auto op_desc = node_.GetOpDesc();
    AttrUtils::SetInt(op_desc, ATTR_NAME_UNKNOWN_SHAPE_TYPE, unknown_shape_type);
    op_desc->SetOpKernelLibName("aicpu_tf_kernel");
    domi::TaskDef task_def;
    task_def.set_type(RT_MODEL_TASK_KERNEL_EX);
    domi::KernelExDef *kernel_ex_def = task_def.mutable_kernel_ex();
    kernel_ex_def->set_kernel_ext_info_size(12);
    kernel_ex_def->set_op_index(op_desc->GetId());

    std::vector<uint8_t> buffer;
    BuildAiCpuExtInfo(unknown_shape_type, *op_desc, buffer);
    kernel_ex_def->set_kernel_ext_info(buffer.data(), buffer.size());
    kernel_ex_def->set_kernel_ext_info_size(buffer.size());
    return task_def;
  }

  domi::TaskDef BuildAicpuTask(int unknown_shape_type) {
    auto op_desc = node_.GetOpDesc();
    domi::TaskDef task_def;
    AttrUtils::SetInt(op_desc, ATTR_NAME_UNKNOWN_SHAPE_TYPE, unknown_shape_type);
    op_desc->SetOpKernelLibName("aicpu_ascend_kernel");
    task_def.set_type(RT_MODEL_TASK_KERNEL);
    domi::KernelDef *kernel_def = task_def.mutable_kernel();

    AicpuTaskStruct args{};
    args.head.length = sizeof(args);
    args.head.ioAddrNum = op_desc->GetInputsSize() + op_desc->GetOutputsSize();
    kernel_def->set_args(reinterpret_cast<const char *>(&args), args.head.length);
    kernel_def->set_args_size(args.head.length);

    std::vector<uint8_t> buffer;
    BuildAiCpuExtInfo(unknown_shape_type, *op_desc, buffer);
    kernel_def->set_kernel_ext_info(buffer.data(), buffer.size());
    kernel_def->set_kernel_ext_info_size(buffer.size());
    domi::KernelContext *context = kernel_def->mutable_context();

    context->set_kernel_type(6);    // ccKernelType::AI_CPU
    context->set_op_index(op_desc->GetId());
    uint16_t args_offset[9] = {0};
    context->set_args_offset(args_offset, 9 * sizeof(uint16_t));
    return task_def;
  }

  static void BuildAiCpuExtInfo(int32_t type, OpDesc &op_desc, std::vector<uint8_t> &buffer) {
    uint64_t ext_bitmap = 0;  // FWK_ADPT_EXT_BITMAP
    uint32_t update_addr = 0; // FWK_ADPT_EXT_UPDATE_ADDR
    uint32_t topic_type = 0;
    ExtInfoBuilder(buffer)
        .AddUnknownShapeType(type)
        .AddShapeAndType(op_desc.GetInputsSize(), aicpu::FWKAdapter::FWK_ADPT_EXT_INPUT_SHAPE)
        .AddShapeAndType(op_desc.GetOutputsSize(), aicpu::FWKAdapter::FWK_ADPT_EXT_OUTPUT_SHAPE)
        .AddExtInfo(aicpu::FWKAdapter::FWK_ADPT_EXT_ASYNCWAIT, hybrid::AsyncWaitInfo{})
        .AddExtInfo(aicpu::FWKAdapter::FWK_ADPT_EXT_SESSION_INFO, hybrid::AicpuSessionInfo{})
        .AddExtInfo(aicpu::FWKAdapter::FWK_ADPT_EXT_BITMAP, ext_bitmap)
        .AddExtInfo(aicpu::FWKAdapter::FWK_ADPT_EXT_UPDATE_ADDR, update_addr)
        .AddExtInfo(aicpu::FWKAdapter::FWK_ADPT_EXT_TOPIC_TYPE, topic_type);
  }
 private:
  const Node &node_;
};

class AiCoreTaskDefBuilder {
 public:
  explicit AiCoreTaskDefBuilder(const Node &node) : node_(node) {}

  domi::TaskDef BuildTask(bool is_dynamic = false) {
    auto op_desc = node_.GetOpDesc();
    op_desc->SetOpKernelLibName("AIcoreEngine");

    AttrUtils::SetStr(op_desc, TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF");
    std::vector<char> atomic_kernel(128);
    std::vector<char> relu_kernel(256);
    op_desc->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, std::make_shared<OpKernelBin>("relu_xxx", std::move(relu_kernel)));

    std::vector<uint8_t> args(100, 0);
    domi::TaskDef task_def;
    task_def.set_type(RT_MODEL_TASK_KERNEL);
    auto kernel_info = task_def.mutable_kernel();
    kernel_info->set_args(args.data(), args.size());
    kernel_info->set_args_size(100);
    kernel_info->mutable_context()->set_kernel_type(static_cast<uint32_t>(ccKernelType::TE));
    kernel_info->set_kernel_name(node_.GetName());
    kernel_info->set_block_dim(1);
    uint16_t args_offset = 0;
    kernel_info->mutable_context()->set_args_offset(&args_offset, 2 * sizeof(uint16_t));
    kernel_info->mutable_context()->set_op_index(node_.GetOpDesc()->GetId());

    if (is_dynamic) {
      AttrUtils::SetBool(op_desc, "support_dynamicshape", true);
      AttrUtils::SetInt(op_desc, "op_para_size", 32);
      AttrUtils::SetStr(op_desc, "compile_info_key", "ddd");
      AttrUtils::SetStr(op_desc, "compile_info_json", "{}");
    }
    return task_def;
  }

  domi::TaskDef BuildAtomicAddrCleanTask() {
    auto op_desc = node_.GetOpDesc();
    AttrUtils::SetInt(op_desc, "atomic_op_para_size", 16);
    AttrUtils::SetStr(op_desc, "_atomic_compile_info_key", "ddd");
    AttrUtils::SetStr(op_desc, "_atomic_compile_info_json", "{}");
    std::vector<char> atomic_kernel(128);
    op_desc->SetExtAttr(EXT_ATTR_ATOMIC_TBE_KERNEL,
                        std::make_shared<OpKernelBin>("atomic_xxx", std::move(atomic_kernel)));

    vector<int64_t> atomic_output_index = {0};
    AttrUtils::SetListInt(op_desc, ATOMIC_ATTR_OUTPUT_INDEX, atomic_output_index);
    auto task_def = BuildTask(false);
    return task_def;
  }

  domi::TaskDef BuildTaskWithHandle() {
    auto op_desc = node_.GetOpDesc();
    op_desc->SetOpKernelLibName("AIcoreEngine");

    AttrUtils::SetStr(op_desc, TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF");
    std::vector<char> atomic_kernel(128);
    std::vector<char> relu_kernel(256);
    std::string bin_name = op_desc->GetName() + "_tvm";
    op_desc->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, std::make_shared<OpKernelBin>(bin_name, std::move(relu_kernel)));

    domi::TaskDef task_def;
    task_def.set_type(RT_MODEL_TASK_ALL_KERNEL);
    auto *kernel_with_handle = task_def.mutable_kernel_with_handle();
    kernel_with_handle->set_original_kernel_key("");
    kernel_with_handle->set_node_info("");
    kernel_with_handle->set_block_dim(32);
    kernel_with_handle->set_args_size(64);
    string args(64, '1');
    kernel_with_handle->set_args(args.data(), 64);
    domi::KernelContext *context = kernel_with_handle->mutable_context();
    context->set_op_index(op_desc->GetId());
    context->set_kernel_type(2);    // ccKernelType::TE
    uint16_t args_offset[9] = {0};
    context->set_args_offset(args_offset, 9 * sizeof(uint16_t));

    AttrUtils::SetBool(op_desc, "support_dynamicshape", true);
    AttrUtils::SetInt(op_desc, "op_para_size", 32);

    AttrUtils::SetStr(op_desc, "compile_info_key", "ddd");
    AttrUtils::SetStr(op_desc, "compile_info_json", "{}");
    return task_def;
  }

 private:
  const Node &node_;
};

}  // namespace ge
#endif //OPENSOURCEGE_AIR_TESTS_ST_STUBS_UTILS_TASKDEF_BUILDER_H_
