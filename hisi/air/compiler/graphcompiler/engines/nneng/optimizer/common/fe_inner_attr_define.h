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

#ifndef FUSION_ENGINE_OPTIMIZER_COMMON_FE_INNER_ATTR_DEFINE_H_
#define FUSION_ENGINE_OPTIMIZER_COMMON_FE_INNER_ATTR_DEFINE_H_

#include <string>
#include "common/aicore_util_attr_define.h"

namespace fe {
const std::string GLOBALWORKSPACE_STATUS = "globalworkspace_spec_workspace";

const std::string GLOBALWORKSPACE_STATUS_BYTES = "globalworkspace_spec_workspace_bytes";

const std::string GLOBALWORKSPACE_REF = "globalworkspace_ref";

// sgt
const std::string TBE_OP_THREAD_ATOMIC_OUTPUT_INDEX = "tbe_op_thread_atomic_output_index";

const std::string TBE_OP_THREAD_ATOMIC_WORKSPACE_FLAG = "tbe_op_thread_atomic_workspace_flag";

const std::string ATTR_NAME_THREAD_COMPRESS_PARAMETERS = "thread_compress_parameters";

const std::string ATTR_NAME_THREAD_WEIGHT_REPEAT = "_thread_weight_repeat";

const std::string ATTR_NAME_SGT_SLICE_SHAPE = "_sgt_slice_shape";

const std::string ATTR_NAME_SGT_ORI_SLICE_SHAPE = "_sgt_ori_slice_shape";
// end sgt
const std::string PERM = "perm";

const std::string CONCAT_DIM = "concat_dim";

const std::string SPLIT_DIM = "split_dim";

const std::string AIPP_CONV_FLAG = "Aipp_Conv_Flag";

const std::string TBE_OP_ATOMIC_OUTPUT_INDEX = "tbe_op_atomic_output_index";

const std::string TBE_OP_ATOMIC_WORKSPACE_INDEX = "tbe_op_atomic_workspace_index";

const std::string TBE_OP_ATOMIC_WORKSPACE_FLAG = "tbe_op_atomic_workspace_flag";

const std::string ATTR_NAME_CACHE_READ_MODE = "cache_read_mode";

const std::string IS_CHECK_SUPPORTED = "isCheckSupported";

const std::string INFER_FORMAT = "infer_format";

const std::string INFER_RESHAPE_TYPE = "_infer_reshape_type";

const std::string INPUT_ND_TO_OTHER_FORMAT = "_input_nd_to_other_format";

const std::string KEEP_DIMS = "keep_dims";

const std::string CUSTOM_OP_IMPL_CONFIG_PATH = "_custom_op_impl_config_path";

const std::string ATTR_NAME_COMPRESS_PARAMETERS = "compress_parameters";

const std::string ATTR_NAME_FE_GROUP = "_fe_group";

const std::string ATTR_NAME_FE_PROPAGAT_HEAVY_FORMAT = "_fe_propagat_heavvy_format";

const std::string ATTR_NAME_GROUPS = "groups";

const std::string ATTR_NAME_WEIGHT_REPEAT = "_weight_repeat";

const std::string ATTR_NAME_FE_WEIGHT_COMPRESS = "_fe_weight_compress";

const std::string ATTR_NAME_WEIGHT_COMPRESS = "_weight_compress";

const std::string ATTR_NAME_DTYPE_IS_UPDATED = "dtype_is_updated";

const std::string FORMAT_AGNOSTIC = "_format_agnostic";

const std::string ATTR_NAME_ATOMIC_CLEAN_NODE = "atomic_clean_node_ptr";

const std::string ATOMIC_CLEAN_OP_TYPE = "AtomicAddrClean";

const std::string DYNAMIC_ATOMIC_CLEAN_OP_TYPE = "DynamicAtomicAddrClean";

const std::string TBE_ATOMIC_KERNEL = "tbeAtomicKernel";

const std::string ATTR_NAME_IS_CONNECTED_TO_NETOUTPUT = "is_connected_to_netoutput";

const std::string IS_GE_OP = "_is_ge_op";

const std::string INPUT_FORMAT_AGNOSTIC_EXCEPTION = "_format_agnostic_except_input";

const std::string OUTPUT_FORMAT_AGNOSTIC_EXCEPTION = "_format_agnostic_except_output";

const std::string CONSUMER_FUSIBLE_LIST = "_consumers_fusible_list";

const std::string IS_CUSTOM_OP = "_is_custom_op";

const std::string NEED_DUPLICATE = "_need_duplicate";

const std::string FUSION_FAILED_ID_ATTR = "fusion_failed";

const std::string NON_PERSISTENT_CUSTOM_OP_FLAG = "_custom_op_flag";

const std::string COMPILE_INFO_JSON = "compile_info_json";

const std::string COMPILE_INFO_KEY = "compile_info_key";

const std::string ATOMIC_COMPILE_INFO_JSON = "_atomic_compile_info_json";

const std::string ATOMIC_COMPILE_INFO_KEY = "_atomic_compile_info_key";

const std::string IS_FIRST_LAYER_CONV = "_is_first_layer_conv";

const std::string ATTR_STRIDE_ATTR_STRIDE = "stride";

const std::string FORMAT_CONTINUOUS = "_format_continuous";

const std::string REFRESH_CONTINUOUS_FLAG = "_refresh_continuous_flag";

const std::string KEEP_DTYPE = "_keep_dtype";

const std::string ROLLBACK_IF_FAILED = "_rollback_if_failed";

const std::string ATTR_NAME_PARENT_NODE = "parentNode";

const std::string ATTR_NAME_IS_COMPIED_FUSION_OP = "_is_compiled_fusion_op";

const std::string ATTR_NAME_OP_INFER_DEPENDS = "_op_infer_depends";

const std::string kOpPrecisionModeStr = "op_precision_mode_str";

const std::string kIsComeFromConstOp = "_is_come_from_const_op";
}
#endif  // FUSION_ENGINE_OPTIMIZER_COMMON_FE_INNER_ATTR_DEFINE_H_
