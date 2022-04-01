/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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
#include "graph/passes/mds_kernels/base_mds_kernel.h"

namespace ge {
namespace mds_cut_pass {
shared_ptr<DeploySchedulerKernel> GetKernelByType(const NodePtr &node) {
  if (node == nullptr) {
    REPORT_INNER_ERROR("E19999", "Param node is nullptr, check invalid");
    GELOGE(FAILED, "[Check][Param] parameter node is nullptr.");
    return nullptr;
  }
  DeployKernelFactory &factory = DeployKernelFactory::Instance();
  const auto &type = NodeUtils::GetNodeType(node);
  return factory.Create(type);
}
}  // namespace mds_cut_pass

Status DeploySchedulerKernel::CutN(const NodePtr &node, NodePtr &input_node) {
  GE_CHECK_NOTNULL(node);
  const auto &op_desc = node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  GELOGD("[MDS] start to cut-n process for node %s", op_desc->GetName().c_str());
  bool is_distributed = false;
  for (const auto &in_anchor : node->GetAllInDataAnchors()) {
    GE_CHECK_NOTNULL(in_anchor);
    auto src_anchor = in_anchor->GetPeerOutAnchor();
    if (src_anchor == nullptr) {
      continue;
    }
    auto tensor_desc = op_desc->MutableInputDesc(in_anchor->GetIdx());
    auto src_node = src_anchor->GetOwnerNode();
    GE_CHECK_NOTNULL(src_node);
    auto src_op_desc = src_node->GetOpDesc();
    auto src_tensor_desc = src_op_desc->MutableOutputDesc(src_anchor->GetIdx());
    GE_CHECK_NOTNULL(src_tensor_desc);
    // peer out shape is cutted already
    if (MdsUtils::IsDistributedDeploySupported(src_tensor_desc, CutType::kCutN)) {
      if (MdsUtils::IsDistributedDeploySupported(tensor_desc, CutType::kCutN)) {
        tensor_desc->SetShape(src_tensor_desc->GetShape());
        is_distributed = true;
      } else {
        MDS_REQUIRE_SUCCESS(
            MdsUtils::DataGather(src_anchor, in_anchor), "[CutN] failed to gather between node[%s][%d] to node[%s][%d]",
            src_op_desc->GetName().c_str(), src_anchor->GetIdx(), op_desc->GetName().c_str(), in_anchor->GetIdx());
      }
    } else {
      if (MdsUtils::IsDistributedDeploySupported(tensor_desc, CutType::kCutN)) {
        MDS_REQUIRE_SUCCESS(MdsUtils::DataSlice(src_anchor, in_anchor, CutType::kCutN, input_node),
                            "[CutN] failed to slice between node[%s][%d] to node[%s][%d]",
                            src_op_desc->GetName().c_str(), src_anchor->GetIdx(), op_desc->GetName().c_str(),
                            in_anchor->GetIdx());
        is_distributed = true;
      } else {
        tensor_desc->SetShape(src_tensor_desc->GetShape());
      }
    }

    if (node->GetType() == HCOMALLREDUCE) {
      MDS_REQUIRE_SUCCESS(MdsUtils::SetAttrForHcomNode(node->GetOpDesc(), kDeployNumber),
                          "[DataReduce][Modify] set attr for allreduce node for %s failed", node->GetName().c_str());
    }
  }
  // call infer shape, update output shape
  if (is_distributed) {
    MDS_REQUIRE_SUCCESS(MdsUtils::InferShapeAndType(node), "[CutN] %s call infershape failed", node->GetName().c_str());
  }
  return SUCCESS;
}

Status DeploySchedulerKernel::CutH(const NodePtr &node, NodePtr &input_node) {
  GE_CHECK_NOTNULL(node);
  auto op_desc = node->GetOpDesc();
  GE_CHECK_NOTNULL(op_desc);
  GELOGD("Start to cut-h process for node %s", op_desc->GetName().c_str());
  for (auto &in_anchor : node->GetAllInDataAnchors()) {
    GE_CHECK_NOTNULL(in_anchor);
    auto src_anchor = in_anchor->GetPeerOutAnchor();
    if (src_anchor == nullptr) {
      continue;
    }
    auto tensor_desc = op_desc->MutableInputDesc(in_anchor->GetIdx());
    auto src_node = src_anchor->GetOwnerNode();
    GE_CHECK_NOTNULL(src_node);
    auto src_op_desc = src_node->GetOpDesc();
    auto src_tensor_desc = src_op_desc->MutableOutputDesc(src_anchor->GetIdx());
    GE_CHECK_NOTNULL(src_tensor_desc);
    // peer out shape is cutted already
    if (MdsUtils::IsDistributedDeploySupported(src_tensor_desc, CutType::kCutH)) {
      if (MdsUtils::IsDistributedDeploySupported(tensor_desc, CutType::kCutH)) {
        MDS_REQUIRE_SUCCESS(HaloExchangeProcess(node, in_anchor->GetIdx()),
                            "[CutH] failed to do overlap between node[%s][%d] to node[%s][%d]",
                            src_op_desc->GetName().c_str(), src_anchor->GetIdx(), op_desc->GetName().c_str(),
                            in_anchor->GetIdx());
      } else {
        MDS_REQUIRE_SUCCESS(
            MdsUtils::DataGather(src_anchor, in_anchor), "[CutH] failed to gather between node[%s][%d] to node[%s][%d]",
            src_op_desc->GetName().c_str(), src_anchor->GetIdx(), op_desc->GetName().c_str(), in_anchor->GetIdx());
      }
    } else {
      if (MdsUtils::IsDistributedDeploySupported(tensor_desc, CutType::kCutH)) {
        MDS_REQUIRE_SUCCESS(MdsUtils::DataSlice(src_anchor, in_anchor, CutType::kCutH, input_node),
                            "[CutH] failed to slice between node[%s][%d] to node[%s][%d]",
                            src_op_desc->GetName().c_str(), src_anchor->GetIdx(), op_desc->GetName().c_str(),
                            in_anchor->GetIdx());
      } else {
        MDS_REQUIRE_SUCCESS(HaloExchangeProcess(node, in_anchor->GetIdx(), true),
                            "[CutH] failed to do overlap between node[%s][%d] to node[%s][%d]",
                            src_op_desc->GetName().c_str(), src_anchor->GetIdx(), op_desc->GetName().c_str(),
                            in_anchor->GetIdx());
      }
    }
    if (MdsUtils::IsGradNode(node)) {
      MDS_REQUIRE_SUCCESS(MdsUtils::DataReduce(src_anchor, in_anchor), "[DataReduce][Modify] node %s failed",
                          node->GetName().c_str());
    }
  }

  // call infer shape, update output shape
  MDS_REQUIRE_SUCCESS(MdsUtils::InferShapeAndType(node), "[CutH] %s call infer shape failed", node->GetName().c_str());
  return SUCCESS;
}

Status DeploySchedulerKernel::DynamicCutH(const NodePtr &node) {
  (void)node;
  return SUCCESS;
}

Status DeploySchedulerKernel::DynamicCutN(const NodePtr &node) {
  (void)node;
  return SUCCESS;
}

Status DeploySchedulerKernel::HaloExchangeProcess(NodePtr node, int64_t index, bool local_slice) const {
  (void)node;
  (void)index;
  (void)local_slice;
  return SUCCESS;
}
}  // namespace ge
