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

#ifndef FUSION_ENGINE_OPTIMIZER_COMMON_UTIL_OP_INFO_UTIL_H_
#define FUSION_ENGINE_OPTIMIZER_COMMON_UTIL_OP_INFO_UTIL_H_

#include <fusion_rule_manager/fusion_rule_data/fusion_rule_pattern.h>
#include <map>
#include <string>
#include <vector>
#include "common/aicore_util_types.h"
#include "common/util/constants.h"
#include "graph/types.h"
#include "tensor_engine/fusion_api.h"
#include "register/graph_optimizer/buffer_fusion/buffer_fusion_pass_base.h"
#include "register/graph_optimizer/fusion_common/pattern_fusion_base_pass.h"
#include "register/graph_optimizer/graph_fusion/graph_fusion_pass_base.h"

namespace fe {
const std::vector<OpImplType> TBE_OP_IMPL_TYPE_VEC{EN_IMPL_CUSTOM_TBE, EN_IMPL_HW_TBE, EN_IMPL_PLUGIN_TBE,
                                                          EN_IMPL_NON_PERSISTENT_CUSTOM_TBE};

const std::map<OpImplType, domi::ImplyType> IMPL_TYPE_MAP{
    {EN_IMPL_CUSTOM_TIK, domi::ImplyType::BUILDIN},
    {EN_IMPL_CUSTOM_TBE, domi::ImplyType::TVM},
    {EN_IMPL_HW_TIK, domi::ImplyType::BUILDIN},
    {EN_IMPL_HW_TBE, domi::ImplyType::TVM},
    {EN_IMPL_RL, domi::ImplyType::BUILDIN},
    {EN_IMPL_PLUGIN_TBE, domi::ImplyType::TVM},
    {EN_IMPL_VECTOR_CORE_HW_TBE, domi::ImplyType::TVM},
    {EN_IMPL_VECTOR_CORE_CUSTOM_TBE, domi::ImplyType::TVM},
    {EN_IMPL_NON_PERSISTENT_CUSTOM_TBE, domi::ImplyType::TVM},
    {EN_IMPL_HW_DSA, domi::ImplyType::BUILDIN}};

const std::map<ge::Format, std::string> GE_FORMAT_STRING_MAP{
    {ge::FORMAT_NCHW, "NCHW"},
    {ge::FORMAT_NHWC, "NHWC"},
    {ge::FORMAT_ND, "ND"},
    {ge::FORMAT_NC1HWC0, "NC1HWC0"},
    {ge::FORMAT_FRACTAL_Z, "FRACTAL_Z"},
    {ge::FORMAT_NC1C0HWPAD, "NC1C0HWPAD"},
    {ge::FORMAT_NHWC1C0, "NHWC1C0"},
    {ge::FORMAT_FSR_NCHW, "FSR_NCHW"},
    {ge::FORMAT_FRACTAL_DECONV, "FRACTAL_DECONV"},
    {ge::FORMAT_C1HWNC0, "C1HWNC0"},
    {ge::FORMAT_FRACTAL_DECONV_TRANSPOSE, "FRACTAL_DECONV_TRANSPOSE"},
    {ge::FORMAT_FRACTAL_DECONV_SP_STRIDE_TRANS, "FRACTAL_DECONV_SP_STRIDE_TRANS"},
    {ge::FORMAT_NC1HWC0_C04, "NC1HWC0_C04"},
    {ge::FORMAT_FRACTAL_Z_C04, "FRACTAL_Z_C04"},
    {ge::FORMAT_CHWN, "CHWN"},
    {ge::FORMAT_FRACTAL_DECONV_SP_STRIDE8_TRANS, "DECONV_SP_STRIDE8_TRANS"},
    {ge::FORMAT_NC1KHKWHWC0, "NC1KHKWHWC0"},
    {ge::FORMAT_BN_WEIGHT, "BN_WEIGHT"},
    {ge::FORMAT_FILTER_HWCK, "FILTER_HWCK"},
    {ge::FORMAT_HWCN, "HWCN"},
    {ge::FORMAT_HASHTABLE_LOOKUP_LOOKUPS, "LOOKUP_LOOKUPS"},
    {ge::FORMAT_HASHTABLE_LOOKUP_KEYS, "LOOKUP_KEYS"},
    {ge::FORMAT_HASHTABLE_LOOKUP_VALUE, "LOOKUP_VALUE"},
    {ge::FORMAT_HASHTABLE_LOOKUP_OUTPUT, "LOOKUP_OUTPUT"},
    {ge::FORMAT_HASHTABLE_LOOKUP_HITS, "LOOKUP_HITS"},
    {ge::FORMAT_MD, "MD"},
    {ge::FORMAT_NDHWC, "NDHWC"},
    {ge::FORMAT_NCDHW, "NCDHW"},
    {ge::FORMAT_DHWCN, "DHWCN"},
    {ge::FORMAT_DHWNC, "DHWNC"},
    {ge::FORMAT_NDC1HWC0, "NDC1HWC0"},
    {ge::FORMAT_FRACTAL_Z_3D, "FRACTAL_Z_3D"},
    {ge::FORMAT_FRACTAL_Z_3D_TRANSPOSE, "FRACTAL_Z_3D_TRANSPOSE"},
    {ge::FORMAT_C1HWNCoC0, "C1HWNCoC0"},
    {ge::FORMAT_FRACTAL_NZ, "FRACTAL_NZ"},
    {ge::FORMAT_CN, "CN"},
    {ge::FORMAT_NC, "NC"},
    {ge::FORMAT_FRACTAL_ZN_LSTM, "FRACTAL_ZN_LSTM"},
    {ge::FORMAT_RESERVED, "FORMAT_RESERVED"},
    {ge::FORMAT_ND_RNN_BIAS, "ND_RNN_BIAS"},
    {ge::FORMAT_FRACTAL_ZN_RNN, "FRACTAL_ZN_RNN"},
    {ge::FORMAT_ALL, "ALL"}};

const std::map<domi::ImplyType, std::string> GE_IMPL_TYPE_STRING_MAP{
    {domi::ImplyType::BUILDIN, "BUILTIN"}, {domi::ImplyType::TVM, "TVM"},         {domi::ImplyType::CUSTOM, "CUSTOM"},
    {domi::ImplyType::AI_CPU, "AI_CPU"},   {domi::ImplyType::INVALID, "INVALID"},
};

const std::map<GraphFusionPassType, std::string> PASS_TYPE_STRING_MAP{
    {BUILT_IN_GRAPH_PASS, "built-in-ai-core-graph-pass"},
    {CUSTOM_AI_CORE_GRAPH_PASS, "custom-ai-core-graph-pass"},
    {BUILT_IN_VECTOR_CORE_GRAPH_PASS, "built-in-vector-core-graph-pass"},
    {CUSTOM_VECTOR_CORE_GRAPH_PASS, "custom-vector-core-graph-pass"},
    {SECOND_ROUND_BUILT_IN_GRAPH_PASS, "second-round-graph-pass"},
    {BUILT_IN_BEFORE_TRANSNODE_INSERTION_GRAPH_PASS, "built-in-before-transnode-insertion-graph-pass"},
    {BUILT_IN_PREPARE_GRAPH_PASS, "built-in-prepare-graph-pass"},
    {BUILT_IN_BEFORE_QUANT_OPTIMIZATION_GRAPH_PASS, "built-in-before-quant-optimization-graph-pass"},
};

const std::map<BufferFusionPassType, std::string> BUFFER_FUSION_PASS_TYPE_STRING_MAP{
    {BUILT_IN_AI_CORE_BUFFER_FUSION_PASS, "build-in-ai-core-buffer_fusion-pass"},
    {BUILT_IN_VECTOR_CORE_BUFFER_FUSION_PASS, "build-in-vector-core-buffer_fusion-pass"},
    {CUSTOM_AI_CORE_BUFFER_FUSION_PASS, "custom-ai-core-buffer_fusion-pass"},
    {CUSTOM_VECTOR_CORE_BUFFER_FUSION_PASS, "custom-vector-core-buffer_fusion-pass"},
};

const std::map<RuleType, std::string> RULE_TYPE_STRING_MAP{
    {RuleType::BUILT_IN_GRAPH_RULE, "built-in-graph-rule"}, {RuleType::CUSTOM_GRAPH_RULE, "custom-graph-rule"},
};

/* record the classification information utilized by CheckDtypeSupported;
 * If the value gap between two data type is less than HIGH_GAP(10),
 * this two data type is under same classification.
 * For example, the value gap between float and float16 is 1,
 * so float and float16 are same classification.
 */
const int32_t LOW_GAP = 0;
const int32_t HIGH_GAP = 10;
const std::map<ge::DataType, int32_t> DATATYPE_PRIORITY_MAP{
    {ge::DT_DOUBLE, 0},  {ge::DT_FLOAT, 1},   {ge::DT_BF16, 2},     {ge::DT_FLOAT16, 3},
    {ge::DT_INT64, 20},  {ge::DT_INT32, 21},  {ge::DT_INT16, 22},   {ge::DT_INT8, 23},
    {ge::DT_INT4, 24},   {ge::DT_UINT64, 40}, {ge::DT_UINT32, 41},  {ge::DT_UINT16, 42},
    {ge::DT_UINT8, 43},  {ge::DT_BOOL, 60}};

const int32_t LOW_GAP_AMPLIFIED = 0;
const int32_t HIGH_GAP_AMPLIFIED = 1000000;
const std::map<ge::DataType, int32_t> DATATYPE_PRIORITY_MAP_AMPLIFIED{
    {ge::DT_DOUBLE, 0},       {ge::DT_FLOAT, 100000},   {ge::DT_BF16, 200000},     {ge::DT_FLOAT16, 300000},
    {ge::DT_INT64, 2000000},  {ge::DT_INT32, 2100000},  {ge::DT_INT16, 2200000},  {ge::DT_INT8, 2300000},
    {ge::DT_INT4, 2400000},   {ge::DT_UINT64, 4000000}, {ge::DT_UINT32, 4100000}, {ge::DT_UINT16, 4200000},
    {ge::DT_UINT8, 4300000},  {ge::DT_BOOL, 6000000}};

enum class ForbiddenDtype { FORBIDDEN_NONE = 0, FORBIDDEN_BF16, FORBIDDEN_FP16, FORBIDDEN_DOUBLE };

enum class CheckSupportMode { DTYPE_MODE = 0, DTYPE_FORMAT_MODE, ACCURACY_MODE };

enum class OpNotSupportedReasonID {
  EN_REASON_ID_START = 0,
  EN_TYPE_NOT_FOUND,
  EN_INPUTS_NUM_NOT_MATCH,
  EN_OUTPUTS_NUM_NOT_MATCH,
  EN_PRE_COMPILE_FUNC_IS_NULL,
  EN_INPUTS_NOT_SUPPORT,
  EN_OUTPUTS_NOT_SUPPORT,
  EN_ATTRS_NOT_SUPPORT,
  EN_PARAM_TYPE_NOT_SUPPORT,
  EN_OPERATOR_NOT_SUPPORT,
  EN_INPUTS_AND_OUTPUTS_NOT_ACCURACY_SUPPORT,
  EN_TENSOR_SHAPE_CONTAINS_UNKNOWN_DIM,
  EN_NOT_SUPPORT_DYNAMIC_SHAPE,
  EN_REASON_ID_RESERVED
};

const std::map<OpNotSupportedReasonID, std::string> ID_REASON_MAP{
    {OpNotSupportedReasonID::EN_TYPE_NOT_FOUND,
     "The type of this op is not found in op store, check "
     "whether the op store has this type of op."},
    {OpNotSupportedReasonID::EN_INPUTS_NUM_NOT_MATCH,
     "The number of inputs in op desc and op store "
     "does not match, check whether the number of "
     "input from the op in the op store and the graph "
     "is consistent."},
    {OpNotSupportedReasonID::EN_OUTPUTS_NUM_NOT_MATCH,
     "The number of outputs in op desc and op store "
     "does not match, check whether the number of "
     "output from the op in the op store and the "
     "graph is consistent."},
    {OpNotSupportedReasonID::EN_PRE_COMPILE_FUNC_IS_NULL,
     "The pre_compile_func is nullptr, check "
     "whether this type of op does not exist in "
     "tbe-builtin and then go to the tbe-plugin."},
    {OpNotSupportedReasonID::EN_INPUTS_NOT_SUPPORT,
     "The dtype, format or shape of input in op desc is "
     "not supported in op store, check the dtype, "
     "format or shape of input between the op store and "
     "the graph."},
    {OpNotSupportedReasonID::EN_OUTPUTS_NOT_SUPPORT,
     "The dtype, format or shape of output in op desc "
     "is not supported in op store, check the dtype, "
     "format or shape of output between the op store "
     "and the graph."},
    {OpNotSupportedReasonID::EN_ATTRS_NOT_SUPPORT,
     "The number of attrs in op desc and op store does "
     "not match, check whether the attrs in op store are "
     "all in graph."},
    {OpNotSupportedReasonID::EN_PARAM_TYPE_NOT_SUPPORT,
     "The inputs or outputs of op desc do not meet "
     "the paramtype requirements in op store, check "
     "the paramtype in op store and the number of "
     "inputs or outputs in graph."},
    {OpNotSupportedReasonID::EN_OPERATOR_NOT_SUPPORT,
     "The op desc is not supported in the operator "
     "implementation, consult the operator developer "
     "for details."},
    {OpNotSupportedReasonID::EN_INPUTS_AND_OUTPUTS_NOT_ACCURACY_SUPPORT,
     "The inputs and outputs of op desc is not accuracy supported in op store, "
     "check the dtype, format or shape of inputs or outputs between the op "
     "store and the graph."},
    {OpNotSupportedReasonID::EN_TENSOR_SHAPE_CONTAINS_UNKNOWN_DIM,
     "The shape of op desc's input or"
     " output contains unknown dim."},
    {OpNotSupportedReasonID::EN_NOT_SUPPORT_DYNAMIC_SHAPE,
     "This op which is dynamic shape is not configured to support dynamic shape in op store."}};

const std::string STR_NAME = "name";
const std::string STR_INPUT_LOWERCASE = "input";
const std::string STR_OUTPUT_LOWERCASE = "output";
const std::string STR_PATH = "path";
const std::string STR_IMP_PATH = "imp_path";
const std::string STR_PATTERN = "pattern";
const std::string STR_FORMAT = "format";
const std::string STR_UNKNOWN_SHAPE_FORMAT = "unknownshape_format";
constexpr char const *STR_INPUT_MEM_CONTINUES = "inputMemContinues";
constexpr char const *STR_OUTPUT_MEM_CONTINUES = "outputMemContinues";
constexpr char const *ATTR_NAME_CONTINUOUS_INPUT = "continuous_input";
constexpr char const *ATTR_NAME_CONTINUOUS_OUTPUT = "continuous_output";
const std::string STR_QUANT_MODE = "quant_mode";
const std::string STR_QUANT_HIGH_PRECISION = "quant_high_precision";
const std::string STR_QUANT_HIGH_PERFORMANCE = "quant_high_performance";
const std::string STR_NEED_CHECK_SUPPORT = "needCheckSupport";
const std::string STR_INT8 = "int8";
const std::string STR_BOOL = "bool";
const std::string STR_DTYPE = "dtype";
const std::string CONCATD = "ConcatD";
const std::string CONCATV2D = "ConcatV2D";
const std::string SPLITD = "SplitD";
const std::string SPLITVD = "SplitVD";
const std::string STRIDEDREAD = "StridedRead";
const std::string STRIDEDWRITE = "StridedWrite";
const std::string DEPTHWISE6TO4 = "DepthwiseWeight6DTo4D";
const std::string DEPTHWISE4TO6 = "DepthwiseWeight4DTo6D";
const std::string FRAMEWORKOP = "FrameworkOp";
const std::string PERMUTE = "Permute";

const std::string SWITCH = "Switch";
const std::string REFORMAT = "ReFormat";

const std::string CONVBNFILTERHOST = "ConvBnFilterHost";
const std::string CONVBNBIASHOST = "ConvBnBiasHost";

const std::string SWITCHN = "SwitchN";
const std::string GROUPPADDING = "GroupPadding";
constexpr char const *FCOP = "FullyConnection";
constexpr char const *CONV2D_COMPRESS = "Conv2DCompress";
constexpr char const *FC_COMPRESS = "FullyConnectionCompress";
constexpr char const *MATMULV2_COMPRESS = "MatMulV2Compress";
const std::string COMPRESSOP = "Compress";
const std::string COMPRESSFCOP = "CompressFcOp";
const std::string SWAPCO = "SwapCo";
const std::string SWAPCI = "SwapCi";
const std::string BNHOST = "BnHost";
const std::string ATTR_NAME_STREAM_LABEL = "_stream_label";
const std::string NEED_RUN_AFTER_GETSHAPE = "_need_run_after_getshape";
const std::string OP_PARA_SIZE = "op_para_size";
const std::string ATOMIC_OP_PARA_SIZE = "atomic_op_para_size";
const std::string NULL_OP_FILE = "Null";
const std::string AICORE_NULL = "AICore_Null";
const std::string OP_THREAD_PARA_SIZE = "op_thread_para_size";

const std::string TRANSDATA_INPUT_NAME = "src";
const std::string TRANSDATA_OUTPUT_NAME = "dst";
const std::string CAST_INPUT_NAME = "x";
const std::string CAST_OUTPUT_NAME = "y";
const std::string TRANSPOSE_INPUT_NAME = "x";
const std::string TRANSPOSE_OUTPUT_NAME = "y";
const std::string RESHAPE_INPUT_NAME = "x";
const std::string RESHAPE_SHAPE_NAME = "shape";
const std::string RESHAPE_OUTPUT_NAME = "y";
const std::string SQUEEZE_V2_INPUT_NAME = "x";
const std::string SQUEEZE_V2_OUTPUT_NAME = "y";
const std::string UNSQUEEZE_V2_INPUT_NAME = "x";
const std::string UNSQUEEZE_V2_OUTPUT_NAME = "y";

const std::string N_AXIS_NAME = "N";
const std::string C_AXIS_NAME = "C";
const std::string H_AXIS_NAME = "H";
const std::string W_AXIS_NAME = "W";
const std::string D_AXIS_NAME = "D";
const std::string AXES_ATTR_NAME = "axes";
const std::string AXIS_ATTR_NAME = "axis";

const std::string STR_NOT_SUPPORTED = "not_supported";
const std::string STR_FULLY_SUPPORTED = "fully_supported";
const std::string STR_PARTIALLY_SUPPORTED = "partially_supported";

const int32_t SHAPE_NUMBER_16 = 16;
const int32_t SHAPE_NUMBER_32 = 32;
const int32_t SHAPE_NUMBER_64 = 64;
const int32_t NI = 16;
const int32_t X0 = 16;
const int32_t LSTM_NI = 4;

const int32_t MINUS_VALUE_ONE = 1;
const int32_t MINUS_VALUE_TWO = 2;

const int32_t SIZE_OF_CN = 2;

const uint32_t LOW_DIMENSION_NUM_THD = 3;
const uint32_t NCHW_DIMENSION_NUM = 4;

const uint32_t MINIMUM_NZ_SHAPE_DIM_NUM = 2;
const uint32_t MINIMUM_ND_TO_RNN_SHAPE_NUM = 2;
#define DIMENSION_BITMAP(shift_num) (0x8 >> (shift_num))

const uint32_t NOT_SUPPORTED_REASON_OFFSET_UNIT = 0xF;
const uint32_t NOT_SUPPORTED_REASON_OFFSET_BIT_NUM = 4;
const int64_t NOT_SUPPORTED_FLAG_BIT = 0x8000000000000000;


const std::string STR_NON_PERSISTENT_CUSTOM_TBE = "non-persistent-tbe-custom";

/* CAST OP's input and output name. May be weird... */
const std::string CAST_ATTR_DSTT = "DstT";
const std::string CAST_ATTR_SRCT = "SrcT";
const std::string CAST_ATTR_DST_TYPE = "dst_type";
const std::string CAST_ATTR_SRC_TYPE = "src_type";
const std::string CAST_ATTR_TRUNCATE = "truncate";

const std::string ATTR_NAME_INPUT_FORMAT = "input_format";
const std::string ATTR_NAME_OUTPUT_FORMAT = "output_format";

const std::string ATTR_NAME_SRC_FORMAT = "src_format";
const std::string ATTR_NAME_DST_FORMAT = "dst_format";

// The align size of data memory
const uint32_t DATA_MEMORY_ALIGN_SIZE = 32;

// The default value used to fill in the dims
// when the size of dims is less than 4.
const uint32_t SHAPE_DIM_DEFAULT_VALUE = 1;

const int32_t NON_PERSISTENT_CUSTOM_PRIORITY = -1;
const int64_t SHAPE_DIM_VALUE_C04 = 4;

const uint32_t TENSOR_INDEX_FILTER_COMPRESS = 1;
const uint32_t TENSOR_INDEX_COMPRESS_INDEX = 2;
const uint32_t COMPRESSOP_INDEX_WEIGHT_COMPRESS = 0;
const uint32_t COMPRESSOP_INDEX_COMPRESS_INDEX = 1;

// unknown shape value
const int64_t UNKNOWN_SHAPE_VALUE = -1;

// default value of groups
const int64_t GROUPS_DEFAULT_VALUE = 1;

// rnn op attr
const int64_t RNN_HIDDEN_SIZE_DEFAULT_VALUE = 1;
const int64_t RNN_INPUT_SIZE_DEFAULT_VALUE = 1;
const int64_t RNN_STATE_SIZE_DEFAULT_VALUE = -1;

const int32_t NCHW_DIM_N = 0;
const int32_t NCHW_DIM_C = 1;
const int32_t NCHW_DIM_H = 2;
const int32_t NCHW_DIM_W = 3;

const int32_t NC1HWC0_DIM_N = 0;
const int32_t NC1HWC0_DIM_C1 = 1;
const int32_t NC1HWC0_DIM_C0 = 4;
const int32_t NC1HWC0_DIM_H = 2;
const int32_t NC1HWC0_DIM_W = 3;

const int32_t NDC1HWC0_DIM_N = 0;
const int32_t NDC1HWC0_DIM_D = 1;
const int32_t NDC1HWC0_DIM_C1 = 2;
const int32_t NDC1HWC0_DIM_C0 = 5;
const int32_t NDC1HWC0_DIM_H = 3;
const int32_t NDC1HWC0_DIM_W = 4;

const int32_t C1HWNCoC0_DIM_C1 = 0;
const int32_t C1HWNCoC0_DIM_H = 1;
const int32_t C1HWNCoC0_DIM_W = 2;
const int32_t C1HWNCoC0_DIM_N = 3;
const int32_t C1HWNCoC0_DIM_Co = 4;
const int32_t C1HWNCoC0_DIM_C0 = 5;

const int32_t C1DHWNCoC0_DIM_C1 = 0;
const int32_t C1DHWNCoC0_DIM_D = 1;
const int32_t C1DHWNCoC0_DIM_H = 2;
const int32_t C1DHWNCoC0_DIM_W = 3;

const int32_t NHWC_DIM_N = 0;
const int32_t NHWC_DIM_H = 1;
const int32_t NHWC_DIM_W = 2;
const int32_t NHWC_DIM_C = 3;

const int32_t HWCN_DIM_H = 0;
const int32_t HWCN_DIM_W = 1;
const int32_t HWCN_DIM_C = 2;
const int32_t HWCN_DIM_N = 3;

const int32_t CHWN_DIM_C = 0;
const int32_t CHWN_DIM_H = 1;
const int32_t CHWN_DIM_W = 2;
const int32_t CHWN_DIM_N = 3;

const int32_t NDHWC_DIM_N = 0;
const int32_t NDHWC_DIM_D = 1;
const int32_t NDHWC_DIM_H = 2;
const int32_t NDHWC_DIM_W = 3;
const int32_t NDHWC_DIM_C = 4;
const uint32_t DIMENSION_NUM_FIVE = 5;

const int32_t NCDHW_DIM_N = 0;
const int32_t NCDHW_DIM_C = 1;
const int32_t NCDHW_DIM_D = 2;
const int32_t NCDHW_DIM_H = 3;
const int32_t NCDHW_DIM_W = 4;

const int32_t DHWCN_DIM_D = 0;
const int32_t DHWCN_DIM_H = 1;
const int32_t DHWCN_DIM_W = 2;
const int32_t DHWCN_DIM_C = 3;
const int32_t DHWCN_DIM_N = 4;

const int32_t DHWNC_DIM_D = 0;
const int32_t DHWNC_DIM_H = 1;
const int32_t DHWNC_DIM_W = 2;
const int32_t DHWNC_DIM_N = 3;
const int32_t DHWNC_DIM_C = 4;

// dim default size value
const size_t DIM_DEFAULT_SIZE = 4;

const std::string OP_TYPE_PLACE_HOLDER = "PlaceHolder";
const std::string OP_TYPE_END = "End";
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
const std::string SQUEEZE_V2 = "SqueezeV2";
const std::string UNSQUEEZE_V2 = "UnsqueezeV2";

constexpr char const *CONV2D = "Conv2D";
constexpr char const *MATMULV2OP = "MatMulV2";
const std::string DEPTHWISECONV2D = "DepthwiseConv2D";
const std::string POOLING = "Pooling";
const std::string ELTWISE = "Eltwise";
const std::string RELU = "Relu";
const std::string RELU6 = "Relu6";
const std::string LEAKY_RELU = "LeakyRelu";
const std::string READ_SELECT = "ReadSelect";
const std::string ASCEND_QUANT = "AscendQuant";
const std::string ASCEND_DEQUANT = "AscendDequant";
const std::string OP_TYPE_PHONY_CONCAT = "PhonyConcat";
const std::string NEXT_ITERATION = "NextIteration";

const std::unordered_set<std::string> PLACE_OR_END_SET({
    OP_TYPE_PLACE_HOLDER, OP_TYPE_END, DATA,
    CONSTANT, CONSTANTOP, VARIABLE
});

const std::map<bool, std::string> BOOL_STR_MAP{{true, kStrTrue}, {false, kStrFalse}};

const std::map<te::CheckSupportedResult, std::string> CHECKSUPPORTED_STR_MAP = {
        {te::NOT_SUPPORTED, STR_NOT_SUPPORTED},
        {te::FULLY_SUPPORTED, STR_FULLY_SUPPORTED},
        {te::PARTIALLY_SUPPORTED, STR_PARTIALLY_SUPPORTED}};

const std::map<OpImplType, std::string> IMPL_TYPE_STRING_MAP{
        {EN_IMPL_CUSTOM_TBE, "tbe-custom"},
        {EN_IMPL_HW_TBE, "tbe-builtin"},
        {EN_IMPL_VECTOR_CORE_HW_TBE, "tbe-builtin-vector-core"},
        {EN_IMPL_VECTOR_CORE_CUSTOM_TBE, "tbe-custom-vector-core"},
        {EN_IMPL_NON_PERSISTENT_CUSTOM_TBE, "tbe-custom-non-persistent"},
};

// the origin fromat
const std::vector<ge::Format> FE_IDENTIFIABLE_ORIGIN_FORMAT_VECTOR = {
    ge::FORMAT_NCHW,  ge::FORMAT_NHWC,  ge::FORMAT_HWCN,  ge::FORMAT_CHWN,
    ge::FORMAT_NDHWC, ge::FORMAT_NCDHW, ge::FORMAT_DHWCN, ge::FORMAT_DHWNC};

// heavy format dependened on C channel
const std::vector<ge::Format> FE_HEAVY_FORMAT_DEPENDENED_C_VECTOR = {
    ge::FORMAT_NC1HWC0,     ge::FORMAT_C1HWNCoC0,     ge::FORMAT_FRACTAL_Z,
    ge::FORMAT_NDC1HWC0,    ge::FORMAT_FRACTAL_Z_3D,  ge::FORMAT_FRACTAL_Z_3D_TRANSPOSE,
    ge::FORMAT_NC1HWC0_C04, ge::FORMAT_FRACTAL_Z_C04, ge::FORMAT_FRACTAL_Z_G};

// C04 format
const std::vector<ge::Format> FE_C04_FORMAT_VECTOR = {ge::FORMAT_NC1HWC0_C04, ge::FORMAT_FRACTAL_Z_C04};

const std::vector<ge::Format> FE_GROUP_RELA_FORMAT_VECTOR = {ge::FORMAT_FRACTAL_Z, ge::FORMAT_FRACTAL_Z_3D};

const std::map<ge::Format, std::map<std::string, int32_t>> FE_AXIS_INDEX_OF_FORMAT = {
    {ge::FORMAT_NCHW, {{"N", NCHW_DIM_N}, {"C", NCHW_DIM_C}, {"H", NCHW_DIM_H}, {"W", NCHW_DIM_W}}},
    {ge::FORMAT_HWCN, {{"N", HWCN_DIM_N}, {"C", HWCN_DIM_C}, {"H", HWCN_DIM_H}, {"W", HWCN_DIM_W}}},
    {ge::FORMAT_NHWC, {{"N", NHWC_DIM_N}, {"C", NHWC_DIM_C}, {"H", NHWC_DIM_H}, {"W", NHWC_DIM_W}}},
    {ge::FORMAT_CHWN, {{"N", CHWN_DIM_N}, {"C", CHWN_DIM_C}, {"H", CHWN_DIM_H}, {"W", CHWN_DIM_W}}},
    {ge::FORMAT_NDHWC,
     {{"N", NDHWC_DIM_N}, {"C", NDHWC_DIM_C}, {"H", NDHWC_DIM_H}, {"W", NDHWC_DIM_W}, {"D", NDHWC_DIM_D}}},
    {ge::FORMAT_NCDHW,
     {{"N", NCDHW_DIM_N}, {"C", NCDHW_DIM_C}, {"H", NCDHW_DIM_H}, {"W", NCDHW_DIM_W}, {"D", NCDHW_DIM_D}}},
    {ge::FORMAT_DHWCN,
     {{"N", DHWCN_DIM_N}, {"C", DHWCN_DIM_C}, {"H", DHWCN_DIM_H}, {"W", DHWCN_DIM_W}, {"D", DHWCN_DIM_D}}},
    {ge::FORMAT_DHWNC,
     {{"N", DHWNC_DIM_N}, {"C", DHWNC_DIM_C}, {"H", DHWNC_DIM_H}, {"W", DHWNC_DIM_W}, {"D", DHWNC_DIM_D}}}};

const std::map<ge::DataType, std::string> DATATYPE_STRING_MAP {{ge::DT_FLOAT, "float32"},
                                                                      {ge::DT_FLOAT16, "float16"},
                                                                      {ge::DT_INT8, "int8"},
                                                                      {ge::DT_INT16, "int16"},
                                                                      {ge::DT_INT32, "int32"},
                                                                      {ge::DT_INT64, "int64"},
                                                                      {ge::DT_UINT8, "uint8"},
                                                                      {ge::DT_UINT16, "uint16"},
                                                                      {ge::DT_UINT32, "uint32"},
                                                                      {ge::DT_UINT64, "uint64"},
                                                                      {ge::DT_BOOL, "bool"},
                                                                      {ge::DT_DOUBLE, "double"},
                                                                      {ge::DT_DUAL, "dual"},
                                                                      {ge::DT_DUAL_SUB_INT8, "dual_sub_int8"},
                                                                      {ge::DT_DUAL_SUB_UINT8, "dual_sub_uint8"},
                                                                      {ge::DT_STRING, "string"},
                                                                      {ge::DT_COMPLEX64, "complex64"},
                                                                      {ge::DT_COMPLEX128, "complex128"},
                                                                      {ge::DT_QINT8, "qint8"},
                                                                      {ge::DT_QINT16, "qint16"},
                                                                      {ge::DT_QINT32, "qint32"},
                                                                      {ge::DT_QUINT8, "quint8"},
                                                                      {ge::DT_QUINT16, "quint16"},
                                                                      {ge::DT_RESOURCE, "resource"},
                                                                      {ge::DT_STRING_REF, "string_ref"},
                                                                      {ge::DT_INT4, "int4"},
                                                                      {ge::DT_BF16, "bfloat16"}};
const std::unordered_set<int> BUILT_IN_IMPLY_TYPE{
    EN_IMPL_HW_CONSTANT_CCE, EN_IMPL_HW_GENERAL_CCE, EN_IMPL_HW_TIK, EN_IMPL_HW_TBE,
    EN_IMPL_RL, EN_IMPL_PLUGIN_TBE, EN_IMPL_VECTOR_CORE_HW_TBE};

const std::vector<uint32_t> vector_of_dtype_and_c0 = {
    SHAPE_NUMBER_16,  // DT_FLOAT = 0,
    SHAPE_NUMBER_16,  // DT_FLOAT16 = 1,
    SHAPE_NUMBER_32,  // DT_INT8 = 2,
    SHAPE_NUMBER_16,  // DT_INT32 = 3,
    SHAPE_NUMBER_32,  // DT_UINT8 = 4,
    SHAPE_NUMBER_16,
    SHAPE_NUMBER_16,  // DT_INT16 = 6,
    SHAPE_NUMBER_16,  // DT_UINT16 = 7,
    SHAPE_NUMBER_16,  // DT_UINT32 = 8,
    SHAPE_NUMBER_16,  // DT_INT64 = 9,
    SHAPE_NUMBER_16,  // DT_UINT64 = 10,
    SHAPE_NUMBER_16,  // DT_DOUBLE = 11,
    SHAPE_NUMBER_16,  // DT_BOOL = 12,
    SHAPE_NUMBER_16,  // DT_DUAL = 13,
    SHAPE_NUMBER_16,  // DT_DUAL_SUB_INT8 = 14,
    SHAPE_NUMBER_16,  // DT_DUAL_SUB_UINT8 = 15,
    SHAPE_NUMBER_16,  // DT_COMPLEX64 = 16,
    SHAPE_NUMBER_16,  // DT_COMPLEX128 = 17,
    SHAPE_NUMBER_16,  // DT_QINT8 = 18,
    SHAPE_NUMBER_16,  // DT_QINT16 = 19,
    SHAPE_NUMBER_16,  // DT_QINT32 = 20,
    SHAPE_NUMBER_16,  // DT_QUINT8 = 21,
    SHAPE_NUMBER_16,  // DT_QUINT16 = 22,
    SHAPE_NUMBER_16,  // DT_RESOURCE = 23,
    SHAPE_NUMBER_16,  // DT_STRING_REF = 24,
    SHAPE_NUMBER_16,  // DT_DUAL = 25,
    SHAPE_NUMBER_16,  // DT_VARIANT = 26,
    SHAPE_NUMBER_16,  // DT_BF16 = 27,
    SHAPE_NUMBER_16,  // DT_UNDEFINED,
    SHAPE_NUMBER_64,  // DT_INT4 = 29,
};

enum class UbMatchedType {
  UBMATCHTYPE_AIPP = 0,
  UBMATCHTYPE_CONV2D,
  UBMATCHTYPE_READ_WRITE_SELECT,
  UBMATCHTYPE_STRIDED_READ_WRITE_SELECT,
  UBMATCHTYPE_DEQUANT,
  UBMATCHTYPE_REQUANT,
  UBMATCHTYPE_ELTWISE,
  UBMATCHTYPE_POOLING,
  UBMATCHTYPE_DEQUANTS16,
  UBMATCHTYPE_REQUANTS16,
  MATCHED_RESERVED
};

const std::map<std::string, UbMatchedType> op_pattern_to_matched_map = {
        {"aipp", UbMatchedType::UBMATCHTYPE_AIPP},
        {"write_select", UbMatchedType::UBMATCHTYPE_READ_WRITE_SELECT},
        {"read_select", UbMatchedType::UBMATCHTYPE_READ_WRITE_SELECT},
        {"strided_write", UbMatchedType::UBMATCHTYPE_STRIDED_READ_WRITE_SELECT},
        {"strided_read", UbMatchedType::UBMATCHTYPE_STRIDED_READ_WRITE_SELECT},
        {"dequant", UbMatchedType::UBMATCHTYPE_DEQUANT},
        {"requant", UbMatchedType::UBMATCHTYPE_REQUANT},
        {"ElemWise", UbMatchedType::UBMATCHTYPE_ELTWISE},
        {"Pool2d", UbMatchedType::UBMATCHTYPE_POOLING},
        {"MaxPool", UbMatchedType::UBMATCHTYPE_POOLING},
        {"dequant_s16", UbMatchedType::UBMATCHTYPE_DEQUANTS16},
        {"requant_s16", UbMatchedType::UBMATCHTYPE_REQUANTS16}
};

const set<ge::Format> FE_FORMAT_SET {
  ge::FORMAT_FRACTAL_NZ,
  ge::FORMAT_FRACTAL_ZN_RNN,
  ge::FORMAT_ND_RNN_BIAS
};

const std::unordered_set<std::string> kValidReshapeTypeOfNchw {
  "N", "C", "H", "W",  // 1D
  "NC", "NH", "NW", "CH", "CW", "HW", // 2D
  "NCH", "NCW", "NHW", "CHW" // 3D
};

const std::unordered_set<std::string> kValidReshapeTypeOfNhwc {
  "N", "H", "W", "C",
  "NH", "NW", "NC", "HW", "HC", "WC",
  "NHW", "NHC", "NWC", "HWC"
};

const std::unordered_set<std::string> kValidReshapeTypeOfHwcn {
    "H", "W", "C", "N",
    "HW", "HC", "HN", "WC", "WN", "CN",
    "HWC", "HWN", "HCN", "WCN"
};

const std::unordered_set<std::string> kValidReshapeTypeOfChwn {
    "C", "H", "W", "N",
    "CH", "CW", "CN", "HW", "HN", "WN",
    "CHW", "CHN", "CWN", "HWN"
};

const std::unordered_set<std::string> kValidReshapeTypeOfNdhwc {
    "N", "D", "H", "W", "C",
    "ND", "NH", "NW", "NC", "DH", "DW", "DC", "HW", "HC", "WC",
    "NDH", "NDW", "NDC", "NHW", "NHC", "NWC", "DHW", "DHC", "DWC", "HWC",
    "NDHW", "NDHC", "NDWC", "NHWC", "DHWC"
};

const std::unordered_set<std::string> kValidReshapeTypeOfNcdhw {
    "N", "C", "D", "H", "W",
    "NC", "ND", "NH", "NW", "CD", "CH", "CW", "DH", "DW", "HW",
    "NCD", "NCH", "NCW", "NDH", "NDW", "NHW", "CDH", "CDW", "CHW", "DHW",
    "NCDH", "NCDW", "NCHW", "NDHW", "CDHW"
};

const std::unordered_set<std::string> kValidReshapeTypeOfDhwcn {
    "D", "H", "W", "C", "N",
    "DH", "DW", "DC", "DN", "HW", "HC", "HN", "WC", "WN", "CN",
    "DHW", "DHC", "DHN", "DWC", "DWN", "DCN", "HWC", "HWN", "HCN", "WCN",
    "DHWC", "DHWN", "DHCN", "DWCN", "HWCN"
};

const std::map<ge::Format, std::unordered_set<std::string>> kAllValidReshapeType {
    {ge::FORMAT_NCHW, kValidReshapeTypeOfNchw},
    {ge::FORMAT_NHWC, kValidReshapeTypeOfNhwc},
    {ge::FORMAT_HWCN, kValidReshapeTypeOfHwcn},
    {ge::FORMAT_CHWN, kValidReshapeTypeOfChwn},
    {ge::FORMAT_NDHWC, kValidReshapeTypeOfNdhwc},
    {ge::FORMAT_NCDHW, kValidReshapeTypeOfNcdhw},
    {ge::FORMAT_DHWCN, kValidReshapeTypeOfDhwcn},
};

const std::map<ge::Format, size_t> kFullSizeOfFormat {
    {ge::FORMAT_NCHW, NCHW_DIMENSION_NUM},
    {ge::FORMAT_NHWC, NCHW_DIMENSION_NUM},
    {ge::FORMAT_HWCN, NCHW_DIMENSION_NUM},
    {ge::FORMAT_CHWN, NCHW_DIMENSION_NUM},
    {ge::FORMAT_NDHWC, DIMENSION_NUM_FIVE},
    {ge::FORMAT_NCDHW, DIMENSION_NUM_FIVE},
    {ge::FORMAT_DHWCN, DIMENSION_NUM_FIVE},
    {ge::FORMAT_ND, NCHW_DIMENSION_NUM},
};

const std::map<size_t, std::map<ge::Format, std::string>> kDefaultReshapeType {
    {0, {{ge::FORMAT_NCHW, ""}, {ge::FORMAT_NHWC, ""}, {ge::FORMAT_HWCN, ""},
         {ge::FORMAT_CHWN, ""}, {ge::FORMAT_NDHWC, ""},
         {ge::FORMAT_NCDHW, ""}, {ge::FORMAT_DHWCN, ""}}},

    {1, {{ge::FORMAT_NCHW, "C"}, {ge::FORMAT_NHWC, "C"}, {ge::FORMAT_HWCN, "C"},
         {ge::FORMAT_CHWN, "C"}, {ge::FORMAT_NDHWC, "C"},
         {ge::FORMAT_NCDHW, "C"}, {ge::FORMAT_DHWCN, "C"}}},

    {2, {{ge::FORMAT_NCHW, "CH"}, {ge::FORMAT_NHWC, "HW"}, {ge::FORMAT_HWCN, "CN"},
         {ge::FORMAT_CHWN, "WN"}, {ge::FORMAT_NDHWC, "WC"},
         {ge::FORMAT_NCDHW, "HW"}, {ge::FORMAT_DHWCN, "CN"}}},

    {3, {{ge::FORMAT_NCHW, "CHW"}, {ge::FORMAT_NHWC, "HWC"}, {ge::FORMAT_HWCN, "WCN"},
         {ge::FORMAT_CHWN, "HWN"}, {ge::FORMAT_NDHWC, "HWC"},
         {ge::FORMAT_NCDHW, "DHW"}, {ge::FORMAT_DHWCN, "WCN"}}},

    {4, {{ge::FORMAT_NDHWC, "DHWC"}, {ge::FORMAT_NCDHW, "CDHW"}, {ge::FORMAT_DHWCN, "HWCN"}}}
};

const std::map<OpConstValueDepend, te::OP_VALUE_DEPEND> VALUE_DEPEND_MAP{
    {CONST_IGNORE, te::VALUE_DEPEND_IGNORE},
    {CONST_REQUIRED, te::VALUE_DEPEND_REQUIRED},
    {CONST_OPTIONAL, te::VALUE_DEPEND_OPTIONAL}};

enum class LxFusionOptimizeResult {
  NO_FUSION_STRATEGY,
  L1_FUSION_STRATEGY,
  L2_FUSION_STRATEGY,
  BOTH_FUSION_STRATEGY
};
};  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_COMMON_UTIL_OP_INFO_UTIL_H_
