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

#include "format_selector/manager/format_dtype_setter.h"
#include "common/configuration.h"
#include "ops_store/ops_kernel_manager.h"

namespace fe {
static const size_t kConvMinInputSize = 2;

FormatDtypeSetter::FormatDtypeSetter(const std::string& engine_name,
                                     OpStoreAdapterManagerPtr op_store_adapter_manager_ptr)
    : FormatDtypeManagerBase(op_store_adapter_manager_ptr), engine_name_(engine_name) {}

FormatDtypeSetter::~FormatDtypeSetter() {}

Status FormatDtypeSetter::SetSupportFormatDtype(const ge::ComputeGraph& graph) const {
  FE_TIMECOST_START(SetSupportFormatDtype);
  for (const ge::NodePtr &node : graph.GetAllNodes()) {
    Status result = SetSupportFormatDtypeByNode(node);
    if (result != SUCCESS) {
      return result;
    }
  }
  FE_TIMECOST_END(SetSupportFormatDtype, "SetSupportFormatDtype during FEGraphOptimizer::OptimizeOriginalGraph");
  return SUCCESS;
}

void FormatDtypeSetter::JudgeFirstLayerConv(const ge::NodePtr& node, const ge::OpDescPtr& op_desc) const {
  bool enable_small_channel = Configuration::Instance(AI_CORE_NAME).GetEnableSmallChannel();

  int first_lay_conv2d = (op_desc->GetType() == CONV2D &&
                          node->GetAllInDataAnchors().size() >= kConvMinInputSize &&
                          enable_small_channel);
  if (first_lay_conv2d) {
    auto in_data_anchor = node->GetInDataAnchor(0);
    auto weight_anchor = node->GetInDataAnchor(1);

    bool has_no_father = (in_data_anchor == nullptr || in_data_anchor->GetPeerOutAnchor() == nullptr ||
                          in_data_anchor->GetPeerOutAnchor()->GetOwnerNode() == nullptr ||
                          in_data_anchor->GetPeerOutAnchor()->GetOwnerNode()->GetOpDesc() == nullptr);
    bool weight_not_exist = (weight_anchor == nullptr || weight_anchor->GetPeerOutAnchor() == nullptr ||
                             weight_anchor->GetPeerOutAnchor()->GetOwnerNode() == nullptr ||
                             weight_anchor->GetPeerOutAnchor()->GetOwnerNode()->GetOpDesc() == nullptr);
    if (!has_no_father && !weight_not_exist) {
      auto peer_out_anchor = in_data_anchor->GetPeerOutAnchor();
      auto weight_out_anchor = weight_anchor->GetPeerOutAnchor();
      /* Peer in data anchors is 1 is the necessary condition. Because
       * we do not have transdata supporting NC1HWC0_C04 to 4D. */
      if (peer_out_anchor->GetPeerInDataAnchors().size() == 1) {
        ge::OpDescPtr father_op_desc = peer_out_anchor->GetOwnerNode()->GetOpDesc();
        ge::OpDescPtr weight_op_desc = weight_out_anchor->GetOwnerNode()->GetOpDesc();
        std::string father_op_type = father_op_desc->GetType();

        bool weight_qualified = CheckWeightTypeQualified(weight_out_anchor->GetOwnerNode(), CONSTANT);
        FE_LOGD("This op %s predecessor on the first edge is %s.", op_desc->GetName().c_str(),
                father_op_desc->GetName().c_str());
        /* First layer means the op in front of  */
        FE_LOGI("Weight qualification result is %u, feature map type is %s",
                weight_qualified, father_op_type.c_str());
        if (weight_qualified && father_op_type == AIPP) {
          FE_LOGD("This op %s is the first layer conv.", op_desc->GetName().c_str());
          (void)ge::AttrUtils::SetBool(op_desc, IS_FIRST_LAYER_CONV, true);
        }
        if (father_op_type == "BNInferenceD") {
          auto input_node = peer_out_anchor->GetOwnerNode();
          auto input_anchor = input_node->GetInDataAnchor(0);
          bool has_no_input = (input_anchor == nullptr || input_anchor->GetPeerOutAnchor() == nullptr ||
                               input_anchor->GetPeerOutAnchor()->GetOwnerNode() == nullptr ||
                               input_anchor->GetPeerOutAnchor()->GetOwnerNode()->GetOpDesc() == nullptr);
          if (!has_no_input) {
            auto peer_anchor = input_anchor->GetPeerOutAnchor();
            if (peer_anchor->GetPeerInDataAnchors().size() == 1) {
              ge::OpDescPtr input_op_desc = peer_anchor->GetOwnerNode()->GetOpDesc();
              if (input_op_desc->GetType() == AIPP) {
                FE_LOGD("This op %s is the first layer conv.", op_desc->GetName().c_str());
                (void)ge::AttrUtils::SetBool(op_desc, IS_FIRST_LAYER_CONV, true);
              }
            }
          }
        }
      }
    }
  }
}

Status FormatDtypeSetter::SetSupportFormatDtypeByNode(ge::NodePtr node_ptr) const {
    HeavyFormatInfo heavy_foramt_info;
    return SetSupportFormatDtypeByNode(node_ptr, heavy_foramt_info);
}

Status FormatDtypeSetter::SetSupportFormatDtypeByNode(ge::NodePtr node_ptr,
                                                      const HeavyFormatInfo& heavy_format_info) const {
  // 1. check the node_ptr and the op_desc_ptr
  FE_CHECK_NOTNULL(node_ptr);
  ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);

  // 2. check the imply_type
  string op_name = op_desc_ptr->GetName();
  string op_type = op_desc_ptr->GetType();
  int imply_type = -1;
  if (!ge::AttrUtils::GetInt(op_desc_ptr, FE_IMPLY_TYPE, imply_type)) {
    FE_LOGD("Op[name=%s,type=%s]: get the attribute FE_IMPLY_TYPE failed.", op_name.c_str(), op_type.c_str());
    return SUCCESS;
  }

  // 3. get op_kernel_info_ptr by op_impl_type and op_type
  OpImplType op_impl_type = static_cast<OpImplType>(imply_type);
  OpKernelInfoPtr op_kernel_info_ptr =
          OpsKernelManager::Instance(engine_name_).GetOpKernelInfoByOpType(op_impl_type, op_type);
  if (op_kernel_info_ptr == nullptr) {
    FE_LOGW("Engine[%s] not support op_impl_type[%d].", engine_name_.c_str(), op_impl_type);
    return SUCCESS;
  }

  /* 4. Judge whether it's first layer convolution.
   * The first layer convolution is able to use feature C0 = 4 to boost the
   * computation. */
  JudgeFirstLayerConv(node_ptr, op_desc_ptr);

  // 5. save the support format and data_types
  bool is_dynamic_check = IsOpDynamicImpl(op_desc_ptr);
  FormatDtypeSelectorBasePtr selector = GetSelector(op_kernel_info_ptr, is_dynamic_check);
  FE_CHECK_NOTNULL(selector);
  return selector->SetSupportFormatDtype(op_kernel_info_ptr, heavy_format_info, *(op_desc_ptr.get()), is_dynamic_check);
}
}  // namespace fe
