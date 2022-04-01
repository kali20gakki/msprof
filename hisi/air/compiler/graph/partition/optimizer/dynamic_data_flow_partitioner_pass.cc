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

#include "graph/partition/optimizer/dynamic_data_flow_partitioner_pass.h"
#include "framework/common/util.h"
#include "framework/common/types.h"
#include "graph/debug/ge_attr_define.h"
#include "inc/graph/utils/node_utils.h"
#include "inc/graph/utils/op_desc_utils.h"
#include "inc/graph/utils/type_utils.h"

#define REQUIRE(cond, ...)                                     \
  do {                                                         \
    if (!(cond)) {                                             \
      REPORT_INNER_ERROR("E19999", __VA_ARGS__);               \
      GELOGE(FAILED, "[Data Flow Partitioner Pass]" __VA_ARGS__); \
      return FAILED;                                           \
    }                                                          \
  } while (false)

#define REQUIRE_SUCCESS(cond, ...) REQUIRE(((cond) == SUCCESS), __VA_ARGS__)

namespace ge {
namespace {
  const std::unordered_set<std::string> kDataFlowPartitionerDataFlowOps = {STACK, STACKPUSH, STACKPOP, STACKCLOSE};
  inline bool IsDataFlowOps(const std::string &op_type) {
    return (kDataFlowPartitionerDataFlowOps.count(op_type) != 0UL);
  }
} // namespace

  Status DynamicDataFlowPartitionerPass::Run(ge::ComputeGraphPtr &graph,
                                             const std::vector<std::shared_ptr<BaseCluster>> &sorted_unique_clusters,
                                             bool &result) {
  /* If the handle and the corresponding resource  ops container are empty, execute */
  if (data_flow_ops_.empty()) {
    REQUIRE_SUCCESS(GetDataFlowOpMapping(graph), "[GetDataFlowOpMapping] failed");
  }
  REQUIRE_SUCCESS(GetDataFlowOpAttr(sorted_unique_clusters), "[GetDataFlowOpAttr] failed");
  REQUIRE_SUCCESS(GetDataFlowOpNeedProcess(result), "[GetDataFlowOpNeedProcess] failed");
  if (result) {
    REQUIRE_SUCCESS(GetUnknownDataFlowOp(), "[GetUnknownDataFlowOp] failed");
    REQUIRE_SUCCESS(MarkDataFlowOpAttr(graph), "[MarkDataFlowOpAttr] failed");
  } else {
    data_flow_ops_.clear();
  }
  ClearDataFlowsource();
  return SUCCESS;
}

Status DynamicDataFlowPartitionerPass::GetDataFlowOpMapping(const ge::ComputeGraphPtr &graph) {
  for (const auto &node : graph->GetAllNodes()) {
    GE_CHECK_NOTNULL(node);
    int64_t handle;
    if (AttrUtils::GetInt(node->GetOpDesc(), ATTR_NAME_DATA_FLOW_HANDLE, handle)) {
      (void)data_flow_ops_[handle].insert(node);
      GELOGD("Get data flow node %s, handle %ld is success.", node->GetName().c_str(), handle);
    }
  }
  return SUCCESS;
}

Status DynamicDataFlowPartitionerPass::GetDataFlowOpAttr(
    const std::vector<std::shared_ptr<BaseCluster>> &sorted_unique_clusters) {
  for (const auto &cluster : sorted_unique_clusters) {
    for (const auto &node : cluster->Nodes()) {
      const auto op_type = node->GetType();
      if (IsDataFlowOps(op_type)) {
        if (cluster->IsUnknownShape()) {
          data_flow_ops_attr_[node] = true;
        } else {
          data_flow_ops_attr_[node] = false;
        }
      }
    }
  }
  return SUCCESS;
}

Status DynamicDataFlowPartitionerPass::GetDataFlowOpNeedProcess(bool &result) {
  bool is_need_process = false;
  for (const auto &resop : data_flow_ops_) {
    if (resop.second.size() <= 1) {
      continue;
    }
    bool srcop_attr = data_flow_ops_attr_.find(*(resop.second.begin()))->second;
    for (const auto &dstop : resop.second) {
      bool dstop_attr = data_flow_ops_attr_.find(dstop)->second;
      if (srcop_attr != dstop_attr) {
        is_need_process = true;
        data_flow_handle_.insert(resop.first);
      }
    }
  }
  result = is_need_process;
  return SUCCESS;
}

Status DynamicDataFlowPartitionerPass::GetUnknownDataFlowOp() {
  for (const auto &op_handle : data_flow_handle_) {
    if (data_flow_ops_.find(op_handle) != data_flow_ops_.end()) {
      for (const auto &dst_op : data_flow_ops_.find(op_handle)->second) {
        unknown_data_flow_ops_.insert(dst_op);
        GELOGD("Unknown data flow handle %ld, node %s.", op_handle, dst_op->GetName().c_str());
      }
    }
  }
  return SUCCESS;
}

Status DynamicDataFlowPartitionerPass::MarkDataFlowOpAttr(const ge::ComputeGraphPtr &graph) const {
  for (auto &node : graph->GetAllNodes()) {
    GE_CHECK_NOTNULL(node);
    const auto op_type = node->GetType();
    if (IsDataFlowOps(op_type)) {
      if (unknown_data_flow_ops_.find(node) != unknown_data_flow_ops_.end()) {
        auto desc = node->GetOpDesc();
        GE_CHECK_NOTNULL(desc);
        (void)AttrUtils::SetBool(desc, "_force_unknown_shape", true);
      }
    }
  }

  return SUCCESS;
}

void DynamicDataFlowPartitionerPass::ClearDataFlowsource() {
  data_flow_handle_.clear();
  unknown_data_flow_ops_.clear();
  data_flow_ops_attr_.clear();
}
} // namespace ge