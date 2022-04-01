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
#ifndef TF_UTIL_H_
#define TF_UTIL_H_

#include "util/util.h"
#include "proto/tensorflow/types.pb.h"
#include "proto/fwk_adapter.pb.h"

using TFDataType = ::domi::tensorflow::DataType;
namespace aicpu {
/**
 * Generate the unique id by timestamp
 * @return uniqueId
 */
uint64_t GenerateUniqueId();

/**
 * Convert the Ge data to tf data type
 * @param data_type ge's data type
 * @param is_ref whether the base data type is ref
 * @return tf's data type
 */
TFDataType ConvertGeDataType2TfDataType(ge::DataType data_type, bool is_ref=false);

/**
 * Convert string to TFDataType.
 * @param elem element attr
 * @param data_type data type
 * @return TFDataType
 */
bool ConvertString2TfDataType(const std::string &elem, TFDataType &data_type);

/**
 * Identify and set ShapeType attr for ge node
 * @param node ge node
 * @return status whether this operation success
 */
ge::Status CheckAndSetUnknowType(ge::NodePtr &node);

/**
 * Get unknown type with outputs shape desc
 * @param node ge node
 * @param shape_type node shape type
 * @return status whether this operation success
 */
ge::Status GetUnKnowTypeByOutDesc(const ge::NodePtr &node, int32_t &shape_type);

/**
 * Create node def for ge node
 * @param  node Ge node
 * @return status whether operation successful
 */
ge::Status CreateNodeDef(const ge::Node &node);

/**
 * Update node info
 * @param  node Ge node
 * @param  all_op_info Ge op info
 * @return status whether operation successful
 */
ge::Status UpdataOpInfo(const ge::Node &node, const map<string, OpFullInfo> &all_op_info);

/**
 * Calculation workspace size for node
 * @param  node Ge node
 * @param  workspace_size workspace_size
 * @return status whether operation successful
 */
ge::Status CalcWorkspaceSize(const ge::Node &node, int64_t &workspace_size);

/**
 * Calc the size of tf's node_def, firstly transform the
 *  Node to tf's node_def,then get the node_def's size,
 *  if the operator has the func_def, then calc the
 *  func_def's size together.
 * @param node original GE node info
 * @param node_def_bytes tf node def
 * @param func_def_lib_bytes tf function def library
 * @param node_def_size the size of node def
 * @param func_def_lib_size the size of function def library
 * @return status whether operation successful
 */
ge::Status ParseNodeDefAndFuncDef(const ge::Node &node,
                                   ge::Buffer &node_def_bytes,
                                   ge::Buffer &func_def_lib_bytes,
                                   int64_t &node_def_size,
                                   int64_t &func_def_lib_size);

/**
 * Build the kernelRunParam
 * @param op_desc Op description
 * @param kernel_run_param fake kernel_run_param just the input and
 *  output data_addr is not real)
 * @param skip_dim_check
 * @return status whether operation successful
 */
ge::Status BuildKernelRunParam(const ge::OpDesc &op_desc,
                                ::aicpu::FWKAdapter::KernelRunParam &kernel_run_param,
                                bool skip_dim_check = false);

/**
 * Set the aicpu::FWKAdapter::TensorDataInfo, the data is from Ge Tensor
 * @param ge_tensor_desc Original Ge Tensor
 * @param tensor_data_info The input or output data, defined by protobuf
 * @param is_ref if output is ref
 * @param skip_dim_check if skip dim
 * @param is_output if is output
 * @return status whether operation successful
 */
aicpu::State SetTensorDataInfo(const ge::GeTensorDesc &ge_tensor_desc,
                               ::aicpu::FWKAdapter::TensorDataInfo &tensor_data_info,
                               bool is_ref = false,
                               bool skip_dim_check = false,
                               bool is_output = false);
/**
 * Calc the running size of Operator,then GE will alloc the memsize from runtime
 * The size is consist of the part as follow:
 *   1.Input and output size;
 *   2.NodeDef in tf; 3.FuncDef in tf.
 * @param node Node information, return task_memsize in node's attr
 * @return status whether this operation successful
 */
ge::Status CalcTfOpRunningParam(const ge::Node &node);

}
#endif // TF_UTIL_H_