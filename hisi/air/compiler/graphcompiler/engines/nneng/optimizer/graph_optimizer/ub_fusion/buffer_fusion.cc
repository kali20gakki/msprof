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

#include "graph_optimizer/ub_fusion/buffer_fusion.h"
#include <memory>
#include "common/configuration.h"
#include "graph_optimizer/fusion_common/fusion_pass_name.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_bnupdate_eltwise_eltwise_fusion_pass.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_bnupdate_eltwise_fusion_pass.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_common_rules0_fusion_pass.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_common_rules2_fusion_pass.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_dynamic_elemwise_broadcast_fusion_pass.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_dynamic_elemwise_reduce_fusion_pass.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_eltwise_fusion_pass.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_eltwise_quant_fusion_pass.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_eltwise_write_select_fussion_pass.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_multi_output_fusion_pass.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_read_select_eltwise_fussion_pass.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_reduce_elemwise_fusion_pass.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_elemwise_quant_fusion_pass.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_fixpipe_fusion_pass.h"

using std::shared_ptr;

namespace fe {
/*
 * @brief: match defined fusion pattern from graph and assign scope id to fusion
 * op
 */
Status BufferFusion::MatchFusionPatternFromGraph(ge::ComputeGraph &graph) {
  // ub fusion, te fusion && cce aicore fusion
  if (MatchFusionPattern(graph) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][PostProcess][MtcFusPtn] Failed to do UB fusion.");
    return FAILED;
  }

  return SUCCESS;
}

/*
 * @brief: create fusion graph with scope_id create by
 * MatchFusionPatternFromGraph,
 *        i.e. nodes have same scope_id will be fused into one fusion op,
 *        the topo of graph will be changed.
 */
Status BufferFusion::BuildFusionGraph(ge::ComputeGraph &graph) {
  // merge fusion node
  if (Configuration::Instance(engine_name_).EnableL1Fusion()) {
    FE_CHECK_NOTNULL(l1_fusion_graph_merge_ptr_);
    if (l1_fusion_graph_merge_ptr_->MergeFusionGraph(graph) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][PostProcess][BuildFusGraph] Failed to merge fusion graph.");
      return FAILED;
    }
  }

  FE_CHECK_NOTNULL(ub_fusion_graph_merge_ptr_);
  if (ub_fusion_graph_merge_ptr_->MergeFusionGraph(graph) != SUCCESS) {
    REPORT_FE_ERROR("[SubGraphOpt][PostProcess][BuildFusGraph] Failed to merge fusion graph.");
    return FAILED;
  }
  return SUCCESS;
}

Status BufferFusion::MatchFusionPattern(ge::ComputeGraph &graph) {
  if (cycle_detector_ == nullptr) {
    FE_MAKE_SHARED(cycle_detector_ = std::make_shared<FusionCycleDetector>(), return FAILED);
    cycle_detector_->Initialize(graph);
  }

  Status ret = RunBuiltInFusion(graph);
  if (ret != SUCCESS) {
    return ret;
  }
  return SUCCESS;
}

Status BufferFusion::RunBuiltInFusion(ge::ComputeGraph &graph) {
  BufferFusionPassType pass_type = (engine_name_ == fe::AI_CORE_NAME) ? BUILT_IN_AI_CORE_BUFFER_FUSION_PASS
                                                                      : BUILT_IN_VECTOR_CORE_BUFFER_FUSION_PASS;
  if (RunRegisterBufferFusionPass(graph, pass_type) != SUCCESS) {
    return FAILED;
  }
  if (RunUnRegisterBufferFusionPass(graph) != SUCCESS) {
    return FAILED;
  }
  return SUCCESS;
}

Status BufferFusion::RunRegisterBufferFusionPass(ge::ComputeGraph &graph, BufferFusionPassType pass_type) {
  string graph_name = graph.GetName();
  string pass_type_str = GetBufferFusionPassTypeString(pass_type);
  FE_LOGD("GraphName[%s]PassType[%s]: start to run register buffer fusion pass.", graph_name.c_str(),
          pass_type_str.c_str());
  FE_CHECK(fusion_priority_mgr_ptr_ == nullptr,
           REPORT_FE_ERROR("[SubGraphOpt][PostProcess][RunRegBufFus] Failed to run register buffer fusion pass,\
           fusion_priority_mgr_ptr_ is null."), return FAILED);
  if (fusion_priority_mgr_ptr_->sorted_buffer_fusion_vector_.empty()) {
    FE_LOGD("No fusion pass get read, BufferFusionPassType:[%u].", pass_type);
    return SUCCESS;
  }
  for (const auto &sorted_buffer_fusion_info : fusion_priority_mgr_ptr_->sorted_buffer_fusion_vector_) {
    FE_LOGD("Start buffer Fusion:%s Priority:%d", sorted_buffer_fusion_info.name.c_str(),
            FusionPriorityManager::GetRealPriority(sorted_buffer_fusion_info.priority));
    int32_t priority = FusionPriorityManager::GetRealPriority(sorted_buffer_fusion_info.priority);
    FE_LOGD_IF(priority < CUSTOM_PASS_PRIORITY_MIN,
               "Start to run buffer fusion, pass name:%s, pass type:%s, configured priority:%d, engine:%s.",
               sorted_buffer_fusion_info.name.c_str(), pass_type_str.c_str(), priority, engine_name_.c_str());
    FE_LOGD_IF(priority >= CUSTOM_PASS_PRIORITY_MIN,
               "Start to run buffer fusion, pass name:%s, pass type:%s, default priority:%d, engine:%s.",
               sorted_buffer_fusion_info.name.c_str(), pass_type_str.c_str(), priority, engine_name_.c_str());

    BufferFusionPassRunnerPtr buffer_fusion_pass_runner_ptr = nullptr;
    FE_MAKE_SHARED(buffer_fusion_pass_runner_ptr = std::make_shared<BufferFusionPassRunner>(
        sorted_buffer_fusion_info.name, sorted_buffer_fusion_info.buffer_fusion_pass_create_fn, cycle_detector_),
                   return FAILED);
    FE_CHECK_NOTNULL(buffer_fusion_pass_runner_ptr);
    Status ret = buffer_fusion_pass_runner_ptr->Run(graph);
    if (ret != SUCCESS) {
      REPORT_FE_ERROR(
          "[SubGraphOpt][UB][Match] Run buffer fusion pass failed, result %u, graph name[%s], pass[%s], type[%s]", ret,
          graph_name.c_str(), sorted_buffer_fusion_info.name.c_str(), pass_type_str.c_str());
      return ret;
    }
    FE_LOGD("Run buffer fusion pass successfully, pass name:%s.", sorted_buffer_fusion_info.name.c_str());
  }
  FE_LOGD("GraphName[%s]PassType[%s]: end to run register buffer fusion pass.", graph_name.c_str(),
          pass_type_str.c_str());
  return SUCCESS;
}

Status BufferFusion::AddVectorCorePass(const std::shared_ptr<PassManager> &tbe_ub_fusion_pass) {
  BufferFusionPassBase *(*reduce_eltwise_fn)();
  reduce_eltwise_fn = []() -> BufferFusionPassBase * { return new (std::nothrow) TbeReduceElemwiseFusionPass(); };
  FE_CHECK_NOTNULL(reduce_eltwise_fn);
  BufferFusionPassRunner *reduce_elemwise = new (std::nothrow)
      BufferFusionPassRunner(REDUCE_ELEMWISE_UB_PASS, reduce_eltwise_fn, cycle_detector_);
  FE_CHECK_NOTNULL(reduce_elemwise);
  tbe_ub_fusion_pass->AddPass(REDUCE_ELEMWISE_UB_PASS, AI_CORE_NAME, reduce_elemwise, UB_FUSION);

  BufferFusionPassBase *(*tbe_eltwise_fn)();
  tbe_eltwise_fn = []() -> BufferFusionPassBase * { return new (std::nothrow) TbeEltwiseFusionPass(); };
  FE_CHECK_NOTNULL(tbe_eltwise_fn);
  BufferFusionPassRunner *tbe_eltwise =
      new (std::nothrow) BufferFusionPassRunner(ELTWISE_UB_PASS, tbe_eltwise_fn, cycle_detector_);
  FE_CHECK_NOTNULL(tbe_eltwise);
  tbe_ub_fusion_pass->AddPass(REDUCE_ELEMWISE_UB_PASS, AI_CORE_NAME, tbe_eltwise, UB_FUSION);
  return SUCCESS;
}

/*
 * @brief: the whole fusion process
 * @return bool: if fusion process success
 */
Status BufferFusion::RunUnRegisterBufferFusionPass(ge::ComputeGraph &graph) {
  // create pass manager
  std::shared_ptr<PassManager> tbe_ub_fusion_pass(
      new (std::nothrow) PassManager(fusion_priority_mgr_ptr_->GetFusionConfigParserPtr()));
  FE_CHECK(tbe_ub_fusion_pass == nullptr,
           REPORT_FE_ERROR("[SubGraphOpt][PostProcess][RunUnRegBufFus] Pass manager is null."), return FAILED);
  BufferFusionPassBase *(*create_fn)() = nullptr;

  if (engine_name_ == AI_CORE_NAME) {
    FE_LOGD("GraphName[%s]: start to RunUnRegisterBufferFusionPass in engine [%s]", graph.GetName().c_str(),
            engine_name_.c_str());
    // 5. TbeFixpipeFusionPass
    create_fn = []() -> BufferFusionPassBase * { return new (std::nothrow) TbeFixPipeFusionPass(); };
    FE_CHECK_NOTNULL(create_fn);
    BufferFusionPassRunner *fixpipe_pass = new (std::nothrow)
            BufferFusionPassRunner(FIXPIPE_FUSION_PASS, create_fn, cycle_detector_);
    create_fn = nullptr;
    FE_CHECK_NOTNULL(fixpipe_pass);
    tbe_ub_fusion_pass->AddPass(FIXPIPE_FUSION_PASS, AI_CORE_NAME, fixpipe_pass, UB_FUSION);
    // 6. TbeCommonRules2FusionPass
    create_fn = []() -> BufferFusionPassBase * { return new (std::nothrow) TbeCommonRules2FusionPass(); };
    FE_CHECK_NOTNULL(create_fn);
    BufferFusionPassRunner *common_rules2 = new (std::nothrow)
        BufferFusionPassRunner(COMMON_RULES2_UB_PASS, create_fn, cycle_detector_);
    create_fn = nullptr;
    FE_CHECK_NOTNULL(common_rules2);
    tbe_ub_fusion_pass->AddPass(COMMON_RULES2_UB_PASS, AI_CORE_NAME, common_rules2, UB_FUSION);
    // 7. TbeCommonRules0FusionPass
    create_fn = []() -> BufferFusionPassBase * { return new (std::nothrow) TbeCommonRules0FusionPass(); };
    FE_CHECK_NOTNULL(create_fn);
    BufferFusionPassRunner *common_rules0 = new (std::nothrow)
        BufferFusionPassRunner(COMMON_RULES0_UB_PASS, create_fn, cycle_detector_);
    create_fn = nullptr;
    FE_CHECK_NOTNULL(common_rules0);
    tbe_ub_fusion_pass->AddPass(COMMON_RULES0_UB_PASS, AI_CORE_NAME, common_rules0, UB_FUSION);

    // 15. TbeBnupdateEltwiseFusionPass
    create_fn = []() -> BufferFusionPassBase * { return new (std::nothrow) TbeBnupdateEltwiseFusionPass(); };
    FE_CHECK_NOTNULL(create_fn);
    BufferFusionPassRunner *bn_update_eltwise = new (std::nothrow)
        BufferFusionPassRunner(BNUPDATE_ELTWISE_UB_PASS, create_fn, cycle_detector_);
    create_fn = nullptr;
    FE_CHECK_NOTNULL(bn_update_eltwise);
    tbe_ub_fusion_pass->AddPass(BNUPDATE_ELTWISE_UB_PASS, AI_CORE_NAME, bn_update_eltwise, UB_FUSION);

    // 16. TbeBnupdateEltwiseEltwiseFusionPass
    create_fn = []() -> BufferFusionPassBase * { return new (std::nothrow) TbeBnupdateEltwiseEltwiseFusionPass(); };
    FE_CHECK_NOTNULL(create_fn);
    BufferFusionPassRunner *bn_update_eltwise_eltwise = new (std::nothrow)
        BufferFusionPassRunner(BNUPDATE_ELTWISE_ELTWISE_UB_PASS, create_fn, cycle_detector_);
    create_fn = nullptr;
    FE_CHECK_NOTNULL(bn_update_eltwise_eltwise);
    tbe_ub_fusion_pass->AddPass(BNUPDATE_ELTWISE_ELTWISE_UB_PASS, AI_CORE_NAME, bn_update_eltwise_eltwise, UB_FUSION);

    // 19. TbeMultiOutputFusionPass
    create_fn = []() -> BufferFusionPassBase * { return new (std::nothrow) TbeMultiOutputFusionPass(); };
    FE_CHECK_NOTNULL(create_fn);
    BufferFusionPassRunner *multi_output =
        new (std::nothrow) BufferFusionPassRunner(MULTIOUTPUT_UB_PASS, create_fn, cycle_detector_);
    create_fn = nullptr;
    FE_CHECK_NOTNULL(multi_output);
    tbe_ub_fusion_pass->AddPass(MULTIOUTPUT_UB_PASS, AI_CORE_NAME, multi_output, UB_FUSION);

    // 20. TbeReduceElemwiseFusionPass
    create_fn = []() -> BufferFusionPassBase * { return new (std::nothrow) TbeReduceElemwiseFusionPass(); };
    FE_CHECK_NOTNULL(create_fn);
    BufferFusionPassRunner *reduce_elemwise = new (std::nothrow)
        BufferFusionPassRunner(REDUCE_ELEMWISE_UB_PASS, create_fn, cycle_detector_);
    create_fn = nullptr;
    FE_CHECK_NOTNULL(reduce_elemwise);
    tbe_ub_fusion_pass->AddPass(REDUCE_ELEMWISE_UB_PASS, AI_CORE_NAME, reduce_elemwise, UB_FUSION);

    // 22. read_select + eltwise
    create_fn = []() -> BufferFusionPassBase * { return new (std::nothrow) TbeReadSelectEltwiseFusionPass(); };
    FE_CHECK_NOTNULL(create_fn);
    BufferFusionPassRunner *read_select_eltwise_pass = new (std::nothrow)
        BufferFusionPassRunner(READSELECT_ELTWISE_UB_PASS, create_fn, cycle_detector_);
    create_fn = nullptr;
    FE_CHECK_NOTNULL(read_select_eltwise_pass);
    tbe_ub_fusion_pass->AddPass(READSELECT_ELTWISE_UB_PASS, AI_CORE_NAME, read_select_eltwise_pass, UB_FUSION);

    // 23. eltwise + write_select
    create_fn = []() -> BufferFusionPassBase * { return new (std::nothrow) TbeEltwiseWriteSelectFusionPass(); };
    FE_CHECK_NOTNULL(create_fn);
    BufferFusionPassRunner *eltwise_write_select_pass = new (std::nothrow)
        BufferFusionPassRunner(ELTWISE_WRITESELECT_UB_PASS, create_fn, cycle_detector_);
    create_fn = nullptr;
    FE_CHECK_NOTNULL(eltwise_write_select_pass);
    tbe_ub_fusion_pass->AddPass(ELTWISE_WRITESELECT_UB_PASS, AI_CORE_NAME, eltwise_write_select_pass, UB_FUSION);

    // 24. TbeEltwiseQuantFusionPass
    create_fn = []() -> BufferFusionPassBase * { return new (std::nothrow) TbeEltwiseQuantFusionPass(); };
    FE_CHECK_NOTNULL(create_fn);
    BufferFusionPassRunner *elemwise_quant_pass = new (std::nothrow)
        BufferFusionPassRunner(ELTWISE_QUANT_UB_PASS, create_fn, cycle_detector_);
    create_fn = nullptr;
    FE_CHECK_NOTNULL(elemwise_quant_pass);
    tbe_ub_fusion_pass->AddPass(ELTWISE_QUANT_UB_PASS, AI_CORE_NAME, elemwise_quant_pass, UB_FUSION);

    // 25. TbeEltwiseFusionPass
    create_fn = []() -> BufferFusionPassBase * { return new (std::nothrow) TbeEltwiseFusionPass(); };
    FE_CHECK_NOTNULL(create_fn);
    BufferFusionPassRunner *elemwise =
        new (std::nothrow) BufferFusionPassRunner(ELTWISE_UB_PASS, create_fn, cycle_detector_);
    create_fn = nullptr;
    FE_CHECK_NOTNULL(elemwise);
    tbe_ub_fusion_pass->AddPass(ELTWISE_UB_PASS, AI_CORE_NAME, elemwise, UB_FUSION);

    // 26. TbeDynamicElemwiseReduceFusionPass
    create_fn = []() -> BufferFusionPassBase * { return new (std::nothrow) TbeDynamicElemwiseReduceFusionPass(); };
    FE_CHECK_NOTNULL(create_fn);
    BufferFusionPassRunner *dyn_elemwise_reduce = new (std::nothrow)
        BufferFusionPassRunner(DYNAMIC_ELEMWISE_REDUCE_UB_PASS, create_fn, cycle_detector_);
    create_fn = nullptr;
    FE_CHECK_NOTNULL(dyn_elemwise_reduce);
    tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_REDUCE_UB_PASS, AI_CORE_NAME, dyn_elemwise_reduce, UB_FUSION);

    // 27. TbeDynamicElemwiseBroadcastFusionPass
    create_fn = []() -> BufferFusionPassBase * { return new (std::nothrow) TbeDynamicElemwiseBroadcastFusionPass(); };
    FE_CHECK_NOTNULL(create_fn);
    BufferFusionPassRunner *dyn_elemwise_broadcast = new (std::nothrow)
        BufferFusionPassRunner(DYNAMIC_ELEMWISE_BROADCAST_UB_PASS, create_fn, cycle_detector_);
    create_fn = nullptr;
    FE_CHECK_NOTNULL(dyn_elemwise_broadcast);
    tbe_ub_fusion_pass->AddPass(DYNAMIC_ELEMWISE_BROADCAST_UB_PASS, AI_CORE_NAME, dyn_elemwise_broadcast, UB_FUSION);

    // 28. TbeElemwiseQuantFusionPass
    create_fn = []() -> BufferFusionPassBase * { return new (std::nothrow) TbeElemwiseQuantFusionPass(); };
    FE_CHECK_NOTNULL(create_fn);
    BufferFusionPassRunner *tbe_elemwise_quant = new (std::nothrow)
            BufferFusionPassRunner(ELEMWISE_QUANT_UB_PASS, create_fn, cycle_detector_);
    create_fn = nullptr;
    FE_CHECK_NOTNULL(tbe_elemwise_quant);
    tbe_ub_fusion_pass->AddPass(ELEMWISE_QUANT_UB_PASS, AI_CORE_NAME, tbe_elemwise_quant, UB_FUSION);
  } else if (engine_name_ == VECTOR_CORE_NAME) {
    FE_LOGD("GraphName[%s]: start to RunUnRegisterBufferFusionPass in engine [%s].", graph.GetName().c_str(),
            engine_name_.c_str());
    // add passes of vectorcore to pass manager
    AddVectorCorePass(tbe_ub_fusion_pass);
  }
  // pass manager run to call each pass to do fusion pattern matching and
  // assign scope id to fusion op.
  Status ret = tbe_ub_fusion_pass->Run(graph);
  if (ret != SUCCESS && ret != NOT_CHANGED) {
    REPORT_FE_ERROR("[SubGraphOpt][PostProcess][RunUnRegBufFus] Tbe ub fusion failed.");
    return FAILED;
  }
  FE_LOGD("GraphName[%s]: end to RunUnRegisterBufferFusionPass in engine [%s].", graph.GetName().c_str(),
          engine_name_.c_str());
  return SUCCESS;
}

}  // namespace fe
