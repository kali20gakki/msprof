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

#include "auto_thread_task_builder.h"
#include "inc/ffts_utils.h"

namespace ffts {
AutoTheadTaskBuilder::AutoTheadTaskBuilder() {
  mode_type_ = ModeType::AUTO_MODE_TYPE;
}
AutoTheadTaskBuilder::~AutoTheadTaskBuilder() {}


Status AutoTheadTaskBuilder::Initialize() {
  FFTS_MAKE_SHARED(aic_aiv_dynamic_task_builder_ptr_ = std::make_shared<AICAIVDynamicTaskBuilder>(), return FAILED);
  FFTS_MAKE_SHARED(aic_aiv_auto_task_builder_ptr_ = std::make_shared<AICAIVAutoTaskBuilder>(), return FAILED);
  FFTS_MAKE_SHARED(mix_aic_aiv_auto_task_builder_ptr_ = std::make_shared<MixAICAIVAutoTaskBuilder>(), return FAILED);
  FFTS_MAKE_SHARED(aicpu_auto_task_builder_ptr_ = std::make_shared<AicpuAutoTaskBuilder>(), return FAILED);
  return SUCCESS;
}

void AutoTheadTaskBuilder::SetCtxIdList(ge::NodePtr &node, uint32_t &context_id, const uint32_t &window_size) const {
  // for node self
  vector<uint32_t> context_id_list;
  for (size_t i = 0; i < window_size;  i++) {
    context_id_list.push_back(context_id++);
  }
  (void)ge::AttrUtils::SetListInt(node->GetOpDesc(), kAutoCtxIdList, context_id_list);
}

void AutoTheadTaskBuilder::GetStartOrEndFlag(const ge::NodePtr &node, bool &conn_start, bool &conn_end) const {
  for (auto up_node : node->GetInAllNodes()) {
    ge::OpDescPtr up_op_desc = up_node->GetOpDesc();
    if (up_op_desc->GetType() == CONSTANT || up_op_desc->GetType() == CONSTANTOP) {
      continue;
    }
    if (!IsSubGraphData(up_op_desc)) {
      conn_start = false;
      break;
    }
  }
  for (auto next_node : node->GetOutAllNodes()) {
    ge::OpDescPtr next_op_desc = next_node->GetOpDesc();
    if (IsSubGraphNetOutput(next_op_desc)) {
      conn_end = true;
      break;
    }
  }
}

void AutoTheadTaskBuilder::SetAllAttrInFirstNode(ge::ComputeGraph &sgt_graph,
                                                 const vector<uint32_t> &at_start_ctx_id_list,
                                                 const uint32_t &out_label_ctx_id) const {
  // Record in_label at_start at_end out_label and nodeself in first node.
  vector<uint32_t> all_ctx_id_list;
  all_ctx_id_list.push_back(0);
  for (size_t i = 0; i < at_start_ctx_id_list.size(); i++) {
    all_ctx_id_list.push_back(at_start_ctx_id_list[i]);
  }
  all_ctx_id_list.push_back(out_label_ctx_id);
  ge::AttrUtils::SetListInt(&sgt_graph, "_all_ctx_id_list", all_ctx_id_list);
}

void AutoTheadTaskBuilder::SetAttrExceptCtxIdList(ge::ComputeGraph &sgt_graph,
                                                  const vector<uint32_t> &at_start_ctx_id_list,
                                                  const vector<uint32_t> &at_end_ctx_id_list, int &count_node_conn_end,
                                                  const uint32_t &out_label_ctx_id,
                                                  std::vector<ge::NodePtr> &sub_graph_nodes,
                                                  uint64_t &total_context_number) {
  uint32_t at_end_pre_cnt = count_node_conn_end;
  bool first_node_conn_start = true;
  // set context id to node
  for (auto node : sgt_graph.GetDirectNode()) {
    ge::OpDescPtr op_desc = node->GetOpDesc();
    if (IsNoCtx(node)) {
      continue;
    }

    // deal nodes connect at_start or at_end
    bool conn_start = true;
    bool conn_end = false;
    GetStartOrEndFlag(node, conn_start, conn_end);

    FFTS_LOGD("Deal with node: %s", op_desc->GetName().c_str());

    if (conn_start) {
      (void)ge::AttrUtils::SetStr(&sgt_graph, kFftsFirstOpName, op_desc->GetName());
      // at_start connext node which is first node has this attribute
      FFTS_LOGD("Start node_name: %s, first_node_conn_start: %d (1).",
                op_desc->GetName().c_str(), first_node_conn_start);
      if (first_node_conn_start) {
        ge::AttrUtils::SetInt(op_desc, kAutoInlabelCtxId, total_context_number);
        first_node_conn_start = false;

        // Record in_label at_start at_end out_label and nodeself in first node.
        SetAllAttrInFirstNode(sgt_graph, at_start_ctx_id_list, out_label_ctx_id);
      }
      ge::AttrUtils::SetListInt(op_desc, kAutoAtStartCtxIdList, at_start_ctx_id_list);
    }
    if (conn_end) {
      // node connect at_end which is last node has this attribute
      count_node_conn_end--;
      FFTS_LOGD("End node_name: %s, count_node_conn_end: %d (0).", op_desc->GetName().c_str(), count_node_conn_end);
      if (count_node_conn_end == 0) {
        ge::AttrUtils::SetInt(op_desc, kAutoOutlabelCtxId, out_label_ctx_id);
        ge::AttrUtils::SetInt(op_desc, kAutoAtEndPreCnt, at_end_pre_cnt);
      }
      ge::AttrUtils::SetListInt(op_desc, kAutoAtEndCtxIdList, at_end_ctx_id_list);
    }
    sub_graph_nodes.push_back(node);
  }
}

Status AutoTheadTaskBuilder::GenFftsPlusContextId(ge::ComputeGraph &sgt_graph,
                                                  std::vector<ge::NodePtr> &sub_graph_nodes,
                                                  uint64_t &ready_context_num, uint64_t &total_context_number) {
  uint32_t contextId = total_context_number + 1;
  uint32_t window_size = 1;
  for (const auto &node : sgt_graph.GetDirectNode()) {
    ThreadSliceMapPtr slice_info_ptr = nullptr;
    slice_info_ptr = node->GetOpDesc()->TryGetExtAttr(kAttrSgtStructInfo, slice_info_ptr);
    if (slice_info_ptr != nullptr && slice_info_ptr->parallel_window_size > 0) {
      window_size = slice_info_ptr->parallel_window_size;
      break;
    }
  }
  window_size = window_size > 0xFFFF ? kDefaultWindowSize : window_size;
  FFTS_LOGD("Auto mode, current windown size: %d.", window_size);
  ready_context_num = 1;
  // auto theading
  vector<uint32_t> at_start_ctx_id_list;
  int count_node_conn_end = 0;

  // generate at_start context id
  for (size_t i = 0; i < window_size; i++) {
    at_start_ctx_id_list.push_back(contextId++);
  }

  FFTS_LOGD("auto threading at start ctx id list size: %zu", at_start_ctx_id_list.size());

  // generate node context id
  for (auto node : sgt_graph.GetDirectNode()) {
    ge::OpDescPtr op_desc = node->GetOpDesc();
    if (IsNoCtx(node)) {
      continue;
    }
    SetCtxIdList(node, contextId, window_size);
    // node->at-end when node has output from out sgt_graph
    bool conn_end = false;
    for (auto next_node : node->GetOutAllNodes()) {
      ge::OpDescPtr next_op_desc = next_node->GetOpDesc();
      if (IsSubGraphNetOutput(next_op_desc)) {
        conn_end = true;
        break;
      }
    }
    if (conn_end) {
      // generate at_end(context id, pre_cnt) and label(context id).
      count_node_conn_end++;
    }
  }

  // generate at_end and output_label context id
  vector<uint32_t> at_end_ctx_id_list;
  for (size_t i = 0; i < window_size; i++) {
    at_end_ctx_id_list.push_back(contextId++);
  }
  uint32_t out_label_ctx_id = contextId++;
  FFTS_LOGD("auto threading at end pre cnt: %u, out_label_ctx_id: %u", count_node_conn_end, out_label_ctx_id);

  SetAttrExceptCtxIdList(sgt_graph, at_start_ctx_id_list, at_end_ctx_id_list, count_node_conn_end, out_label_ctx_id,
                         sub_graph_nodes, total_context_number);
  total_context_number = contextId;
  return SUCCESS;
}

Status AutoTheadTaskBuilder::GenInLabelAtStartCtxDef(const ge::NodePtr &node,
                                                     domi::FftsPlusTaskDef *ffts_plus_task_def) const {
  ge::OpDescPtr op_desc = node->GetOpDesc();

  ThreadSliceMapPtr slice_info_ptr = nullptr;
  slice_info_ptr = node->GetOpDesc()->TryGetExtAttr(kAttrSgtStructInfo, slice_info_ptr);

  FFTS_LOGD("GenInLabelAtStartCtxDef node's name: %s", node->GetName().c_str());
  FFTS_CHECK_NOTNULL(slice_info_ptr);
  if (!slice_info_ptr->thread_mode) {
    FFTS_LOGD("Manual node[%s] don't generate inlabel and at_start.", node->GetOpDesc()->GetName().c_str());
    return SUCCESS;
  }

  // check
  uint32_t in_label_ctx_id;
  vector<uint32_t> at_start_ctx_id_list;
  (void)ge::AttrUtils::GetListInt(op_desc, kAutoAtStartCtxIdList, at_start_ctx_id_list);
  if (!ge::AttrUtils::GetInt(op_desc, kAutoInlabelCtxId, in_label_ctx_id) || (at_start_ctx_id_list.empty())) {
    REPORT_FFTS_ERROR("[GenerateTask][GenSubGraphTaskDef][GenInLabelAtStartCtxDef] Node %s has no in_label or at_start \
                      context id", node->GetName().c_str());
    return FAILED;
  }

  // generate in_label context def
  domi::FftsPlusCtxDef *ffts_plus_ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();
  ffts_plus_ctx_def->set_context_type(RT_CTX_TYPE_LABEL);
  domi::FftsPlusLabelCtxDef *in_label_ctx_def = ffts_plus_ctx_def->mutable_label_ctx();
  in_label_ctx_def->set_pred_cnt(0);
  in_label_ctx_def->set_pred_cnt_init(0);
  in_label_ctx_def->set_successor_num(at_start_ctx_id_list.size());
  for (size_t i = 0; i < at_start_ctx_id_list.size(); i++) {
    in_label_ctx_def->add_successor_list(i + 1);
  }

  // generate at_start_list context def
  for (size_t i = 0; i < at_start_ctx_id_list.size(); i++) {
    ffts_plus_ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();
    ffts_plus_ctx_def->set_context_type(RT_CTX_TYPE_AT_START);
    domi::FftsPlusAtStartCtxDef *at_start_ctx_def = ffts_plus_ctx_def->mutable_at_start_ctx();
    at_start_ctx_def->set_aten(1);
    at_start_ctx_def->set_pred_cnt(1);
    at_start_ctx_def->set_pred_cnt_init(1);
    at_start_ctx_def->set_thread_id(i);
    at_start_ctx_def->set_thread_id_init(i);
    at_start_ctx_def->set_thread_dim(slice_info_ptr->slice_instance_num);
    at_start_ctx_def->set_thread_window_size(slice_info_ptr->parallel_window_size);
    at_start_ctx_def->set_successor_num(0);
  }
  return SUCCESS;
}

Status AutoTheadTaskBuilder::GenOutLabelAtEndCtxDef(const ge::NodePtr &node,
                                                    domi::FftsPlusTaskDef *ffts_plus_task_def) const {
  ge::OpDescPtr op_desc = node->GetOpDesc();

  ThreadSliceMapPtr slice_info_ptr = nullptr;
  slice_info_ptr = node->GetOpDesc()->TryGetExtAttr(kAttrSgtStructInfo, slice_info_ptr);
  FFTS_CHECK_NOTNULL(slice_info_ptr);
  if (!slice_info_ptr->thread_mode) {
    FFTS_LOGD("Manual node[%s] don't generate outlabel and at_end.", node->GetOpDesc()->GetName().c_str());
    return SUCCESS;
  }

  // check
  vector<uint32_t> at_end_ctx_id_list;
  (void)ge::AttrUtils::GetListInt(op_desc, kAutoAtEndCtxIdList, at_end_ctx_id_list);
  uint32_t at_end_pre_cnt = 0;
  (void)ge::AttrUtils::GetInt(op_desc, kAutoAtEndPreCnt, at_end_pre_cnt);
  uint32_t out_label_ctx_id = 0;
  (void)ge::AttrUtils::GetInt(op_desc, kAutoOutlabelCtxId, out_label_ctx_id);

  if (at_end_ctx_id_list.empty() || (at_end_pre_cnt == 0) || (out_label_ctx_id == 0)) {
    REPORT_FFTS_ERROR("[GenerateTask][GenSubGraphTaskDef][GenOutLabelAtEndCtxDef] Node %s has no out_label, at_end \
                      context id and at_end_pre_cnt", node->GetName().c_str());
    return FAILED;
  }

  // generate at_end context def
  for (size_t i = 0; i < at_end_ctx_id_list.size(); i++) {
    domi::FftsPlusCtxDef *ffts_plus_ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();
    ffts_plus_ctx_def->set_context_type(RT_CTX_TYPE_AT_END);
    domi::FftsPlusAtEndCtxDef *at_end_ctx_def = ffts_plus_ctx_def->mutable_at_end_ctx();
    at_end_ctx_def->set_aten(1);  // auto thread
    at_end_ctx_def->set_pred_cnt(at_end_pre_cnt);
    at_end_ctx_def->set_pred_cnt_init(at_end_pre_cnt);
    at_end_ctx_def->set_at_start_slot_num(1);
    at_end_ctx_def->add_succ_at_start_slot(i + 1);
    at_end_ctx_def->set_out_label_slot_num(1);
    at_end_ctx_def->add_succ_out_label_slot(out_label_ctx_id);
  }

  // generate out_label context def
  domi::FftsPlusCtxDef *ffts_plus_ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();
  ffts_plus_ctx_def->set_context_type(RT_CTX_TYPE_LABEL);
  domi::FftsPlusLabelCtxDef *out_label_ctx_def = ffts_plus_ctx_def->mutable_label_ctx();
  out_label_ctx_def->set_pred_cnt(slice_info_ptr->slice_instance_num);
  out_label_ctx_def->set_pred_cnt_init(slice_info_ptr->slice_instance_num);
  out_label_ctx_def->set_successor_num(0);

  return SUCCESS;
}

Status AutoTheadTaskBuilder::AddSuccListInCtx(domi::FftsPlusTaskDef *ffts_plus_task_def,
                                              const FFTSPlusTaskBuilderPtr &task_builder,
                                              const vector<uint32_t> &context_id_list,
                                              const vector<uint32_t> &output_context_id_list,
                                              const bool &flag_add_write_back) const {
  if (!flag_add_write_back) {
    if (output_context_id_list.empty() || context_id_list.empty() ||
        output_context_id_list.size() != context_id_list.size()) {
      return FAILED;
    }
    FFTS_LOGD("Curren node don't need to write_back, start to add first at_end[%u] to first current context's[%u] \
              successor_list.", output_context_id_list[0], context_id_list[0]);

    for (size_t i = 0; i < output_context_id_list.size(); i++) {
      Status status = task_builder->UpdateSuccList(output_context_id_list[i], context_id_list[i], ffts_plus_task_def);
      if (status != SUCCESS) {
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

Status AutoTheadTaskBuilder::FillContextSuccList(const ge::NodePtr &sub_node, domi::FftsPlusTaskDef *ffts_plus_task_def,
                                                 const FFTSPlusTaskBuilderPtr &task_builder,
                                                 const vector<uint32_t> &context_id_list,
                                                 const vector<uint32_t> &at_end_ctx_id_list) const {
  bool netoutput_flag = false;
  for (auto output_node : sub_node->GetOutAllNodes()) {
    bool other_netoutput = netoutput_flag && output_node->GetType() == "NetOutput";
    if (other_netoutput) {
      continue;
    }
    if (output_node->GetType() == "NetOutput") {
      netoutput_flag = true;
    }
    ge::OpDescPtr node_desc = output_node->GetOpDesc();
    vector<uint32_t> output_context_id_list;
    (void)ge::AttrUtils::GetListInt(node_desc, kAutoCtxIdList, output_context_id_list);

    FFTS_LOGD("Current output_node name is: %s.", node_desc->GetName().c_str());
    bool flag_add_write_back = false;
    bool flag_add_end_to_write_back = node_desc->GetType() == "NetOutput";
    if (flag_add_end_to_write_back) {
      output_context_id_list = at_end_ctx_id_list;
      // if context need to write back, add at_end_ctx_id to data_write_back's succ_list
      bool already_add = false;

      for (size_t i = 0; i < context_id_list.size(); i++) {
        domi::FftsPlusCtxDef* ffts_plus_ctx =
                ffts_plus_task_def->mutable_ffts_plus_ctx(static_cast<int>(context_id_list[i]));
        uint32_t type = ffts_plus_ctx->context_type();
        switch (type) {
          case RT_CTX_TYPE_AICORE:
          case RT_CTX_TYPE_AIV:
            FFTSPlusTaskBuilder::add_at_end_to_write_back_succ_list(at_end_ctx_id_list[i],
                ffts_plus_ctx->mutable_aic_aiv_ctx(), ffts_plus_task_def, already_add);
            break;
          case RT_CTX_TYPE_MIX_AIC:
          case RT_CTX_TYPE_MIX_AIV:
            FFTSPlusTaskBuilder::add_at_end_to_write_back_succ_list(at_end_ctx_id_list[i],
                ffts_plus_ctx->mutable_mix_aic_aiv_ctx(), ffts_plus_task_def, already_add);
            break;
          case RT_CTX_TYPE_SDMA:
            FFTSPlusTaskBuilder::add_at_end_to_write_back_succ_list(at_end_ctx_id_list[i],
                ffts_plus_ctx->mutable_sdma_ctx(), ffts_plus_task_def, already_add);
            break;
          case RT_CTX_TYPE_NOTIFY_WAIT:
          case RT_CTX_TYPE_NOTIFY_RECORD:
            FFTSPlusTaskBuilder::add_at_end_to_write_back_succ_list(at_end_ctx_id_list[i],
                ffts_plus_ctx->mutable_notify_ctx(), ffts_plus_task_def, already_add);
            break;
          case RT_CTX_TYPE_WRITE_VALUE:
            FFTSPlusTaskBuilder::add_at_end_to_write_back_succ_list(at_end_ctx_id_list[i],
                ffts_plus_ctx->mutable_write_value_ctx(), ffts_plus_task_def, already_add);
          default:
            break;
        }
      }
      if (already_add) {
        flag_add_write_back = true;
        continue;
      }
    }

    Status status = AddSuccListInCtx(ffts_plus_task_def, task_builder, context_id_list, output_context_id_list,
                                     flag_add_write_back);
    if (status != SUCCESS) {
      REPORT_FFTS_ERROR("[GenerateTask][AutoTheadTaskBuilder][AddSuccListInCtx] Add succ_list in context failed.");
      return status;
    }
  }
  return SUCCESS;
}

Status AutoTheadTaskBuilder::GenSubGraphTaskDef(std::vector<ge::NodePtr> &sub_graph_nodes,
                                                domi::TaskDef &task_def) {
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  FFTS_CHECK_NOTNULL(ffts_plus_task_def);

  if (!sub_graph_nodes.empty()) {
    Status status = GenInLabelAtStartCtxDef(sub_graph_nodes[0], ffts_plus_task_def);
    if (status != SUCCESS) {
      return FAILED;
    }
  }

  for (auto &sub_node : sub_graph_nodes) {
    TaskBuilderType task_builder_type;
    Status status = GetNodeContextTypeByNode(sub_node, task_builder_type);
    if (status != SUCCESS) {
      return FAILED;
    }
    FFTSPlusTaskBuilderPtr task_builder = GetTaskBuilder(task_builder_type);
    FFTS_CHECK_NOTNULL(task_builder);
    status = task_builder->GenerateTaskDef(sub_node, ffts_plus_task_def);
    if (status != SUCCESS) {
      return status;
    }
  }

  Status status = GenOutLabelAtEndCtxDef(sub_graph_nodes[sub_graph_nodes.size() - 1], ffts_plus_task_def);
  if (status != SUCCESS) {
    return status;
  }
  FFTS_LOGD("Current ffts_plus_task_def size is: %u.", ffts_plus_task_def->ffts_plus_ctx_size());

  for (auto &sub_node : sub_graph_nodes) {
    ge::OpDescPtr op_desc = sub_node->GetOpDesc();
    vector<uint32_t> at_start_ctx_id_list;
    ge::AttrUtils::GetListInt(op_desc, kAutoAtStartCtxIdList, at_start_ctx_id_list);
    vector<uint32_t> context_id_list;
    ge::AttrUtils::GetListInt(op_desc, kAutoCtxIdList, context_id_list);
    vector<uint32_t> at_end_ctx_id_list;
    ge::AttrUtils::GetListInt(op_desc, kAutoAtEndCtxIdList, at_end_ctx_id_list);

    FFTS_LOGD("Current sub_node name is: %s, at_start_ctx_id_list size: %zu, context_id_list size: %zu, \
              at_end_ctx_id_list size: %zu.", sub_node->GetName().c_str(), at_start_ctx_id_list.size(),
              context_id_list.size(), at_end_ctx_id_list.size());
    // fill at_start's succ_list
    FFTSPlusTaskBuilderPtr task_builder = nullptr;
    FFTS_MAKE_SHARED(task_builder = std::make_shared<FFTSPlusTaskBuilder>(), return FAILED);
    if (!at_start_ctx_id_list.empty()) {
      for (size_t i = 0; i < at_start_ctx_id_list.size(); i++) {
        FFTS_LOGD("index: %zu, context_id_list: %u, at_start_ctx_id_list: %u, ffts_plus_task_def size: %d.", i,
                  context_id_list[i], at_start_ctx_id_list[i], ffts_plus_task_def->ffts_plus_ctx_size());
        status = task_builder->UpdateSuccList(context_id_list[i], at_start_ctx_id_list[i], ffts_plus_task_def);
        if (status != SUCCESS) {
          return status;
        }
      }
    }

    FFTS_LOGD("Current sub_node name is: %s.", sub_node->GetName().c_str());
    status = GenerateDataTaskDef(sub_node, ffts_plus_task_def, mode_type_);
    if (status != SUCCESS) {
      return status;
    }

    // fill node context's succ_list
    status = FillContextSuccList(sub_node, ffts_plus_task_def, task_builder, context_id_list, at_end_ctx_id_list);
    if (status != SUCCESS) {
      FFTS_LOGE("Fill context succ list failed. Op[%s, optype[%s]]",
                op_desc->GetName().c_str(), op_desc->GetType().c_str());
      return status;
    }
  }
  return SUCCESS;
}
}  // namespace ffts
