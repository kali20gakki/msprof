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

#include "common/fusion_op_comm.h"

#include <memory>

#include "common/configuration.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/sgt_slice_type.h"
#include "common/aicore_util_constants.h"
#include "common/aicore_util_attr_define.h"
#include "common/util/constants.h"
#include "common/util/op_info_util.h"
#include "common/op_info_common.h"
#include "common/fusion_statistic/fusion_statistic_writer.h"
#include "google/protobuf/text_format.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/detail/attributes_holder.h"
#include "graph/detail/model_serialize_imp.h"
#include "graph/model_serialize.h"
#include "graph/op_kernel_bin.h"
#include "graph/tuning_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/type_utils.h"
#include "proto/ge_ir.pb.h"
#include "register/graph_optimizer/buffer_fusion/buffer_fusion_constant.h"
using ge::AnchorPtr;
using ge::AttrUtils;
using ge::ComputeGraph;
using ge::DataAnchor;
using ge::GRAPH_FAILED;
using ge::GRAPH_SUCCESS;
using ge::graphStatus;
using ge::GraphUtils;
using ge::NodePtr;
using ge::OpDesc;

namespace fe {
const string ORIGINAL_NODES = "original_nodes";
const std::string  MIX_AIC_PREFIX = "_mix_aic";
const std::string  MIX_AIV_PREFIX = "_mix_aiv";
inline int64_t GetImplyTypeToFusionOp(vector<ge::NodePtr> &fus_nodelist) {
  int64_t imply_type = -1;
  for (ge::NodePtr node : fus_nodelist) {
    if (AttrUtils::GetInt(node->GetOpDesc(), FE_IMPLY_TYPE, imply_type)) {
      return imply_type;
    }
  }
  return imply_type;
}

ge::OpDescPtr FusionOpComm::CreateFusionOp(vector<ge::NodePtr> &fus_nodelist, const string &engine_name) {
  if (fus_nodelist.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][PostProcess][CrtFusOp] fusNodelist is empty.");
    return nullptr;
  }

  ge::NodePtr first_node = fus_nodelist[0];
  FE_CHECK(first_node == nullptr,
           REPORT_FE_ERROR("[SubGraphOpt][PostProcess][CrtFusOp] CreateFusionOp nullptr Pointer."), return nullptr);
  ge::OpDescPtr fus_opdef = std::shared_ptr<OpDesc>(new (std::nothrow) OpDesc());
  FE_CHECK(fus_opdef == nullptr,
           REPORT_FE_ERROR("[SubGraphOpt][PostProcess][CrtFusOp] CreateFusionOp nullptr Pointer."), return nullptr);

  std::string fusion_node_name;
  fusion_node_name.clear();

  for (ge::NodePtr &node : fus_nodelist) {
    FE_CHECK(node == nullptr, REPORT_FE_ERROR("[SubGraphOpt][PostProcess][CrtFusOp] pScopeAllocator is nullptr."),
             return nullptr);
    fusion_node_name += node->GetOpDesc()->GetName();
    if (fusion_node_name.size() > kMaxOpNmaLen) {
      fusion_node_name = first_node->GetOpDesc()->GetName() + "_ub_fusion";
      break;
    }
  }
  fus_opdef->SetName(fusion_node_name);

  ge::OpDescPtr first_op_desc = first_node->GetOpDesc();
  if (CopyFusionScopeAttr(first_op_desc, fus_opdef) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][PostProcess][CrtFusOp] Failed to set fusion scope attr for Op[%s, optype[%s]].",
                    fus_opdef->GetName().c_str(), fus_opdef->GetType().c_str());
    return nullptr;
  }

  // copy pass_name
  string pass_name;
  if (ge::AttrUtils::GetStr(first_op_desc, PASS_NAME_ATTR, pass_name)) {
    if (!ge::AttrUtils::SetStr(fus_opdef, PASS_NAME_ATTR, pass_name)) {
      REPORT_FE_ERROR("[SubGraphOpt][PostProcess][CrtFusOp] Op[%s, optype[%s]]: fail to set the attribute %s.",
                      first_op_desc->GetName().c_str(), first_op_desc->GetType().c_str(), PASS_NAME_ATTR.c_str());
      return nullptr;
    }
  }

  // copy session graph id
  string session_graph_id;
  if (ge::AttrUtils::GetStr(first_op_desc, ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id)) {
    if (!ge::AttrUtils::SetStr(fus_opdef, ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id)) {
      REPORT_FE_ERROR("[SubGraphOpt][PostProcess][CrtFusOp] Op[%s]: fail to set the attribute %s.",
                      fusion_node_name.c_str(), ge::ATTR_NAME_SESSION_GRAPH_ID.c_str());
      return nullptr;
    }
  }

  int64_t imply_type = GetImplyTypeToFusionOp(fus_nodelist);
  FE_CHECK(imply_type == -1,
           REPORT_FE_ERROR("[SubGraphOpt][PostProcess][CrtFusOp] The imply_type -1 is invalid, it must be in [0,11]."),
           return nullptr);

  if (!AttrUtils::SetInt(fus_opdef, FE_IMPLY_TYPE, imply_type)) {
    REPORT_FE_ERROR("[SubGraphOpt][PostProcess][CrtFusOp] Op[%s]: set imply_type failed, the imply_type is %ld.",
                    fusion_node_name.c_str(), imply_type);
    return nullptr;
  }

  FE_LOGD("Op[%s]: the implytype is %ld", fusion_node_name.c_str(), imply_type);

  ffts::ThreadSliceMapPtr slice_info_ptr = nullptr;
  slice_info_ptr = first_node->GetOpDesc()->TryGetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr);
  if (!slice_info_ptr || !slice_info_ptr->thread_mode) {
    return SetTBEFusionOp(fus_nodelist, fus_opdef, engine_name);
  } else {
    return SetMultiKernelTBEFusionOp(fus_nodelist, fus_opdef, engine_name);
  }

}

inline void SetListStrAttrToFusionOp(const vector<ge::NodePtr> &fus_nodelist, const ge::OpDescPtr &fus_opdef,
                                     const string &attr_name) {
  vector<string> str_val;
  for (const ge::NodePtr &node : fus_nodelist) {
    if (ge::AttrUtils::GetListStr(node->GetOpDesc(), attr_name, str_val)) {
      (void)ge::AttrUtils::SetListStr(fus_opdef, attr_name, str_val);
      break;
    }
  }
}

inline void SetStrAttrToFusionOp(const vector<ge::NodePtr> &fus_nodelist, const ge::OpDescPtr &fus_opdef,
                                 const string &attr_name) {
  string str_val;
  for (const ge::NodePtr &node : fus_nodelist) {
    if (ge::AttrUtils::GetStr(node->GetOpDesc(), attr_name, str_val)) {
      (void)ge::AttrUtils::SetStr(fus_opdef, attr_name, str_val);
      break;
    }
  }
}

inline void SetStrVecAttrToFusionOp(const vector<ge::NodePtr> &fus_nodelist, const ge::OpDescPtr &fus_opdef,
                                    const string &attr_name) {
  vector<string> str_vec_val;
  for (const ge::NodePtr &node : fus_nodelist) {
    if (ge::AttrUtils::GetListStr(node->GetOpDesc(), attr_name, str_vec_val)) {
      (void)ge::AttrUtils::SetListStr(fus_opdef, attr_name, str_vec_val);
      break;
    }
  }
}

inline void SetListListInt64AttrToFusionOp(const vector<ge::NodePtr> &fus_nodelist,
                                           const ge::OpDescPtr &fus_opdef,
                                           const string &attr_name) {
  vector<vector<int64_t>> int_val;
  for (const ge::NodePtr &node : fus_nodelist) {
    if (ge::AttrUtils::GetListListInt(node->GetOpDesc(), attr_name, int_val)) {
      (void)ge::AttrUtils::SetListListInt(fus_opdef, attr_name, int_val);
      break;
    }
  }
}

inline void SetListInt64AttrToFusionOp(const vector<ge::NodePtr> &fus_nodelist,
                                       const ge::OpDescPtr &fus_opdef,
                                       const string &attr_name) {
  vector<int64_t> int_val;
  for (const ge::NodePtr &node : fus_nodelist) {
    if (ge::AttrUtils::GetListInt(node->GetOpDesc(), attr_name, int_val)) {
      (void)ge::AttrUtils::SetListInt(fus_opdef, attr_name, int_val);
      break;
    }
  }
}

inline void SetInt64AttrToFusionOp(const vector<ge::NodePtr> &fus_nodelist, const ge::OpDescPtr &fus_opdef,
                                   const string &attr_name) {
  int64_t int_val;
  for (const ge::NodePtr &node : fus_nodelist) {
    if (ge::AttrUtils::GetInt(node->GetOpDesc(), attr_name, int_val)) {
      (void)ge::AttrUtils::SetInt(fus_opdef, attr_name, int_val);
      break;
    }
  }
}

inline void SetListBytesAttrToFusionOp(const vector<ge::NodePtr> &fus_nodelist,
                                       const ge::OpDescPtr &fus_opdef,
                                       const string &attr_name) {
  vector<ge::Buffer> byte_val;
  for (const ge::NodePtr &node : fus_nodelist) {
    if (ge::AttrUtils::GetListBytes(node->GetOpDesc(), attr_name, byte_val)) {
      (void)ge::AttrUtils::SetListBytes(fus_opdef, attr_name, byte_val);
      break;
    }
  }
}

inline void SetBytesAttrToFusionOp(const vector<ge::NodePtr> &fus_nodelist,
                                   const ge::OpDescPtr &fus_opdef,
                                   const string &attr_name) {
  ge::Buffer int_val;
  for (const ge::NodePtr &node : fus_nodelist) {
    if (ge::AttrUtils::GetBytes(node->GetOpDesc(), attr_name, int_val)) {
      (void)ge::AttrUtils::SetBytes(fus_opdef, attr_name, int_val);
      FE_LOGD("Tbe tbe_kernel_buffer_ size:%lu.", int_val.GetSize());
      break;
    }
  }
}

inline void SetListBoolAttrToFusionOp(const vector<ge::NodePtr> &fus_nodelist,
                                      const ge::OpDescPtr &fus_opdef,
                                      const string &attr_name) {
  vector<bool> bool_val;
  for (const ge::NodePtr &node : fus_nodelist) {
    if (ge::AttrUtils::GetListBool(node->GetOpDesc(), attr_name, bool_val)) {
      (void)ge::AttrUtils::SetListBool(fus_opdef, attr_name, bool_val);
      break;
    }
  }
}

inline void SetBoolAttrToFusionOp(const vector<ge::NodePtr> &fus_nodelist,
                                  const ge::OpDescPtr &fus_opdef,
                                  const string &attr_name) {
  bool bool_val = false;
  for (const ge::NodePtr &node : fus_nodelist) {
    if (ge::AttrUtils::GetBool(node->GetOpDesc(), attr_name, bool_val)) {
      (void)ge::AttrUtils::SetBool(fus_opdef, attr_name, bool_val);
      break;
    }
  }
}

inline void SetStrPathToFusionOp(const vector<ge::NodePtr> &fus_nodelist,
                                 const ge::OpDescPtr &fus_opdef,
                                 const string &path_name) {
    string path_str;
    for (const ge::NodePtr &node : fus_nodelist) {
        if (node->GetOpDesc() == nullptr) {
          break;
        }
        path_str = node->GetOpDesc()->TryGetExtAttr(path_name, string(""));
        if (path_str.empty()) {
            fus_opdef->SetExtAttr(path_name, path_str);
            break;
        }
    }
}

inline void SetMemAttrToFusionOp(const vector<ge::NodePtr> &fus_nodelist,
                                 const ge::OpDescPtr &fus_opdef, const string &attr_name) {
  bool bool_val = false;
  for (const ge::NodePtr &node : fus_nodelist) {
    if (ge::AttrUtils::GetBool(node->GetOpDesc(), attr_name, bool_val) && bool_val) {
      (void)ge::AttrUtils::SetBool(fus_opdef, attr_name, bool_val);
      break;
    }
  }
}

inline void SetKernelNameToFusionOp(const vector<ge::NodePtr> &fus_nodelist,
                                    const ge::OpDescPtr &fus_opdef) {
  string kernel_name;
  string kernel_name1;
  for (const ge::NodePtr &node : fus_nodelist) {
    bool flag = ge::AttrUtils::GetStr(node->GetOpDesc(), MIX_AIC_PREFIX + node->GetOpDesc()->GetName() +
                                      "_kernelname", kernel_name);
    flag = flag && ge::AttrUtils::GetStr(node->GetOpDesc(), MIX_AIV_PREFIX + node->GetOpDesc()->GetName() +
                                         "_kernelname", kernel_name1);
    if (flag) {
      (void)ge::AttrUtils::SetStr(fus_opdef, MIX_AIC_PREFIX + fus_opdef->GetName() + "_kernelname", kernel_name);
      (void)ge::AttrUtils::SetStr(fus_opdef, MIX_AIV_PREFIX + fus_opdef->GetName() + "_kernelname", kernel_name1);
      break;
    }
    if (ge::AttrUtils::GetStr(node->GetOpDesc(), node->GetOpDesc()->GetName() + "_kernelname", kernel_name)) {
      (void)ge::AttrUtils::SetStr(fus_opdef, fus_opdef->GetName() + "_kernelname", kernel_name);
      break;
    }
  }
}

inline void SetMultiKernelThreadKernelNameToFusionOp(const vector<ge::NodePtr> &fus_nodelist,
                                                     const ge::OpDescPtr &fus_opdef) {
  vector<string> kernel_names;
  for (const ge::NodePtr &node : fus_nodelist) {
    if (ge::AttrUtils::GetListStr(node->GetOpDesc(), kThreadKernelName, kernel_names)) {
      (void)ge::AttrUtils::SetListStr(fus_opdef, kThreadKernelName, kernel_names);
      break;
    }
  }
}


inline void SetL1FusionSubGraphNoToFusionOp(const vector<ge::NodePtr> &fus_nodelist,
                                            const ge::OpDescPtr &fus_opdef) {
  string attr_name_l1_fusion_sub_graph_no = "_L1_fusion_sub_graph_no";
  string str_value;
  for (const ge::NodePtr &node : fus_nodelist) {
    if (ge::AttrUtils::GetStr(node->GetOpDesc(), attr_name_l1_fusion_sub_graph_no, str_value)) {
      (void)ge::AttrUtils::SetStr(fus_opdef, attr_name_l1_fusion_sub_graph_no, str_value);
      break;
    }
  }
}

inline void SetNoTaskAndDumpNeeded(const vector<ge::NodePtr> &fus_nodelist, const ge::OpDescPtr &fus_opdef) {
  bool str_value = false;
  for (const ge::NodePtr &node : fus_nodelist) {
    if (ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NO_TASK_AND_DUMP_NEEDED, str_value)) {
      (void)ge::AttrUtils::SetBool(fus_opdef, ge::ATTR_NO_TASK_AND_DUMP_NEEDED, str_value);
      break;
    }
  }
}

inline void SetSqrtmodeToFusionOp(const vector<ge::NodePtr> &fus_nodelist, const ge::OpDescPtr &fus_opdef) {
  bool sqrt_mode = false;
  string attr_name = "sqrt_mode";
  string attr_name_quant = "quant_sqrt_mode";
  string attr_name_dequant = "dequant_sqrt_mode";
  string quant_op = "AscendQuant";
  string dequant_op = "AscendDequant";

  for (const ge::NodePtr &node : fus_nodelist) {
    if (ge::AttrUtils::GetBool(node->GetOpDesc(), attr_name, sqrt_mode)) {
      if (node->GetType() == quant_op || node->GetType() == dequant_op) {
        string attr_name_tmp = node->GetType() == quant_op ? attr_name_quant : attr_name_dequant;
        (void)ge::AttrUtils::SetBool(fus_opdef, attr_name_tmp, sqrt_mode);
      }
    }
  }

  return;
}

inline void SetBoolAttrToFusionOp(const ge::NodePtr &node_ptr, const ge::OpDescPtr &fus_opdef,
                                  const string &attr_name) {
  bool bool_val = false;
  if (ge::AttrUtils::GetBool(node_ptr->GetOpDesc(), attr_name, bool_val)) {
    (void)ge::AttrUtils::SetBool(fus_opdef, attr_name, bool_val);
  }
}

inline void SetStrAttrToFusionOp(const ge::NodePtr &node_ptr, const ge::OpDescPtr &fus_opdef,
                                 const string &attr_name) {
  string str_val;
  if (ge::AttrUtils::GetStr(node_ptr->GetOpDesc(), attr_name, str_val)) {
    (void)ge::AttrUtils::SetStr(fus_opdef, attr_name, str_val);
  }
}

inline void SetL1InfoToFusionOp(const vector<ge::NodePtr> &fus_nodelist, const ge::OpDescPtr &fus_opdef) {
  if (fus_nodelist.empty()) {
    FE_LOGD("fusNodelist is empty");
    return;
  }
  ge::NodePtr node = fus_nodelist[0];
  FE_CHECK(node == nullptr, FE_LOGD("node is nullptr"), return);

  // ATTR_NAME_L1_FUSION_GROUP_ID,
  // L1_OPTIMIZED,ATTR_NAME_FUSION_GROUP_TYPE,TVM_ATTR_NAME_WORKSPACE_TYPE
  int64_t groupid;

  std::vector<int64_t> l1_output_offset;
  if (ge::AttrUtils::GetInt(node->GetOpDesc(), ge::ATTR_NAME_L1_FUSION_GROUP_ID, groupid)) {
    (void)ge::AttrUtils::SetInt(fus_opdef, ge::ATTR_NAME_L1_FUSION_GROUP_ID, groupid);
  }
  SetBoolAttrToFusionOp(node, fus_opdef, NEED_RE_PRECOMPILE);
  SetStrAttrToFusionOp(node, fus_opdef, ge::ATTR_NAME_FUSION_GROUP_TYPE);
  SetStrAttrToFusionOp(node, fus_opdef, ge::ATTR_NAME_CONTINUOUS_STREAM_LABEL);

  std::vector<int32_t> tvm_workspace_types;
  if (ge::AttrUtils::GetListInt(node->GetOpDesc(), ge::TVM_ATTR_NAME_WORKSPACE_TYPE, tvm_workspace_types)) {
    (void)ge::AttrUtils::SetListInt(fus_opdef, ge::TVM_ATTR_NAME_WORKSPACE_TYPE, tvm_workspace_types);
  }

  for (const ge::NodePtr &node_tmp : fus_nodelist) {
    std::vector<int64_t> l1_output_offset_temp;
    if (ge::AttrUtils::GetListInt(node_tmp->GetOpDesc(), ge::ATTR_NAME_OUTPUT_OFFSET_FOR_BUFFER_FUSION,
                                  l1_output_offset_temp)) {
      for (auto &offset : l1_output_offset_temp) {
        l1_output_offset.push_back(offset);
      }
    }
  }
  (void)ge::AttrUtils::SetListInt(fus_opdef, ge::ATTR_NAME_OUTPUT_OFFSET_FOR_BUFFER_FUSION, l1_output_offset);
  SetBoolAttrToFusionOp(fus_nodelist, fus_opdef, kLxNoReuseMemFlag);
  int32_t output_real_calc_flag = 0;
  for (const ge::NodePtr &node_to_fuse : fus_nodelist) {
    if (ge::AttrUtils::GetInt(node_to_fuse->GetOpDesc(), ge::ATTR_NAME_GET_TENSOR_ACTUAL_SIZE, output_real_calc_flag)) {
      (void)ge::AttrUtils::SetInt(fus_opdef, ge::ATTR_NAME_GET_TENSOR_ACTUAL_SIZE, output_real_calc_flag);
      FE_LOGI("L1Fusion -- AddFusionConcatEdge : [%d]", output_real_calc_flag);
      break;
    }
  }
  return;
}

inline void SetL2InfoToFusionOp(const vector<ge::NodePtr> &fus_nodelist, const ge::OpDescPtr &fus_opdef) {
  if (fus_nodelist.empty()) {
    FE_LOGD("fusNodelist is empty");
    return;
  }
  ge::NodePtr node = fus_nodelist[0];
  FE_CHECK(node == nullptr, FE_LOGD("node is nullptr"), return);

  // ATTR_NAME_L2_FUSION_GROUP_ID,
  // L2_OPTIMIZED,ATTR_NAME_FUSION_GROUP_TYPE,ATTR_NAME_CONTINUOUS_STREAM_LABEL
  int64_t groupid;
  string continuous_stream_label;
  if (ge::AttrUtils::GetInt(node->GetOpDesc(), ge::ATTR_NAME_L2_FUSION_GROUP_ID, groupid)) {
    (void)ge::AttrUtils::SetInt(fus_opdef, ge::ATTR_NAME_L2_FUSION_GROUP_ID, groupid);
  }

  SetBoolAttrToFusionOp(node, fus_opdef, NEED_RE_PRECOMPILE);
  SetStrAttrToFusionOp(node, fus_opdef, ge::ATTR_NAME_FUSION_GROUP_TYPE);
  SetStrAttrToFusionOp(node, fus_opdef, ge::ATTR_NAME_CONTINUOUS_STREAM_LABEL);

  SetBoolAttrToFusionOp(fus_nodelist, fus_opdef, kLxNoReuseMemFlag);
  int32_t output_real_calc_flag = 0;
  for (const ge::NodePtr &node_tmp : fus_nodelist) {
    if (ge::AttrUtils::GetInt(node_tmp->GetOpDesc(), ge::ATTR_NAME_GET_TENSOR_ACTUAL_SIZE, output_real_calc_flag)) {
      (void)ge::AttrUtils::SetInt(fus_opdef, ge::ATTR_NAME_GET_TENSOR_ACTUAL_SIZE, output_real_calc_flag);
      break;
    }
  }
  return;
}

inline void RefreshFusionOpType(const ge::OpDescPtr &fus_opdef, const ge::NodePtr &first_node) {
  if (first_node == nullptr || first_node->GetOpDesc() == nullptr) {
    return;
  }
  std::string fusion_op_type;
  (void)ge::AttrUtils::GetStr(first_node->GetOpDesc(), UB_FUSION_OP_TYPE, fusion_op_type);
  if (!fusion_op_type.empty()) {
    fus_opdef->SetType(fusion_op_type);
  }
}
ge::OpDescPtr FusionOpComm::SetMultiKernelTBEFusionOp(const vector<ge::NodePtr> &fus_nodelist,
                                                      ge::OpDescPtr &fus_opdef,
                                                      const string &engine_name) {
  if (fus_nodelist.empty()) {
    FE_LOGE("fusNodelist is empty.");
    return nullptr;
  }
  ge::NodePtr first_node = fus_nodelist[0];
  for (const auto &node : fus_nodelist) {
    if (node->GetOpDesc()->HasAttr(ge::TVM_ATTR_NAME_THREAD_MAGIC)) {
      FE_LOGD("Op[%s, %s] has attr TVM_ATTR_NAME_THREAD_MAGIC.",
              node->GetOpDesc()->GetName().c_str(),  node->GetOpDesc()->GetType().c_str());
      first_node = node;
      break;
    }
  }

  // AddTvmNode
  AddTvmNode(fus_opdef);

  // 1 type same with fusion node_list
  fus_opdef->SetType(first_node->GetOpDesc()->GetType());
  (void)ge::AttrUtils::SetBool(fus_opdef, ATTR_NAME_IS_COMPIED_FUSION_OP, true);

  // set json infos to fusion op
  SetStrPathToFusionOp(fus_nodelist, fus_opdef, ge::ATTR_NAME_OP_FILE_PATH);
  SetBoolAttrToFusionOp(fus_nodelist, fus_opdef, AIPP_CONV_FLAG);
  SetListStrAttrToFusionOp(fus_nodelist, fus_opdef, ge::TVM_ATTR_NAME_THREAD_MAGIC);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, FUSION_OP_SLICE_INFO);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, ge::ATTR_NAME_ENGINE_NAME_FOR_LX);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, ge::ATTR_NAME_KKERNEL_LIB_NAME_FOR_LX);
  SetListInt64AttrToFusionOp(fus_nodelist, fus_opdef, ATTR_NAME_THREAD_TBE_KERNEL_SIZE);
  SetListBytesAttrToFusionOp(fus_nodelist, fus_opdef, ge::ATTR_NAME_THREAD_TBE_KERNEL_BUFFER);
  SetListInt64AttrToFusionOp(fus_nodelist, fus_opdef, ge::TVM_ATTR_NAME_THREAD_BLOCKDIM);
  SetBoolAttrToFusionOp(fus_nodelist, fus_opdef, NEED_RE_PRECOMPILE);
  SetListBoolAttrToFusionOp(fus_nodelist, fus_opdef, ge::TVM_ATTR_NAME_THREAD_N_BATCH_SPLIT);
  SetMultiKernelThreadKernelNameToFusionOp(fus_nodelist, fus_opdef);
  SetInt64AttrToFusionOp(fus_nodelist, fus_opdef, FE_IMPLY_TYPE);

  SetListStrAttrToFusionOp(fus_nodelist, fus_opdef, ge::TVM_ATTR_NAME_THREAD_METADATA);
  SetListListInt64AttrToFusionOp(fus_nodelist, fus_opdef, TBE_OP_THREAD_ATOMIC_OUTPUT_INDEX);
  SetListInt64AttrToFusionOp(fus_nodelist, fus_opdef, TBE_OP_THREAD_ATOMIC_WORKSPACE_FLAG);
  SetListInt64AttrToFusionOp(fus_nodelist, fus_opdef, ATTR_NAME_THREAD_TVM_CACHE_READ_MODE);
  SetBoolAttrToFusionOp(fus_nodelist, fus_opdef, ATTR_NAME_FE_WEIGHT_COMPRESS);

  SetMemAttrToFusionOp(fus_nodelist, fus_opdef, ge::ATTR_NAME_CONTINUOUS_INPUT);
  SetMemAttrToFusionOp(fus_nodelist, fus_opdef, ge::ATTR_NAME_CONTINUOUS_OUTPUT);
  SetSqrtmodeToFusionOp(fus_nodelist, fus_opdef);
  SetDataDumpAttrToFusionOp(fus_nodelist, fus_opdef);

  // set attrs related to unknown shape
  SetBoolAttrToFusionOp(fus_nodelist, fus_opdef, ATTR_NAME_SUPPORT_DYNAMIC_SHAPE);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, COMPILE_INFO_JSON);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, COMPILE_INFO_KEY);

  SetListInt64AttrToFusionOp(fus_nodelist, fus_opdef, OP_THREAD_PARA_SIZE);
  SetBoolAttrToFusionOp(fus_nodelist, fus_opdef, ATTR_NAME_LX_FUSION_PASS);
  SetStrVecAttrToFusionOp(fus_nodelist, fus_opdef, ATTR_NAME_OP_INFER_DEPENDS);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, ATTR_NAME_KERNEL_LIST_FIRST_NAME);
  SetBoolAttrToFusionOp(fus_nodelist, fus_opdef, ge::ATTR_NAME_REFERENCE);

  // for ffts_plus
  SetBoolAttrToFusionOp(fus_nodelist, fus_opdef, kTypeFFTSPlus);
  SetListStrAttrToFusionOp(fus_nodelist, fus_opdef, ATTR_NAME_THREAD_CUBE_VECTOR_CORE_TYPE);

  if (Configuration::Instance(engine_name).EnableL1Fusion()) {
    SetL1InfoToFusionOp(fus_nodelist, fus_opdef);
    SetL1FusionSubGraphNoToFusionOp(fus_nodelist, fus_opdef);
    SetNoTaskAndDumpNeeded(fus_nodelist, fus_opdef);
  }
  std::string build_mode_value = Configuration::Instance(engine_name).GetGeContextOptionValue(ge::BUILD_MODE);
  if (Configuration::Instance(engine_name).GetBufferFusionMode() == EN_L2_FUSION ||
      build_mode_value == ge::BUILD_MODE_TUNING) {
    SetL2InfoToFusionOp(fus_nodelist, fus_opdef);
    SetNoTaskAndDumpNeeded(fus_nodelist, fus_opdef);
  }
  if (Configuration::Instance(engine_name).GetDumpOriginalNodesEnable()) {
    SetOriginalNodesOpDescToFusionOp(fus_nodelist, fus_opdef);
  }

  vector<int64_t> work_space_bytes = first_node->GetOpDesc()->GetWorkspaceBytes();
  fus_opdef->SetWorkspaceBytes(work_space_bytes);

  vector<int64_t> work_space = first_node->GetOpDesc()->GetWorkspace();
  fus_opdef->SetWorkspace(work_space);

  std::vector<ge::OpKernelBinPtr> tbe_kernel_ptr_vec =
      first_node->GetOpDesc()->TryGetExtAttr(ge::OP_EXTATTR_NAME_THREAD_TBE_KERNEL, vector<ge::OpKernelBinPtr>());
  if (tbe_kernel_ptr_vec.empty()) {
    FE_LOGE("Op[%s,optype[%s]]: the tbe_kernel_ptr_old is nullptr.", first_node->GetOpDesc()->GetName().c_str(),
            first_node->GetOpDesc()->GetType().c_str());
    return nullptr;
  }
  std::vector<ge::OpKernelBinPtr> list_buffer_vec;
  for (auto &tbe_kernel_ptr : tbe_kernel_ptr_vec) {
    std::string kernel_name;
    ge::AttrUtils::GetStr(first_node->GetOpDesc(), first_node->GetOpDesc()->GetName() + "_kernelname", kernel_name);
    auto buffer_size = tbe_kernel_ptr->GetBinDataSize();
    std::vector<char> buffer_vec(tbe_kernel_ptr->GetBinData(), tbe_kernel_ptr->GetBinData() + buffer_size);
    ge::OpKernelBinPtr tbe_kernel_ptr_fused =
        std::make_shared<ge::OpKernelBin>(kernel_name, std::move(buffer_vec));
    list_buffer_vec.emplace_back(tbe_kernel_ptr_fused);
  }

  if (!list_buffer_vec.empty()) {
    fus_opdef->SetExtAttr(ge::OP_EXTATTR_NAME_THREAD_TBE_KERNEL, list_buffer_vec);
  } else {
    FE_LOGW("list buffer vector of op %s is empty.", fus_opdef->GetName().c_str());
  }
  (void)ge::AttrUtils::SetStr(fus_opdef, ge::ATTR_NAME_TBE_KERNEL_NAME, fus_opdef->GetName());
  fus_opdef->SetOpEngineName(first_node->GetOpDesc()->GetOpEngineName());
  fus_opdef->SetOpKernelLibName(first_node->GetOpDesc()->GetOpKernelLibName());
  return fus_opdef;
}

Status FusionOpComm::CopyFusionScopeAttr(const ge::OpDescPtr &src_op_desc, ge::OpDescPtr &dst_op_desc) const {
  if (src_op_desc == nullptr || dst_op_desc == nullptr) {
    return FAILED;
  }
  int64_t scope_id = 0;
  bool is_l1_fusion = false;
  if (!GetFusionScopeAttr(src_op_desc, scope_id, is_l1_fusion)) {
    FE_LOGE("Failed to get scope attr from node[%s, %s].",
            src_op_desc->GetName().c_str(), src_op_desc->GetType().c_str());
    return FAILED;
  }
  if (is_l1_fusion) {
    if (!ge::AttrUtils::SetInt(dst_op_desc, L1_SCOPE_ID_ATTR, scope_id)) {
      FE_LOGE("Failed to set fusion scope attr[%s] for Op[%s, %s].",
              L1_SCOPE_ID_ATTR.c_str(), dst_op_desc->GetName().c_str(), dst_op_desc->GetType().c_str());
      return FAILED;
    }
  } else {
    if (!ge::AttrUtils::SetInt(dst_op_desc, SCOPE_ID_ATTR, scope_id)) {
      FE_LOGE("Failed to set fusion scope attr[%s] for Op[%s, %s].",
              SCOPE_ID_ATTR.c_str(), dst_op_desc->GetName().c_str(), dst_op_desc->GetType().c_str());
      return FAILED;
    }
  }
  if (ge::AttrUtils::GetInt(src_op_desc, FAILED_L1_SCOPE_ID_ATTR, scope_id)) {
    (void)ge::AttrUtils::SetInt(dst_op_desc, FAILED_L1_SCOPE_ID_ATTR, scope_id);
  }
  return SUCCESS;
}
Status SetTbeKernelBin(const ge::OpDescPtr &fus_opdef, const ge::NodePtr node, const string &attr_name,
                       const string &key_name)
{
  FE_LOGD("Set kernel bin of node:%s, attr:%s, key_name:%s", node->GetName().c_str(), attr_name.c_str(),
          key_name.c_str());
  ge::OpKernelBinPtr tbe_kernel_ptr_old =
      node->GetOpDesc()->TryGetExtAttr(attr_name, ge::OpKernelBinPtr());
  FE_CHECK(tbe_kernel_ptr_old == nullptr,
           REPORT_FE_ERROR("[MergeFusionNode][SetTbeKernelBin] Op[%s]: the tbe_kernel_ptr_old is nullptr.",
                           node->GetName().c_str()), return FAILED);
  auto buffer_size = tbe_kernel_ptr_old->GetBinDataSize();
  std::vector<char> buffer_vec(tbe_kernel_ptr_old->GetBinData(), tbe_kernel_ptr_old->GetBinData() + buffer_size);
  ge::OpKernelBinPtr tbe_kernel_ptr_fused = nullptr;
  FE_MAKE_SHARED(tbe_kernel_ptr_fused =
                     std::make_shared<ge::OpKernelBin>(key_name, std::move(buffer_vec)), return FAILED);
  if (tbe_kernel_ptr_fused) {
    fus_opdef->SetExtAttr(attr_name, tbe_kernel_ptr_fused);
  }
  return SUCCESS;
}

Status SetNormalFusionOpAttr(const vector<ge::NodePtr> &fus_nodelist, ge::OpDescPtr &fus_opdef,
                             const ge::NodePtr first_node)
{
  FE_LOGD("Set normal fusion node attribute.");
  std::string kernel_name;
  ge::AttrUtils::GetStr(first_node->GetOpDesc(), first_node->GetOpDesc()->GetName() + "_kernelname", kernel_name);
  if (SetTbeKernelBin(fus_opdef, first_node, ge::OP_EXTATTR_NAME_TBE_KERNEL, kernel_name) != SUCCESS) {
    return FAILED;
  }
  (void)ge::AttrUtils::SetStr(fus_opdef, ge::ATTR_NAME_TBE_KERNEL_NAME, fus_opdef->GetName());
  SetInt64AttrToFusionOp(fus_nodelist, fus_opdef, ATTR_NAME_TBE_KERNEL_SIZE);
  SetBytesAttrToFusionOp(fus_nodelist, fus_opdef, ge::ATTR_NAME_TBE_KERNEL_BUFFER);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, ge::TVM_ATTR_NAME_METADATA);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, ATTR_NAME_KERNEL_LIST_FIRST_NAME);
  return SUCCESS;
}

Status SetMixFusionOpAttr(const vector<ge::NodePtr> &fus_nodelist, ge::OpDescPtr &fus_opdef,
                          const ge::NodePtr first_node)
{
  FE_LOGD("Set mix fusion node attribute.");
  SetInt64AttrToFusionOp(fus_nodelist, fus_opdef, MIX_AIC_PREFIX + ATTR_NAME_TBE_KERNEL_SIZE);
  SetInt64AttrToFusionOp(fus_nodelist, fus_opdef, MIX_AIV_PREFIX + ATTR_NAME_TBE_KERNEL_SIZE);
  SetBytesAttrToFusionOp(fus_nodelist, fus_opdef, MIX_AIC_PREFIX + ge::ATTR_NAME_TBE_KERNEL_BUFFER);
  SetBytesAttrToFusionOp(fus_nodelist, fus_opdef, MIX_AIV_PREFIX + ge::ATTR_NAME_TBE_KERNEL_BUFFER);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, MIX_AIC_PREFIX + ge::TVM_ATTR_NAME_METADATA);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, MIX_AIV_PREFIX + ge::TVM_ATTR_NAME_METADATA);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, MIX_AIC_PREFIX + ge::ATTR_NAME_TBE_KERNEL_NAME);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, MIX_AIV_PREFIX + ge::ATTR_NAME_TBE_KERNEL_NAME);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, MIX_AIC_PREFIX + ATTR_NAME_KERNEL_LIST_FIRST_NAME);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, MIX_AIV_PREFIX + ATTR_NAME_KERNEL_LIST_FIRST_NAME);
  std::string kernel_name;
  (void)ge::AttrUtils::GetStr(first_node->GetOpDesc(), MIX_AIC_PREFIX + first_node->GetOpDesc()->GetName() +
                              "_kernelname", kernel_name);
  if (SetTbeKernelBin(fus_opdef, first_node, MIX_AIC_PREFIX + ge::OP_EXTATTR_NAME_TBE_KERNEL, kernel_name) != SUCCESS) {
    return FAILED;
  }
  (void)ge::AttrUtils::GetStr(first_node->GetOpDesc(), MIX_AIV_PREFIX + first_node->GetOpDesc()->GetName() +
                              "_kernelname", kernel_name);
  if (SetTbeKernelBin(fus_opdef, first_node, MIX_AIV_PREFIX + ge::OP_EXTATTR_NAME_TBE_KERNEL, kernel_name) != SUCCESS) {
    return FAILED;
  }
  SetInt64AttrToFusionOp(fus_nodelist, fus_opdef, kTaskRadio);
  SetInt64AttrToFusionOp(fus_nodelist, fus_opdef, kModeInArgsFirstField);
  std::string mix_l2;
  if (ge::AttrUtils::GetStr(first_node->GetOpDesc(), ATTR_NAME_ALIAS_ENGINE_NAME, mix_l2)) {
    (void)ge::AttrUtils::SetStr(fus_opdef, ATTR_NAME_ALIAS_ENGINE_NAME, mix_l2);
  }
  return SUCCESS;
}

ge::OpDescPtr FusionOpComm::SetTBEFusionOp(const vector<ge::NodePtr> &fus_nodelist,
                                           ge::OpDescPtr &fus_opdef,
                                           const string &engine_name) {
  if (fus_nodelist.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][PostProcess][SetTBEFusOp] fusNodelist is empty.");
    return nullptr;
  }
  ge::NodePtr first_node = fus_nodelist[0];
  for (const auto &node : fus_nodelist) {
    if (node->GetOpDesc()->HasAttr(ge::TVM_ATTR_NAME_MAGIC)) {
      FE_LOGD("Op[%s, %s] has attr TVM_ATTR_NAME_MAGIC.",
              node->GetOpDesc()->GetName().c_str(),  node->GetOpDesc()->GetType().c_str());
      first_node = node;
      break;
    }
  }

  // AddTvmNode
  AddTvmNode(fus_opdef);

  // 1 type same with fusion node_list
  fus_opdef->SetType(first_node->GetOpDesc()->GetType());
  RefreshFusionOpType(fus_opdef, first_node);
  (void)ge::AttrUtils::SetBool(fus_opdef, ATTR_NAME_IS_COMPIED_FUSION_OP, true);

  // set json infos to fusion op
  SetStrPathToFusionOp(fus_nodelist, fus_opdef, ge::ATTR_NAME_OP_FILE_PATH);
  SetBoolAttrToFusionOp(fus_nodelist, fus_opdef, AIPP_CONV_FLAG);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, ge::TVM_ATTR_NAME_MAGIC);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, FUSION_OP_SLICE_INFO);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, ge::ATTR_NAME_ENGINE_NAME_FOR_LX);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, ge::ATTR_NAME_KKERNEL_LIB_NAME_FOR_LX);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, fe::ATTR_NAME_CUBE_VECTOR_CORE_TYPE);
  std::string core_type;
  (void)ge::AttrUtils::GetStr(fus_opdef, fe::ATTR_NAME_CUBE_VECTOR_CORE_TYPE, core_type);
  FE_LOGD("Fused node:%s, core type:%s", fus_opdef->GetName().c_str(), core_type.c_str());
  Status ret = FAILED;
  if ((core_type == "MIX_AIC") || (core_type == "MIX_AIV")) {
    ret = SetMixFusionOpAttr(fus_nodelist, fus_opdef, first_node);
  } else {
    ret = SetNormalFusionOpAttr(fus_nodelist, fus_opdef, first_node);
  }
  if (ret == FAILED) {
    return nullptr;
  }
  SetInt64AttrToFusionOp(fus_nodelist, fus_opdef, ge::TVM_ATTR_NAME_BLOCKDIM);
  SetBoolAttrToFusionOp(fus_nodelist, fus_opdef, NEED_RE_PRECOMPILE);
  SetBoolAttrToFusionOp(fus_nodelist, fus_opdef, ge::ATTR_N_BATCH_SPILT);
  SetKernelNameToFusionOp(fus_nodelist, fus_opdef);
  SetInt64AttrToFusionOp(fus_nodelist, fus_opdef, FE_IMPLY_TYPE);

  SetListInt64AttrToFusionOp(fus_nodelist, fus_opdef, TBE_OP_ATOMIC_OUTPUT_INDEX);
  SetInt64AttrToFusionOp(fus_nodelist, fus_opdef, TBE_OP_ATOMIC_WORKSPACE_FLAG);
  SetBoolAttrToFusionOp(fus_nodelist, fus_opdef, ATTR_NAME_FE_WEIGHT_COMPRESS);

  SetMemAttrToFusionOp(fus_nodelist, fus_opdef, ge::ATTR_NAME_CONTINUOUS_INPUT);
  SetMemAttrToFusionOp(fus_nodelist, fus_opdef, ge::ATTR_NAME_CONTINUOUS_OUTPUT);
  SetSqrtmodeToFusionOp(fus_nodelist, fus_opdef);
  SetDataDumpAttrToFusionOp(fus_nodelist, fus_opdef);

  // set attrs related to unknown shape
  SetBoolAttrToFusionOp(fus_nodelist, fus_opdef, ATTR_NAME_SUPPORT_DYNAMIC_SHAPE);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, COMPILE_INFO_JSON);
  SetStrAttrToFusionOp(fus_nodelist, fus_opdef, COMPILE_INFO_KEY);

  SetInt64AttrToFusionOp(fus_nodelist, fus_opdef, OP_PARA_SIZE);
  SetBoolAttrToFusionOp(fus_nodelist, fus_opdef, ATTR_NAME_LX_FUSION_PASS);
  SetStrVecAttrToFusionOp(fus_nodelist, fus_opdef, ATTR_NAME_OP_INFER_DEPENDS);
  SetBoolAttrToFusionOp(fus_nodelist, fus_opdef, ge::ATTR_NAME_REFERENCE);

  if (Configuration::Instance(engine_name).EnableL1Fusion()) {
    SetL1InfoToFusionOp(fus_nodelist, fus_opdef);
    SetL1FusionSubGraphNoToFusionOp(fus_nodelist, fus_opdef);
    SetNoTaskAndDumpNeeded(fus_nodelist, fus_opdef);
  }
  std::string build_mode_value = Configuration::Instance(engine_name).GetGeContextOptionValue(ge::BUILD_MODE);
  if (Configuration::Instance(engine_name).GetBufferFusionMode() == EN_L2_FUSION ||
      build_mode_value == ge::BUILD_MODE_TUNING) {
    SetL2InfoToFusionOp(fus_nodelist, fus_opdef);
    SetNoTaskAndDumpNeeded(fus_nodelist, fus_opdef);
  }
  if (Configuration::Instance(engine_name).GetDumpOriginalNodesEnable()) {
    SetOriginalNodesOpDescToFusionOp(fus_nodelist, fus_opdef);
  }
  vector<int64_t> work_space_bytes = first_node->GetOpDesc()->GetWorkspaceBytes();
  fus_opdef->SetWorkspaceBytes(work_space_bytes);

  vector<int64_t> work_space = first_node->GetOpDesc()->GetWorkspace();
  fus_opdef->SetWorkspace(work_space);
  fus_opdef->SetOpEngineName(first_node->GetOpDesc()->GetOpEngineName());
  fus_opdef->SetOpKernelLibName(first_node->GetOpDesc()->GetOpKernelLibName());
  return fus_opdef;
}

void FusionOpComm::SetDataDumpAttrToFusionOp(const vector<ge::NodePtr> &fus_nodelist,
                                             ge::OpDescPtr &fus_opdef) const {
  std::vector<ge::NodePtr> original_nodes;
  for (ge::NodePtr node : fus_nodelist) {
    original_nodes.push_back(node);
  }
  (void)RecordOriginalNames(original_nodes, fus_opdef);

  bool is_multi_op = false;
  for (const ge::NodePtr &node : fus_nodelist) {
    if (ge::AttrUtils::GetBool(node->GetOpDesc(), ge::ATTR_NAME_DATA_DUMP_IS_MULTIOP, is_multi_op)) {
      (void)ge::AttrUtils::SetBool(fus_opdef, ge::ATTR_NAME_DATA_DUMP_IS_MULTIOP, is_multi_op);
      break;
    }
  }
}

void FusionOpComm::SetOriginalNodesOpDescToFusionOp(const vector<ge::NodePtr> &fus_nodelist,
                                                    const ge::OpDescPtr &fus_opdef) const {
  ge::ModelSerializeImp model_serialize_imp;
  ge::proto::OpDef op_def_proto;
  std::string op_def_str;
  std::vector<std::string> original_nodes;
  for (const ge::NodePtr &node : fus_nodelist) {
    if (model_serialize_imp.SerializeNode(node, &op_def_proto)) {
      try {
        google::protobuf::TextFormat::PrintToString(op_def_proto, &op_def_str);
      } catch (const std::exception &e) {
        FE_LOGW("Fail to PrintToString. Error message is %s.", e.what());
      }
      original_nodes.push_back(op_def_str);
    } else {
      FE_LOGW("Serialize node[name:%s] not success.", node->GetName().c_str());
    }
  }
  if (!ge::AttrUtils::SetListStr(fus_opdef, ORIGINAL_NODES, original_nodes)) {
    FE_LOGW("Set original_nodes not success to node [%s].", fus_opdef->GetName().c_str());
  }
}

void FusionOpComm::RecordOriginalNames(std::vector<ge::NodePtr> original_nodes, ge::OpDescPtr &op_desc) const {
  std::vector<std::string> original_names;
  for (ge::NodePtr node_tmp : original_nodes) {
    std::vector<std::string> names_tmp;
    ge::OpDescPtr opdesc_tmp = node_tmp->GetOpDesc();
    FE_CHECK(opdesc_tmp == nullptr, FE_LOGW("opdescTmp is null"), return);
    bool is_has_attr = ge::AttrUtils::GetListStr(opdesc_tmp, ge::ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, names_tmp);
    if (is_has_attr) {
      if (!names_tmp.empty()) {
        for (auto &node_name : names_tmp) {
          if (!node_name.empty()) {
            original_names.push_back(node_name);
          }
        }
      }
    } else {
      original_names.push_back(opdesc_tmp->GetName());
    }
  }

  if (!ge::AttrUtils::SetListStr(op_desc, ge::ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES, original_names)) {
    FE_LOGW("Set ATTR_NAME_DATA_DUMP_ORIGIN_OP_NAMES fail.");
    return;
  }
}

Status FusionOpComm::AddTvmNode(ge::OpDescPtr op_desc) const {
  if (!ge::AttrUtils::SetInt(op_desc, ge::ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM))) {
    REPORT_FE_ERROR("[SubGraphOpt][PostProcess][AddTvmNode] Op[%s]: failed to set the attribute %s.",
                    op_desc->GetName().c_str(), ge::ATTR_NAME_IMPLY_TYPE.c_str());
    return FAILED;
  }
  return SUCCESS;
}
}  // namespace fe
