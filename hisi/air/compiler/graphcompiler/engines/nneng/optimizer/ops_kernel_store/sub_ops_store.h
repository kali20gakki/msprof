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

#ifndef FUSION_ENGINE_OPTIMIZER_OPS_KERNEL_STORE_SUB_OPS_STORE_H_
#define FUSION_ENGINE_OPTIMIZER_OPS_KERNEL_STORE_SUB_OPS_STORE_H_
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "common/math_util.h"
#include "common/op_info_common.h"
#include "common/util/json_util.h"
#include "adapter/common/op_store_adapter_manager.h"
#include "format_selector/manager/format_dtype_querier.h"
#include "ops_store/op_kernel_info_constructor.h"
#include "ops_store/op_kernel_info.h"
#include "graph/node.h"
#include "graph/op_desc.h"
#include "graph/types.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/node_utils.h"

namespace fe {
using RootSet = std::unordered_set<ge::NodePtr>;

struct NodeGeneralInfo {
  bool is_found_in_opstore = false;
  bool is_sub_graph_node = false;
  bool is_support_dynamic_shape = false;
  bool is_limited_range = false;
  TbeOpInfoPtr op_info;
  OpKernelInfoPtr op_kernel;
  RootSet disjoint_root_set;
  std::map<ge::GeTensorDescPtr, RootSet> inputs_root_map;
};
using NodeGeneralInfoPtr = std::shared_ptr<NodeGeneralInfo>;

struct SupportedFormatAndDtype {
  OpKernelInfoPtr op_kernel_info_ptr;
  IndexNameMap input_index_name_map;
  IndexNameMap output_index_name_map;
  map<string, vector<ge::Format>> suppport_formats_map;
  map<string, vector<ge::DataType>> support_data_types_map;
  string reason;
  bool is_input;

  SupportedFormatAndDtype(OpKernelInfoPtr op_kernel_info_ptr_param, string reason_param)
      : op_kernel_info_ptr(std::move(op_kernel_info_ptr_param)),
        reason(std::move(reason_param)),
        is_input(false) {}
};

using FormatDtypeQuerierPtr = std::shared_ptr<FormatDtypeQuerier>;
/**
 * @brief Store ops kernel info of only one specific implement.
 * FEOpsKernelInfoStore consists of at most 8 SubOpsStore
 */
class SubOpsStore {
 public:
  friend class FEOpsKernelInfoStore;
  explicit SubOpsStore(OpStoreAdapterManagerPtr op_store_adapter_manager_ptr);

  virtual ~SubOpsStore();

  SubOpsStore(const SubOpsStore &) = delete;

  SubOpsStore &operator=(const SubOpsStore &) = delete;

  /*
   * @ingroup fe
   * @brief : Initialize the SubOpsStore, load the op info from the specific.json file
   * @param[in] options: none
   * @return : SUCCESS/FAILED
   */
  virtual Status InitializeSubStore(std::string engine_name);

  /*
   * @ingroup fe
   * @brief : finalize the SubOpsStore, clear all op info and op kernel info;
   * @param[in] None
   * @return : SUCCESS/FAILED
   */
  Status FinalizeSubStore();

  /*
   * @ingroup fe
   * @brief : set the the FEOpsStoreInfo of this SubOpsStore;
   * @param[in|out] store_info : the FEOpsStoreInfo of this SubOpsStore;
   * @return : void
   */
  void SetSubStoreInfo(const FEOpsStoreInfo &sub_store_info);

  /*
   * @brief : Check whether the input op_desc is supported in this sub ops store
   */
  virtual bool CheckSubStoreSupported(const ge::NodePtr &node, OpKernelInfoPtr &op_kernel_info_ptr,
                                      UnSupportedReason &reason, const CheckSupportMode &check_mode,
                                      const bool &check_unknown_shape) const;

  void SetSubStoreType(const std::string &sub_store_type);

  void GetSubStoreType(std::string &sub_store_type) const;

 protected:
  /*
   *  @ingroup fe
   *  @brief   check all attr value specified in OpKernelInfo to OpDesc,
   *           each Value of OpDesc Attr should containing in OpKernelInfo
   *  @param   [in] op_desc       : OpDesc from graph_ir node
   *  @param   [in] op_kernel_info : OpKernelInfo(om_optype) from sub ops store
   *  @return  true or false
   */
  bool CheckAttrSupport(const ge::NodePtr &node, const OpKernelInfo &op_kernel_info, std::string &reason) const;

  bool CheckParamType(const ge::NodePtr &node, OpKernelInfoPtr op_kernel_info_ptr,
                      const std::map<uint32_t, string> &input_index_name_map,
                      const std::map<uint32_t, string> &output_index_name_map, std::string &reason) const;

  bool CheckAllTensorsParamType(const ge::NodePtr &node, bool is_input,
                                const vector<InputOrOutputInfoPtr> &all_tesnors_info,
                                const map<uint32_t, string> &index_name_map, std::string &reason) const;

  bool CheckInputSupported(const ge::NodePtr &node, uint32_t input_size,
                           SupportedFormatAndDtype &info) const;

  bool CheckAllTensorsSupportedAccurateMode(const ge::NodePtr &node, SupportedFormatAndDtype &info) const;

  bool CheckOutputSupported(const ge::NodePtr &node, uint32_t output_size,
                            SupportedFormatAndDtype &info) const;

  bool CheckOpSupported(const ge::NodePtr &node_ptr, OpKernelInfoPtr &op_kernel_info_ptr, const bool &is_dynamic_impl,
                        UnSupportedReason &reason) const;

 private:
  bool CheckSingleTensorAccurateMode(uint32_t size, uint32_t type_index, const ge::NodePtr &node,
                                     SupportedFormatAndDtype &info, bool &need_continue) const;

  bool CheckSingleTensorSupportedAccurateMode(const ge::NodePtr &node, uint32_t index,
                                              uint32_t type_index, SupportedFormatAndDtype &info,
                                              bool &check_flag) const;

  bool CheckFormatAndDtypeNormalMode(const ge::NodePtr &node, const string &name,
                                     const ge::GeTensorDescPtr &tensor_desc,
                                     const SupportedFormatAndDtype &info, string &reason) const;

  /*
   * @ingroup fe
   * @brief: check whether the dtype is supported by the FEOpsKernelInfoStore;
   * @param[in] tensor_desc_ptr    : a GeTensorDescPtr of the input/output tensor;
   * @param[in] tensor_desc_info_ptr : a InputOrOutputInfoPtr of the input
   * @return: true if supported, else false;
   */
  bool CheckDtypeSupported(const ge::NodePtr &node, const ge::GeTensorDescPtr &tensor_desc_ptr,
                           InputOrOutputInfoPtr input_or_output_info_ptr,
                           const vector<ge::DataType> &support_data_types,
                           const OpKernelInfoPtr &op_kernel_info_ptr) const;

  /*
   * @ingroup fe
   * @brief: check whether the dtype is supported by the FEOpsKernelInfoStore;
   * @param[in] tensor_desc_ptr    : a GeTensorDescPtr of the input/output tensor;
   * @param[in] tensor_desc_info_ptr : a InputOrOutputInfoPtr of the input
   * @return: true if supported, else false;
   */
  bool CheckAccuracyDtypeSupported(ge::ConstGeTensorDescPtr c_tensor_desc_ptr,
                                   uint32_t type_index, InputOrOutputInfoPtr input_or_output_info_ptr,
                                   const vector<ge::DataType> &support_data_types) const;

  /*
   *  @ingroup fe
   *  @brief   check op_desc_attr.Value() is in info_op_attr.std::vector<Values>
   *  @param   [in] op_desc_attr : one of GeAttrValue from OpDesc
   *  @param   [in] info_op_attr : one of std::vector<GeAttrValue> from OpKernelInfo
   *  @param   [in] attr_type   : enum of ValueType, from OpKernelInfo
   *  @return  true or false
   */
  bool CheckAttrValue(const ge::GeAttrValue &op_desc_attr, const std::vector<ge::GeAttrValue> &info_op_attr,
                      ge::GeAttrValue::ValueType attr_type) const;

  /*
   * @ingroup fe
   * @brief: check whether the format is supported by this sub ops store
   * @param[in] c_tensor_desc_ptr    : a ConstGeTensorDescPtr of the input/output tensor;
   * @param[in] tensor_desc_info_ptr : a InputOrOutputInfoPtr of the input
   * @return: true if supported, else false;
   */
  bool CheckFormatSupported(const ge::NodePtr &node, ge::ConstGeTensorDescPtr c_tensor_desc_ptr,
                            InputOrOutputInfoPtr input_or_output_info_ptr,
                            const vector<ge::Format> &support_formats) const;

  /*
   * @ingroup fe
   * @brief: check whether the format is supported by this sub ops store
   * @param[in] c_tensor_desc_ptr    : a ConstGeTensorDescPtr of the input/output tensor;
   * @param[in] type_index         : the index of format in op_store
   * @param[in] tensor_desc_info_ptr : a InputOrOutputInfoPtr of the input
   * @return: true if supported, else false;
   */
  bool CheckAccuracyFormatSupported(ge::ConstGeTensorDescPtr c_tensor_desc_ptr,
                                    uint32_t type_index, InputOrOutputInfoPtr input_or_output_info_ptr,
                                    const vector<ge::Format> &support_formats) const;

  /*
   * @ingroup fe
   * @brief: check whether the input whose value depend is required links with a const or constant node.
   * @param[in] info           : a struct object about op kernel info
   * @param[in] input_name     : the name of input desc
   * @param[in] is_input_const : the value from GetIsInputConst
   * @return: true if supported, else false;
   */
  bool CheckInputConstValueDepend(SupportedFormatAndDtype &info, const string &input_name,
                                  const bool &is_input_const) const;

  void LogAllFormatAndDtype(const SupportedFormatAndDtype &info, const string &tensor_name,
                            std::ostringstream &reason_oss, string &reason) const;

  bool IsAllDtypeExpected(ge::DataType dtype_expected, ge::DataType dtype_unexpected,
                          const vector<ge::DataType> &dtypes) const;

  void GetIsInputConstNodeVec(const ge::NodePtr &node, vector<bool> &is_input_const_vec) const;

 protected:
  bool init_flag_;

  FEOpsStoreInfo sub_store_info_;

  std::string sub_store_type_;

  std::string engine_name_;

  FormatDtypeQuerierPtr format_dtype_querier_ptr_;

  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;

};

}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_OPS_KERNEL_STORE_SUB_OPS_STORE_H_
