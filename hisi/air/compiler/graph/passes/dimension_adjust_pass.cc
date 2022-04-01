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

#include "graph/passes/dimension_adjust_pass.h"

#include <memory>
#include <string>
#include <vector>
#include "graph/utils/node_utils.h"
#include "inc/kernel.h"
#include "inc/kernel_factory.h"

namespace ge {
namespace {
const int32_t kDataInputIndex = 0;
const int32_t kRemoveInputIndex = 1;
}  // namespace

Status DimensionAdjustPass::Run(ge::NodePtr &node) {
  GE_CHECK_NOTNULL(node);
  OpDescPtr op_desc_ptr = node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc_ptr);

  std::string type;
  Status ret = GetOriginalType(node, type);
  if (ret != SUCCESS) {
    REPORT_CALL_ERROR("E19999", "Get OriginalType of op:%s(%s) failed",
                      node->GetName().c_str(), node->GetType().c_str());
    GELOGE(ret, "[Get][OriginalType] of op:%s(%s) failed", node->GetName().c_str(), node->GetType().c_str());
    return ret;
  }

  KernelFactory &factory = KernelFactory::Instance();
  shared_ptr<Kernel> op_kernel = factory.Create(type);
  if (op_kernel == nullptr) {
    return SUCCESS;
  }
  bool is_unknown = false;
  auto ret_status = NodeUtils::GetNodeUnknownShapeStatus(*node, is_unknown);
  if (ret_status != GRAPH_SUCCESS) {
    GELOGW("Get node unknown status failed, node name:%s, type:%s.", node->GetName().c_str(), node->GetType().c_str());
    return INTERNAL_ERROR;
  }
  if (is_unknown) {
    GELOGI("Current node %s, type %s is unknown shape which should be skip.",
           node->GetName().c_str(), node->GetType().c_str());
    return SUCCESS;
  }

  // call compute function
  ret = op_kernel->Compute(node);
  if (ret != SUCCESS) {
    if (ret == NOT_CHANGED || ret == UNSUPPORTED) {
      return SUCCESS;
    }
    REPORT_CALL_ERROR("E19999", "kernel compute for op:%s(%s) failed",
                      node->GetName().c_str(), node->GetType().c_str());
    GELOGE(ret, "[Call][Compute] for op:%s(%s) failed", node->GetName().c_str(), node->GetType().c_str());
    return ret;
  }
  // Need to handle axis_input of node like ExpandDims
  if (node->GetAllInDataAnchors().size() > static_cast<size_t>(kRemoveInputIndex)) {
    auto axis_node_out_anchor = node->GetInDataAnchor(kRemoveInputIndex)->GetPeerOutAnchor();
    GE_CHECK_NOTNULL(axis_node_out_anchor);
    auto axis_node = axis_node_out_anchor->GetOwnerNode();
    // 1.Copy control dependency of axis node
    ret = PassUtils::UnlinkNodeWithControlCopy(node, kRemoveInputIndex);
    if (ret != SUCCESS) {
      REPORT_CALL_ERROR("E19999", "Unlink op:%s(%s) data input:%u with control edge copy failed",
                        node->GetName().c_str(), node->GetType().c_str(), kRemoveInputIndex);
      GELOGE(ret, "[Unlink][Op] %s(%s) data input:%u with control edge copy failed",
             node->GetName().c_str(), node->GetType().c_str(), kRemoveInputIndex);
      return ret;
    }
    // 2.Remove const axis node without any output
    if ((axis_node->GetType() == CONSTANT || axis_node->GetType() == CONSTANTOP) &&
        axis_node->GetOutDataNodesSize() == 0) {
      ret = IsolateAndDeleteNode(axis_node, {});
      GE_CHK_GRAPH_STATUS_RET(ret, "[Remove][Node] %s failed.", axis_node->GetName().c_str());
      GELOGI("Remove useless axis input const %s", axis_node->GetName().c_str());
    }
  }

  std::vector<int32_t> data_relink_io_map = {kDataInputIndex};
  return IsolateAndDeleteNode(node, data_relink_io_map);
}

NodePtr DimensionAdjustPass::AddIdentityNodeToGraph(const std::string &name, const GeTensorDesc &tensor,
                                                    ComputeGraphPtr &graph) const {
  if (graph == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param graph is nullptr, check invalid");
    GELOGE(INTERNAL_ERROR, "[Check][Param] Comput graph ptr is nullptr in creating identity node.");
    return nullptr;
  }

  OpDescPtr desc = MakeShared<OpDesc>("", "");
  if (desc == nullptr) {
    REPORT_CALL_ERROR("E19999", "New OpDesc failed");
    GELOGE(MEMALLOC_FAILED, "[New][OpDesc] failed.");
    return nullptr;
  }

  desc->SetName(name);
  desc->SetType(IDENTITY);
  auto ret = desc->AddInputDesc(tensor);
  auto ret2 = desc->AddOutputDesc(tensor);
  if ((ret != GRAPH_SUCCESS) || (ret2 != GRAPH_SUCCESS)) {
    REPORT_CALL_ERROR("E19999", "Add input or ouput desc to op:%s(%s) failed",
                      desc->GetName().c_str(), desc->GetType().c_str());
    GELOGE(INTERNAL_ERROR, "[Add][GeTensorDesc] to op:%s(%s) failed",
           desc->GetName().c_str(), desc->GetType().c_str());
    return nullptr;
  }

  return graph->AddNodeFront(desc);
}
}  // namespace ge
