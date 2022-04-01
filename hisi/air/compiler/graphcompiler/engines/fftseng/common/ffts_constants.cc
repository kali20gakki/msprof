/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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
#include <unordered_set>
#include <map>
#include "graph/types.h"
#include "inc/ffts_type.h"

namespace ffts {
const uint32_t kManualMode = 0;

const uint32_t kAutoMode = 1;

const size_t kMaxCacheOperationSize = 64;

const std::string kFFTSPlusEngineName = "ffts_plus";

const std::string EM_PARAM = "parameter";

const std::string STR_SOC_VERSION = "soc_version";

const std::string EM_INPUT_PARAM_EMPTY = "E10004";

const std::string kThreadScopeId = "_thread_scope_id";

const std::string kThreadMode = "_thread_mode";

const std::string kTypeFFTSPlus = "_ffts_plus";

const std::string kDnnVmAICpu = "DNN_VM_AICPU";

const std::string kDnnVmAICpuAscend = "DNN_VM_AICPU_ASCEND";

const std::string kDnnVmGeLocal = "DNN_VM_GE_LOCAL";

const std::string kDnnHccl = "DNN_HCCL";

const std::string kDnnVmRts = "DNN_VM_RTS";

const std::string kAIcoreEngineName = "AIcoreEngine";

const std::string kNoMemReuse = "_no_mem_reuse";

const std::string kNoStreamSplit = "_no_stream_split";

const std::string kHcclSubTasks = "hccl_sub_tasks";

const std::string kCtxIdList = "ctx_id_list";

const std::string kCachePersistSize = "_cache_persist_size";

const std::string kCachePersist = "_cache_persist";

const std::string kCachePersistScopeIds = "_cache_persist_scope_ids";

const std::string kLxNoReuseMemFlag = "no_reuse_mem_flag";

const std::string kFFTSMode = "ffts_mode";

const std::string kFFTSPlus = "ffts-plus";

const std::string kFFTS = "ffts";

const std::string kSocInfo = "SoCInfo";

const std::string kAttrCompositeEngineName = "_composite_engine_name";

const std::string kAttrCompositeKernelLibName = "_composite_engine_kernel_lib_name";

const std::string ATTR_NAME_PARENT_NODE = "parentNode";

const std::string kContextId = "_context_id";

const std::string kSuccList = "_succ_list";

const std::string kSuccListList = "_succ_list_list";

const std::string kHcclOutDegree0Num = "_hccl_list_out_degree_0_num";

const std::string kDefaultContextId = "_default_context_id";

const std::string kPrefetchEnableBm = "_prefetch_enable_bm";

const std::string kWriteBackBm = "_write_back_bm";

const std::string kInvalidateBm = "_invalidate_bm";

const std::string kAutoInlabelCtxId = "_in_label_ctx_id";

const std::string kAutoCtxIdList = "_context_id_list";

const std::string kAutoAtStartCtxIdList = "_at_start_ctx_id_list";

const std::string kAutoOutlabelCtxId = "_out_label_ctx_id";

const std::string kAutoAtEndCtxIdList = "_at_end_ctx_id_list";

const std::string kAutoAtEndPreCnt = "_at_end_pre_cnt";

const std::string FE_IMPLY_TYPE = "_fe_imply_type";

const std::string kTypePhonyConcat = "PhonyConcat";

const std::string kAttrAICoreCtxDef = "_aicore_ctx_def";

const std::string kAttrAICoreCtxType = "_aicore_ctx_type";

const std::string kAtomicAddrClean = "AtomicAddrClean";

const char *kCoreTypeAIC = "AIC";

const char *kCoreTypeAIV = "AIV";

const char *kCoreTypeMixAIC = "MIX_AIC";

const char *kCoreTypeMixAIV = "MIX_AIV";

const std::string kAttrThreadCoreType = "_thread_cube_vector_core_type";

const std::string kAdjacencyList = "adjacency_list";

const std::string kModeInArgsFirstField = "_mode_in_args_first_field";

const std::string  ATTR_NAME_ALIAS_ENGINE_NAME = "_alias_engine_name";

const std::string  kFftsFirstOpName = "_ffts_first_op_name";

// mode == 1 indicates we need reserve 8 Bytes for the args beginning
const int64_t IS_MIX_FIRST_FIELD_MODE = 1;

const size_t kMaxOpNmaLen = 512;

const uint32_t kGetFirstAvailableLabel = 1000;

const size_t kMaxPretchNum = 4;

const uint32_t kMaxIdx = 64;

const uint32_t kMaxPersistNum = 8;

const uint32_t kDefaultWindowSize = 4;

const std::map<ge::DataType, uint32_t> DATATYPE_SIZE_MAP {
        {ge::DT_FLOAT, sizeof(float)},
        {ge::DT_FLOAT16, sizeof(int16_t)},
        {ge::DT_BF16, sizeof(int16_t)},
        {ge::DT_INT8, sizeof(int8_t)},
        {ge::DT_INT32, sizeof(int32_t)},
        {ge::DT_UINT8, sizeof(uint8_t)},
        {ge::DT_UINT32, sizeof(uint32_t)},
        {ge::DT_INT16, sizeof(int16_t)},
        {ge::DT_UINT16, sizeof(uint16_t)},
        {ge::DT_INT64, sizeof(int64_t)},
        {ge::DT_UINT64, sizeof(uint64_t)},
        {ge::DT_DOUBLE, sizeof(double)},
        {ge::DT_BOOL, sizeof(bool)},
        {ge::DT_DUAL, sizeof(float) + sizeof(int8_t)},
        {ge::DT_DUAL_SUB_UINT8, sizeof(int8_t)},
        {ge::DT_DUAL_SUB_INT8, sizeof(int8_t)}
};

const std::string kCoreTypeHcomReduce = "HcomReduce";
const std::string kCoreTypeHcomAllReduce = "HcomAllReduce";
const std::string kCoreTypeHcomAllGather = "HcomAllGather";
const std::string kCoreTypeHcomBroadCast = "HcomBroadcast";
const std::string kCoreTypeHcomReduceScatter = "HcomReduceScatter";
const std::string kCoreTypeHcomSend = "HcomSend";
const std::string kCoreTypeHcomReceive = "HcomReceive";
const std::string kCoreTypeHcomRemoteRead = "HcomRemoteRead";
const std::string kCoreTypeHcomRemoteRefRead = "HcomRemoteRefRead";
const std::string kCoreTypeHcomRemoteWrite = "HcomRemoteWrite";
const std::string kCoreTypeHcomRemoteScatterWrite = "HcomRemoteScatterWrite";
const std::string kCoreTypeHcomAllToAllV = "HcomAllToAllV";
const std::string kCoreTypeHcomGatherAllToAllV = "HcomGatherAllToAllV";
const std::unordered_set<std::string> kHCCLOpType = {kCoreTypeHcomReduce, kCoreTypeHcomAllReduce,
                                                     kCoreTypeHcomAllGather, kCoreTypeHcomBroadCast,
                                                     kCoreTypeHcomReduceScatter, kCoreTypeHcomSend,
                                                     kCoreTypeHcomReceive, kCoreTypeHcomRemoteRead,
                                                     kCoreTypeHcomRemoteRefRead, kCoreTypeHcomRemoteWrite,
                                                     kCoreTypeHcomRemoteScatterWrite, kCoreTypeHcomAllToAllV,
                                                     kCoreTypeHcomGatherAllToAllV};

const std::unordered_set<std::string> CONTROL_OP_V2_TYPE = {"If", "While", "Case"};

const std::unordered_set<std::string> kWeightTypes = {"Const", "Constant", "Variable"};

const std::string PLACEHOLDER = "PlaceHolder";

const std::string END = "End";

const std::string DATA = "Data";

const std::string NETOUTPUT = "NetOutput";

const std::string RESHAPE = "Reshape";

const std::string VARIABLE = "Variable";

const std::string TRANSDATA = "TransData";

const std::string TRANSDATARNN = "TransDataRNN";

const std::string TRANSPOSE = "TransposeD";

const std::string CAST = "Cast";

const std::string ANN_DATA = "AnnData";

const std::string AIPPDATA = "AippData";

const std::string AIPP = "Aipp";

const std::string CONSTANTOP = "Constant";

const std::string CONSTANT = "Const";

const std::string GETNEXT = "GetNext";

const std::string CONV2D = "Conv2D";

const std::string MATMULV2OP = "MatMulV2";

const std::string DEPTHWISECONV2D = "DepthwiseConv2D";

const std::string POOLING = "Pooling";

const std::string ELTWISE = "Eltwise";

const std::string RELU = "Relu";

const std::string RELU6 = "Relu6";

const std::string LEAKY_RELU = "LeakyRelu";

const std::string READ_SELECT = "ReadSelect";

const std::string NEXT_ITERATION = "NextIteration";

const std::unordered_set<int> BUILT_IN_IMPLY_TYPE {
        static_cast<int>(OpImplType::EN_IMPL_HW_CONSTANT_CCE), static_cast<int>(OpImplType::EN_IMPL_HW_GENERAL_CCE),
        static_cast<int>(OpImplType::EN_IMPL_HW_TIK), static_cast<int>(OpImplType::EN_IMPL_HW_TBE),
        static_cast<int>(OpImplType::EN_IMPL_RL), static_cast<int>(OpImplType::EN_IMPL_PLUGIN_TBE),
        static_cast<int>(OpImplType::EN_IMPL_VECTOR_CORE_HW_TBE)
};

const std::string ATTR_NAME_KERNEL_LIST_FIRST_NAME = "_kernel_list_first_name";
}  // namespace ffts
