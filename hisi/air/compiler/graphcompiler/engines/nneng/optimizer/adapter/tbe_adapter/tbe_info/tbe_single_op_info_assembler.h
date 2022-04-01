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

#ifndef FUSION_ENGINE_OPTIMIZER_ADAPTER_TBE_ADAPTER_TBE_INFO_TBE_SINGLE_OP_INFO_ASSEMBLER_H_
#define FUSION_ENGINE_OPTIMIZER_ADAPTER_TBE_ADAPTER_TBE_INFO_TBE_SINGLE_OP_INFO_ASSEMBLER_H_

#include "adapter/tbe_adapter/tbe_info/tbe_info_assembler.h"

#include "tensor_engine/fusion_api.h"

namespace fe {

using TbeInfoAssemblerPtr = std::shared_ptr<fe::TbeInfoAssembler>;

struct OpTensorStruct {
  uint32_t index;
  OpParamType op_param_type;
  bool is_last_dynamic_tensor;
};

const std::map<std::string, ge::GeAttrValue::ValueType> ATTR_STRING_TO_VALUETYPE_MAP{
    {"str", ge::GeAttrValue::VT_STRING},
    {"int", ge::GeAttrValue::VT_INT},
    {"float", ge::GeAttrValue::VT_FLOAT},
    {"bool", ge::GeAttrValue::VT_BOOL},
    {"listStr", ge::GeAttrValue::VT_LIST_STRING},
    {"listInt", ge::GeAttrValue::VT_LIST_INT},
    {"listFloat", ge::GeAttrValue::VT_LIST_FLOAT},
    {"listBool", ge::GeAttrValue::VT_LIST_BOOL},
    {"listListInt", ge::GeAttrValue::VT_LIST_LIST_INT},
    {"tensor", ge::GeAttrValue::VT_TENSOR},
    {"listTensor", ge::GeAttrValue::VT_LIST_TENSOR}};

class TbeSingleOpInfoAssembler {
 public:
  TbeSingleOpInfoAssembler();

  ~TbeSingleOpInfoAssembler();

  Status AssembleSingleTbeInfo(ge::Node *node, te::TbeOpInfo &tbe_op_info, const string &engine_name);
  Status Initialize();
 private:
  TbeInfoAssemblerPtr tbe_info_assembler_ptr_;

  Status FeedInputInfoToSingleTbeInfo(const ge::OpDescPtr &op_desc_ptr,
                                      const vector<OpTensorStruct> op_input_tensor_struct,
                                      te::TbeOpInfo &tbe_op_info);

  Status FeedOutputInfoToSingleTbeInfo(const ge::OpDescPtr &op_desc_ptr,
                                       const vector<OpTensorStruct> op_output_tensor_struct,
                                       te::TbeOpInfo &tbe_op_info);

  Status JudgeShapeToSetFlag(const ge::OpDescPtr &op_desc, te::TbeOpInfo &op_info, bool &flag, string in_out) const;

  /*
   *  @ingroup fe
   *  @brief   set Attrs to single_tbe_op_info
   *  @param   [in]  op              op desc ptr
   *  @param   [in/out]  op_info      tbe data item
   *  @return  SUCCESS or FAILED
   */
  Status FeedAttrsToSingleTbeOpInfo(const ge::OpDescPtr &op_desc_ptr, te::TbeOpInfo &tbe_op_info) const;
  /*
    *  @ingroup fe
    *  @brief   set Attrs:flagint64 to tbe_op_info
    *  @param   [in]  node            input node pointer
    *  @param   [in/out]  op_info      tbe data item
    *  @return  SUCCESS or FAILED
    */
  Status FeedFlagInt64ToTbeOpInfo(const ge::Node *node, te::TbeOpInfo &op_info) const;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_ADAPTER_TBE_ADAPTER_TBE_INFO_TBE_SINGLE_OP_INFO_ASSEMBLER_H_
