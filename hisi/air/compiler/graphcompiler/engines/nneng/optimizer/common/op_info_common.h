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

#ifndef FUSION_ENGINE_OPTIMIZER_COMMON_OP_INFO_COMMON_H_
#define FUSION_ENGINE_OPTIMIZER_COMMON_OP_INFO_COMMON_H_

#include <map>
#include <string>
#include <vector>
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/string_utils.h"
#include "ops_store/op_kernel_info.h"
#include "external/graph/types.h"

namespace fe {
struct HeavyFormatInfo {
  ge::Format expected_heavy_format = ge::FORMAT_RESERVED;
  int32_t sub_format = 0;
  int32_t anchor_index = 0;
  bool is_input = false;
  HeavyFormatInfo() {}
  HeavyFormatInfo(ge::Format expected_heavy_format_param, int32_t sub_format_param, int32_t anchor_index_param,
                  bool is_input_param)
      : expected_heavy_format(expected_heavy_format_param),
        sub_format(sub_format_param),
        anchor_index(anchor_index_param),
        is_input(is_input_param) {}
};

using IndexNameMap = std::map<uint32_t, std::string>;
#define IS_INPUT_TO_STRING(is_input) ((is_input) ? "input" : "output")
enum InputOrOutputIndex { INPUT_INDEX = 0, OUTPUT_INDEX = 1, INPUT_OUTPUT_INDEX_BOTTOM = 2 };

const std::unordered_map<ge::Format, std::unordered_set<ge::Format>> kValidTransFormat = {
        {ge::FORMAT_ND, {ge::FORMAT_FRACTAL_NZ}},
        {ge::FORMAT_NHWC, {ge::FORMAT_NC1HWC0, ge::FORMAT_FRACTAL_Z}},
        {ge::FORMAT_FRACTAL_NZ, {ge::FORMAT_ND}},
        {ge::FORMAT_NC1HWC0, {ge::FORMAT_NHWC}}
};

struct UnSupportedReason {
  std::string reason;
  uint64_t reason_id = static_cast<uint64_t>(OpNotSupportedReasonID::EN_REASON_ID_RESERVED);
};
/*
 *  @ingroup fe
 *  @brief   get input index and name in op kernel info map
 *  @param   [in]  op_desc
 *  @param   [in]  op_kernel_info
 *  @param   [out] input_map
 *  @return  SUCCESS or FAILED
 */
Status GetInputIndexNameMap(const ge::OpDesc &op_desc, const OpKernelInfo &op_kernel_info, IndexNameMap &input_map);

/*
 *  @ingroup fe
 *  @brief   get output index and name in op kernel info map
 *  @param   [in]  op_desc
 *  @param   [in]  op_kernel_info
 *  @param   [out] output_map
 *  @return  SUCCESS or FAILED
 */
Status GetOutputIndexNameMap(const ge::OpDesc &op_desc, const OpKernelInfo &op_kernel_info, IndexNameMap &output_map);

void CheckSpecialCases(const std::vector<InputOrOutputInfoPtr>& input_or_output_info, IndexNameMap& index_name_map,
                       uint32_t index, uint32_t op_desc_input_or_output_size, bool& has_found);

bool CheckInputSubString(const std::string& op_desc_input_name, const std::string& info_input_name);

Status GetDefaultReshapeType(const ge::Format& original_format, size_t old_dims_size, std::string& reshape_type);

Status ExpandDimension(std::vector<int64_t> &dims, const std::string &op_name, const ge::Format &original_format,
                       const ge::Format &final_format, const uint32_t &tensor_index, const std::string &reshape_type);

string GetShapeDims(const ge::GeShape &shape);

/**
 * is PlaceHolder, End, Data, Const or Variable
 * @param op_type current op_type
 * @return result
 */
bool IsPlaceOrEnd(const std::string &op_type);

/**
 * is ND or MD
 * @param format current format
 * @return result
 */
bool IsNd(const ge::Format &format);

void CheckStridedReadInConv2d(const vector<ge::NodePtr> &conv_nodes, vector<ge::NodePtr> &fusion_nodes);

bool IsTbeOp(ge::NodePtr node);

bool IsSupportedTransType(const ge::Format& ori_format, const ge::Format& final_format);

bool IsOpTranspose(const std::string &op_type);

bool CheckOpConstOrVariableInOriGraph(const ge::OpDescPtr &op_desc);

ge::Format GetCurOpOriginFormat(const ge::GeTensorDesc &cur_tensor_desc);

ge::GeShape GetCurOpOriginShape(const ge::GeTensorDesc &cur_tensor_desc);

void LogFormatMap(const map<string, vector<ge::Format>> &format_map);
void LogDataTypeMap(const map<string, vector<ge::DataType>> &data_type_map);

/**
 * if old_formats is NCHW,NHWC, old_data_types is float16,
 * then new_formats is NCHW,NHWC, new_data_types is float16,float16
 * @param old_formats old formats
 * @param old_data_types old data_types
 * @param new_formats new formats
 * @param new_data_types new data_types
 * @return SUCCESS or FAILED
 */
Status GenerateUnionFormatDtype(const vector<ge::Format> &old_formats, const vector<ge::DataType> &old_data_types,
                                vector<ge::Format> &new_formats, vector<ge::DataType> &new_data_types);

/* Get All input and output kernel info */
Status GetAllInputAndOutputKernelInfo(const OpKernelInfoPtr &op_kernel_info_ptr, const ge::NodePtr &current_node,
                                      const std::vector<IndexNameMap> &tensor_map,
                                      std::vector<std::vector<InputOrOutputInfoPtr>> &input_and_output_kernel);

bool IsScalarInput(const ge::GeShape &shape);
bool IsSameShape(const ge::GeShape &first_shape, const ge::GeShape &second_shape);
bool CheckOriginFormatIdentifiable(const ge::Format &format);
bool CheckOriginFormatsIdentifiable(const vector<ge::Format> &formats);
bool CheckOriginShapeDimNum(const ge::GeShape &shape, const size_t &dim_min);

// dimNum must be >= dim_min
bool CheckOriginShapesDimNum(const vector<ge::GeShape> &shapes, const size_t &dim_min);
bool CheckAccuracyOriginShapesDimNum(const vector<ge::GeShape> &shapes, const size_t &dim_size);

bool IsEsBoard();

bool IsSpecialCast(const ge::NodePtr &node_ptr);

bool CheckVirtualOp(const ge::OpDescPtr op_desc_ptr);

int32_t GetAxisIndexByFormat(const ge::Format &format, const string &axis);

bool GetDimValueByFormatAndShape(const ge::Format &format, const ge::GeShape &shape, string axis, int64_t &dim_value);

Status GetGroupAttributeWithVerify(ge::OpDescPtr op_desc_ptr, int64_t &group);

std::string GetRealNodeType(ge::OpDescPtr OpDescPtr);

/* Only when the weight node or its predecessor(s) is(are) expected, it's a qualified weight.
 * Because first layer conv feature can only be effective when it's inference scenario.
 * If weight is not expected, we will traverse all the way to the top input node. */
bool CheckWeightTypeQualified(const ge::NodePtr &weight_node, const string& expected_type);

Status GetInputOutputNameMap(const ge::OpDesc &op_desc, const OpKernelInfoPtr &op_kernel_info_ptr,
                             IndexNameMap &input_map, IndexNameMap &output_map);
Status GetOutputNameMap(const ge::OpDesc& op_desc, const OpKernelInfoPtr& op_kernel_info_ptr,
                        IndexNameMap& output_map);
bool GetInputOutputNameMap(const ge::NodePtr &node, const OpKernelInfoPtr &op_kernel_info_ptr,
                           IndexNameMap &input_map, IndexNameMap &output_map,
                           UnSupportedReason &reason);

void CheckHasNoFather(bool is_input, int32_t index, const ge::NodePtr &node, ge::InDataAnchorPtr &in_data_anchor,
                      bool &has_no_father);

// If a subgraph has been optimized by L2fusion, some nodes in the subgraph will have the lx_fusion_pass attribute
bool CheckL2FusionFusionStrategy(const ge::ComputeGraph& graph);

// If a subgraph has been optimized by L2buffer, all nodes in the subgraph should have lx_fusion_pass attr:false
bool CheckL2BufferFusionStrategy(ge::ComputeGraph& graph);

// is need to reshape when format is fz or fz_3d
bool IsNeedReshape(const ge::OpDescPtr& op_desc_ptr);

// if parent node of place holder is const, copy weight attr value of const node to place holder node
void CopyWeightAttrToPlaceHolder(ge::NodePtr &node);
// if input or output is lx addr ,think it not valid
bool InvalidMemType(const ge::OpDescPtr &node_desc);

// check is there fusion_scope or _l1_fusion_scope attr on opdesc
bool HasFusionScopeAttr(const ge::OpDescPtr &op_desc);
// get _l1_fusion_scope attr value from opdesc first
// if _l1_fusion_scope is not on opdesc, then try to get fusion_scope attr
bool GetFusionScopeAttr(const ge::OpDescPtr &op_desc, int64_t &scope_id);
bool GetFusionScopeAttr(const ge::OpDescPtr &op_desc, int64_t &scope_id, bool &is_l1_fusion);

void RemoveL1FusionScopeAttr(const ge::OpDescPtr &op_desc);

bool IsOpDynamicImpl(const ge::OpDescPtr &op_desc_ptr);
bool IsOpDynamicImpl(const ge::OpDesc &op_desc);

inline bool IsDtypeSensitiveOp(const std::string &op_type) {
  return op_type == CAST;
}

/**
 * If one of the dims is 0, the tensor is a zero-shape tensor.
 */
bool IsZeroShapeTensor(const ge::GeTensorDescPtr &tensor);

/**
 * If one of the tensors is zero shape tensor, the operator is
 * a zero-shape operator.
 */
bool IsStaticZeroShapeOp(const ge::OpDescPtr &op_desc);

bool IsLifeCycleEnd(const ge::Node &node, const ge::GeTensorDescPtr &input_desc, int input_idx);

bool IsAiCoreOp(const ge::NodePtr &node);

bool CheckTransFormatValid(const ge::NodePtr &node);

void CheckFusionTransNode(ge::NodePtr &cube_node, std::vector<ge::NodePtr> &trans_nodes,
                          std::vector<ge::NodePtr> &fusion_nodes);
}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_COMMON_OP_INFO_COMMON_H_
