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

#include "graph/passes/mds_pass.h"

namespace ge {
Status ModelDeploySchedulerPass::Run(ComputeGraphPtr graph) {
  GE_CHECK_NOTNULL(graph);
  compute_graph_ = graph;
  if (!MdsUtils::IsMDSNeeded()) {
    GELOGI("[MDS][%s] no need to deploy.", GetGraphName());
    return SUCCESS;
  }
  // Set the cut support information based on the engine of the node
  GELOGI("[MDS][%s] start to get node's cut info.", GetGraphName());
  EnginePlacer engine_placer;
  engine_placer.SetComputeGraph(compute_graph_);
  GE_CHK_STATUS_RET(engine_placer.Run());
  GE_CHK_STATUS_RET(SetNodeCutInfo(compute_graph_));
  GELOGI("[MDS][%s] start to deploy and dump.", GetGraphName());
  GE_DUMP(compute_graph_, "BeforeMDS");
  MDS_REQUIRE_SUCCESS_AND_DUMP(SMDPProcess(), "[MDS][SMDPProcess] failed, graph_name:[%s]", GetGraphName());
  MDS_REQUIRE_SUCCESS_AND_DUMP(CutProcess(), "[MDS][CutProcess] failed, graph_name:[%s]", GetGraphName());
  MDS_REQUIRE_SUCCESS_AND_DUMP(SMDPProcess(false), "[MDS][SMDPProcess] failed, graph_name:[%s]", GetGraphName());
  MDS_REQUIRE_SUCCESS_AND_DUMP(PiplineProcess(), "[MDS][PiplineProcess] failed, graph_name:[%s]", GetGraphName());
  MDS_REQUIRE_SUCCESS_AND_DUMP(SetDeployInfo(), "[MDS][SetDeployInfo] failed, graph_name:[%s]", GetGraphName());
  MDS_REQUIRE_SUCCESS_AND_DUMP(compute_graph_->TopologicalSorting(), "[MDS][Topo] failed, graph_name:[%s]",
                               GetGraphName());
  GE_DUMP(compute_graph_, "AfterMDS");
  GELOGI("[MDS][%s] deploy successfully and dump.", graph->GetName().c_str());
  return SUCCESS;
}

Status ModelDeploySchedulerPass::CutProcess() {
  GE_CHECK_NOTNULL(compute_graph_);
  if (!compute_graph_->GetAllSubgraphs().empty() || compute_graph_->GetParentGraph() != nullptr) {
    GELOGW("[MDS][CutProcess] graph with subgraphs is not supported now. graph_name:[%s]", GetGraphName());
    return SUCCESS;
  }
  auto type = MdsUtils::TryGetGraphCutType(compute_graph_);
  switch (type) {
    case CutType::kCutN:
      MDS_REQUIRE_SUCCESS(CutNProcess(compute_graph_), "[MDS][CutNProcess] failed, graph_name:[%s]",
                          GetGraphName());
      break;
    case CutType::kCutH:
      MDS_REQUIRE_SUCCESS(CutHProcess(compute_graph_), "[MDS][CutHProcess] failed, graph_name:[%s]",
                          GetGraphName());
      break;
    case CutType::kDynamicCutN:
      MDS_REQUIRE_SUCCESS(CutNProcess(compute_graph_, true), "[MDS][CutNProcess] failed, graph_name:[%s]",
                          GetGraphName());
      break;
    case CutType::kDynamicCutH:
      MDS_REQUIRE_SUCCESS(CutHProcess(compute_graph_, true), "[MDS][CutHProcess] failed, graph_name:[%s]",
                          GetGraphName());
      break;
    case CutType::kDynamicCutAll:
      MDS_REQUIRE_SUCCESS(DynamicCutAll(compute_graph_), "[MDS][DynamicCutAll] failed, graph_name:[%s]",
                          GetGraphName());
      break;
    default:
      GELOGI("[MDS][CutProcess] could not cut, just return. graph_name:[%s]", GetGraphName());
  }

  return SUCCESS;
}

Status ModelDeploySchedulerPass::CutNProcess(const ComputeGraphPtr &compute_graph, bool is_dynamic) {
  GE_CHECK_NOTNULL(compute_graph);
  // step 0: Cut
  for (const auto &node : compute_graph->GetDirectNode()) {
    auto op_kernel = mds_cut_pass::GetKernelByType(node);
    if (op_kernel == nullptr) {
      op_kernel = MakeShared<DeploySchedulerKernel>();
    }
    if (is_dynamic) {
      MDS_REQUIRE_SUCCESS(op_kernel->DynamicCutN(node), "[MDS][DYNAMIC_CUTN] failed, node:[%s]",
                          node->GetName().c_str());
    } else {
      MDS_REQUIRE_SUCCESS(op_kernel->CutN(node, input_node_), "[MDS][CUTN] failed, node:[%s]", node->GetName().c_str());
    }
    bool is_grad_compute_node = false;
    if (ge::AttrUtils::GetBool(node->GetOpDesc(), ATTR_NAME_GRADIENT_NODE, is_grad_compute_node) &&
        is_grad_compute_node) {
      grad_compute_nodes_.push_back(node);
    }
  }
  // For single output multi reference insertion allgather, allreduce nodes,
  // do breadth fusion optimization in the future
  MDS_REQUIRE_SUCCESS(HcomNodeFusionProcess(), "[MDS][CUTN][HcomNodeFusionProcess] failed, compute graph:[%s]",
                      compute_graph->GetName().c_str());
  return SUCCESS;
}

Status ModelDeploySchedulerPass::CutHProcess(const ComputeGraphPtr &compute_graph, bool is_dynamic) {
  GE_CHECK_NOTNULL(compute_graph);
  for (NodePtr &node : compute_graph->GetDirectNode()) {
    auto op_kernel = mds_cut_pass::GetKernelByType(node);
    if (op_kernel == nullptr) {
      op_kernel = MakeShared<DeploySchedulerKernel>();
    }
    if (is_dynamic) {
      MDS_REQUIRE_SUCCESS(op_kernel->DynamicCutH(node), "[MDS][DYNAMIC_CUTH] failed, node:[%s]",
                          node->GetName().c_str());
    } else {
      MDS_REQUIRE_SUCCESS(op_kernel->CutH(node, input_node_), "[MDS][CUTH] failed, node:[%s]", node->GetName().c_str());
    }
  }
  return SUCCESS;
}

Status ModelDeploySchedulerPass::DynamicCutAll(const ComputeGraphPtr &compute_graph) {
  std::vector<NodePtr> input_nodes;
  std::vector<NodePtr> output_nodes;
  auto compute_graph0 = GraphUtils::CloneGraph(compute_graph, "", input_nodes, output_nodes);
  auto compute_graph1 = GraphUtils::CloneGraph(compute_graph, "", input_nodes, output_nodes);
  MDS_REQUIRE_SUCCESS(CutNProcess(compute_graph0, true), "[MDS][CutNProcess] failed, graph_name:[%s]",
                      compute_graph0->GetName().c_str());
  MDS_REQUIRE_SUCCESS(CutHProcess(compute_graph1, true), "[MDS][CutHProcess] failed, graph_name:[%s]",
                      compute_graph1->GetName().c_str());
  return SUCCESS;
}

Status ModelDeploySchedulerPass::SMDPProcess(bool before_cut) const {
  if (before_cut) {
    MDS_REQUIRE_SUCCESS(SMDPModelState(), "[SMDPProcess][SMDPModelState] failed, graph_name:[%s]", GetGraphName());
    MDS_REQUIRE_SUCCESS(SMDPWeight(), "[SMDPProcess][SMDPWeight] failed, graph_name:[%s]", GetGraphName());
  } else {
    MDS_REQUIRE_SUCCESS(SMDPGradient(), "[SMDPProcess][SMDPGradient] failed, graph_name:[%s]", GetGraphName());
  }
  return SUCCESS;
}

Status ModelDeploySchedulerPass::SetDeployInfo() {
  vector<GeAttrValue::NAMED_ATTRS> deployInfo;
  REQUIRE(!ge::AttrUtils::GetListNamedAttrs(compute_graph_, ATTR_NAME_DEPLOY_INFO, deployInfo),
          "%s already has deployed before!", GetGraphName());
  std::multimap<int64_t, GraphInputs> deploys;
  for (int64_t j = 0; j < kDeployNumber; j++) {
    int64_t device_id = j;
    GraphInputs graph_inputs;
    if (input_node_ != nullptr) {
      GeTensorPtr graph_input = MakeShared<GeTensor>(input_node_->GetOpDesc()->GetOutputDesc(0));
      vector<uint8_t> data{static_cast<uint8_t>(device_id)};
      graph_input->SetData(data);
      graph_inputs.push_back(graph_input);
    }
    deploys.emplace(j, graph_inputs);
  }
  return MdsUtils::SetDeployInfo(compute_graph_, deploys);
}

Status ModelDeploySchedulerPass::SetNodeCutInfo(const ComputeGraphPtr &compute_graph) const {
  auto instance_ptr = ge::GELib::GetInstance();
  if (instance_ptr == nullptr || !instance_ptr->InitFlag()) {
    REPORT_INNER_ERROR("E19999", "GeLib is not init before, check invalid");
    GELOGE(GE_CLI_GE_NOT_INITIALIZED, "[Check][Param] GE is not initialized");
    return FAILED;
  }
  for (const auto &node : compute_graph->GetDirectNode()) {
    GE_CHECK_NOTNULL(node);
    auto kernel_lib_name = node->GetOpDesc()->GetOpKernelLibName();
    OpsKernelInfoStorePtr kernel_info = instance_ptr->OpsKernelManagerObj().GetOpsKernelInfoStore(kernel_lib_name);
    if (kernel_info == nullptr) {
      GELOGW("[Get][OpsKernelInfoStore] by name:%s failed", kernel_lib_name.c_str());
      continue;
    }
    GE_CHK_STATUS_RET(kernel_info->SetCutSupportedInfo(node));
  }
  return SUCCESS;
}

Status ModelDeploySchedulerPass::PiplineProcess() const {
  GELOGI("[MDS][PIPLINE] just return for now");
  return SUCCESS;
}
Status ModelDeploySchedulerPass::HcomNodeFusionProcess() const {
  GELOGI("[MDS][FUSION] just return for now");
  return SUCCESS;
}
Status ModelDeploySchedulerPass::SMDPModelState() const {
  GELOGI("[MDS][SMDP0] just return for now");
  return SUCCESS;
}
Status ModelDeploySchedulerPass::SMDPGradient() const {
  GELOGI("[MDS][SMDP1] just return for now");
  return SUCCESS;
}
Status ModelDeploySchedulerPass::SMDPWeight() const {
  GELOGI("[MDS][SMDP2] just return for now");
  return SUCCESS;
}
}  // namespace ge
