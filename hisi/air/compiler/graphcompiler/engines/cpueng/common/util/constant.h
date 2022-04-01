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
#ifndef AICPU_CONSTANT_H_
#define AICPU_CONSTANT_H_

#include <string>

namespace aicpu {

// Original type
const std::string kOriginalType = "original_type";

// Framework Op
const std::string kFrameworkOp = "FrameworkOp";

const std::string kOpsFlagClose = "0";

// Operator Library priority configuration item
const std::string kKernelTypeItem = "ImplKernelType";

// Attr unknown_shape
const std::string kAttrNameUnknownShape = "_aicpu_unknown_shape";

// Path based on aicpu_engine directory
const std::string kConfigFilePath =
    "/usr/local/HiAI/runtime/lib64/plugin/opskernel/config/init.conf";

const std::string kTfOpsFileBasedOnEnvPath =
    "/op_impl/built-in/aicpu/tf_kernel/config/tf_kernel.json";

const std::string kTfOpsFileRelativePath =
    "/ops/op_impl/built-in/aicpu/tf_kernel/config/tf_kernel.json";

const std::string kHostCpuOpsFileRelativePath = 
    "/ops/op_impl/built-in/aicpu/aicpu_kernel/config/host_cpu_kernel.json";

const std::string kHostCpuOpsFileBasedOnEnvPath =
    "/op_impl/built-in/aicpu/aicpu_kernel/config/host_cpu_kernel.json";

const std::string kCceOpsFilePath =
    "/usr/local/HiAI/runtime/ops/op_impl/built-in/aicpu/"
    "cce_kernel/config/cce_kernel.json";

const std::string kAicpuOpsFileBasedOnEnvPath =
    "/op_impl/built-in/aicpu/aicpu_kernel/config/aicpu_kernel.json";

const std::string kAicpuOpsFileRelativePath =
    "/ops/op_impl/built-in/aicpu/aicpu_kernel/config/aicpu_kernel.json";

const std::string kIr2TfFilePath =
    "/usr/local/HiAI/runtime/lib64/plugin/opskernel/config/"
    "ir2tf_op_mapping_lib.json";

const std::string kIr2TfFileRelativePath = "config/ir2tf_op_mapping_lib.json";

const std::string kAiCpuOpsParallelRuleFileRelativePath = "plugin/opskernel/config/aicpu_ops_parallel_rule.json";

const std::string kDvppKernelFilePath =
    "/usr/local/HiAI/runtime/lib64/plugin/opskernel/libdvpp_kernels.so";

const std::string kAicpuCustOpsFileRelativePath =
    "/ops/op_impl/custom/cpu/config/cust_aicpu_kernel.json";

const std::string kAicpuCustOpsFileBasedOnEnvPath =
    "/op_impl/custom/cpu/config/cust_aicpu_kernel.json";

// Internal kernelinfo store implement type
const std::string kImplKernelType = "ImplKernelType";

// Min op fusion number
const std::string kOpFusionMinNum = "OpFusionMinNum";

// TF debug mode
const std::string kTfDebugMode = "TfDebugMode";

// Op check mode
const std::string kOpCheckMode = "OpCheckMode";

// Load libcpu_kernels.so in model
const std::string kLoadCpuKernelsInModel = "LoadCpuKernelsInModel";

// default load type for libcpu_kernels.so
constexpr uint64_t kDefaultLoadTypeForCpuKernels = 0;

// Tensorflow kernel info store choice
const std::string kTfKernelInfoChoice = "TFKernel";

// CCE kernel info store choice
const std::string kCceKernelInfoChoice = "CCEKernel";

// AICPU kernel info store choice
const std::string kAicpuKernelInfoChoice = "AICPUKernel";

// CUST AICPU kernel info store choice
const std::string kCustAicpuKernelInfoChoice = "CUSTAICPUKernel";

//HOST CPU kernel info store choice
const std::string kHostCpuKernelInfoChoice = "HOSTCPUKernel";

// Separator in config file item
const std::string kConfigItemSeparator = ",";

// Framework type
const std::string kFrameworkType = "framework_type";

// Workspace reuse flag
const std::string kWorkspaceReuseFlag = "workspace_reuse_flag";

// PlaceHolder Op
const std::string kPlaceholderOp = "PlaceHolder";

// End Op
const std::string kEndOp = "End";

// Tf fused Op
const std::string kFunctionOp = "FunctionOp";

// Tf node def
const std::string kTfNodeDef = "node_def";

// aicpu private
const std::string kAicpuPrivate = "_aicpu_private";

// cust aicpu type false
const std::string kCustAicpuFlag = "_cust_aicpu_flag";

// Aicpu Customized op def
const std::string kCustomizedOpDef = "customized_op_def";

const std::string kAicpuSoLibName = "libaicpu_kernels.so";

// Tf func def
const std::string kTfFuncDef = "func_def";

// Tf op def
const std::string kTfOpDef = "op_def";

// topic type
const std::string kTopicType = "topic_type";

// resource
const std::string kResource = "_resource";

const std::string kResourceQueue = "RES_QUEUE";

const std::string kResourceChannel = "RES_CHANNEL";

const std::string kResourceVdecChannel = "RES_VDEC_CHANNEL";

// async flag
const std::string kAsyncFlag = "async_flag";

// SupportBlockDim flag
const std::string kSupportBlockDim = "_support_blockdim_flag";

// BlockDim by Index
const std::string kBlockDimByIndex = "_blockdim_index";

const std::string kKernelSo = "kernelSo";

// funcName
const std::string kFuncName = "funcName";

// workspaceSize
const std::string kWorkspaceSize = "workspaceSize";

// opKernelLib
const std::string kOpKernelLib = "opKernelLib";

// Tf input data type
const std::string kTfInDataType = "t_in_datatype";

// Tf output data type
const std::string kTfOutDataType = "t_out_datatype";

// Input tensor desc
const std::string kInputTensorDesc = "input_tensor_desc";

// Output tensor desc
const std::string kOutputTensorDesc = "output_tensor_desc";

// ROOT_GRAPH_ID
const std::string kAttrNameRootGraphId = "_root_graph_id";

// env name for dump_ge_graph
const std::string kDumpGeGraph = "DUMP_GE_GRAPH";

const std::string kTfEngine = "DNN_VM_AICPU";

const std::string kAicpuEngine = "DNN_VM_AICPU_ASCEND";

const std::string kHostCpuEngine = "DNN_VM_HOST_CPU";

const std::string kTfOpsKernelInfo = "aicpu_tf_kernel";

const std::string kTfGraphOptimizer = "aicpu_tf_optimizer";

const std::string kTfOpsKernelBuilder = "aicpu_tf_builder";

const std::string kAicpuOpsKernelInfo = "aicpu_ascend_kernel";

const std::string kHostCpuOpsKernelInfo = "DNN_VM_HOST_CPU_OP_STORE";

const std::string kAicpuGraphOptimizer = "aicpu_ascend_optimizer";

const std::string kHostCpuGraphOptimizer = "DNN_VM_HOST_CPU_OPTIMIZER";

const std::string kAicpuOpsKernelBuilder = "aicpu_ascend_builder";

const std::string kHostCpuOpsKernelBuilder = "host_cpu_builder";

const std::string kAutoCastMode = "AutoCastMode";

const std::string kAttrNameThreadScopeId = "_thread_scope_id";

const std::string kAttrNameSgtStruct = "_sgt_struct_info";

const int kEventFftsPlusMsg = 23;
const int kEventHwTsKernelMsg = 3;

const uint32_t kAicpuBlockDim = 1;

const uint32_t kCtxTypeAicpu = 12;

const int kDefaultNum = 0;

const uint32_t kAicpuManualSliceNum = 1;

const std::string kTfKernelSo = "libtf_kernel.so";
const std::string kTfFunctionName = "Run";
const std::string kAttrNameFftsPlusCtxDef = "_ffts_plus_aicpu_ctx_def";
const std::string kCustomizedTailOpDef = "sgt_tail_customized_op_def";
const uint32_t kDefaultAicpuBlockDim = 1;
}  // namespace aicpu

#endif  // AICPU_CONSTANT_H_
