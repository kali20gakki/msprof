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

#ifndef FFTS_ENGINE_INC_FFTS_TYPE_H
#define FFTS_ENGINE_INC_FFTS_TYPE_H

#include <unordered_set>
#include <map>
#include "graph/types.h"

namespace ffts {

enum class CACHE_OPERATION {
  PREFETCH = 0,
  INVALIDATE = 1,
  WRITE_BACK = 2,
  CACHE_OPERATION_BOTTOM = 3
};

enum class ThreadMode {
  MANUAL_THREAD = 0,
  AUTO_THREAD,
  THREAD_MODE_RESERVED
};

enum class FFTSMode {
  FFTS_MODE_NO_FFTS = 0,
  FFTS_MODE_FFTS,
  FFTS_MODE_FFTS_PLUS,
  FFTS_MODE_RESERVED
};

enum class ModeType {
  MANUAL_MODE_TYPE = 0,
  AUTO_MODE_TYPE,
  DYNAMIC_MODE_TYPE,
  MIX_L2_MODE_TYPE
};

enum class TaskBuilderType {
  EN_TASK_TYPE_AIC_AIV = 0,   // ai core op, aic or aiv
  EN_TASK_TYPE_AIC_AIV_AUTO,
  EN_TASK_TYPE_MIX_AIC_AIV,   // mix op, contain aic & aiv
  EN_TASK_TYPE_MIX_AIC_AIV_AUTO,
  EN_TASK_TYPE_AIC_AIV_DYNAMIC,
  EN_TASK_TYPE_MIX_L2_AIC_AIV,
  EN_TASK_TYPE_COLLECTION_COMMICATE,   // collection ops
  EN_TASK_TYPE_AICPU,                  // aicpu ops
  EN_TASK_TYPE_AICPU_AUTO,
  EN_TASK_TYPE_RUNTIME_CONTROL,   // runtime ops
  EN_TASK_TYPE_RESERVED                 // reserved value
};

enum class OpImplType {
  EN_IMPL_CUSTOM_CONSTANT_CCE = 0,   // custom constant op
  EN_IMPL_CUSTOM_TIK,                // custom tik op
  EN_IMPL_CUSTOM_TBE,                // custom tbe op
  EN_IMPL_HW_CONSTANT_CCE,           // Huawei built-in constant op
  EN_IMPL_HW_GENERAL_CCE,            // Huawei built-in cce op
  EN_IMPL_HW_TIK,                    // Huawei built-in tik op
  EN_IMPL_HW_TBE,                    // Huawei built-in tbe op
  EN_IMPL_RL,                        // RL op
  EN_IMPL_PLUGIN_TBE,                // Huawei built-in tbe plugin op
  EN_IMPL_VECTOR_CORE_HW_TBE,        // Huawei built-in tbe op
  EN_IMPL_VECTOR_CORE_CUSTOM_TBE,    // custom tbe op
  EN_IMPL_NON_PERSISTENT_CUSTOM_TBE, // custom tbe op
  EN_RESERVED                        // reserved value
};

void inline SetBitOne(const uint32_t pos, uint32_t &bm) {
  bm |= static_cast<uint32_t>(0x1U << pos);
}

void inline SetBitOne(const uint32_t pos, uint64_t &bm) {
  bm |= static_cast<uint64_t>(0x1UL << pos);
}

extern const uint32_t kManualMode;

extern const uint32_t kAutoMode;

extern const size_t kMaxCacheOperationSize;

extern const std::string kFFTSPlusEngineName;

extern const std::string EM_PARAM;

extern const std::string STR_SOC_VERSION;

extern const std::string EM_INPUT_PARAM_EMPTY;

extern const std::string kThreadScopeId;

extern const std::string kThreadMode;

extern const std::string kTypeFFTSPlus;

extern const std::string kFFTSPlusCoreName;

extern const std::string kDnnVmAICpu;

extern const std::string kDnnVmAICpuAscend;

extern const std::string kDnnVmGeLocal;

extern const std::string kDnnHccl;

extern const std::string kDnnVmRts;

extern const std::string kAIcoreEngineName;

extern const std::string kNoMemReuse;

extern const std::string kNoStreamSplit;

extern const std::string kHcclSubTasks;

extern const std::string kCtxIdList;

extern const std::string kCachePersistSize;

extern const std::string kCachePersist;

extern const std::string kCachePersistScopeIds;

extern const std::string kLxNoReuseMemFlag;

extern const std::string kFFTSMode;

extern const std::string kFFTSPlus;

extern const std::string kFFTS;

extern const std::string kSocInfo;

extern const std::string kAttrCompositeEngineName;

extern const std::string kAttrCompositeKernelLibName;

extern const std::string ATTR_NAME_PARENT_NODE;

extern const std::string kContextId;

extern const std::string kSuccList;

extern const std::string kSuccListList;

extern const std::string kHcclOutDegree0Num;

extern const std::string kDefaultContextId;

extern const std::string kPrefetchEnableBm;

extern const std::string kWriteBackBm;

extern const std::string kInvalidateBm;

extern const std::string kAutoInlabelCtxId;

extern const std::string kAutoCtxIdList;

extern const std::string kAutoAtStartCtxIdList;

extern const std::string kAutoOutlabelCtxId;

extern const std::string kAutoAtEndCtxIdList;

extern const std::string kAutoAtEndPreCnt;

extern const std::string FE_IMPLY_TYPE;

extern const std::string kTypePhonyConcat;

extern const std::string kAttrAICoreCtxDef;

extern const std::string kAttrAICoreCtxType;

extern const std::string kAtomicAddrClean;

extern const char *kCoreTypeAIC;

extern const char *kCoreTypeAIV;

extern const char *kCoreTypeMixAIC;

extern const char *kCoreTypeMixAIV;

extern const std::string kAttrThreadCoreType;

extern const std::string kAdjacencyList;

extern const std::string kModeInArgsFirstField;

extern const std::string ATTR_NAME_ALIAS_ENGINE_NAME;

extern const std::string kFftsFirstOpName;

// mode == 1 indicates we need reserve 8 Bytes for the args beginning
extern const int64_t IS_MIX_FIRST_FIELD_MODE;

extern const size_t kMaxOpNmaLen;

extern const uint32_t kDefaultWindowSize;

extern const uint32_t kGetFirstAvailableLabel;

extern const size_t kMaxPretchNum;

extern const uint32_t kMaxIdx;

extern const uint32_t kMaxPersistNum;

extern const std::map<ge::DataType, uint32_t> DATATYPE_SIZE_MAP;

extern const std::string kCoreTypeHcomReduce;

extern const std::string kCoreTypeHcomAllReduce;

extern const std::string kCoreTypeHcomAllGather;

extern const std::string kCoreTypeHcomBroadCast;

extern const std::string kCoreTypeHcomReduceScatter;

extern const std::string kCoreTypeHcomSend;

extern const std::string kCoreTypeHcomReceive;

extern const std::string kCoreTypeHcomRemoteRead;

extern const std::string kCoreTypeHcomRemoteRefRead;

extern const std::string kCoreTypeHcomRemoteWrite;

extern const std::string kCoreTypeHcomRemoteScatterWrite;

extern const std::string kCoreTypeHcomAllToAllV;

extern const std::string kCoreTypeHcomGatherAllToAllV;

extern const std::unordered_set<std::string> kHCCLOpType;

extern const std::unordered_set<std::string> CONTROL_OP_V2_TYPE;

extern const std::unordered_set<std::string> kWeightTypes;

extern const std::string PLACEHOLDER;

extern const std::string END;

extern const std::string DATA;

extern const std::string NETOUTPUT;

extern const std::string RESHAPE;

extern const std::string VARIABLE;

extern const std::string TRANSDATA;

extern const std::string TRANSDATARNN;

extern const std::string TRANSPOSE;

extern const std::string CAST;

extern const std::string ANN_DATA;

extern const std::string AIPPDATA;

extern const std::string AIPP;

extern const std::string CONSTANTOP;

extern const std::string CONSTANT;

extern const std::string GETNEXT;

extern const std::string CONV2D;

extern const std::string MATMULV2OP;

extern const std::string DEPTHWISECONV2D;

extern const std::string POOLING;

extern const std::string ELTWISE;

extern const std::string RELU;

extern const std::string RELU6;

extern const std::string LEAKY_RELU;

extern const std::string READ_SELECT;

extern const std::string NEXT_ITERATION;

extern const std::unordered_set<int> BUILT_IN_IMPLY_TYPE;

extern const std::string ATTR_NAME_KERNEL_LIST_FIRST_NAME;
}
#endif // FFTS_ENGINE_INC_FFTS_TYPE_H
