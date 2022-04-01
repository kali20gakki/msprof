/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#include "graph/partition/engine_place.h"

#include <mutex>

#include "framework/common/op/ge_op_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "init/gelib.h"
#include "graph/partition/optimizer/dynamic_data_flow_engine_reassign_pass.h"

namespace ge {
namespace {
std::mutex check_support_cost_mutex;
}
Status EnginePlacer::Check() const {
  if (compute_graph_ == nullptr) {
    REPORT_INNER_ERROR("E19999", "compute_graph_ is nullptr, check invalid.");
    GELOGE(GE_GRAPH_NULL_INPUT, "[Check][Param] compute_graph_ is nullptr.");
    return FAILED;
  }
  std::shared_ptr<GELib> instance_ptr = GELib::GetInstance();
  if ((instance_ptr == nullptr) || (!instance_ptr->InitFlag())) {
    REPORT_INNER_ERROR("E19999", "GELib instance is nullptr or it is not InitFlag, check invalid.");
    GELOGE(GE_CLI_GE_NOT_INITIALIZED, "[Get][GELib] Run enginePlacer failed, because GELib is invalid.");
    return FAILED;
  }
  return SUCCESS;
}

Status EnginePlacer::SelectEngine(const NodePtr &node, bool &is_check_support_success) {
  auto op_desc = node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  std::string engine_name;
  std::string kernel_name;
  // Check if this node has assigned engine
  bool has_engine_attr =
      AttrUtils::GetStr(op_desc, ATTR_NAME_ENGINE_NAME_FOR_LX, engine_name) && !engine_name.empty();
  bool has_kernel_attr =
      AttrUtils::GetStr(op_desc, ATTR_NAME_KKERNEL_LIB_NAME_FOR_LX, kernel_name) && !kernel_name.empty();

  if (op_desc->GetOpEngineName().empty()) {
    if (has_kernel_attr && has_engine_attr) {
      UpdateOpdescWithAttr(op_desc, engine_name, kernel_name);
    } else {
      // Call placer cost model to get the "best" engine for this node
      std::string selected_engine_name = DNNEngineManager::GetInstance().GetDNNEngineName(node);
      // If can't get op's engine name, keep check support finish and return failed
      if (selected_engine_name.empty()) {
        is_check_support_success = false;
        ErrorManager::GetInstance().ATCReportErrMessage(
            "EZ3003", {"opname", "optype"}, {op_desc->GetName(), op_desc->GetType()});
        GELOGE(GE_CLI_GE_NOT_INITIALIZED, "[Check][Param] Can not find engine of op name %s type %s",
               op_desc->GetName().c_str(), op_desc->GetType().c_str());
        return FAILED;
      }
    }
  } else {
    UpdateAttrWithOpdesc(op_desc, engine_name, kernel_name);
  }

  // Record the node assigned atomic_engine name
  GELOGD("Assigning DNNEngine %s to node %s, op type %s", op_desc->GetOpEngineName().c_str(), node->GetName().c_str(),
         node->GetType().c_str());
  node_atomic_engine_map_.emplace(node, op_desc->GetOpEngineName());
  return SUCCESS;
}

void EnginePlacer::UpdateAttrWithOpdesc(const OpDescPtr op_desc, const std::string attr_engine_name,
                                        const std::string attr_kernel_name) {
  if (attr_engine_name.empty()) {
    (void) AttrUtils::SetStr(op_desc, ATTR_NAME_ENGINE_NAME_FOR_LX, op_desc->GetOpEngineName());
  } else {
    if (attr_engine_name != op_desc->GetOpEngineName()) {
      GELOGW("[%s] of attr[%s] in op[%s] is not equal to engine[%s]", attr_engine_name.c_str(),
             ATTR_NAME_ENGINE_NAME_FOR_LX.c_str(), op_desc->GetName().c_str(), op_desc->GetOpEngineName().c_str());
    }
  }

  if (attr_kernel_name.empty()) {
    (void) AttrUtils::SetStr(op_desc, ATTR_NAME_KKERNEL_LIB_NAME_FOR_LX, op_desc->GetOpKernelLibName());
  } else {
    if (attr_kernel_name != op_desc->GetOpKernelLibName()) {
      GELOGW("[%s] of attr[%s] in op[%s] is not equal to [%s]", attr_kernel_name.c_str(),
             ATTR_NAME_KKERNEL_LIB_NAME_FOR_LX.c_str(), op_desc->GetName().c_str(),
             op_desc->GetOpKernelLibName().c_str());
    }
  }
}

void EnginePlacer::UpdateOpdescWithAttr(const OpDescPtr op_desc, const std::string attr_engine_name,
                                        const std::string attr_kernel_name) {
  if (op_desc->GetOpEngineName().empty()) {
    op_desc->SetOpEngineName(attr_engine_name);
  } else {
    if (op_desc->GetOpEngineName() != attr_engine_name) {
      GELOGW("engine[%s] in op[%s] is not equal to [%s] of attr[%s]", op_desc->GetOpEngineName().c_str(),
             op_desc->GetName().c_str(), attr_engine_name.c_str(), ATTR_NAME_ENGINE_NAME_FOR_LX.c_str());
    }
  }

  if (op_desc->GetOpKernelLibName().empty()) {
    op_desc->SetOpKernelLibName(attr_kernel_name);
  } else {
    if (op_desc->GetOpKernelLibName() != attr_kernel_name) {
      GELOGW("kernel[%s] in op[%s] is not equal to [%s] of attr[%s]", op_desc->GetOpKernelLibName().c_str(),
             op_desc->GetName().c_str(), attr_kernel_name.c_str(), ATTR_NAME_KKERNEL_LIB_NAME_FOR_LX.c_str());
    }
  }
}

Status EnginePlacer::Run(bool direct_node_flag) {
  std::lock_guard<std::mutex> lock(check_support_cost_mutex);

  GELOGD("Engine placer starts.");
  if (Check() != SUCCESS) {
    return FAILED;
  }
  bool is_check_support_success = true;
  // Assign engine for each node in the graph
  DNNEngineManager::GetInstance().InitPerformanceStatistic();
  for (const auto &node_ptr : compute_graph_->GetNodes(direct_node_flag)) {
    GE_CHECK_NOTNULL(node_ptr);
    SelectEngine(node_ptr, is_check_support_success);
  }

  for (auto &it : DNNEngineManager::GetInstance().GetCheckSupportCost()) {
    GEEVENT("The time cost of %s::CheckSupported is [%lu] micro second.", it.first.c_str(), it.second);
  }
  GELOGD("Engine placer ends.");
  return is_check_support_success ? SUCCESS : FAILED;
}

Status EnginePlacer::AssignCompositeEngine() {
  if (OpsKernelManager::GetInstance().GetCompositeEngines().empty()) {
    GELOGI("No composite engine registers, ignore assign composite engine");
    return SUCCESS;
  }
  std::vector<ComputeGraphPtr> subgraphs;
  if (GraphUtils::GetSubgraphsRecursively(compute_graph_, subgraphs) != GRAPH_SUCCESS) {
    GE_CHECK_NOTNULL(compute_graph_);
    REPORT_CALL_ERROR("E19999", "Get subgraphs contained in graph %s failed", compute_graph_->GetName().c_str());
    GELOGE(FAILED, "[Get][Subgraphs] Get subgraphs contained in graph %s failed", compute_graph_->GetName().c_str());
    return FAILED;
  }
  for (const auto &subgraph : subgraphs) {
    (void)subgraph->DelAttr(ATTR_NAME_COMPOSITE_ENGINE_NAME);
  }
  std::reverse(subgraphs.begin(), subgraphs.end());
  subgraphs.emplace_back(compute_graph_);
  for (const auto &subgraph : subgraphs) {
    for (const auto &node : subgraph->GetDirectNode()) {
      std::string composite_engine_name = DNNEngineManager::GetInstance().GetCompositeEngineName(node, 1);
      GELOGD("Assign composite engine %s to node %s, op type %s", composite_engine_name.c_str(),
             node->GetName().c_str(), node->GetType().c_str());
      node_composite_engine_map_.insert(std::make_pair(node, composite_engine_name));
    }
  }
  return SUCCESS;
}

const NodeEngineMap &EnginePlacer::GetNodeEngineMap(bool is_composite_engine_mode) const {
  return is_composite_engine_mode ? node_composite_engine_map_ : node_atomic_engine_map_;
}

Status EnginePlacer::ReAssignEngine() {
  std::vector<std::pair<std::string, std::shared_ptr<EngineReAssignPass>>> passes;
  passes.emplace_back(std::make_pair("DynamicDataFlowReAssign", MakeShared<DynamicDataFlowReAssign>()));

  for (const auto &pass_item : passes) {
    GE_CHECK_NOTNULL(pass_item.second);
    const Status ret = pass_item.second->Run(compute_graph_, node_atomic_engine_map_, node_composite_engine_map_);
    if (ret == SUCCESS) {
      GELOGD("Engine reassign pass %s return SUCCESS.", pass_item.first.c_str());
    } else if (ret == NOT_CHANGED) {
      GELOGD("Engine reassign pass %s return NOT_CHANGED.", pass_item.first.c_str());
    } else {
      REPORT_CALL_ERROR("E19999", "Engine reassign pass %s run failed.", pass_item.first.c_str());
      GELOGE(ret, "[Call][Run] Engine reassign pass %s failed.", pass_item.first.c_str());
      return ret;
    }
  }
  return SUCCESS;
}
}  // namespace ge
