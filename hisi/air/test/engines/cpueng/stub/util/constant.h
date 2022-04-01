/**
* Copyright (c) Huawei Technologies Co., Ltd. 2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* Description: constant.h.
*/
#ifndef AICPU_CONSTANT_H_
#define AICPU_CONSTANT_H_

#include <string>

namespace aicpu {

const std::string kOpsFlagClose = "0";

// Operator Library priority configuration item
const std::string kKernelTypeItem = "ImplKernelType";

// Path based on aicpu_engine directory
const std::string kConfigFilePath = "/usr/local/HiAI/runtime/lib64/plugin/opskernel/config/init.conf";

const std::string kInitConfigFileRelativePath = "../../../../test/engines/cpueng/stub/config/init.conf";

//Attr unknown_shape
const std::string kAttrNameUnknownShape = "_aicpu_unknown_shape";

const std::string kTfOpsFileBasedOnEnvPath = "/config/tf_kernel.json";

const std::string kTfOpsFileRelativePath = "/test/engines/cpueng/stub/config/tf_kernel.json";

const std::string kCceOpsFilePath = "/usr/local/HiAI/runtime/ops/op_impl/built-in/aicpu/"
                                      "cce_kernel/config/cce_kernel.json";

const std::string kAicpuOpsFileBasedOnEnvPath = "/jenkins/agent/workspace/ST_Test/air/build/test/engines/cpueng/stub/config/aicpu_kernel.json";

const std::string kAicpuOpsFileRelativePath = "/test/engines/cpueng/stub/config/aicpu_kernel.json";

const std::string kAicpuCustOpsFileRelativePath = "/test/engines/cpueng/stub/config/cust_aicpu_kernel.json";

const std::string kAicpuCustOpsFileBasedOnEnvPath = "/jenkins/agent/workspace/ST_Test/air/build/test/engines/cpueng/stub/config/cust_aicpu_kernel.json";

const std::string kHostCpuOpsFileBasedOnEnvPath = "/jenkins/agent/workspace/ST_Test/air/build/test/engines/cpueng/stub/config/host_cpu_kernel.json";

const std::string kHostCpuOpsFileRelativePath = "/test/engines/cpueng/stub/config/host_cpu_kernel.json";

const std::string kCustomKernelSoRelativePath = "/cpueng/stub/config";

const std::string kIr2tfFilePath = "/usr/local/HiAI/runtime/lib64/plugin/opskernel/config/ir2tf_op_mapping_lib.json";

const std::string kIr2tfFileRelativePath = "config/ir2tf_op_mapping_lib.json";

const std::string kAiCpuOpsParallelRuleFileRelativePath = "../stub/config/aicpu_ops_parallel_rule.json";

const std::string kDvppKernelFilePath = "/usr/local/HiAI/runtime/lib64/plugin/opskernel/libdvpp_kernels.so";

// Internal kernelinfo store implement type
const std::string kImplKernelType = "ImplKernelType";

// Min op fusion number
const std::string kOpFusionMinNum = "OpFusionMinNum";

// TF debug mode
const std::string kTfDebugMode = "TfDebugMode";

// op check mode
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

// HOSTCPU kernel info store choice
const std::string kHostCpuKernelInfoChoice = "HOSTCPUKernel";

// CUST AICPU kernel info store choice
const std::string kCustAicpuKernelInfoChoice = "CUSTAICPUKernel";

// Separator in config file item
const std::string kConfigItemSeparator = ",";

// Framework type
const std::string kFrameworkType = "framework_type";

// Original type
const std::string kOriginalType = "original_type";

// Workspace reuse flag
const std::string kWorkspaceReuseFlag = "workspace_reuse_flag";

// Framework Op
const std::string kFrameworkOp = "FrameworkOp";

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
const std::string kResource = "resource";

// SupportBlockDim flag
const std::string kSupportBlockDim = "_support_blockdim_flag";

// BlockDim by Index
const std::string kBlockDimByIndex = "_blockdim_index";

const std::string kResourceQueue = "RES_QUEUE";

const std::string kResourceChannel = "RES_CHANNEL";

const std::string kResourceVdecChannel = "RES_VDEC_CHANNEL";

//async flag
const std::string kAsyncFlag = "async_flag";

// kernelSo
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
const std::string kInputTensorDesc  = "input_tensor_desc";

// Output tensor desc
const std::string kOutputTensorDesc = "output_tensor_desc";

// env name for dump_ge_graph
const std::string kDumpGeGraph = "DUMP_GE_GRAPH";

// ROOT_GRAPH_ID
const std::string kAttrNameRootGraphId = "_root_graph_id";
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
}

// auto cast mode
const std::string kAutoCastMode = "AutoCastMode";

#endif  // AICPU_CONSTANT_H_