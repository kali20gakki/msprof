/**
 * @file ops_kernel_constants.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 *
 * @brief constants
 *
 * @version 1.0
 *
 */

#ifndef FUSION_ENGINE_OPSKERNEL_OPS_KERNEL_STORE_OPS_KERNEL_CONSTANTS_H_
#define FUSION_ENGINE_OPSKERNEL_OPS_KERNEL_STORE_OPS_KERNEL_CONSTANTS_H_

#include <string>
#include "common/aicore_util_types.h"
#include "common/aicore_util_constants.h"
namespace fe {
const int32_t COMPUTE_COST_DEFAULT = 10;

const std::string STR_INT = "int";
const std::string STR_FLOAT = "float";
const std::string STR_BOOL = "bool";
const std::string STR_STR = "str";
const std::string STR_LIST_INT = "listInt";
const std::string STR_LIST_FLOAT = "listFloat";
const std::string STR_LIST_BOOL = "listBool";
const std::string STR_LIST_STR = "listStr";
const std::string STR_LIST_LIST_INT = "listListInt";

const std::string STR_NAME = "name";
const std::string STR_INPUT_LOWERCASE = "input";
const std::string STR_OUTPUT_LOWERCASE = "output";
const std::string STR_INPUT_FIRST_LETTER_UPPERCASE = "Input";
const std::string STR_OUTPUT_FIRST_LETTER_UPPERCASE = "Output";
const std::string STR_OP_FILE = "opFile";
const std::string STR_OP_INTERFACE = "opInterface";
const std::string STR_PRECISION_POLICY = "precision_reduce";
const std::string STR_PRECISION_POLICY_BF16 = "precision_reduce_bf16";
const std::string STR_RESHAPE_TYPE = "reshapeType";
const std::string STR_FLAG = "flag";
const std::string STR_OP = "op";
const std::string STR_PARAM_TYPE = "paramType";
const std::string STR_CONST_VALUE_DEPEND = "valueDepend";
const std::string STR_DTYPE = "dtype";
const std::string STR_FORMAT = "format";
const std::string STR_ALL = "all";
const std::string STR_LIST = "list";
const std::string STR_REQUIRED = "required";
const std::string STR_ATTR = "attr";
const std::string STR_ATTR_PREFIX = "attr_";
const std::string STR_TYPE = "type";
const std::string STR_VALUE = "value";
const std::string STR_DEFAULT_VALUE = "defaultValue";
const std::string STR_PATTERN = "pattern";
const std::string STR_HEAVY_OP = "heavyOp";
const std::string STR_SLICE_PATTERN = "slicePattern";
const std::string STR_IMP_PATH = "imp_path";
const std::string STR_PATH = "path";
const std::string STR_NEED_CHECK_SUPPORT = "needCheckSupport";
const std::string STR_DYNAMIC_FORMAT = "dynamicFormat";
const std::string STR_DYNAMIC_SHAPE_SUPPORT = "dynamicShapeSupport";
const std::string STR_DYNAMIC_RANK_SUPPORT = "dynamicRankSupport";
const std::string STR_UNKNOWN_SHAPE_FORMAT = "unknownshape_format";
const std::string STR_INPUT_MEM_CONTINUES = "inputMemContinues";
const std::string STR_OUTPUT_MEM_CONTINUES = "outputMemContinues";
const std::string STR_DYNAMIC_COMPILE_STATIC = "dynamicCompileStatic";
const std::string kCoreType = "coreType";
const std::string STR_RANGE_LIMIT = "rangeLimit";

const std::string STR_RANGE_LIMITED = "limited";
const std::string STR_RANGE_UNLIMITED = "unlimited";
const std::string STR_RANGE_DYNAMIC = "dynamic";
const std::map<std::string, bool> STR_BOOL_MAP{{kStrTrue, true}, {kStrFalse, false}};
const std::set<std::string> STR_RANGE_LIMIT_SET{STR_RANGE_LIMITED, STR_RANGE_UNLIMITED, STR_RANGE_DYNAMIC};

// maps utilized to transform string to enum
const std::map<std::string, OpParamType> PARAM_TYPE_MAP{
        {"dynamic", DYNAMIC}, {"optional", OPTIONAL}, {"required", REQUIRED}, {"reserved", RESERVED}};

const std::map<std::string, OpConstValueDepend> CONST_VALUE_DEPEND_MAP{
        {"required", CONST_REQUIRED}, {"optional", CONST_OPTIONAL}};

const std::map<std::string, OpPattern> OP_PATTERN_MAP{{"formatAgnostic", OP_PATTERN_FORMAT_AGNOSTIC},
                                                      {"broadcast", OP_PATTERN_BROADCAST},
                                                      {"reduce", OP_PATTERN_REDUCE},
                                                      {"dynamic", OP_PATTERN_OP_CUSTOMIZE}};

const std::map<std::string, SlicePattern> STR_SLICE_PATTERN_MAP{
        {"elemwise", ELEMENT_WISE},
        {"elemwiseBroadcast", ELEMENT_WISE_BROADCAST},
        {"broadcast", BROADCAST},
        {"slidingWindow", SLIDING_WINDOW},
        {"slidingWindowDeconv", SLIDING_WINDOW_DECONV},
        {"cubeMatmul", CUBE_MATMUL},
        {"reduce", SLICE_PATTERN_REDUCE},
        {"resize", SLICE_PATTERN_RESIZE},
        {"scatter", SLICE_PATTERN_SCATTER},
        {"segment", SLICE_PATTERN_SEGMENT},
};

const std::uint8_t kPrecisionReduceToRemoveShift = 1;
const std::uint8_t kPrecisionReduceBlackShift = 4;
const std::uint8_t kPrecisionReduceWhiteShift = 2;

const std::uint8_t kPrecisionReduceToRemoveBlackList = 1 << (kPrecisionReduceBlackShift +
                                                                    kPrecisionReduceToRemoveShift); // 0b00100000
const std::uint8_t kPrecisionReduceToAddBlackList    = 1 << kPrecisionReduceBlackShift;      // 0b00010000
const std::uint8_t kPrecisionReduceToRemoveWhiteList = 1 << (kPrecisionReduceWhiteShift +
                                                                    kPrecisionReduceToRemoveShift); // 0b00001000;
const std::uint8_t kPrecisionReduceToAddWhiteList    = 1 << kPrecisionReduceWhiteShift;      // 0b00000100;
const std::uint8_t kPrecisionReduceToRemoveGrayList  = 1 << kPrecisionReduceToRemoveShift;   // 0b00000010;
const std::uint8_t kPrecisionReduceToAddGrayList     = 1;                                    // 0b00000001;

const std::set<std::string> kPrecisionReduceListType = {"gray-list", "white-list", "black-list"};
const std::set<std::string> kPrecisionReduceUpdateType = {"to-add", "to-remove"};

const std::set<std::uint8_t> kPrecisionReduceUpdateTemplate = {
  kPrecisionReduceToRemoveBlackList,
  kPrecisionReduceToRemoveWhiteList,
  kPrecisionReduceToAddGrayList,
  kPrecisionReduceToAddGrayList | kPrecisionReduceToRemoveBlackList,
  kPrecisionReduceToAddGrayList | kPrecisionReduceToRemoveWhiteList,

  kPrecisionReduceToAddWhiteList,
  kPrecisionReduceToAddWhiteList | kPrecisionReduceToRemoveGrayList,
  kPrecisionReduceToAddWhiteList | kPrecisionReduceToRemoveBlackList,

  kPrecisionReduceToAddBlackList,
  kPrecisionReduceToAddBlackList | kPrecisionReduceToRemoveGrayList,
  kPrecisionReduceToAddBlackList | kPrecisionReduceToRemoveWhiteList,
};
}

#endif  // FUSION_ENGINE_OPSKERNEL_OPS_KERNEL_STORE_OPS_KERNEL_CONSTANTS_H_
