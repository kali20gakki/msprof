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

#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_comm/l2_fusion_comm.h"
#include <set>
#include <utility>
#include "common/math_util.h"
#include "external/graph/types.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/node_utils.h"
#include "graph_optimizer/spacesize_calculator/tensor_compute_util.h"

namespace fe {
const uint64_t ONE_KILO_BYTE = 1024;  // 1KB
const uint64_t MINI_L2_SIZE_MB = 8;   // 8MB

uint64_t GetL2Size() {
  // below l2 hardware parameter, referred from optimizer_profile.cc
  const uint64_t l2_size = MINI_L2_SIZE_MB * ONE_KILO_BYTE * ONE_KILO_BYTE;
  return l2_size;
}

uint64_t GetL2PageNum() {
  // below l2 hardware parameter, referred from optimizer_profile.cc
  const uint64_t page_num = 64;
  return page_num;
}

Status L2FusionComm::GetL2HardwareSet(k_l2_buffer_t &l2) const {
  l2.l2_buffer_size = GetL2Size();
  l2.page_num = GetL2PageNum();
  const uint64_t max_data_num = 8;
  l2.max_data_num = max_data_num;
  return fe::SUCCESS;
}

Status CalcTensorSize(ge::GeTensorDesc &tensor_desc, int64_t &tensor_size) {
  // verify the tensor
  FE_CHECK(TensorComputeUtil::VerifyTensor(tensor_desc) != SUCCESS,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][CaclTensorSize] Fail to verify this tensor."),
           return FAILED);

  int64_t element_cnt;
  FE_CHECK(TensorComputeUtil::GetElementCountByMultiply(tensor_desc, element_cnt) != SUCCESS,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][CaclTensorSize] Fail to calculate tensor size."), return FAILED);
  ge::DataType data_type = tensor_desc.GetDataType();
  int32_t output_real_calc_flag = 0;
  FE_CHECK(
      TensorComputeUtil::GetTensorSizeByDataType(element_cnt, data_type, tensor_size, output_real_calc_flag) != SUCCESS,
      REPORT_FE_ERROR("[StreamOpt][L2Opt][CaclTensorSize] Fail to get tensor size by element count and datatype."),
      return FAILED);
  return SUCCESS;
}

// output param: data_size
Status L2FusionComm::GetGraphDataSize(ge::OpDescPtr opdef, ge::GeTensorDesc &tensor, uint32_t data_type,
                                      int64_t &data_size) const {
  data_size = 0;

  // AIPP situation, the input size, we should use the size of fmk
  if (data_type == INPUT_DATA) {
    bool support_aipp = false;

    if (ge::AttrUtils::GetBool(opdef, AIPP_CONV_FLAG, support_aipp)) {
      if (support_aipp) {
        int64_t size = 0;
        FE_CHECK(ge::TensorUtils::GetSize(tensor, size) != ge::GRAPH_SUCCESS,
                 REPORT_FE_ERROR("[StreamOpt][L2Opt][GetGphDataSize] Get size failed!"),
                 return fe::FAILED);
        data_size = size;
        return fe::SUCCESS;
      }
    }
  }

  int64_t size = 0;
  FE_CHECK(CalcTensorSize(tensor, size) != fe::SUCCESS,
           REPORT_FE_ERROR("[StreamOpt][L2Opt][GetGphDataSize] Calc tensor size failed!"), return fe::FAILED);
  data_size = size;

  return fe::SUCCESS;
}

// data_type: 0-input, 1-output
// data_type: 0-input, 1-output
Status L2FusionComm::GetGraphDataSize(ge::NodePtr node, uint32_t data_id, uint32_t data_type,
                                      int64_t &data_size) const {
  if (data_type == INPUT_DATA) {
    // dual type, we need get the related size
    for (size_t i = 0; i < node->GetAllInDataAnchors().size(); ++i) {
      auto in_data_anchor = node->GetInDataAnchor(i);
      auto pre_out_data_anchor = in_data_anchor->GetPeerOutAnchor();

      if (pre_out_data_anchor == nullptr) {
        continue;
      }
      auto pre_node = pre_out_data_anchor->GetOwnerNode();
      if (IsNonValidOp(pre_node)) {
        continue;
      }

      auto pre_op_desc_ptr = pre_out_data_anchor->GetOwnerNode()->GetOpDesc();
      ge::GeTensorDesc output_tensor = pre_op_desc_ptr->GetInputDesc(pre_out_data_anchor->GetIdx());
      bool is_dual_output = (in_data_anchor->GetIdx() == static_cast<int32_t>(data_id)) &&
                            static_cast<uint32_t>(output_tensor.GetDataType()) == static_cast<uint32_t>(ge::DT_DUAL);

      if (is_dual_output) {
        FE_CHECK(GetGraphDataSize(pre_op_desc_ptr, output_tensor, OUTPUT_DATA, data_size) != fe::SUCCESS,
                 REPORT_FE_ERROR("[StreamOpt][L2Opt][GetGphDataSize] GetGraphDataSize failed!"), return fe::FAILED);
        return fe::SUCCESS;
      }
    }

    ge::GeTensorDesc tensor = node->GetOpDesc()->GetInputDesc(data_id);
    FE_CHECK(GetGraphDataSize(node->GetOpDesc(), tensor, INPUT_DATA, data_size) != fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][GetGphDataSize] GetGraphDataSize failed!"), return fe::FAILED);
  } else {
    ge::GeTensorDesc tensor = node->GetOpDesc()->GetOutputDesc(data_id);
    FE_CHECK(GetGraphDataSize(node->GetOpDesc(), tensor, OUTPUT_DATA, data_size) != fe::SUCCESS,
             REPORT_FE_ERROR("[StreamOpt][L2Opt][GetGphDataSize] GetGraphDataSize failed!"), return fe::FAILED);
  }

  return fe::SUCCESS;
}

void L2FusionComm::DisplayParserData(const k_l2_task_datas_map_t &data) const {
  FE_LOGD("L2 Parser has %lu tasks", data.size());
  for (k_l2_task_datas_map_t::const_iterator it = data.begin(); it != data.end(); ++it) {
    FE_LOGD("L2 parser Info: node_name=%s, node_id = %u, input_size = %lu, output_size = %lu", it->node_name.c_str(),
            it->node_id, it->input.size(), it->output.size());
  }
  return;
}

using k_l2_task_data_allocs_map_view_t = std::map<uint32_t, k_l2_task_data_allocs_t>;
using k_l2_task_data_allocs_pair_view_t = std::pair<uint32_t, k_l2_task_data_allocs_t>;
void DisplayL2DataInfo1(string title, const k_l2_data_allocs_t &input, rtL2Ctrl_t &l2ctrl, k_l2_data_allocs_t data) {
  int64_t page_size = GetL2Size() / GetL2PageNum();
  for (auto it = data.cbegin(); it != data.cend(); ++it) {
    FE_LOGD("%s: data_id = %lu, data_in_l2_id = %d, if_input = %u, data_size = %u", title.c_str(), it->first,
            it->second.data_in_l2_id, (uint32_t)(input.count(it->first)),
            l2ctrl.data[it->second.data_in_l2_id].L2_data_section_size);
    FE_LOGD("l2PageN = %lu, l2_addr0 = %lu, l2Addr=%lu, ddr_addr_key = %lu, ddr_addr = %lu", it->second.l2PageNum,
            it->second.data_in_l2_addr - (l2ctrl.data[it->second.data_in_l2_id].L2_page_offset_base) * page_size,
            it->second.data_in_l2_addr, it->first, l2ctrl.data[it->second.data_in_l2_id].L2_mirror_addr);
    FE_LOGD("offset = %2u, prev_offset = %2d", (uint32_t)l2ctrl.data[it->second.data_in_l2_id].L2_page_offset_base,
            (int32_t)l2ctrl.data[it->second.data_in_l2_id].prev_L2_page_offset_base);
  }
  return;
}

void L2FusionComm::DisplayL2AllocInfo(const k_l2_task_data_allocs_map_t &alloc) const {
  k_l2_task_data_allocs_map_view_t view;
  for (k_l2_task_data_allocs_map_t::const_iterator it = alloc.begin(); it != alloc.end(); ++it) {
    size_t find = it->first.find("_l2split");
    find = (find != string::npos) ? atoi((it->first.substr(find + 8)).c_str()) : 2048;
    uint32_t index = (it->second.node_id) | ((static_cast<uint32_t>(find)) << 16);
    view.insert(k_l2_task_data_allocs_pair_view_t(index, it->second));
  }

  for (k_l2_task_data_allocs_map_view_t::iterator it = view.begin(); it != view.end(); ++it) {
    rtL2Ctrl_t l2ctrl = it->second.l2ctrl;
    k_l2_task_data_allocs_t &l2_info = it->second;

    FE_LOGD("node %s, l2ctrl.size = %lu, input_num = %zu, output_num = %zu", l2_info.node_name.c_str(), l2ctrl.size,
            l2_info.input.size(), l2_info.output.size());
    FE_LOGD("convergeNum = %zu, standing_num = %zu", l2_info.converge.size(), l2_info.standing_data.size());

    DisplayL2DataInfo1("standing", l2_info.input, l2ctrl, l2_info.standing_data);
    DisplayL2DataInfo1("output", l2_info.input, l2ctrl, l2_info.output);
    DisplayL2DataInfo1("converge", l2_info.input, l2ctrl, l2_info.converge);
  }
  return;
}

CCE_DEFINE_SINGLETON(L2FusionComm);

}  // namespace fe

namespace fe {

bool CheckData(ge::OpDescPtr parent_op_desc) {
  if (parent_op_desc->GetType() == OP_TYPE_PLACE_HOLDER) {
    string parent_op_type;
    if (!ge::AttrUtils::GetStr(parent_op_desc, PARENT_OP_TYPE, parent_op_type)) {
      return false;
    }

    if (parent_op_type == fe::DATA) {
      return true;
    }
  }
  return false;
}
// nonvalid op include Const,Constant,End,PlaceHolder except that the
// parent_op_type is data.
bool IsNonValidOp(ge::NodePtr node) {
  bool is_non_valid = false;
  std::set<std::string> set;
  // add const op
  set.insert(fe::CONSTANT);
  set.insert(fe::CONSTANTOP);
  set.insert(fe::OP_TYPE_END);
  std::set<std::string>::const_iterator it = set.find(node->GetType());
  if (it != set.end()) {
    is_non_valid = true;
  }
  if (node->GetType() == fe::OP_TYPE_PLACE_HOLDER) {
    bool is_data = CheckData(node->GetOpDesc());
    if (!is_data) {
      is_non_valid = true;
    }
  }
  FE_LOGD("Node named %s, op type is %s, is_non_valid is %d.", node->GetName().c_str(), node->GetType().c_str(),
          is_non_valid);

  return is_non_valid;
}

bool IsNonValidOpOrData(ge::NodePtr node) {
  bool is_data = false;
  std::set<std::string> set;
  set.insert(fe::DATA);
  std::set<std::string>::const_iterator it = set.find(node->GetType());
  if (it != set.end()) {
    is_data = true;
  }
  if (node->GetType() == fe::OP_TYPE_PLACE_HOLDER) {
    is_data = CheckData(node->GetOpDesc());
  }
  FE_LOGD("Node named %s, op type is %s, is_data value is %d.", node->GetName().c_str(), node->GetType().c_str(),
          is_data);

  return (is_data || IsNonValidOp(node));
}
bool IsConstInput(const ge::ConstNodePtr &node, size_t index) {
  FE_CHECK(node == nullptr, FE_LOGW("node is null!"), return false);
  return IsConstInput(*node, index);
}
// const input include PlaceHolder nddes except that the parent_op_type is data.
bool IsConstInput(const ge::Node &node, const size_t index) {
  bool ret = true;
  if (index < node.GetAllInDataAnchors().size()) {
    for (const auto &anchor : node.GetAllInDataAnchors()) {
      if (anchor->GetIdx() != static_cast<int>(index)) {
        continue;
      }
      auto peer_anchor = anchor->GetPeerOutAnchor();
      if (!peer_anchor) {
        break;
      }
      auto owner_node = peer_anchor->GetOwnerNode();
      if (!owner_node) {
        break;
      }
      if (owner_node->GetOpDesc()->GetType() != OP_TYPE_PLACE_HOLDER) {
        return false;
      }
      ret = (!CheckData(owner_node->GetOpDesc()));
    }
  }
  return ret;
}

}  // namespace fe
