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

#ifndef FUSION_ENGINE_UTILS_COMMON_CONSTANTS_DEFINE_H_
#define FUSION_ENGINE_UTILS_COMMON_CONSTANTS_DEFINE_H_

#include <map>
#include <string>
#include <vector>
#include "common/aicore_util_attr_define.h"
#include "common/aicore_util_types.h"
#include "common/aicore_util_constants.h"

namespace fe {
const std::string ATTR_NAME_L2CACHE_GRAPH_READ_MODE = "_fe_l2cache_graph_read_mode";
const std::string ATTR_NAME_ATOMIC_CLEAN_NODE_PTR = "atomic_clean_node_ptr";
const std::string ATTR_NAME_INPUT_IS_VAR = "INPUT_IS_VAR";
const std::string ATTR_NAME_OUTPUT_IS_VAR = "OUTPUT_IS_VAR";
const std::string ATTR_NAME_NO_REUSE_MEM_FLAG = "no_reuse_mem_flag";
const std::string ATTR_NAME_IS_FIRST_NODE = "is_first_node";
const std::string ATTR_NAME_IS_LAST_NODE = "is_last_node";

// op type
const std::string OP_TYPE_DATA = "Data";
const std::string OP_TYPE_ANN_DATA = "AnnData";
const std::string OP_TYPE_AIPP_DATA = "AippData";
const std::string OP_TYPE_CONST = "Const";
const std::string OP_TYPE_CONSTANT = "Constant";
const std::string OP_TYPE_VARIABLE = "Variable";

const std::string OP_TYPE_ROIPOOLING = "ROIPooling";
const std::string OP_TYPE_SSD_DETECTION_OUTPUT = "SSDDetectionOutput";

const std::set<std::string> LIFECYCLE_IS_END_OPS = {OP_TYPE_DATA, OP_TYPE_AIPP_DATA, OP_TYPE_ANN_DATA,
                                                    OP_TYPE_CONST, OP_TYPE_CONSTANT};
const std::set<std::string> LIFECYCLE_IS_NOT_END_OPS = {OP_TYPE_VARIABLE};
// config key in fe.ini
const std::string CONFIG_KEY_ROOT = "rootdir";
const std::string CONFIG_KEY_CUSTOM_PASS_FILE = "fusionpassmgr.aicore.custompasspath";
const std::string CONFIG_KEY_BUILTIN_PASS_FILE = "fusionpassmgr.aicore.graphpasspath";
const std::string CONFIG_KEY_GRAPH_FILE = "fusionrulemgr.aicore.graphfilepath";
const std::string CONFIG_KEY_CUSTOM_FILE = "fusionrulemgr.aicore.customfilepath";
const std::string CONFIG_KEY_ORIGINAL_NODES_ENABLE = "dump.originalnodes.enable";
const std::string CONFIG_KEY_MIX_L2_ENABLE = "mix_l2.enable";
const std::string CONFIG_KEY_MAY_DUPLICATE_IN_AUTO_FUSION = "may.duplicate.in.autofusion";
const std::string CONFIG_KEY_SGT_SLICE = "sgtslice";
const std::string VECTOR_CORE_CONFIG_KEY_CUSTOM_PASS_FILE = "fusionpassmgr.vectorcore.custompasspath";
const std::string VECTOR_CORE_CONFIG_KEY_BUILTIN_PASS_FILE = "fusionpassmgr.vectorcore.graphpasspath";
const std::string VECTOR_CORE_CONFIG_KEY_GRAPH_FILE = "fusionrulemgr.vectorcore.graphfilepath";
const std::string VECTOR_CORE_CONFIG_KEY_CUSTOM_FILE = "fusionrulemgr.vectorcore.customfilepath";
const std::string DSA_CORE_CONFIG_KEY_CUSTOM_PASS_FILE = "fusionpassmgr.dsacore.custompasspath";
const std::string DSA_CORE_CONFIG_KEY_BUILTIN_PASS_FILE = "fusionpassmgr.dsacore.graphpasspath";
const std::string DSA_CORE_CONFIG_KEY_GRAPH_FILE = "fusionrulemgr.dsacore.graphfilepath";
const std::string DSA_CORE_CONFIG_KEY_CUSTOM_FILE = "fusionrulemgr.dsacore.customfilepath";
const std::string FUSION_CONFIG_BUILT_IN_FILE = "fusion.config.built-in.file";

// constants value
const int32_t PRIORITY_START_NUM = 0;
const uint32_t MAX_L2_DATANUM = 8;
const int32_t OP_STORE_FORMAT_SIZE = 6;  // The size of opstore config items in fe.ini file

const int32_t DATA_VISIT_DIST_THRESHOLD = 5; // data distance threshlod for rc cache optimization
const int32_t MEM_REUSE_DIST_THRESHOLD = 2; // mem reuse distance threshold
enum class L2CacheReadMode {
  RM_NONE = -1,
  READ_LAST = 1,
  READ_INVALID = 2,
  NOT_NEED_WRITEBACK = 3
};

const std::map<L2CacheReadMode, std::string> L2CACHE_READ_MODE_STRING_MAP{
  {L2CacheReadMode::RM_NONE, "None"},
  {L2CacheReadMode::READ_LAST, "Read Last"},
  {L2CacheReadMode::READ_INVALID, "Read Invalid"},
  {L2CacheReadMode::NOT_NEED_WRITEBACK, "Not Need Writeback"}
};

// The index of opstre config item in fe.ini file
enum OpStoreInfoIndex {
    IDX_PRIORITY = 0,     // priority index
    IDX_OPIMPL_TYPE,      // op_impl_type index
    IDX_CFG_FILEPATH,     // cfg_file_path index
    IDX_OPIMPL_FILEPATH,  // op_impl_file_path index
    IDX_NEED_PRECOMPILE,  // need precompile
    IDX_NEED_COMPILE,      // need compile
    IDX_BOTTOM
};

const std::map<std::string, bool> GE_SWITCH_MAP{ {"1", true}, {"0", false} };
const std::map<std::string, bool> GE_DISABLE_REUSE_MEMORY_MAP{{"1", false}, {"0", true}};
const std::map<std::string, AutoTuneMode> AUTO_TUNE_MODE_MAP {
    {"GA", TUNE_MODE_AUTO_TUNE},
    {"RL", TUNE_MODE_RL_TUNE},
    {"GA|RL" ,TUNE_MODE_AUTO_AND_RL_TUNE},
    {"GA,RL" ,TUNE_MODE_AUTO_AND_RL_TUNE},
    {"RL|GA" ,TUNE_MODE_AUTO_AND_RL_TUNE},
    {"RL,GA" ,TUNE_MODE_AUTO_AND_RL_TUNE}
};
/* List of all precision mode */
const std::vector<std::string> PRECISION_MODE_LIST = {ALLOW_MIX_PRECISION, FORCE_FP16, FORCE_FP32,
    ALLOW_FP32_TO_FP16, MUST_KEEP_ORIGIN_DTYPE, FORCE_BF16, FORCE_LOWERPRECISION, ALLOW_FP32_TO_BF16,
    ALLOW_FP32_TO_LOWPRECISION, ALLOW_MIX_PRECISION_FP16, ALLOW_MIX_PRECISION_BF16, ALLOW_FP32_TO_LOWPRECISION};

const char ASCEND_OPP_PATH[] = "ASCEND_OPP_PATH";
const std::string OP_STOREINFO_PREFIX = "op.store.";
const std::string NEED_PRECOMPILE_TRUE = "true";
const std::string NEED_COMPILE_TRUE = "true";
const std::map<std::string, BufferOptimize> BUFFER_OPTIMIZE_MAP{
    {OFF_OPTIMIZE, EN_OFF_OPTIMIZE}, {L1_OPTIMIZE, EN_L1_OPTIMIZE}, {L2_OPTIMIZE, EN_L2_OPTIMIZE}
};

const std::map<BufferOptimize, std::string> BUFFER_OPTIMIZE_STR_MAP{
    {EN_OFF_OPTIMIZE, OFF_OPTIMIZE}, {EN_L1_OPTIMIZE, L1_OPTIMIZE}, {EN_L2_OPTIMIZE, L2_OPTIMIZE}
};

const std::string BUFFER_OPTIMIZE_UNKNOWN = "unknown-buffer-optimize";
const std::string BUFFER_FUSION_MODE_UNKNOWN = "unknown-buffer-fusion-mode";

const std::map<BufferOptimize, std::string> BUFFER_OPTIMIZE_STRING_MAP{
    {EN_UNKNOWN_OPTIMIZE, "unknown_optimize"},
    {EN_OFF_OPTIMIZE, "off_optimize"},
    {EN_L1_OPTIMIZE, "l1_optimize"},
    {EN_L2_OPTIMIZE, "l2_optimize"}
};

const std::map<BufferFusionMode, std::string> BUFFER_FUSION_MODE_STRING_MAP{
    {EN_OPTIMIZE_DISABLE, "optimize_disable"},
    {EN_L2_BUFFER, "l2_buffer"},
    {EN_L2_FUSION, "l2_fusion"},
};

const std::map<AppendArgsMode, std::string> APPEND_ARGS_MODE_STRING_MAP{
    {AppendArgsMode::NO_ARGS, "no-append-args"},
    {AppendArgsMode::L2_BUFFER_ARGS, "L2-buffer-append-args"},
    {AppendArgsMode::L2_CACHE_ARGS, "L2-cache-append-args"}
};

const std::map<OpPattern, std::string> OP_PATTERN_STRING_MAP{{OP_PATTERN_OP_KERNEL, "kernel"},
                                                             {OP_PATTERN_FORMAT_AGNOSTIC, "formatAgnostic"},
                                                             {OP_PATTERN_BROADCAST, "broadcast"},
                                                             {OP_PATTERN_REDUCE, "reduce"},
                                                             {OP_PATTERN_OP_CUSTOMIZE, "dynamic"}};

const std::map<PrecisionPolicy, std::string> PRECISION_POLICY_STRING_MAP{
    {WHITE, "white-list"}, {BLACK, "black-list"}, {GRAY, "gray-list"}
};

const std::map<std::string, ge::DataType> STR_DTYPE_MAP{{"float", ge::DT_FLOAT},
                                                               {"float32", ge::DT_FLOAT},
                                                               {"float16", ge::DT_FLOAT16},
                                                               {"int8", ge::DT_INT8},
                                                               {"int16", ge::DT_INT16},
                                                               {"int32", ge::DT_INT32},
                                                               {"int64", ge::DT_INT64},
                                                               {"uint8", ge::DT_UINT8},
                                                               {"uint16", ge::DT_UINT16},
                                                               {"uint32", ge::DT_UINT32},
                                                               {"uint64", ge::DT_UINT64},
                                                               {"bool", ge::DT_BOOL},
                                                               {"double", ge::DT_DOUBLE},
                                                               {"dual", ge::DT_DUAL},
                                                               {"dual_sub_int8", ge::DT_DUAL_SUB_INT8},
                                                               {"dual_sub_uint8", ge::DT_DUAL_SUB_UINT8},
                                                               {"int4", ge::DT_INT4},
                                                               {"bfloat16", ge::DT_BF16}};
// The config keys below in the configuration file is mandatory.
const std::vector<std::string> MANDATORY_CONFIG_ITEMS{};

// The default value for the keys which is not configured in the files.
const std::map<std::string, std::string> DEFAULT_CONFIG_ITEM_VALUE{};

// maps soc_version to ISA arch VERSION
const std::map<std::string, ISAArchVersion> SOC_ISA_ARCH_VERSION_MAP{
    {SOC_VERSION_ASCEND610, ISAArchVersion::EN_ISA_ARCH_V200},
    {SOC_VERSION_ASCEND615, ISAArchVersion::EN_ISA_ARCH_V200},
    {SOC_VERSION_ASCEND710, ISAArchVersion::EN_ISA_ARCH_V200},
    {SOC_VERSION_ASCEND710P, ISAArchVersion::EN_ISA_ARCH_V200},
    {SOC_VERSION_ASCEND310, ISAArchVersion::EN_ISA_ARCH_V100},
    {SOC_VERSION_ASCEND910PREMIUMA, ISAArchVersion::EN_ISA_ARCH_V100},
    {SOC_VERSION_ASCEND910A, ISAArchVersion::EN_ISA_ARCH_V100},
    {SOC_VERSION_ASCEND910B, ISAArchVersion::EN_ISA_ARCH_V100},
    {SOC_VERSION_ASCEND910PROA, ISAArchVersion::EN_ISA_ARCH_V100},
    {SOC_VERSION_ASCEND910PROB, ISAArchVersion::EN_ISA_ARCH_V100},
    {SOC_VERSION_HI3796CV300ES, ISAArchVersion::EN_ISA_ARCH_V100},
    {SOC_VERSION_HI3796CV300CS, ISAArchVersion::EN_ISA_ARCH_V200},
    {SOC_VERSION_ASCEND920A, ISAArchVersion::EN_ISA_ARCH_V210},
    {SOC_VERSION_SD3403, ISAArchVersion::EN_ISA_ARCH_V200},
    {SOC_VERSION_ASCEND320, ISAArchVersion::EN_ISA_ARCH_V300}};

constexpr const char* OPS_SUBPATH_ASCEND910 = "ascend910";
constexpr const char* OPS_SUBPATH_ASCEND920 = "ascend920";
constexpr const char* OPS_SUBPATH_ASCEND710 = "ascend710";

const std::map<std::string, std::string> SOC_OPS_SUBPATH_MAP {
    {SOC_VERSION_ASCEND910A, OPS_SUBPATH_ASCEND910}, {SOC_VERSION_ASCEND910B, OPS_SUBPATH_ASCEND910},
    {SOC_VERSION_ASCEND910PROA, OPS_SUBPATH_ASCEND910}, {SOC_VERSION_ASCEND910PROB, OPS_SUBPATH_ASCEND910},
    {SOC_VERSION_ASCEND910PREMIUMA, OPS_SUBPATH_ASCEND910}, {SOC_VERSION_ASCEND920A, OPS_SUBPATH_ASCEND920},
    {SOC_VERSION_ASCEND710P, OPS_SUBPATH_ASCEND710}, {SOC_VERSION_ASCEND710VIR01, OPS_SUBPATH_ASCEND710},
    {SOC_VERSION_ASCEND710VIR02, OPS_SUBPATH_ASCEND710}, {SOC_VERSION_ASCEND710VIR04, OPS_SUBPATH_ASCEND710},
    {SOC_VERSION_ASCEND710VIR08, OPS_SUBPATH_ASCEND710}};
}
#endif  // FUSION_ENGINE_UTILS_COMMON_CONSTANTS_DEFINE_H_
