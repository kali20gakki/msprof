/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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

#include "inc/ffts_utils.h"
#include <sys/time.h>
#include <sstream>
#include <climits>
#include "inc/ffts_error_codes.h"
#include "engine/engine_manager.h"
#include "inc/common/util/error_manager/error_manager.h"
#include "inc/common/util/platform_info.h"
#include "inc/graph/utils/graph_utils.h"
#include "inc/graph/utils/node_utils.h"
#include "inc/graph/debug/ge_attr_define.h"
#include "runtime/rt.h"

namespace ffts {
std::mutex g_report_error_msg_mutex;

FFTSMode GetPlatformFFTSMode() {
  string soc_version = EngineManager::Instance(kFFTSPlusCoreName).GetSocVersion();
  fe::PlatFormInfos platform_infos;
  fe::OptionalInfos opti_compilation_infos;
  auto ret = fe::PlatformInfoManager::Instance().GetPlatformInfos(soc_version, platform_infos,
                                                                  opti_compilation_infos);
  if (ret != SUCCESS) {
    FFTS_LOGW("Fail to get platform info by soc version[%s].", soc_version.c_str());
    return FFTSMode::FFTS_MODE_NO_FFTS;
  }
  string ffts_mode = "";
  platform_infos.GetPlatformRes(kSocInfo, kFFTSMode, ffts_mode);
  FFTS_LOGD("GetPlatformFFTSMode [%s].", ffts_mode.c_str());
  if (ffts_mode == kFFTSPlus) {
    return FFTSMode::FFTS_MODE_FFTS_PLUS;
  } else if (ffts_mode == kFFTS) {
    return FFTSMode::FFTS_MODE_FFTS;
  }
  return FFTSMode::FFTS_MODE_NO_FFTS;
}

int64_t GetMicroSecondTime() {
  struct timeval tv = {0};
  int ret = gettimeofday(&tv, nullptr);
  if (ret != 0) {
    return 0;
  }
  if (tv.tv_sec < 0 || tv.tv_usec < 0) {
    return 0;
  }
  int64_t micro_multiples = 1000000;
  int64_t second = tv.tv_sec;
  FFTS_INT64_MULCHECK(second, micro_multiples);
  int64_t second_to_micro = second * micro_multiples;
  FFTS_INT64_ADDCHECK(second_to_micro, tv.tv_usec);
  return second_to_micro + tv.tv_usec;
}


void LogErrorMessage(std::string error_code, const std::map<std::string, std::string> &args_map) {
  int result = ErrorManager::GetInstance().ReportErrMessage(error_code, args_map);

  FFTS_LOGE_IF(result != 0, "Faild to call ErrorManager::GetInstance(). ReportErrMessage");
}

std::string RealPath(const std::string &path) {
  if (path.empty()) {
    FFTS_LOGI("path string is nullptr.");
    return "";
  }
  if (path.size() >= PATH_MAX) {
    FFTS_LOGI("file path %s is too long! ", path.c_str());
    return "";
  }

  // PATH_MAX is the system marco，indicate the maximum length for file path
  // pclint check one param in stack can not exceed 1K bytes
  char resoved_path[PATH_MAX] = {0x00};

  std::string res;

  // path not exists or not allowed to read return nullptr
  // path exists and readable, return the resoved path
  if (realpath(path.c_str(), resoved_path) != nullptr) {
    res = resoved_path;
  } else {
    FFTS_LOGI("path %s is not exist.", path.c_str());
  }
  return res;
}

void DumpGraph(const ge::ComputeGraph &graph, const std::string &suffix) {
  std::shared_ptr<ge::ComputeGraph> compute_graph_ptr = FFTSComGraphMakeShared<ge::ComputeGraph>(graph);
  ge::GraphUtils::DumpGEGraph(compute_graph_ptr, suffix);
}

void DumpSubGraphAndOnnx(const ge::ComputeGraph &graph, const std::string &suffix) {
  for (auto subgraph : graph.GetAllSubgraphs()) {
    DumpGraphAndOnnx(*subgraph, suffix);
  }
}

void DumpGraphAndOnnx(const ge::ComputeGraph &graph, const std::string &suffix) {
  DumpGraph(graph, suffix);
  ge::GraphUtils::DumpGEGraphToOnnx(graph, suffix);
}

bool IsUnKnownShapeTensor(const ge::OpDesc &op_desc) {
  for (auto &tenosr_desc_ptr : op_desc.GetAllInputsDescPtr()) {
    if (tenosr_desc_ptr == nullptr) {
      continue;
    }
    if (tenosr_desc_ptr->MutableShape().IsUnknownShape()) {
      FFTS_LOGI("Op[%s, type %s] is unknown shape op. Its input is unknown.", op_desc.GetName().c_str(),
                op_desc.GetName().c_str());
      return true;
    }
  }

  for (auto &tenosr_desc_ptr : op_desc.GetAllOutputsDescPtr()) {
    if (tenosr_desc_ptr == nullptr) {
      continue;
    }
    if (tenosr_desc_ptr->MutableShape().IsUnknownShape()) {
      FFTS_LOGI("Op[%s, type %s] is unknown shape op. Its output is unknown.", op_desc.GetName().c_str(),
                op_desc.GetName().c_str());
      return true;
    }
  }
  return false;
}


bool IsUnKnownShapeOp(const ge::OpDesc &op_desc) {
  bool is_unknow_shape = false;

  if (op_desc.GetAllInputsSize() != 0 || op_desc.GetOutputsSize() != 0) {
    is_unknow_shape = IsUnKnownShapeTensor(op_desc);
  }
  return is_unknow_shape;
}

bool HasPeerOutNode(const ge::NodePtr &node, const int &anchor_index, ge::NodePtr &peer_out_node) {
  auto in_anchor = node->GetInDataAnchor(anchor_index);
  FFTS_CHECK(in_anchor == nullptr, FFTS_LOGD("In anchor is nullptr"), return false);
  auto peer_out_anchor = in_anchor->GetPeerOutAnchor();
  FFTS_CHECK(peer_out_anchor == nullptr, FFTS_LOGD("In anchor peer out anchor is nullptr"), return false);
  peer_out_node = peer_out_anchor->GetOwnerNode();
  FFTS_CHECK(peer_out_node == nullptr, FFTS_LOGD("Peer out anchor's out node is nullptr"), return false);
  return true;
}

bool IsSubGraphData(const ge::OpDescPtr &op_desc_ptr) {
  if (op_desc_ptr == nullptr || op_desc_ptr->GetType() != DATA) {
    return false;
  }
  return op_desc_ptr->HasAttr(ge::ATTR_NAME_PARENT_NODE_INDEX);
}

bool CheckSubGraphDataForConstValue(ge::NodePtr &parent_node) {
  ge::NodePtr really_parent_node = nullptr;
  if (ge::NodeUtils::GetInNodeCrossPartionedCallNode(parent_node, 0, really_parent_node) != SUCCESS) {
    FFTS_LOGW("[Op %s, type %s]: failed to getInNodeCrossPartionCallNode.",
              parent_node->GetName().c_str(), parent_node->GetType().c_str());
    return false;
  }
  if (really_parent_node != nullptr) {
    std::string node_type = really_parent_node->GetType();
    FFTS_LOGD("Parent_node:%s type:%s really_parent_node:%s type:%s", parent_node->GetName().c_str(),
              parent_node->GetType().c_str(), really_parent_node->GetName().c_str(),
              really_parent_node->GetType().c_str());
    parent_node = really_parent_node;
    return (node_type == CONSTANT || node_type == CONSTANTOP);
  } else {
    FFTS_LOGD("real parent node for %s is null.", parent_node->GetName().c_str());
  }
  return false;
}

void IsNodeSpecificType(const std::unordered_set<string> &types,
                        ge::NodePtr &node, bool &matched) {
  auto type = node->GetType();
  auto name = node->GetName();
  matched = types.count(type);
  if (matched) {
    return;
  }

  // if it is placeholder, get its parent node
  if (type == PLACEHOLDER) {
    ge::NodePtr parent_node = nullptr;
    parent_node = node->GetOpDesc()->TryGetExtAttr(ATTR_NAME_PARENT_NODE, parent_node);
    if (parent_node != nullptr) {
      node = parent_node;
      type = parent_node->GetType();
      FFTS_LOGD("The parent node of place holder[%s] is [%s, %s].", name.c_str(),
                parent_node->GetName().c_str(), parent_node->GetType().c_str());
      ge::NodePtr really_parent_node = parent_node;
      bool parent_node_invalid = (!types.count(type) && ge::NodeUtils::GetInNodeCrossPartionedCallNode(parent_node, 0,
                                  really_parent_node) != SUCCESS);
      if (parent_node_invalid) {
        FFTS_LOGW("[SubGraphOpt][IsNodeSpecType][Op %s, type %s]: failed to getInNodeCrossPartionCallNode.",
                  node->GetName().c_str(), type.c_str());
        return;
      }
      if (really_parent_node != nullptr) {
        node = really_parent_node;
        type = really_parent_node->GetType();
        FFTS_LOGD("Parent node:%s type:%s really parent node:%s type:%s", parent_node->GetName().c_str(),
                  parent_node->GetType().c_str(), really_parent_node->GetName().c_str(),
                  type.c_str());
      }
      matched = types.count(type);
    }
  } else if (IsSubGraphData(node->GetOpDesc())) {
    matched = CheckSubGraphDataForConstValue(node);
  } else {
    FFTS_LOGD("Cannot match any types for node %s and type %s.", node->GetName().c_str(), type.c_str());
  }
}

bool IsPeerOutWeight(ge::NodePtr &node, const int &anchor_index,
                     ge::NodePtr &peer_out_node) {
  if (node == nullptr) {
    return false;
  }
  auto op_desc = node->GetOpDesc();
  std::string peer_out_node_type;
  bool has_other_node = HasPeerOutNode(node, anchor_index, peer_out_node);
  if (has_other_node) {
    FFTS_LOGD("[IsPeerOutWeight]peer out node is %s.", peer_out_node->GetName().c_str());
    peer_out_node = node->GetInDataAnchor(anchor_index)->GetPeerOutAnchor()->GetOwnerNode();
    bool is_const_node = false;
    IsNodeSpecificType(kWeightTypes, peer_out_node, is_const_node);
    return is_const_node;
  } else {
    return false;
  }
}

bool IsSubGraphNetOutput(const ge::OpDescPtr &op_desc_ptr) {
  if (op_desc_ptr == nullptr || op_desc_ptr->GetType() != NETOUTPUT) {
    return false;
  }
  for (auto &tensor : op_desc_ptr->GetAllInputsDescPtr()) {
    if (ge::AttrUtils::HasAttr(tensor, ge::ATTR_NAME_PARENT_NODE_INDEX)) {
      return true;
    }
  }
  return false;
}
}  // namespace ffts
