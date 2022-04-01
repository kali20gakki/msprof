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
#include <gtest/gtest.h>
#include <iostream>

#include <list>

#define private public
#define protected public
#include "common/sgt_slice_type.h"
#include "inc/ffts_log.h"
#include "inc/ffts_error_codes.h"
#include "inc/ffts_type.h"
#include "task_builder/mode/data_task_builder.h"
#include "task_builder/data_ctx/prefetch_manual_task_builder.h"
#include "task_builder/data_ctx/out_manual_task_builder.h"
#include "task_builder/fftsplus_ops_kernel_builder.h"
#include "optimizer/cache_optimizer/cache_manager.h"
#include "graph/node.h"
#include "graph/utils/tensor_utils.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "runtime/context.h"
#include "runtime/stream.h"
#include "runtime/rt_model.h"
#include "runtime/kernel.h"
#include "runtime/mem.h"
#include "../../../graph_constructor/graph_constructor.h"
using namespace std;
using namespace ffts;
using namespace ge;

class FftsPlusDataContextST : public testing::Test {
 protected:
  void SetUp() {
    prefetch_.SetOperation(CACHE_OPERATION::PREFETCH);
    invalidate_.SetOperation(CACHE_OPERATION::INVALIDATE);
    write_back_.SetOperation(CACHE_OPERATION::WRITE_BACK);
    prefetch_auto_.SetOperation(CACHE_OPERATION::PREFETCH);
    invalidate_auto_.SetOperation(CACHE_OPERATION::INVALIDATE);
    write_back_auto_.SetOperation(CACHE_OPERATION::WRITE_BACK);
    prefetch_dyn_.SetOperation(CACHE_OPERATION::PREFETCH);
    invalidate_dyn_.SetOperation(CACHE_OPERATION::INVALIDATE);
    write_back_dyn_.SetOperation(CACHE_OPERATION::WRITE_BACK);
  }

  void TearDown() {

  }
  void AssembleTensor(GeTensorDescPtr &tensor,
                      const vector<uint32_t> &axes,
                      uint32_t thread_id,
                      uint32_t slice_num,
                      vector<vector<DimRange>> &one_thread) {
    vector<DimRange> one_tensor;
    uint32_t axis_index = 0;
    vector<int64_t> new_dim = tensor->GetShape().GetDims();
    for (int64_t dim : tensor->GetShape().GetDims()) {
      DimRange range = {0, dim};
      if (std::find(axes.begin(), axes.end(), axis_index) != axes.end()) {
        // only the highest dimension needs to be sliced.
        int64_t dim_non_tail = dim / slice_num;
        int64_t low, high;
        if (thread_id == slice_num - 1) {
          low = thread_id * dim_non_tail;
          high = dim;
        } else {
          low = thread_id * dim_non_tail;
          high = (thread_id + 1) * dim_non_tail;
        }
        range = {low, high};
        new_dim[axis_index] = high - low;
      } else {
        range = {0, dim};
      }
      one_tensor.emplace_back(range);
      axis_index++;
    }

    tensor->SetOriginShape(ge::GeShape(new_dim));
    tensor->SetShape(ge::GeShape(new_dim));
    one_thread.emplace_back(one_tensor);
  }

  void AssembleAutoThreadTensor(GeTensorDescPtr &tensor,
                                uint32_t thread_id,
                                uint32_t slice_num,
                                vector<vector<DimRange>> &one_thread) {
    vector<DimRange> one_tensor;
    uint32_t axis_index = 0;
    vector<int64_t> new_dim = tensor->GetShape().GetDims();
    for (int64_t dim : tensor->GetShape().GetDims()) {
      DimRange range = {0, dim};
      if (axis_index == 0) {
        // only the highest dimension needs to be sliced.
        int64_t dim_non_tail = dim / slice_num;
        int64_t low, high;
        if (thread_id == slice_num - 1) {
          low = thread_id * dim_non_tail;
          high = dim;
        } else {
          low = thread_id * dim_non_tail;
          high = (thread_id + 1) * dim_non_tail;
        }
        range = {low, high};
        new_dim[axis_index] = high - low;
      } else {
        range = {0, dim};
      }
      one_tensor.emplace_back(range);
      axis_index++;
    }

    if (one_thread[0].size() == 0) {
      one_thread[0] = one_tensor;
    } else {
      one_thread.emplace_back(one_tensor);
    }
  }

/* index: the index of tensor;
 * axes: dimension indices of those need to be sliced. */
  Status SetManualSlicingInfo(const ge::NodePtr &node, uint32_t index, bool is_input,
                              uint32_t slice_num, uint32_t slice_index, const vector<uint32_t> &axes) {
    auto op_desc = node->GetOpDesc();
    ThreadSliceMapPtr slice_info_ptr;
    slice_info_ptr = op_desc->TryGetExtAttr(kAttrSgtStructInfo, slice_info_ptr);
    if (slice_info_ptr == nullptr) {
      slice_info_ptr = std::make_shared<ThreadSliceMap>();
    }
    slice_info_ptr->thread_mode = 0;
    slice_info_ptr->thread_scope_id = 0;
    slice_info_ptr->is_first_node_in_topo_order = false;
    slice_info_ptr->node_num_in_thread_scope = 1;
    slice_info_ptr->slice_instance_num = slice_num;

    if (slice_index == slice_num - 1) {
      slice_info_ptr->thread_id = {slice_num - 1};
    } else {
      slice_info_ptr->thread_id = {slice_index};
    }

    /* push all tensors into tensor slice */
    /* In manual slicing, there is only one thread in sgt info. */
    vector<vector<DimRange>> one_thread;
    if (is_input) {
      if (slice_info_ptr->input_tensor_slice.empty()) {
        slice_info_ptr->input_tensor_slice.emplace_back(one_thread);
      }
    } else {
      if (slice_info_ptr->output_tensor_slice.empty()) {
        slice_info_ptr->output_tensor_slice.emplace_back(one_thread);
      }
    }

    vector<vector<vector<DimRange>>> *slice = &slice_info_ptr->output_tensor_slice;
    if (is_input) {
      slice = &slice_info_ptr->input_tensor_slice;
    }

    size_t all_tensors_size = op_desc->GetAllInputsSize();
    for (uint32_t i = 0; i < slice_num; i++) {
      if (i != slice_index) {
        continue;
      }

      for (size_t j = 0; j < all_tensors_size; j++) {
        if (j != index) {
          continue;
        }
        GeTensorDescPtr tensor;
        if (is_input) {
          tensor = op_desc->MutableInputDesc(j);
        } else {
          tensor = op_desc->MutableOutputDesc(j);
        }
        FFTS_LOGD("before shape of %s is %s", node->GetName().c_str(),
                fe::StringUtils::IntegerVecToString(tensor->GetShape().GetDims()).c_str());
        AssembleTensor(tensor, axes, slice_info_ptr->thread_id, slice_num, (*slice)[0]);
        FFTS_LOGD("after shape of %s is %s", node->GetName().c_str(),
                fe::StringUtils::IntegerVecToString(tensor->GetShape().GetDims()).c_str());
      }
    }
    op_desc->SetExtAttr(kAttrSgtStructInfo, slice_info_ptr);
    return ffts::SUCCESS;
  }

  Status SetAutoThreadSlicingInfo(const ge::NodePtr &node, uint32_t index, bool is_input, uint32_t slice_num) {
    auto op_desc = node->GetOpDesc();
    ThreadSliceMapPtr slice_info_ptr;
    slice_info_ptr = op_desc->TryGetExtAttr(kAttrSgtStructInfo, slice_info_ptr);
    if (slice_info_ptr == nullptr) {
      slice_info_ptr = std::make_shared<ThreadSliceMap>();
    }
    slice_info_ptr->thread_mode = 1;
    slice_info_ptr->thread_scope_id = 0;
    slice_info_ptr->is_first_node_in_topo_order = false;
    slice_info_ptr->node_num_in_thread_scope = 1;
    slice_info_ptr->slice_instance_num = slice_num;
    slice_info_ptr->parallel_window_size = slice_num;

    slice_info_ptr->thread_id = 0;

    /* push all tensors into tensor slice */
    /* In manual slicing, there is only one thread in sgt info. */
    vector<DimRange> one_thread;
    if (is_input) {
      if (slice_info_ptr->input_tensor_slice.empty()) {
        vector<vector<DimRange>> vv_one_thread;
        vv_one_thread.push_back(one_thread);
        for (size_t i = 0; i < slice_num; i++) {
          slice_info_ptr->input_tensor_slice.emplace_back(vv_one_thread);
        }
      } else {
        for (size_t i = 0; i < slice_num; i++) {
          slice_info_ptr->input_tensor_slice[i].push_back(one_thread);
        }
      }
    } else {
      if (slice_info_ptr->output_tensor_slice.empty()) {
        vector<vector<DimRange>> vv_one_thread;
        vv_one_thread.push_back(one_thread);
        for (size_t i = 0; i < slice_num; i++) {
          slice_info_ptr->output_tensor_slice.emplace_back(vv_one_thread);
        }
      }
      else {
        for(size_t i = 0; i < slice_num; i++) {
          slice_info_ptr->output_tensor_slice[i].emplace_back(one_thread);
        }
      }
    }

    vector<vector<vector<DimRange>>> *slice = &slice_info_ptr->output_tensor_slice;
    if (is_input) {
      slice = &slice_info_ptr->input_tensor_slice;
    }

    size_t all_tensors_size = op_desc->GetAllInputsSize();
    for (uint32_t i = 0; i < slice_num; i++) {
      // if (i != slice_index) {
      //   continue;
      // }

      for (size_t j = 0; j < all_tensors_size; j++) {
        if (j != index) {
          continue;
        }
        GeTensorDescPtr tensor;
        if (is_input) {
          tensor = op_desc->MutableInputDesc(j);
        } else {
          tensor = op_desc->MutableOutputDesc(j);
        }

        FE_LOGD("before shape of %s is %s", node->GetName().c_str(),
                fe::StringUtils::IntegerVecToString(tensor->GetShape().GetDims()).c_str());
        AssembleAutoThreadTensor(tensor, i, slice_num, (*slice)[i]);
        FE_LOGD("after shape of %s is %s", node->GetName().c_str(),
                fe::StringUtils::IntegerVecToString(tensor->GetShape().GetDims()).c_str());
      }
    }
    slice_info_ptr->parallel_window_size = 4;
    op_desc->SetExtAttr(kAttrSgtStructInfo, slice_info_ptr);
    return ffts::SUCCESS;
  }

  void GenerateABAutoThreadingTaskDef(domi::FftsPlusTaskDef *ffts_plus_task_def) {
    size_t slice_num = 4;
    // add in_label context
    auto inlabel_context = ffts_plus_task_def->add_ffts_plus_ctx();
    inlabel_context->set_context_type(RT_CTX_TYPE_LABEL);
    auto in_label = inlabel_context->mutable_label_ctx();
    in_label->set_pred_cnt(0);
    for (uint32_t i = 0; i < slice_num; i++) {
      in_label->add_successor_list(i);
    }

    // add at_start context
    for (uint32_t i = 0; i < slice_num; i++) {
      auto at_start_context = ffts_plus_task_def->add_ffts_plus_ctx();  // at_start's context
      at_start_context->set_context_type(RT_CTX_TYPE_AT_START);
      auto at_start = at_start_context->mutable_at_start_ctx();
      at_start->set_pred_cnt(1);
      at_start->set_pred_cnt_init(1);
      at_start->set_thread_id(0);
      at_start->add_successor_list(i + 1 + slice_num);
    }

    for (uint32_t i = 0; i < slice_num; i++) {
      auto a_context = ffts_plus_task_def->add_ffts_plus_ctx();  // a's context
      a_context->set_context_type(RT_CTX_TYPE_AIV);
      auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
      a_aic_aiv->set_successor_num(1);
      a_aic_aiv->add_successor_list(i + 1 + slice_num * 2);
    }

    for (uint32_t i = 0; i < slice_num; i++) {
      auto b_context = ffts_plus_task_def->add_ffts_plus_ctx();  // b's context
      b_context->set_context_type(RT_CTX_TYPE_AIV);
      auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();
      b_aic_aiv->set_pred_cnt(1);
      b_aic_aiv->set_pred_cnt_init(1);
      b_aic_aiv->set_successor_num(1);
      b_aic_aiv->add_successor_list(13 + i);
    }

    for (uint32_t i = 0; i < slice_num; i++) {
      auto at_end_context = ffts_plus_task_def->add_ffts_plus_ctx();  // at_end's context
      at_end_context->set_context_type(RT_CTX_TYPE_AT_END);
      auto at_end = at_end_context->mutable_at_end_ctx();
      at_end->set_pred_cnt(1);
      at_end->set_pred_cnt_init(1);
      at_end->add_succ_at_start_slot(i + 1);
      at_end->add_succ_out_label_slot(17);
    }

    auto out_label_context = ffts_plus_task_def->add_ffts_plus_ctx();
    out_label_context->set_context_type(RT_CTX_TYPE_LABEL);
    auto out_label = out_label_context->mutable_label_ctx();
    out_label->set_pred_cnt(4);
    out_label->set_pred_cnt_init(4);
  }

  void GenerateABCAutoThreadingTaskDef(domi::FftsPlusTaskDef *ffts_plus_task_def) {
    size_t slice_num = 4;
    // add in_label context
    auto inlabel_context = ffts_plus_task_def->add_ffts_plus_ctx();
    inlabel_context->set_context_type(RT_CTX_TYPE_LABEL);
    auto in_label = inlabel_context->mutable_label_ctx();
    in_label->set_pred_cnt(0);
    for (uint32_t i = 0; i < slice_num; i++) {
      in_label->add_successor_list(i);
    }

    // add at_start context
    for (uint32_t i = 0; i < slice_num; i++) {
      auto at_start_context = ffts_plus_task_def->add_ffts_plus_ctx();  // at_start's context
      at_start_context->set_context_type(RT_CTX_TYPE_AT_START);
      auto at_start = at_start_context->mutable_at_start_ctx();
      at_start->set_pred_cnt(1);
      at_start->set_pred_cnt_init(1);
      at_start->set_thread_id(0);
      at_start->add_successor_list(i + 1 + slice_num);
    }

    for (uint32_t i = 0; i < slice_num; i++) {
      auto a_context = ffts_plus_task_def->add_ffts_plus_ctx();  // a's context
      a_context->set_context_type(RT_CTX_TYPE_AIV);
      auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
      a_aic_aiv->set_successor_num(2);
      a_aic_aiv->add_successor_list(i + 1 + slice_num * 2);
      a_aic_aiv->add_successor_list(i + 1 + slice_num * 3);
    }

    for (uint32_t i = 0; i < slice_num; i++) {
      auto b_context = ffts_plus_task_def->add_ffts_plus_ctx();  // b's context
      b_context->set_context_type(RT_CTX_TYPE_AIV);
      auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();
      b_aic_aiv->set_pred_cnt(1);
      b_aic_aiv->set_pred_cnt_init(1);
      b_aic_aiv->set_successor_num(1);
      b_aic_aiv->add_successor_list(17 + i);
    }

    for (uint32_t i = 0; i < slice_num; i++) {
      auto c_context = ffts_plus_task_def->add_ffts_plus_ctx();  // c's context
      c_context->set_context_type(RT_CTX_TYPE_AIV);
      auto c_aic_aiv = c_context->mutable_aic_aiv_ctx();
      c_aic_aiv->set_pred_cnt(1);
      c_aic_aiv->set_pred_cnt_init(1);
      c_aic_aiv->set_successor_num(1);
      c_aic_aiv->add_successor_list(17 + i);
    }

    for (uint32_t i = 0; i < slice_num; i++) {
      auto at_end_context = ffts_plus_task_def->add_ffts_plus_ctx();  // at_end's context
      at_end_context->set_context_type(RT_CTX_TYPE_AT_END);
      auto at_end = at_end_context->mutable_at_end_ctx();
      at_end->set_pred_cnt(2);
      at_end->set_pred_cnt_init(2);
      at_end->add_succ_at_start_slot(i + 1);
      at_end->add_succ_out_label_slot(21);
    }

    auto out_label_context = ffts_plus_task_def->add_ffts_plus_ctx();
    out_label_context->set_context_type(RT_CTX_TYPE_LABEL);
    auto out_label = out_label_context->mutable_label_ctx();
    out_label->set_pred_cnt(4);
    out_label->set_pred_cnt_init(4);
  }

  void CreateGraph01(ge::NodePtr &b, ComputeGraphPtr &graph) {
    graph = std::make_shared<ComputeGraph>("test");
    GraphConstructor test(graph, "", ge::FORMAT_NCHW, ge::DT_FLOAT16,
                        GeShape({288, 8, 24, 33}));
    test.AddOpDesc("a", "A", 1, 1)
        .AddOpDesc("b", "B", 1, 1)
        .SetInput("b:0", "a:0");

    test.GetNodeByName("b", b);
    ge::AttrUtils::SetInt(b->GetOpDesc(), kContextId, 1);
    ge::AttrUtils::SetInt(b->GetOpDesc(), kPrefetchEnableBm, 0x0001);
    vector<int64_t> input_addrs = {5000};
    ge::AttrUtils::SetListInt(b->GetOpDesc(), "input_addrs", input_addrs);

    NodePtr a;
    test.GetNodeByName("a", a);
    ge::AttrUtils::SetInt(a->GetOpDesc(), kContextId, 0);
    vector<int64_t> output_addrs = {5000};
    ge::AttrUtils::SetListInt(a->GetOpDesc(), "output_addrs", output_addrs);

    /* slice a and b */
    int32_t slice_num = 4;
    vector<uint32_t> axes = {0};

    SetManualSlicingInfo(a, 0 /* index */, true /* is_input */, slice_num, 1 /* slice_index */, axes);
    SetManualSlicingInfo(a, 0 /* index */, false /* is_input */, slice_num, 1 /* slice_index */, axes);
    SetManualSlicingInfo(b, 0 /* index */, true /* is_input */, slice_num, 1 /* slice_index */, axes);
    SetManualSlicingInfo(b, 0 /* index */, false /* is_input */, slice_num, 1 /* slice_index */, axes);
  }

    void CreateAutoThreadingGraph01(ge::NodePtr &b, ComputeGraphPtr &graph) {
    graph = std::make_shared<ComputeGraph>("test");
    GraphConstructor test(graph, "", ge::FORMAT_NCHW, ge::DT_FLOAT16,
                        GeShape({288, 8, 24, 33}));
    test.AddOpDesc("a", "A", 1, 1)
        .AddOpDesc("b", "B", 1, 1)
        .SetInput("b:0", "a:0");

    test.GetNodeByName("b", b);
    vector<uint32_t> context_id_list = {9, 10, 11, 12};
    ge::AttrUtils::SetListInt(b->GetOpDesc(), "_context_id_list", context_id_list);
    ge::AttrUtils::SetInt(b->GetOpDesc(), kPrefetchEnableBm, 0x0001);
    vector<int64_t> input_addrs = {5000};
    ge::AttrUtils::SetListInt(b->GetOpDesc(), "input_addrs", input_addrs);

    NodePtr a;
    test.GetNodeByName("a", a);
    context_id_list = {5, 6, 7, 8};
    ge::AttrUtils::SetListInt(a->GetOpDesc(), "_context_id_list", context_id_list);
    vector<int64_t> output_addrs = {5000};
    ge::AttrUtils::SetListInt(a->GetOpDesc(), "output_addrs", output_addrs);

    /* slice a and b */
    int32_t slice_num = 4;
    vector<uint32_t> axes = {0};

    SetAutoThreadSlicingInfo(a, 0 /* index */, true /* is_input */, slice_num);
    SetAutoThreadSlicingInfo(a, 0 /* index */, false /* is_input */, slice_num);
    SetAutoThreadSlicingInfo(b, 0 /* index */, true /* is_input */, slice_num);
    SetAutoThreadSlicingInfo(b, 0 /* index */, false /* is_input */, slice_num);
  }

  void CreateDynamicGraph01(ge::NodePtr &b, ComputeGraphPtr &graph) {
    graph = std::make_shared<ComputeGraph>("test");
    GraphConstructor test(graph, "", ge::FORMAT_NCHW, ge::DT_FLOAT16,
                        GeShape({-1, 8, 24, 33}));
    test.AddOpDesc("a", "A", 1, 1)
        .AddOpDesc("b", "B", 1, 1)
        .SetInput("b:0", "a:0");

    test.GetNodeByName("b", b);
    vector<uint32_t> context_id_list = {9, 10, 11, 12};
    ge::AttrUtils::SetListInt(b->GetOpDesc(), "_context_id_list", context_id_list);
    ge::AttrUtils::SetInt(b->GetOpDesc(), kPrefetchEnableBm, 0x0001);
    vector<int64_t> input_addrs = {5000};
    ge::AttrUtils::SetListInt(b->GetOpDesc(), "input_addrs", input_addrs);

    NodePtr a;
    test.GetNodeByName("a", a);
    context_id_list = {5, 6, 7, 8};
    ge::AttrUtils::SetListInt(a->GetOpDesc(), "_context_id_list", context_id_list);
    vector<int64_t> output_addrs = {5000};
    ge::AttrUtils::SetListInt(a->GetOpDesc(), "output_addrs", output_addrs);

    /* slice a and b */
    int32_t slice_num = 4;
    SetAutoThreadSlicingInfo(a, 0 /* index */, true /* is_input */, slice_num);
    SetAutoThreadSlicingInfo(a, 0 /* index */, false /* is_input */, slice_num);
    SetAutoThreadSlicingInfo(b, 0 /* index */, true /* is_input */, slice_num);
    SetAutoThreadSlicingInfo(b, 0 /* index */, false /* is_input */, slice_num);
  }

  /* dims[0] is the highest dimension. */
  void CreateGraph02_x(ge::NodePtr &b, ComputeGraphPtr &graph, vector<uint32_t> &axes) {
    graph = std::make_shared<ComputeGraph>("test");
    GraphConstructor test(graph, "", ge::FORMAT_NCHW, ge::DT_FLOAT16,
                          GeShape({288, 8, 24, 33}));
    test.AddOpDesc("a", "A", 1, 1)
        .AddOpDesc("b", "B", 1, 1)
        .SetInput("b:0", "a:0");

    test.GetNodeByName("b", b);
    ge::AttrUtils::SetInt(b->GetOpDesc(), kContextId, 1);
    ge::AttrUtils::SetInt(b->GetOpDesc(), kPrefetchEnableBm, 0x0001);
    vector<int64_t> input_addrs = {5000};
    ge::AttrUtils::SetListInt(b->GetOpDesc(), "input_addrs", input_addrs);

    NodePtr a;
    test.GetNodeByName("a", a);
    ge::AttrUtils::SetInt(a->GetOpDesc(), kContextId, 0);
    vector<int64_t> output_addrs = {20000};
    ge::AttrUtils::SetListInt(a->GetOpDesc(), "output_addrs", output_addrs);

    int32_t slice_num = 4;
    SetManualSlicingInfo(b, 0 /* index */, true /* is_input */, slice_num,
                         1 /* slice_index */, axes);
    SetManualSlicingInfo(b, 0 /* index */, false /* is_input */, slice_num,
                         1 /* slice_index */, axes);
  }

  void CreateGraph02_2_1(ge::NodePtr &b, ComputeGraphPtr &graph) {
    graph = std::make_shared<ComputeGraph>("test");
    GraphConstructor test(graph, "", ge::FORMAT_NCHW, ge::DT_FLOAT16,
                          GeShape({288, 8, 24, 33}));
    test.AddOpDesc("a", "A", 1, 1)
        .AddOpDesc("b", "B", 3, 1)
        .AddOpDesc("c", "D", 1, 1)
        .AddOpDesc("c", "D", 1, 1)
        .SetInput("b:0", "a:0")
        .SetInput("b:1", "c:0")
        .SetInput("b:2", "d:0");

    test.GetNodeByName("b", b);
    ge::AttrUtils::SetInt(b->GetOpDesc(), kContextId, 0);
    ge::AttrUtils::SetInt(b->GetOpDesc(), kPrefetchEnableBm, 0x0007);
    vector<int64_t> input_addrs = {5000, 6000, 7000};
    ge::AttrUtils::SetListInt(b->GetOpDesc(), "input_addrs", input_addrs);

    NodePtr a;
    test.GetNodeByName("a", a);
    vector<int64_t> output_addrs = {20000};
    ge::AttrUtils::SetListInt(a->GetOpDesc(), "output_addrs", output_addrs);

    NodePtr c;
    test.GetNodeByName("c", c);
    output_addrs = {30000};
    ge::AttrUtils::SetListInt(c->GetOpDesc(), "output_addrs", output_addrs);

    NodePtr d;
    test.GetNodeByName("d", d);
    output_addrs = {40000};
    ge::AttrUtils::SetListInt(d->GetOpDesc(), "output_addrs", output_addrs);

    int32_t slice_num = 4;
    vector<uint32_t> axes1 = {0};
    vector<uint32_t> axes2 = {2, 3};
    SetManualSlicingInfo(b, 0 /* index */, true /* is_input */, slice_num,
                         1 /* slice_index */, axes1);
    SetManualSlicingInfo(b, 0 /* index */, false /* is_input */, slice_num,
                         1 /* slice_index */, axes1);

    SetManualSlicingInfo(b, 1 /* index */, true /* is_input */, slice_num,
                         2 /* slice_index */, axes1);

    SetManualSlicingInfo(b, 2 /* index */, true /* is_input */, slice_num,
                         3 /* slice_index */, axes2);
  }

  /* dims[0] is the highest dimension. */
  void CreateGraph02_4_1(ge::NodePtr &b, ComputeGraphPtr &graph, vector<uint32_t> &axes) {
    graph = std::make_shared<ComputeGraph>("test");
    GraphConstructor test(graph, "", ge::FORMAT_NCHW, ge::DT_FLOAT16,
                          GeShape({4, 8, 24, 33}));
    test.AddOpDesc("a", "A", 1, 1)
        .AddOpDesc("b", "B", 1, 1)
        .SetInput("b:0", "a:0");

    test.GetNodeByName("b", b);
    ge::AttrUtils::SetInt(b->GetOpDesc(), kContextId, 1);
    ge::AttrUtils::SetInt(b->GetOpDesc(), kPrefetchEnableBm, 0x0001);
    vector<int64_t> input_addrs = {5000};
    ge::AttrUtils::SetListInt(b->GetOpDesc(), "input_addrs", input_addrs);

    NodePtr a;
    test.GetNodeByName("a", a);
    ge::AttrUtils::SetInt(a->GetOpDesc(), kContextId, 0);
    vector<int64_t> output_addrs = {20000};
    ge::AttrUtils::SetListInt(a->GetOpDesc(), "output_addrs", output_addrs);

    int32_t slice_num = 4;
    SetManualSlicingInfo(b, 0 /* index */, true /* is_input */, slice_num,
                         1 /* slice_index */, axes);
    SetManualSlicingInfo(b, 0 /* index */, false /* is_input */, slice_num,
                         1 /* slice_index */, axes);
  }

  /* slice 288 and 24*/
  void CreateGraph03_x(ge::NodePtr &b, ComputeGraphPtr &graph) {
    graph = std::make_shared<ComputeGraph>("test");
    GraphConstructor test(graph, "", ge::FORMAT_NCHW, ge::DT_FLOAT16,
                          GeShape({288, 8, 24, 33}));
    test.AddOpDesc("a", "A", 1, 1)
        .AddOpDesc("b", "B", 1, 1)
        .SetInput("b:0", "a:0");

    test.GetNodeByName("b", b);
    ge::AttrUtils::SetInt(b->GetOpDesc(), kContextId, 1);
    ge::AttrUtils::SetInt(b->GetOpDesc(), kPrefetchEnableBm, 0x0001);
    vector<int64_t> input_addrs = {5000};
    ge::AttrUtils::SetListInt(b->GetOpDesc(), "input_addrs", input_addrs);

    NodePtr a;
    test.GetNodeByName("a", a);
    ge::AttrUtils::SetInt(a->GetOpDesc(), kContextId, 0);
    vector<int64_t> output_addrs = {20000};
    ge::AttrUtils::SetListInt(a->GetOpDesc(), "output_addrs", output_addrs);

    int32_t slice_num = 4;
    vector<uint32_t> axes = {0, 2};
    SetManualSlicingInfo(b, 0 /* index */, true /* is_input */, slice_num,
                         2 /* slice_index */, axes);
    SetManualSlicingInfo(b, 0 /* index */, false /* is_input */, slice_num,
                         2 /* slice_index */, axes);
  }

  /* a will do the writeback operation */
  void CreateGraph04_x(ge::NodePtr &a, ComputeGraphPtr &graph, vector<uint32_t> &axes) {
    graph = std::make_shared<ComputeGraph>("test");
    GraphConstructor test(graph, "", ge::FORMAT_NCHW, ge::DT_FLOAT16,
                          GeShape({288, 8, 24, 33}));
    test.AddOpDesc("a", "A", 1, 1)
        .AddOpDesc("b", "B", 1, 1)
        .AddOpDesc("c", "C", 1, 1)
        .SetInput("b:0", "a:0")
        .SetInput("c:0", "a:0");

    test.GetNodeByName("a", a);
    ge::AttrUtils::SetInt(a->GetOpDesc(), kContextId, 0);
    ge::AttrUtils::SetInt(a->GetOpDesc(), FE_IMPLY_TYPE, 6);
    ge::AttrUtils::SetInt(a->GetOpDesc(), kThreadScopeId, 1);
    vector<int64_t> output_addrs = {30000};
    ge::AttrUtils::SetListInt(a->GetOpDesc(), "output_addrs", output_addrs);

    NodePtr b;
    test.GetNodeByName("b", b);
    vector<int64_t> input_addrs = {5000};
    ge::AttrUtils::SetInt(b->GetOpDesc(), kContextId, 1);
    ge::AttrUtils::SetInt(b->GetOpDesc(), kThreadScopeId, 1);
    ge::AttrUtils::SetListInt(b->GetOpDesc(), "input_addrs", input_addrs);

    NodePtr c;
    test.GetNodeByName("c", c);
    ge::AttrUtils::SetListInt(c->GetOpDesc(), "input_addrs", input_addrs);
    /* slice a and b */
    int32_t slice_num = 4;
    SetManualSlicingInfo(a, 0 /* index */, true /* is_input */, slice_num,
                         1 /* slice_index */, axes);
    SetManualSlicingInfo(a, 0 /* index */, false /* is_input */, slice_num,
                         1 /* slice_index */, axes);

    SetManualSlicingInfo(b, 0 /* index */, true /* is_input */, slice_num,
                         1 /* slice_index */, axes);
    SetManualSlicingInfo(b, 0 /* index */, false /* is_input */, slice_num,
                         1 /* slice_index */, axes);
  }

  void CreateAutoThreadingGraph04_x(ge::NodePtr &a, ComputeGraphPtr &graph) {
    graph = std::make_shared<ComputeGraph>("test");
    GraphConstructor test(graph, "", ge::FORMAT_NCHW, ge::DT_FLOAT16,
                          GeShape({288, 8, 24, 33}));
    test.AddOpDesc("a", "A", 1, 1)
        .AddOpDesc("b", "B", 1, 1)
        .AddOpDesc("c", "C", 1, 1)
        .SetInput("b:0", "a:0")
        .SetInput("c:0", "a:0");

    test.GetNodeByName("a", a);
    vector<uint32_t> context_id_list = {5, 6, 7, 8};
    ge::AttrUtils::SetListInt(a->GetOpDesc(), "_context_id_list", context_id_list);
    ge::AttrUtils::SetInt(a->GetOpDesc(), FE_IMPLY_TYPE, 6);
    ge::AttrUtils::SetInt(a->GetOpDesc(), kThreadScopeId, 1);
    vector<int64_t> output_addrs = {20000};
    ge::AttrUtils::SetListInt(a->GetOpDesc(), "output_addrs", output_addrs);

    NodePtr b;
    test.GetNodeByName("b", b);
    vector<int64_t> input_addrs = {5000};
    context_id_list = {9, 10, 11, 12};
    ge::AttrUtils::SetListInt(b->GetOpDesc(), "_context_id_list", context_id_list);
    ge::AttrUtils::SetInt(b->GetOpDesc(), kThreadScopeId, 1);
    ge::AttrUtils::SetListInt(b->GetOpDesc(), "input_addrs", input_addrs);

    NodePtr c;
    test.GetNodeByName("c", c);
    ge::AttrUtils::SetListInt(c->GetOpDesc(), "input_addrs", input_addrs);
    /* slice a and b */
    int32_t slice_num = 4;
    SetAutoThreadSlicingInfo(a, 0 /* index */, true /* is_input */, slice_num);
    SetAutoThreadSlicingInfo(a, 0 /* index */, false /* is_input */, slice_num);
    SetAutoThreadSlicingInfo(b, 0 /* index */, true /* is_input */, slice_num);
    SetAutoThreadSlicingInfo(b, 0 /* index */, false /* is_input */, slice_num);
  }

/* a will do the writeback or invalidate operation */
  void CreateGraph04_x_1(ge::NodePtr &a, ComputeGraphPtr &graph, vector<uint32_t> &axes, int scope_of_c) {
    graph = std::make_shared<ComputeGraph>("test");
    GraphConstructor test(graph, "", ge::FORMAT_NCHW, ge::DT_FLOAT16,
                          GeShape({288, 8, 24, 33}));
    test.AddOpDesc("a", "A", 1, 1)
        .AddOpDesc("b", "B", 1, 1)
        .AddOpDesc("pc", "PhonyConcat", 1, 1)
        .AddOpDesc("c", "C", 1, 1)
        .SetInput("b:0", "a:0")
        .SetInput("pc:0", "a:0")
        .SetInput("c:0", "pc:0");

    test.GetNodeByName("a", a);
    ge::AttrUtils::SetInt(a->GetOpDesc(), kContextId, 0);
    ge::AttrUtils::SetInt(a->GetOpDesc(), FE_IMPLY_TYPE, 6);
    ge::AttrUtils::SetInt(a->GetOpDesc(), kThreadScopeId, 1);
    vector<int64_t> output_addrs = {20000};
    ge::AttrUtils::SetListInt(a->GetOpDesc(), "output_addrs", output_addrs);

    NodePtr b;
    test.GetNodeByName("b", b);
    vector<int64_t> input_addrs = {5000};
    ge::AttrUtils::SetInt(b->GetOpDesc(), kContextId, 1);
    ge::AttrUtils::SetInt(b->GetOpDesc(), kThreadScopeId, 1);
    ge::AttrUtils::SetListInt(b->GetOpDesc(), "input_addrs", input_addrs);

    NodePtr pc;
    test.GetNodeByName("pc", pc);
    input_addrs = {30000};
    ge::AttrUtils::SetInt(pc->GetOpDesc(), kThreadScopeId, 1);
    ge::AttrUtils::SetListInt(pc->GetOpDesc(), "input_addrs", input_addrs);

    NodePtr c;
    test.GetNodeByName("c", c);
    if (scope_of_c != 0) {
      ge::AttrUtils::SetInt(c->GetOpDesc(), kThreadScopeId, scope_of_c);
    }
    if (scope_of_c == 1) {
      /* same thread scope as node and b. */
      ge::AttrUtils::SetInt(c->GetOpDesc(), kContextId, 2);
    }
    ge::AttrUtils::SetListInt(c->GetOpDesc(), "input_addrs", input_addrs);
    /* slice a and b */
    int32_t slice_num = 4;
    SetManualSlicingInfo(a, 0 /* index */, true /* is_input */, slice_num,
                         1 /* slice_index */, axes);
    SetManualSlicingInfo(a, 0 /* index */, false /* is_input */, slice_num,
                         1 /* slice_index */, axes);

    SetManualSlicingInfo(b, 0 /* index */, true /* is_input */, slice_num,
                         1 /* slice_index */, axes);
    SetManualSlicingInfo(b, 0 /* index */, false /* is_input */, slice_num,
                         1 /* slice_index */, axes);
  }

  /* a will do the invalidate operation */
  void CreateGraph05_x(ge::NodePtr &a, ComputeGraphPtr &graph, vector<uint32_t> &axes) {
    graph = std::make_shared<ComputeGraph>("test");
    GraphConstructor test(graph, "", ge::FORMAT_NCHW, ge::DT_FLOAT16,
                          GeShape({288, 8, 24, 33}));
    test.AddOpDesc("a", "A", 1, 1)
        .AddOpDesc("b", "B", 1, 1)
        .AddOpDesc("c", "C", 1, 1)
        .SetInput("b:0", "a:0")
        .SetInput("c:0", "a:0");

    test.GetNodeByName("a", a);
    ge::AttrUtils::SetInt(a->GetOpDesc(), kContextId, 0);
    ge::AttrUtils::SetInt(a->GetOpDesc(), FE_IMPLY_TYPE, 6);
    ge::AttrUtils::SetInt(a->GetOpDesc(), kThreadScopeId, 1);
    vector<int64_t> output_addrs = {20000};
    ge::AttrUtils::SetListInt(a->GetOpDesc(), "output_addrs", output_addrs);

    NodePtr b;
    test.GetNodeByName("b", b);
    vector<int64_t> input_addrs = {5000};
    ge::AttrUtils::SetInt(b->GetOpDesc(), kContextId, 1);
    ge::AttrUtils::SetInt(b->GetOpDesc(), FE_IMPLY_TYPE, 6);
    ge::AttrUtils::SetInt(b->GetOpDesc(), kThreadScopeId, 1);
    ge::AttrUtils::SetListInt(b->GetOpDesc(), "input_addrs", input_addrs);

    NodePtr c;
    test.GetNodeByName("c", c);
    ge::AttrUtils::SetInt(c->GetOpDesc(), kContextId, 2);
    ge::AttrUtils::SetInt(c->GetOpDesc(), FE_IMPLY_TYPE, 6);
    ge::AttrUtils::SetInt(c->GetOpDesc(), kThreadScopeId, 1);
    ge::AttrUtils::SetListInt(c->GetOpDesc(), "input_addrs", input_addrs);
    /* slice a and b */
    int32_t slice_num = 4;
    SetManualSlicingInfo(a, 0 /* index */, true /* is_input */, slice_num,
                         1 /* slice_index */, axes);
    SetManualSlicingInfo(a, 0 /* index */, false /* is_input */, slice_num,
                         1 /* slice_index */, axes);
  }

    /* a will do the invalidate operation */
  void CreateAutoThreadGraph05_x(ge::NodePtr &a, ComputeGraphPtr &graph, vector<uint32_t> &axes) {
    graph = std::make_shared<ComputeGraph>("test");
    GraphConstructor test(graph, "", ge::FORMAT_NCHW, ge::DT_FLOAT16,
                          GeShape({288, 8, 24, 33}));
    test.AddOpDesc("a", "A", 1, 1)
        .AddOpDesc("b", "B", 1, 1)
        .AddOpDesc("c", "C", 1, 1)
        .SetInput("b:0", "a:0")
        .SetInput("c:0", "a:0");

    test.GetNodeByName("a", a);
    vector<uint32_t> context_id_list = {5, 6, 7, 8};
    ge::AttrUtils::SetListInt(a->GetOpDesc(), "_context_id_list", context_id_list);
    ge::AttrUtils::SetInt(a->GetOpDesc(), FE_IMPLY_TYPE, 6);
    ge::AttrUtils::SetInt(a->GetOpDesc(), kThreadScopeId, 1);
    vector<int64_t> output_addrs = {20000};
    ge::AttrUtils::SetListInt(a->GetOpDesc(), "output_addrs", output_addrs);

    NodePtr b;
    test.GetNodeByName("b", b);
    vector<int64_t> input_addrs = {5000};
    context_id_list = {9, 10, 11, 12};
    ge::AttrUtils::SetListInt(b->GetOpDesc(), "_context_id_list", context_id_list);
    ge::AttrUtils::SetInt(b->GetOpDesc(), FE_IMPLY_TYPE, 6);
    ge::AttrUtils::SetInt(b->GetOpDesc(), kThreadScopeId, 1);
    ge::AttrUtils::SetListInt(b->GetOpDesc(), "input_addrs", input_addrs);

    NodePtr c;
    test.GetNodeByName("c", c);
    context_id_list = {13, 14, 15, 16};
    ge::AttrUtils::SetListInt(c->GetOpDesc(), "_context_id_list", context_id_list);
    ge::AttrUtils::SetInt(c->GetOpDesc(), FE_IMPLY_TYPE, 6);
    ge::AttrUtils::SetInt(c->GetOpDesc(), kThreadScopeId, 1);
    ge::AttrUtils::SetListInt(c->GetOpDesc(), "input_addrs", input_addrs);
    /* slice a and b */
    int32_t slice_num = 4;
    SetAutoThreadSlicingInfo(a, 0 /* index */, true /* is_input */, slice_num);
    SetAutoThreadSlicingInfo(a, 0 /* index */, false /* is_input */, slice_num);
  }

  PrefetchTaskBuilder prefetch_;
  OutTaskBuilder invalidate_;
  OutTaskBuilder write_back_;
  PrefetchAutoTaskBuilder prefetch_auto_;
  OutAutoTaskBuilder invalidate_auto_;
  OutAutoTaskBuilder write_back_auto_;
  PrefetchDynamicTaskBuilder prefetch_dyn_;
  OutDynamicTaskBuilder invalidate_dyn_;
  OutDynamicTaskBuilder write_back_dyn_;
};

/* A -> B(thread dim = 4), shape is continuous.
 * B is thread 1 and A's output address is 5000, B's input address is
 * 5000.
 *
 * WARNING: Burst len is 100000, which is less than the total size of b's input.
 * need to use 92 data context to do the prefetch.
 * But we only allow up to 4 prefetch data context. */
TEST_F(FftsPlusDataContextST, prefetch_01)
{
  ge::NodePtr b;
  ComputeGraphPtr graph;
  CreateGraph01(b, graph);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_prefetch_enable_bitmap(0);
  a_aic_aiv->set_prefetch_once_bitmap(0);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();
  b_aic_aiv->set_prefetch_enable_bitmap(0);
  b_aic_aiv->set_prefetch_once_bitmap(0);
  // prefetch_.SetBurstLen(100000);
  Status ret = prefetch_.GenManualDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 3);
}

/* A -> B(thread dim = 4), shape is continuous.
 * B is thread 1 and A's output address is 5000, B's input address is
 * 5000.
 *
 * WARNING: Burst len is 10000000, which is larger than the total size of b's input.
 * need to use 1 data context to do the prefetch. */
TEST_F(FftsPlusDataContextST, prefetch_01_1)
{
  ge::NodePtr b;
  ComputeGraphPtr graph;
  CreateGraph01(b, graph);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_prefetch_enable_bitmap(0);
  a_aic_aiv->set_prefetch_once_bitmap(0);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();
  b_aic_aiv->set_prefetch_enable_bitmap(0);
  b_aic_aiv->set_prefetch_once_bitmap(0);

  prefetch_.SetBurstLen(1000000);
  Status ret = prefetch_.GenManualDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 3);
  auto prefetch_context = ffts_plus_task_def->ffts_plus_ctx_[2].data_ctx();
  EXPECT_EQ(prefetch_context.thread_id(), 0);
  EXPECT_EQ(prefetch_context.addr_base(), 5000);
  EXPECT_EQ(prefetch_context.addr_offset(), 0);
  EXPECT_EQ(prefetch_context.non_tail_len_inner(), 912384);
  EXPECT_EQ(prefetch_context.non_tail_num_inner(), 1);
  EXPECT_EQ(prefetch_context.non_tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context.non_tail_stride_inner(), 912384);
  EXPECT_EQ(prefetch_context.non_tail_stride_outter(), 912384);

  EXPECT_EQ(prefetch_context.tail_len_inner(), 912384);
  EXPECT_EQ(prefetch_context.tail_num_inner(), 1);
  EXPECT_EQ(prefetch_context.tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context.tail_stride_inner(), 912384);
  EXPECT_EQ(prefetch_context.tail_stride_outter(), 912384);

  a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[0].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->prefetch_enable_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->prefetch_once_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);

  b_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[1].mutable_aic_aiv_ctx();
  EXPECT_EQ(b_aic_aiv->prefetch_enable_bitmap(), 0x0001);
  EXPECT_EQ(b_aic_aiv->prefetch_once_bitmap(), 0x0001);
  EXPECT_EQ(b_aic_aiv->src_slot_size(), 1);
  EXPECT_EQ(b_aic_aiv->src_slot(0), 2);
}

/* A -> B(thread dim = 4), shape is continuous. auto threading
 * B is thread 1 and A's output address is 5000, B's input address is
 * 5000.
 *
 * WARNING: Burst len is 10000000, which is larger than the total size of b's input.
 * need to use 1 data context to do the prefetch. */
TEST_F(FftsPlusDataContextST, auto_threading_prefetch_01_1)
{
  ge::NodePtr b;
  ComputeGraphPtr graph;
  CreateAutoThreadingGraph01(b, graph);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  GenerateABAutoThreadingTaskDef(ffts_plus_task_def);

  prefetch_auto_.SetBurstLen(1000000);
  Status ret = prefetch_auto_.GenAutoDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 22);
  auto prefetch_context = ffts_plus_task_def->ffts_plus_ctx_[18].data_ctx();
  EXPECT_EQ(prefetch_context.thread_id(), 0);
  EXPECT_EQ(prefetch_context.addr_base(), 5000);
  EXPECT_EQ(prefetch_context.addr_offset(), 912384);
  EXPECT_EQ(prefetch_context.non_tail_len_inner(), 912384);
  EXPECT_EQ(prefetch_context.non_tail_num_inner(), 1);
  EXPECT_EQ(prefetch_context.non_tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context.non_tail_stride_inner(), 3649536);
  EXPECT_EQ(prefetch_context.non_tail_stride_outter(), 3649536);

  EXPECT_EQ(prefetch_context.tail_len_inner(), 912384);
  EXPECT_EQ(prefetch_context.tail_num_inner(), 1);
  EXPECT_EQ(prefetch_context.tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context.tail_stride_inner(), 3649536);
  EXPECT_EQ(prefetch_context.tail_stride_outter(), 3649536);

  prefetch_context = ffts_plus_task_def->ffts_plus_ctx_[19].data_ctx();
  EXPECT_EQ(prefetch_context.thread_id(), 1);
  EXPECT_EQ(prefetch_context.addr_base(), 5000);
  EXPECT_EQ(prefetch_context.addr_offset(), 912384);
  EXPECT_EQ(prefetch_context.non_tail_len_inner(), 912384);
  EXPECT_EQ(prefetch_context.non_tail_num_inner(), 1);
  EXPECT_EQ(prefetch_context.non_tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context.non_tail_stride_inner(), 3649536);
  EXPECT_EQ(prefetch_context.non_tail_stride_outter(), 3649536);

  EXPECT_EQ(prefetch_context.tail_len_inner(), 912384);
  EXPECT_EQ(prefetch_context.tail_num_inner(), 1);
  EXPECT_EQ(prefetch_context.tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context.tail_stride_inner(), 3649536);
  EXPECT_EQ(prefetch_context.tail_stride_outter(), 3649536);

  auto a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[5].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->prefetch_enable_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->prefetch_once_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);

  auto b_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[9].mutable_aic_aiv_ctx();
  EXPECT_EQ(b_aic_aiv->prefetch_enable_bitmap(), 0x0001);
  EXPECT_EQ(b_aic_aiv->prefetch_once_bitmap(), 0x0001);
  EXPECT_EQ(b_aic_aiv->src_slot_size(), 1);
  EXPECT_EQ(b_aic_aiv->src_slot(0), 18);
}

/* A -> B(thread dim = 4), shape is continuous. auto threading
 * B is thread 1 and A's output address is 5000, B's input address is
 * 5000.
 *
 * WARNING: Burst len is 10000000, which is larger than the total size of b's input.
 * need to use 1 data context to do the prefetch. */
TEST_F(FftsPlusDataContextST, dynamic_threading_prefetch_01_1)
{
  ge::NodePtr b;
  ComputeGraphPtr graph;
  CreateDynamicGraph01(b, graph);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  GenerateABAutoThreadingTaskDef(ffts_plus_task_def);

  Status ret = prefetch_dyn_.GenDynamicDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
}

/* A -> B(thread dim = 4), shape is continuous.
 * B is thread 1 and A's output address is 5000, B's input address is
 * 5000.
 *
 * WARNING: Burst len is 5000000, which is less than the total size of b's input.
 * need to use 2 data context to do the prefetch. */
TEST_F(FftsPlusDataContextST, prefetch_01_2)
{
  ge::NodePtr b;
  ComputeGraphPtr graph;
  CreateGraph01(b, graph);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_prefetch_enable_bitmap(0);
  a_aic_aiv->set_prefetch_once_bitmap(0);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();
  b_aic_aiv->set_prefetch_enable_bitmap(0);
  b_aic_aiv->set_prefetch_once_bitmap(0);

  prefetch_.SetBurstLen(500000);
  Status ret = prefetch_.GenManualDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 4);
  auto prefetch_context_1 = ffts_plus_task_def->ffts_plus_ctx_[2].data_ctx();
  EXPECT_EQ(prefetch_context_1.thread_id(), 0);
  EXPECT_EQ(prefetch_context_1.addr_base(), 5000);
  EXPECT_EQ(prefetch_context_1.addr_offset(), 0);
  EXPECT_EQ(prefetch_context_1.non_tail_len_inner(), 500000);
  EXPECT_EQ(prefetch_context_1.non_tail_num_inner(), 1);
  EXPECT_EQ(prefetch_context_1.non_tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context_1.non_tail_stride_inner(), 912384);
  EXPECT_EQ(prefetch_context_1.non_tail_stride_outter(), 912384);

  EXPECT_EQ(prefetch_context_1.tail_len_inner(), 500000);
  EXPECT_EQ(prefetch_context_1.tail_num_inner(), 1);
  EXPECT_EQ(prefetch_context_1.tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context_1.tail_stride_inner(), 912384);
  EXPECT_EQ(prefetch_context_1.tail_stride_outter(), 912384);

  auto prefetch_context_2 = ffts_plus_task_def->ffts_plus_ctx_[3].data_ctx();
  EXPECT_EQ(prefetch_context_2.thread_id(), 0);
  EXPECT_EQ(prefetch_context_2.addr_base(), 5000);
  EXPECT_EQ(prefetch_context_2.addr_offset(), 500000);
  EXPECT_EQ(prefetch_context_2.non_tail_len_inner(), 412384);
  EXPECT_EQ(prefetch_context_2.non_tail_num_inner(), 1);
  EXPECT_EQ(prefetch_context_2.non_tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context_2.non_tail_stride_inner(), 912384);
  EXPECT_EQ(prefetch_context_2.non_tail_stride_outter(), 912384);

  EXPECT_EQ(prefetch_context_2.tail_len_inner(), 412384);
  EXPECT_EQ(prefetch_context_2.tail_num_inner(), 1);
  EXPECT_EQ(prefetch_context_2.tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context_2.tail_stride_inner(), 912384);
  EXPECT_EQ(prefetch_context_2.tail_stride_outter(), 912384);

  a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[0].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->prefetch_enable_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->prefetch_once_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);

  b_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[1].mutable_aic_aiv_ctx();
  EXPECT_EQ(b_aic_aiv->prefetch_enable_bitmap(), 3);
  EXPECT_EQ(b_aic_aiv->prefetch_once_bitmap(), 3);
  EXPECT_EQ(b_aic_aiv->src_slot_size(), 2);
  EXPECT_EQ(b_aic_aiv->src_slot(0), 2);
  EXPECT_EQ(b_aic_aiv->src_slot(1), 3);
}

/* A -> B(thread dim = 4), shape is not continuous.
 * B is thread 1 and A's output address is 20000, B's input address is
 * 5000.
 * b's shape is {288, 8, 24, 33}, and we slice the axis{33}, select the
 * thread 1. range {{0:288},{0:8},{0:24},{8:16}}. */
TEST_F(FftsPlusDataContextST, prefetch_02)
{
  ge::NodePtr b;
  vector<uint32_t> axes = {3};
  ComputeGraphPtr graph;
  CreateGraph02_x(b, graph, axes);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_prefetch_enable_bitmap(0);
  a_aic_aiv->set_prefetch_once_bitmap(0);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();
  b_aic_aiv->set_prefetch_enable_bitmap(0);
  b_aic_aiv->set_prefetch_once_bitmap(0);

  Status ret = prefetch_.GenManualDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 3);
  auto prefetch_context = ffts_plus_task_def->ffts_plus_ctx_[2].data_ctx();
  EXPECT_EQ(prefetch_context.thread_id(), 0);
  EXPECT_EQ(prefetch_context.addr_base(), 5000);
  EXPECT_EQ(prefetch_context.addr_offset(), 16);
  EXPECT_EQ(prefetch_context.non_tail_len_inner(), 16);
  EXPECT_EQ(prefetch_context.non_tail_num_inner(), 55296);
  EXPECT_EQ(prefetch_context.non_tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context.non_tail_stride_inner(), 66);
  EXPECT_EQ(prefetch_context.non_tail_stride_outter(), 3649536);

  EXPECT_EQ(prefetch_context.tail_len_inner(), 16);
  EXPECT_EQ(prefetch_context.tail_num_inner(), 55296);
  EXPECT_EQ(prefetch_context.tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context.tail_stride_inner(), 66);
  EXPECT_EQ(prefetch_context.tail_stride_outter(), 3649536);

  a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[0].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->prefetch_enable_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->prefetch_once_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);

  b_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[1].mutable_aic_aiv_ctx();
  EXPECT_EQ(b_aic_aiv->prefetch_enable_bitmap(), 0x0001);
  EXPECT_EQ(b_aic_aiv->prefetch_once_bitmap(), 0x0001);
  EXPECT_EQ(b_aic_aiv->src_slot_size(), 1);
  EXPECT_EQ(b_aic_aiv->src_slot(0), 2);
}

/* A -> B(thread dim = 4), shape is not continuous.
 * B is thread 1 and A's output address is 20000, B's input address is
 * 5000.
 * b's shape is {288, 8, 24, 33}, and we slice the axis{288}, select the
 * thread 1. range {{72:144},{0:8},{0:24},{0:33}}.
 * Burst len is 200000, need 5 context which is not allowed. */
TEST_F(FftsPlusDataContextST, prefetch_02_1)
{
  ge::NodePtr b;
  vector<uint32_t> axes = {0};
  ComputeGraphPtr graph;
  CreateGraph02_x(b, graph, axes);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_prefetch_enable_bitmap(0);
  a_aic_aiv->set_prefetch_once_bitmap(0);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();
  b_aic_aiv->set_prefetch_enable_bitmap(0);
  b_aic_aiv->set_prefetch_once_bitmap(0);

  prefetch_.burst_len_ = 200000;
  Status ret = prefetch_.GenManualDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 2);
  ;

  a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[0].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->prefetch_enable_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->prefetch_once_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);

  b_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[1].mutable_aic_aiv_ctx();
  EXPECT_EQ(b_aic_aiv->prefetch_enable_bitmap(), 0);
  EXPECT_EQ(b_aic_aiv->prefetch_once_bitmap(), 0);
  EXPECT_EQ(b_aic_aiv->src_slot_size(), 0);
}

/* A -> B(thread dim = 4), shape is not continuous.
 * B is thread 1 and A's output address is 20000, B's input address is
 * 5000.
 * b's shape is {288, 8, 24, 33}, and we slice the axis{288}, select the
 * thread 1. range {{72:144},{0:8},{0:24},{0:33}}.
 * Burst len is 250000, need 4 prefetch context. */
TEST_F(FftsPlusDataContextST, prefetch_02_2)
{
  ge::NodePtr b;
  vector<uint32_t> axes = {0};
  ComputeGraphPtr graph;
  CreateGraph02_x(b, graph, axes);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_prefetch_enable_bitmap(0);
  a_aic_aiv->set_prefetch_once_bitmap(0);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();
  b_aic_aiv->set_prefetch_enable_bitmap(0);
  b_aic_aiv->set_prefetch_once_bitmap(0);

  prefetch_.burst_len_ = 250000;
  Status ret = prefetch_.GenManualDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 6);
  for (size_t i = 2; i <= 4; i++) {
    auto prefetch_context = ffts_plus_task_def->ffts_plus_ctx_[i].data_ctx();
    EXPECT_EQ(prefetch_context.thread_id(), 0);
    EXPECT_EQ(prefetch_context.addr_base(), 5000);
    EXPECT_EQ(prefetch_context.addr_offset(), 912384 + 250000 * (i - 2));
    EXPECT_EQ(prefetch_context.non_tail_len_inner(), 250000);
    EXPECT_EQ(prefetch_context.non_tail_num_inner(), 1);
    EXPECT_EQ(prefetch_context.non_tail_num_outter(), 1);
    EXPECT_EQ(prefetch_context.non_tail_stride_inner(), 3649536);
    EXPECT_EQ(prefetch_context.non_tail_stride_outter(), 3649536);

    EXPECT_EQ(prefetch_context.tail_len_inner(), 250000);
    EXPECT_EQ(prefetch_context.tail_num_inner(), 1);
    EXPECT_EQ(prefetch_context.tail_num_outter(), 1);
    EXPECT_EQ(prefetch_context.tail_stride_inner(), 3649536);
    EXPECT_EQ(prefetch_context.tail_stride_outter(), 3649536);
  }


  auto prefetch_context = ffts_plus_task_def->ffts_plus_ctx_[5].data_ctx();
  EXPECT_EQ(prefetch_context.thread_id(), 0);
  EXPECT_EQ(prefetch_context.addr_base(), 5000);
  EXPECT_EQ(prefetch_context.addr_offset(), 912384 + 250000 * (3));
  EXPECT_EQ(prefetch_context.non_tail_len_inner(), 912384 - 750000);
  EXPECT_EQ(prefetch_context.non_tail_num_inner(), 1);
  EXPECT_EQ(prefetch_context.non_tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context.non_tail_stride_inner(), 3649536);
  EXPECT_EQ(prefetch_context.non_tail_stride_outter(), 3649536);

  EXPECT_EQ(prefetch_context.tail_len_inner(), 912384 - 750000);
  EXPECT_EQ(prefetch_context.tail_num_inner(), 1);
  EXPECT_EQ(prefetch_context.tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context.tail_stride_inner(), 3649536);
  EXPECT_EQ(prefetch_context.tail_stride_outter(), 3649536);


  a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[0].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->prefetch_enable_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->prefetch_once_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);

  b_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[1].mutable_aic_aiv_ctx();
  EXPECT_EQ(b_aic_aiv->src_slot_size(), 4);
  EXPECT_EQ(b_aic_aiv->src_slot(0), 2);
  EXPECT_EQ(b_aic_aiv->src_slot(1), 3);
  EXPECT_EQ(b_aic_aiv->src_slot(2), 4);
  EXPECT_EQ(b_aic_aiv->src_slot(3), 5);

  EXPECT_EQ(b_aic_aiv->prefetch_enable_bitmap(), 0xF);
  EXPECT_EQ(b_aic_aiv->prefetch_once_bitmap(), 0xF);

}


/* A --------\
 * C ------> B
 * D -------/
 * shape is not continuous.
 * B is thread 1 and A's output address is 20000, B's input address is
 * 5000.
 * b's input0 shape is {288, 8, 24, 33}, and we slice the axis{288}
 * into 4, select the thread 1. range {{72:144},{0:8},{0:24},{0:33}}.
 * Burst len is 350000, need 3 prefetch context.
 *
 * b's input1 shape is {288, 8, 24, 33}, and we slice the axis {288} also,
 * Burst len is 350000, need 3 prefetch context. 3+3 is larger
 * than 4, we will not provide prefetch for this input.
 *
 * b's input2 shape is {288, 8, 24, 33}, and we slice the axis {24, 33},
 * Burst len is 350000, need 1 prefetch context. 3+1 is equal to 4,
 * we will provide prefetch for this input.
 *
 * */
TEST_F(FftsPlusDataContextST, prefetch_02_2_1)
{
  ge::NodePtr b;
  ComputeGraphPtr graph;
  CreateGraph02_2_1(b, graph);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();
  b_aic_aiv->set_prefetch_enable_bitmap(0);
  b_aic_aiv->set_prefetch_once_bitmap(0);

  prefetch_.burst_len_ = 350000;
  Status ret = prefetch_.GenManualDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 5);
  for (size_t i = 1; i <= 2; i++) {
    auto prefetch_context = ffts_plus_task_def->ffts_plus_ctx_[i].data_ctx();
    EXPECT_EQ(prefetch_context.thread_id(), 0);
    EXPECT_EQ(prefetch_context.addr_base(), 5000);
    EXPECT_EQ(prefetch_context.addr_offset(), 912384 + 350000 * (i - 1));
    EXPECT_EQ(prefetch_context.non_tail_len_inner(), 350000);
    EXPECT_EQ(prefetch_context.non_tail_num_inner(), 1);
    EXPECT_EQ(prefetch_context.non_tail_num_outter(), 1);
    EXPECT_EQ(prefetch_context.non_tail_stride_inner(), 3649536);
    EXPECT_EQ(prefetch_context.non_tail_stride_outter(), 3649536);

    EXPECT_EQ(prefetch_context.tail_len_inner(), 350000);
    EXPECT_EQ(prefetch_context.tail_num_inner(), 1);
    EXPECT_EQ(prefetch_context.tail_num_outter(), 1);
    EXPECT_EQ(prefetch_context.tail_stride_inner(), 3649536);
    EXPECT_EQ(prefetch_context.tail_stride_outter(), 3649536);
  }


  auto prefetch_context = ffts_plus_task_def->ffts_plus_ctx_[3].data_ctx();
  EXPECT_EQ(prefetch_context.thread_id(), 0);
  EXPECT_EQ(prefetch_context.addr_base(), 5000);
  EXPECT_EQ(prefetch_context.addr_offset(), 912384 + 350000 * (2));
  EXPECT_EQ(prefetch_context.non_tail_len_inner(), 912384 - 700000);
  EXPECT_EQ(prefetch_context.non_tail_num_inner(), 1);
  EXPECT_EQ(prefetch_context.non_tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context.non_tail_stride_inner(), 3649536);
  EXPECT_EQ(prefetch_context.non_tail_stride_outter(), 3649536);

  EXPECT_EQ(prefetch_context.tail_len_inner(), 912384 - 700000);
  EXPECT_EQ(prefetch_context.tail_num_inner(), 1);
  EXPECT_EQ(prefetch_context.tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context.tail_stride_inner(), 3649536);
  EXPECT_EQ(prefetch_context.tail_stride_outter(), 3649536);

  auto prefetch_context2 = ffts_plus_task_def->ffts_plus_ctx_[4].data_ctx();
  EXPECT_EQ(prefetch_context2.thread_id(), 0);
  EXPECT_EQ(prefetch_context2.addr_base(), 7000);
  EXPECT_EQ(prefetch_context2.addr_offset(), (18 * 33 + 24) * 2);
  EXPECT_EQ(prefetch_context2.non_tail_len_inner(), 9* 2);
  EXPECT_EQ(prefetch_context2.non_tail_num_inner(), 6);
  EXPECT_EQ(prefetch_context2.non_tail_num_outter(), 288 * 8);
  EXPECT_EQ(prefetch_context2.non_tail_stride_inner(), 33 * 2);
  EXPECT_EQ(prefetch_context2.non_tail_stride_outter(), 24 * 33 * 2);

  EXPECT_EQ(prefetch_context2.tail_len_inner(), 9* 2);
  EXPECT_EQ(prefetch_context2.tail_num_inner(), 6);
  EXPECT_EQ(prefetch_context2.tail_num_outter(), 288 * 8);
  EXPECT_EQ(prefetch_context2.tail_stride_inner(), 33 * 2);
  EXPECT_EQ(prefetch_context2.tail_stride_outter(), 24 * 33 * 2);


  b_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[0].mutable_aic_aiv_ctx();
  EXPECT_EQ(b_aic_aiv->src_slot_size(), 4);
  EXPECT_EQ(b_aic_aiv->src_slot(0), 1);
  EXPECT_EQ(b_aic_aiv->src_slot(1), 2);
  EXPECT_EQ(b_aic_aiv->src_slot(2), 3);
  EXPECT_EQ(b_aic_aiv->src_slot(3), 4);

  EXPECT_EQ(b_aic_aiv->prefetch_enable_bitmap(), 0xF);
  EXPECT_EQ(b_aic_aiv->prefetch_once_bitmap(), 0xF);

}

/* A -> B(thread dim = 4), shape is not continuous.
 * B is thread 1 and A's output address is 20000, B's input address is
 * 5000.
 * b's shape is {288, 8, 24, 33}, and we slice the axis{24, 33}, select the
 * thread 1. range {{0:288},{0:8},{6:12},{8:16}}. */
TEST_F(FftsPlusDataContextST, prefetch_02_3)
{
  ge::NodePtr b;
  vector<uint32_t> axes = {2, 3};
  ComputeGraphPtr graph;
  CreateGraph02_x(b, graph, axes);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_prefetch_enable_bitmap(0);
  a_aic_aiv->set_prefetch_once_bitmap(0);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();
  b_aic_aiv->set_prefetch_enable_bitmap(0);
  b_aic_aiv->set_prefetch_once_bitmap(0);

  Status ret = prefetch_.GenManualDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 3);
  auto prefetch_context = ffts_plus_task_def->ffts_plus_ctx_[2].data_ctx();
  EXPECT_EQ(prefetch_context.thread_id(), 0);
  EXPECT_EQ(prefetch_context.addr_base(), 5000);
  EXPECT_EQ(prefetch_context.addr_offset(), 6 * 33 * 2 + 16);
  EXPECT_EQ(prefetch_context.non_tail_len_inner(), 16);
  EXPECT_EQ(prefetch_context.non_tail_num_inner(), 6);
  EXPECT_EQ(prefetch_context.non_tail_num_outter(), 8 * 288);
  EXPECT_EQ(prefetch_context.non_tail_stride_inner(), 66);
  EXPECT_EQ(prefetch_context.non_tail_stride_outter(), 33 * 24 * 2);

  EXPECT_EQ(prefetch_context.tail_len_inner(), 16);
  EXPECT_EQ(prefetch_context.tail_num_inner(), 6);
  EXPECT_EQ(prefetch_context.tail_num_outter(), 8 * 288);
  EXPECT_EQ(prefetch_context.tail_stride_inner(), 66);
  EXPECT_EQ(prefetch_context.tail_stride_outter(), 66 * 24);

  a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[0].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->prefetch_enable_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->prefetch_once_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);

  b_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[1].mutable_aic_aiv_ctx();
  EXPECT_EQ(b_aic_aiv->prefetch_enable_bitmap(), 0x0001);
  EXPECT_EQ(b_aic_aiv->prefetch_once_bitmap(), 0x0001);
  EXPECT_EQ(b_aic_aiv->src_slot_size(), 1);
  EXPECT_EQ(b_aic_aiv->src_slot(0), 2);

}


/* A -> B(thread dim = 4), shape is not continuous.
 * B is thread 1 and A's output address is 20000, B's input address is
 * 5000.
 * b's shape is {288, 8, 24, 33}, and we slice the axis{8, 24, 33}, select the
 * thread 1. range {{0:288},{2:4},{6:12},{8:16}}.
 * Because the first axis 288 is not sliced, we need more than 4 prefetch context for
 * a single node which is not allowed by the data stucture of aic/aiv context.
 * */
TEST_F(FftsPlusDataContextST, prefetch_02_4)
{
  ge::NodePtr b;
  vector<uint32_t> axes = {1, 2, 3};
  ComputeGraphPtr graph;
  CreateGraph02_x(b, graph, axes);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_prefetch_enable_bitmap(0);
  a_aic_aiv->set_prefetch_once_bitmap(0);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();
  b_aic_aiv->set_prefetch_enable_bitmap(0);
  b_aic_aiv->set_prefetch_once_bitmap(0);

  Status ret = prefetch_.GenManualDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 2);

}

/* A -> B(thread dim = 4), shape is not continuous.
 * B is thread 1 and A's output address is 20000, B's input address is
 * 5000.
 * b's shape is {288, 8, 24, 33}, and we slice the axis{8, 24, 33}, select the
 * thread 1. range {{0:288},{2:4},{6:12},{8:16}}.
 * Because the first axis 288 is not sliced, we need more than 4 prefetch context for
 * a single node which is not allowed by the data stucture of aic/aiv context.
 * */
TEST_F(FftsPlusDataContextST, prefetch_02_4_1)
{
  ge::NodePtr b;
  vector<uint32_t> axes = {1, 2, 3};
  ComputeGraphPtr graph;
  CreateGraph02_4_1(b, graph, axes);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_prefetch_enable_bitmap(0);
  a_aic_aiv->set_prefetch_once_bitmap(0);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();
  b_aic_aiv->set_prefetch_enable_bitmap(0);
  b_aic_aiv->set_prefetch_once_bitmap(0);

  Status ret = prefetch_.GenManualDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 6);
  uint32_t offset_per_context = 8 * 24 * 33;
  uint32_t offset_in_context = 8 + 6 * 33 + 2 * 24 * 33;
  for (size_t i = 2; i <= 4; i++) {
    auto prefetch_context = ffts_plus_task_def->ffts_plus_ctx_[i].data_ctx();
    EXPECT_EQ(prefetch_context.thread_id(), 0);
    EXPECT_EQ(prefetch_context.addr_base(), 5000);
    EXPECT_EQ(prefetch_context.addr_offset(), offset_per_context * 2 * (i - 2) + offset_in_context * 2);
    EXPECT_EQ(prefetch_context.non_tail_len_inner(), 16);
    EXPECT_EQ(prefetch_context.non_tail_num_inner(), 6);
    EXPECT_EQ(prefetch_context.non_tail_num_outter(), 2);
    EXPECT_EQ(prefetch_context.non_tail_stride_inner(), 66);
    EXPECT_EQ(prefetch_context.non_tail_stride_outter(), 24 * 33 * 2);

    EXPECT_EQ(prefetch_context.tail_len_inner(), 16);
    EXPECT_EQ(prefetch_context.tail_num_inner(), 6);
    EXPECT_EQ(prefetch_context.tail_num_outter(), 2);
    EXPECT_EQ(prefetch_context.tail_stride_inner(), 66);
    EXPECT_EQ(prefetch_context.tail_stride_outter(), 24 * 33 * 2);
  }
  a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[0].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->prefetch_enable_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->prefetch_once_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);

  b_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[1].mutable_aic_aiv_ctx();
  EXPECT_EQ(b_aic_aiv->src_slot_size(), 4);
  EXPECT_EQ(b_aic_aiv->src_slot(0), 2);
  EXPECT_EQ(b_aic_aiv->src_slot(1), 3);
  EXPECT_EQ(b_aic_aiv->src_slot(2), 4);
  EXPECT_EQ(b_aic_aiv->src_slot(3), 5);

  EXPECT_EQ(b_aic_aiv->prefetch_enable_bitmap(), 0xF);
  EXPECT_EQ(b_aic_aiv->prefetch_once_bitmap(), 0xF);
}

/* A -> B(thread dim = 4), shape is not continuous.
 * B is thread 1 and A's output address is 20000, B's input address is
 * 5000.
 * b's shape is {288, 8, 24, 33}, and we slice the axis{288, 24, 33}, select the
 * thread 1. range {{72:144},{0:8},{6:12},{8:16}}.
 * The */
TEST_F(FftsPlusDataContextST, prefetch_02_5)
{
  ge::NodePtr b;
  vector<uint32_t> axes = {0, 2, 3};
  ComputeGraphPtr graph;
  CreateGraph02_x(b, graph, axes);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_prefetch_enable_bitmap(0);
  a_aic_aiv->set_prefetch_once_bitmap(0);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();
  b_aic_aiv->set_prefetch_enable_bitmap(0);
  b_aic_aiv->set_prefetch_once_bitmap(0);

  Status ret = prefetch_.GenManualDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 3);
  auto prefetch_context = ffts_plus_task_def->ffts_plus_ctx_[2].data_ctx();
  EXPECT_EQ(prefetch_context.thread_id(), 0);
  EXPECT_EQ(prefetch_context.addr_base(), 5000);
  EXPECT_EQ(prefetch_context.addr_offset(), (72 * 8 * 24 * 33 + 33 * 6 + 8) * 2);
  EXPECT_EQ(prefetch_context.non_tail_len_inner(), 16);
  EXPECT_EQ(prefetch_context.non_tail_num_inner(), 6);
  EXPECT_EQ(prefetch_context.non_tail_num_outter(), 8 * 72);
  EXPECT_EQ(prefetch_context.non_tail_stride_inner(), 66);
  EXPECT_EQ(prefetch_context.non_tail_stride_outter(), 66 * 24);

  EXPECT_EQ(prefetch_context.tail_len_inner(), 16);
  EXPECT_EQ(prefetch_context.tail_num_inner(), 6);
  EXPECT_EQ(prefetch_context.tail_num_outter(), 8 * 72);
  EXPECT_EQ(prefetch_context.tail_stride_inner(), 66);
  EXPECT_EQ(prefetch_context.tail_stride_outter(), 66 * 24);

  a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[0].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->prefetch_enable_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->prefetch_once_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);

  b_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[1].mutable_aic_aiv_ctx();
  EXPECT_EQ(b_aic_aiv->prefetch_enable_bitmap(), 0x0001);
  EXPECT_EQ(b_aic_aiv->prefetch_once_bitmap(), 0x0001);
  EXPECT_EQ(b_aic_aiv->src_slot_size(), 1);
  EXPECT_EQ(b_aic_aiv->src_slot(0), 2);

}

/* A -> B(thread dim = 4), shape is not continuous.
 * B is thread 1 and A's output address is 20000, B's input address is
 * 5000.
 * b's shape is {288, 8, 24, 33}, and we slice the axis{288, 24}, select the
 * thread 1. range {{144:216},{0:8},{12:18},{0:33}}.
 * Burst len is 250000, need 1 prefetch context. */
TEST_F(FftsPlusDataContextST, prefetch_03)
{
  ge::NodePtr b;
  ComputeGraphPtr graph;
  CreateGraph03_x(b, graph);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_prefetch_enable_bitmap(0);
  a_aic_aiv->set_prefetch_once_bitmap(0);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();
  b_aic_aiv->set_prefetch_enable_bitmap(0);
  b_aic_aiv->set_prefetch_once_bitmap(0);

  prefetch_.burst_len_ = 250000;
  Status ret = prefetch_.GenManualDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 3);

  auto prefetch_context = ffts_plus_task_def->ffts_plus_ctx_[2].data_ctx();
  EXPECT_EQ(prefetch_context.thread_id(), 0);
  EXPECT_EQ(prefetch_context.addr_base(), 5000);
  EXPECT_EQ(prefetch_context.addr_offset(), (12 * 33 + 144 * (8 * 24 * 33)) * 2);
  EXPECT_EQ(prefetch_context.non_tail_len_inner(), 396);
  EXPECT_EQ(prefetch_context.non_tail_num_inner(), 576);
  EXPECT_EQ(prefetch_context.non_tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context.non_tail_stride_inner(), 1584);
  EXPECT_EQ(prefetch_context.non_tail_stride_outter(), 3649536);

  EXPECT_EQ(prefetch_context.tail_len_inner(), 396);
  EXPECT_EQ(prefetch_context.tail_num_inner(), 576);
  EXPECT_EQ(prefetch_context.tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context.tail_stride_inner(), 1584);
  EXPECT_EQ(prefetch_context.tail_stride_outter(), 3649536);


  a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[0].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->prefetch_enable_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->prefetch_once_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);

  b_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[1].mutable_aic_aiv_ctx();
  EXPECT_EQ(b_aic_aiv->src_slot_size(), 1);
  EXPECT_EQ(b_aic_aiv->src_slot(0), 2);
  EXPECT_EQ(b_aic_aiv->prefetch_enable_bitmap(), 0x1);
  EXPECT_EQ(b_aic_aiv->prefetch_once_bitmap(), 0x1);

}


/* A -> B(thread dim = 4), shape is not continuous.
 * B is thread 1 and A's output address is 20000, B's input address is
 * 5000.
 * b's shape is {288, 8, 24, 33}, and we slice the axis{288}, select the
 * thread 1. range {{72:144},{0:8},{0:24},{0:33}}.
 * Burst len is 250000, need 4 prefetch context. */
TEST_F(FftsPlusDataContextST, prefetch_03_1)
{
  ge::NodePtr b;
  vector<uint32_t> axes = {0};
  ComputeGraphPtr graph;
  CreateGraph02_x(b, graph, axes);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_prefetch_enable_bitmap(0);
  a_aic_aiv->set_prefetch_once_bitmap(0);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();
  b_aic_aiv->set_prefetch_enable_bitmap(0);
  b_aic_aiv->set_prefetch_once_bitmap(0);

  prefetch_.burst_len_ = 250000;
  Status ret = prefetch_.GenManualDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 6);
  for (size_t i = 2; i <= 4; i++) {
    auto prefetch_context = ffts_plus_task_def->ffts_plus_ctx_[i].data_ctx();
    EXPECT_EQ(prefetch_context.thread_id(), 0);
    EXPECT_EQ(prefetch_context.addr_base(), 5000);
    EXPECT_EQ(prefetch_context.addr_offset(), 912384 + 250000 * (i - 2));
    EXPECT_EQ(prefetch_context.non_tail_len_inner(), 250000);
    EXPECT_EQ(prefetch_context.non_tail_num_inner(), 1);
    EXPECT_EQ(prefetch_context.non_tail_num_outter(), 1);
    EXPECT_EQ(prefetch_context.non_tail_stride_inner(), 3649536);
    EXPECT_EQ(prefetch_context.non_tail_stride_outter(), 3649536);

    EXPECT_EQ(prefetch_context.tail_len_inner(), 250000);
    EXPECT_EQ(prefetch_context.tail_num_inner(), 1);
    EXPECT_EQ(prefetch_context.tail_num_outter(), 1);
    EXPECT_EQ(prefetch_context.tail_stride_inner(), 3649536);
    EXPECT_EQ(prefetch_context.tail_stride_outter(), 3649536);
  }

  auto prefetch_context = ffts_plus_task_def->ffts_plus_ctx_[5].data_ctx();
  EXPECT_EQ(prefetch_context.thread_id(), 0);
  EXPECT_EQ(prefetch_context.addr_base(), 5000);
  EXPECT_EQ(prefetch_context.addr_offset(), 912384 + 250000 * (3));
  EXPECT_EQ(prefetch_context.non_tail_len_inner(), 912384 - 750000);
  EXPECT_EQ(prefetch_context.non_tail_num_inner(), 1);
  EXPECT_EQ(prefetch_context.non_tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context.non_tail_stride_inner(), 3649536);
  EXPECT_EQ(prefetch_context.non_tail_stride_outter(), 3649536);

  EXPECT_EQ(prefetch_context.tail_len_inner(), 912384 - 750000);
  EXPECT_EQ(prefetch_context.tail_num_inner(), 1);
  EXPECT_EQ(prefetch_context.tail_num_outter(), 1);
  EXPECT_EQ(prefetch_context.tail_stride_inner(), 3649536);
  EXPECT_EQ(prefetch_context.tail_stride_outter(), 3649536);


  a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[0].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->prefetch_enable_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->prefetch_once_bitmap(), 0);
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);

  b_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[1].mutable_aic_aiv_ctx();
  EXPECT_EQ(b_aic_aiv->src_slot_size(), 4);
  EXPECT_EQ(b_aic_aiv->src_slot(0), 2);
  EXPECT_EQ(b_aic_aiv->src_slot(1), 3);
  EXPECT_EQ(b_aic_aiv->src_slot(2), 4);
  EXPECT_EQ(b_aic_aiv->src_slot(3), 5);
  EXPECT_EQ(b_aic_aiv->prefetch_enable_bitmap(), 0xF);
  EXPECT_EQ(b_aic_aiv->prefetch_once_bitmap(), 0xF);

}


/* A -----> C
 *  \-----> B
 * Shape is conitnuous.
 *
 * A and B have the same thread scope id 1 and C have scope id 2.
 * split axis {288}
 * Burst len is 250000
 * need 4 write back data context.
 *
 * */
TEST_F(FftsPlusDataContextST, write_back_01) {
  ge::NodePtr b;
  vector<uint32_t> axes = {0};
  ComputeGraphPtr graph;
  CreateGraph04_x(b, graph, axes);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_successor_num(1);
  a_aic_aiv->add_successor_list(1);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);

  write_back_.burst_len_ = 250000;

  CacheManager cm;
  cm.SetCacheOperation(*graph);
  Status ret = write_back_.GenManualDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 6);
  for (size_t i = 2; i <= 4; i++) {
    EXPECT_EQ(ffts_plus_task_def->ffts_plus_ctx_[i].context_type(), RT_HW_CTX_TYPE_WRITEBACK_DATA);
    auto writeback_context = ffts_plus_task_def->ffts_plus_ctx_[i].data_ctx();
    EXPECT_EQ(writeback_context.thread_id(), 0);
    EXPECT_EQ(writeback_context.addr_base(), 30000);
    EXPECT_EQ(writeback_context.addr_offset(), 250000 * (i - 2));
    EXPECT_EQ(writeback_context.non_tail_len_inner(), 250000);
    EXPECT_EQ(writeback_context.non_tail_num_inner(), 1);
    EXPECT_EQ(writeback_context.non_tail_num_outter(), 1);
    EXPECT_EQ(writeback_context.non_tail_stride_inner(), 912384);
    EXPECT_EQ(writeback_context.non_tail_stride_outter(), 912384);

    EXPECT_EQ(writeback_context.tail_len_inner(), 250000);
    EXPECT_EQ(writeback_context.tail_num_inner(), 1);
    EXPECT_EQ(writeback_context.tail_num_outter(), 1);
    EXPECT_EQ(writeback_context.tail_stride_inner(), 912384);
    EXPECT_EQ(writeback_context.tail_stride_outter(), 912384);
  }

  EXPECT_EQ(ffts_plus_task_def->ffts_plus_ctx_[5].context_type(), RT_HW_CTX_TYPE_WRITEBACK_DATA);
  auto writeback_context = ffts_plus_task_def->ffts_plus_ctx_[5].data_ctx();
  EXPECT_EQ(writeback_context.thread_id(), 0);
  EXPECT_EQ(writeback_context.addr_base(), 30000);
  EXPECT_EQ(writeback_context.addr_offset(), 250000 * 3);
  EXPECT_EQ(writeback_context.non_tail_len_inner(), 912384 - 750000);
  EXPECT_EQ(writeback_context.non_tail_num_inner(), 1);
  EXPECT_EQ(writeback_context.non_tail_num_outter(), 1);
  EXPECT_EQ(writeback_context.non_tail_stride_inner(), 912384);
  EXPECT_EQ(writeback_context.non_tail_stride_outter(), 912384);

  EXPECT_EQ(writeback_context.tail_len_inner(), 912384 - 750000);
  EXPECT_EQ(writeback_context.tail_num_inner(), 1);
  EXPECT_EQ(writeback_context.tail_num_outter(), 1);
  EXPECT_EQ(writeback_context.tail_stride_inner(), 912384);
  EXPECT_EQ(writeback_context.tail_stride_outter(), 912384);

  a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[0].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);
  EXPECT_EQ(a_aic_aiv->successor_num(), 5);
  EXPECT_EQ(a_aic_aiv->successor_list(0), 1);
  EXPECT_EQ(a_aic_aiv->successor_list(1), 2);
  EXPECT_EQ(a_aic_aiv->successor_list(2), 3);
  EXPECT_EQ(a_aic_aiv->successor_list(3), 4);
  EXPECT_EQ(a_aic_aiv->successor_list(4), 5);

}

/* A -----> C
 *  \-----> B
 * Shape is conitnuous.
 *
 * A and B have the same thread scope id 1 and C have scope id 2.
 * split axis {288}
 * Burst len is 1000000
 * need 1 write back data context.
 * */
TEST_F(FftsPlusDataContextST, write_back_01_2) {
  ge::NodePtr a;
  vector<uint32_t> axes = {0};
  ComputeGraphPtr graph;
  CreateGraph04_x(a, graph, axes);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_successor_num(1);
  a_aic_aiv->add_successor_list(1);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);

  write_back_.burst_len_ = 1000000;

  CacheManager cm;
  cm.SetCacheOperation(*graph);
  Status ret = write_back_.GenManualDataCtxDef(a, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 3);

  EXPECT_EQ(ffts_plus_task_def->ffts_plus_ctx_[2].context_type(), RT_HW_CTX_TYPE_WRITEBACK_DATA);
  auto writeback_context = ffts_plus_task_def->ffts_plus_ctx_[2].data_ctx();
  EXPECT_EQ(writeback_context.thread_id(), 0);
  EXPECT_EQ(writeback_context.addr_base(), 30000);
  EXPECT_EQ(writeback_context.addr_offset(), 0);
  EXPECT_EQ(writeback_context.non_tail_len_inner(), 912384);
  EXPECT_EQ(writeback_context.non_tail_num_inner(), 1);
  EXPECT_EQ(writeback_context.non_tail_num_outter(), 1);
  EXPECT_EQ(writeback_context.non_tail_stride_inner(), 912384);
  EXPECT_EQ(writeback_context.non_tail_stride_outter(), 912384);

  EXPECT_EQ(writeback_context.tail_len_inner(), 912384);
  EXPECT_EQ(writeback_context.tail_num_inner(), 1);
  EXPECT_EQ(writeback_context.tail_num_outter(), 1);
  EXPECT_EQ(writeback_context.tail_stride_inner(), 912384);
  EXPECT_EQ(writeback_context.tail_stride_outter(), 912384);

  a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[0].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);
  EXPECT_EQ(a_aic_aiv->successor_num(), 2);
  EXPECT_EQ(a_aic_aiv->successor_list(0), 1);
  EXPECT_EQ(a_aic_aiv->successor_list(1), 2);
}

/* A -----> C
 *  \-----> B
 * Shape is conitnuous. auto threading
 *
 * A and B have the same thread scope id 1 and C have scope id 2.
 * split axis {288}
 * Burst len is 1000000
 * need 1 write back data context.
 * */
TEST_F(FftsPlusDataContextST, auto_threading_write_back_01_2) {
  ge::NodePtr a;
  ComputeGraphPtr graph;
  CreateAutoThreadingGraph04_x(a, graph);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  GenerateABAutoThreadingTaskDef(ffts_plus_task_def);

  write_back_auto_.burst_len_ = 1000000;

  CacheManager cm;
  cm.SetCacheOperation(*graph);
  Status ret = write_back_auto_.GenAutoDataCtxDef(a, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 22);


  // check a's succ_list has data
  bool has_data_ctx = false;
  auto aic_aiv_context_a0 = ffts_plus_task_def->ffts_plus_ctx_[5].aic_aiv_ctx();
  for (auto succ: aic_aiv_context_a0.successor_list()) {
    if (succ == 18) {
      has_data_ctx = true;
    }
  }
  EXPECT_EQ(has_data_ctx, true);

  EXPECT_EQ(ffts_plus_task_def->ffts_plus_ctx_[18].context_type(), RT_HW_CTX_TYPE_WRITEBACK_DATA);
  auto writeback_context = ffts_plus_task_def->ffts_plus_ctx_[18].data_ctx();
  EXPECT_EQ(writeback_context.thread_id(), 0);
  EXPECT_EQ(writeback_context.addr_base(), 20000);
  EXPECT_EQ(writeback_context.addr_offset(), 912384);
  EXPECT_EQ(writeback_context.non_tail_len_inner(), 912384);
  EXPECT_EQ(writeback_context.non_tail_num_inner(), 1);
  EXPECT_EQ(writeback_context.non_tail_num_outter(), 1);
  EXPECT_EQ(writeback_context.non_tail_stride_inner(), 3649536);
  EXPECT_EQ(writeback_context.non_tail_stride_outter(), 3649536);

  EXPECT_EQ(writeback_context.tail_len_inner(), 912384);
  EXPECT_EQ(writeback_context.tail_num_inner(), 1);
  EXPECT_EQ(writeback_context.tail_num_outter(), 1);
  EXPECT_EQ(writeback_context.tail_stride_inner(), 3649536);
  EXPECT_EQ(writeback_context.tail_stride_outter(), 3649536);

  EXPECT_EQ(ffts_plus_task_def->ffts_plus_ctx_[19].context_type(), RT_HW_CTX_TYPE_WRITEBACK_DATA);
  writeback_context = ffts_plus_task_def->ffts_plus_ctx_[19].data_ctx();
  EXPECT_EQ(writeback_context.thread_id(), 1);
  EXPECT_EQ(writeback_context.addr_base(), 20000);
  EXPECT_EQ(writeback_context.addr_offset(), 912384);  // no_tail thread offset
  EXPECT_EQ(writeback_context.non_tail_len_inner(), 912384);
  EXPECT_EQ(writeback_context.non_tail_num_inner(), 1);
  EXPECT_EQ(writeback_context.non_tail_num_outter(), 1);
  EXPECT_EQ(writeback_context.non_tail_stride_inner(), 3649536);
  EXPECT_EQ(writeback_context.non_tail_stride_outter(), 3649536);

  EXPECT_EQ(writeback_context.tail_len_inner(), 912384);
  EXPECT_EQ(writeback_context.tail_num_inner(), 1);
  EXPECT_EQ(writeback_context.tail_num_outter(), 1);
  EXPECT_EQ(writeback_context.tail_stride_inner(), 3649536);
  EXPECT_EQ(writeback_context.tail_stride_outter(), 3649536);

  auto a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[5].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);
  EXPECT_EQ(a_aic_aiv->successor_num(), 2);
  EXPECT_EQ(a_aic_aiv->successor_list(0), 9);
  EXPECT_EQ(a_aic_aiv->successor_list(1), 18);
}

/* A -----> PhonyConcat ---->C
 *  \-----> B
 * Shape is conitnuous.
 *
 * A, B, PhonyConcat have the same thread scope id 1. But D does not have scope id
 *
 * split axis {288}
 * Burst len is 1000000
 * A need 1 write back data context.
 * PhonyConcat does not need writeback;
 * */
TEST_F(FftsPlusDataContextST, write_back_01_3) {
  ge::NodePtr a;
  vector<uint32_t> axes = {0};
  ComputeGraphPtr graph;
  CreateGraph04_x_1(a, graph, axes, 0);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_successor_num(1);
  a_aic_aiv->add_successor_list(1);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();

  write_back_.burst_len_ = 1000000;

  CacheManager cm;
  cm.SetCacheOperation(*graph);
  Status ret = write_back_.GenManualDataCtxDef(a, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 3);

  EXPECT_EQ(ffts_plus_task_def->ffts_plus_ctx_[2].context_type(), RT_HW_CTX_TYPE_WRITEBACK_DATA);
  auto writeback_context = ffts_plus_task_def->ffts_plus_ctx_[2].data_ctx();
  EXPECT_EQ(writeback_context.thread_id(), 0);
  EXPECT_EQ(writeback_context.addr_base(), 20000);
  EXPECT_EQ(writeback_context.addr_offset(), 0);
  EXPECT_EQ(writeback_context.non_tail_len_inner(), 912384);
  EXPECT_EQ(writeback_context.non_tail_num_inner(), 1);
  EXPECT_EQ(writeback_context.non_tail_num_outter(), 1);
  EXPECT_EQ(writeback_context.non_tail_stride_inner(), 912384);
  EXPECT_EQ(writeback_context.non_tail_stride_outter(), 912384);

  EXPECT_EQ(writeback_context.tail_len_inner(), 912384);
  EXPECT_EQ(writeback_context.tail_num_inner(), 1);
  EXPECT_EQ(writeback_context.tail_num_outter(), 1);
  EXPECT_EQ(writeback_context.tail_stride_inner(), 912384);
  EXPECT_EQ(writeback_context.tail_stride_outter(), 912384);

  a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[0].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);
  EXPECT_EQ(a_aic_aiv->successor_num(), 2);
  EXPECT_EQ(a_aic_aiv->successor_list(0), 1);
  EXPECT_EQ(a_aic_aiv->successor_list(1), 2);

}

/* A -----> PhonyConcat ---->C
 *  \-----> B
 * Shape is conitnuous.
 *
 * A, B, PhonyConcat have the same thread scope id 1.
 * But D has a different scope id
 *
 * split axis {288}
 * Burst len is 1000000
 * A need 1 write back data context.
 * PhonyConcat does not need writeback;
 * */
TEST_F(FftsPlusDataContextST, write_back_01_4) {
  ge::NodePtr a;
  vector<uint32_t> axes = {0};
  ComputeGraphPtr graph;
  CreateGraph04_x_1(a, graph, axes, 2);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_successor_num(1);
  a_aic_aiv->add_successor_list(1);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();

  write_back_.burst_len_ = 1000000;

  CacheManager cm;
  cm.SetCacheOperation(*graph);
  Status ret = write_back_.GenManualDataCtxDef(a, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 3);

  EXPECT_EQ(ffts_plus_task_def->ffts_plus_ctx_[2].context_type(), RT_HW_CTX_TYPE_WRITEBACK_DATA);
  auto writeback_context = ffts_plus_task_def->ffts_plus_ctx_[2].data_ctx();
  EXPECT_EQ(writeback_context.thread_id(), 0);
  EXPECT_EQ(writeback_context.addr_base(), 20000);
  EXPECT_EQ(writeback_context.addr_offset(), 0);
  EXPECT_EQ(writeback_context.non_tail_len_inner(), 912384);
  EXPECT_EQ(writeback_context.non_tail_num_inner(), 1);
  EXPECT_EQ(writeback_context.non_tail_num_outter(), 1);
  EXPECT_EQ(writeback_context.non_tail_stride_inner(), 912384);
  EXPECT_EQ(writeback_context.non_tail_stride_outter(), 912384);

  EXPECT_EQ(writeback_context.tail_len_inner(), 912384);
  EXPECT_EQ(writeback_context.tail_num_inner(), 1);
  EXPECT_EQ(writeback_context.tail_num_outter(), 1);
  EXPECT_EQ(writeback_context.tail_stride_inner(), 912384);
  EXPECT_EQ(writeback_context.tail_stride_outter(), 912384);

  a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[0].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);
  EXPECT_EQ(a_aic_aiv->successor_num(), 2);
  EXPECT_EQ(a_aic_aiv->successor_list(0), 1);
  EXPECT_EQ(a_aic_aiv->successor_list(1), 2);

}

/* A -----> C
 *  \-----> B
 * Shape is conitnuous.
 *
 * A and B and C have the same thread scope id 1.
 * split axis {33}
 * Burst len is 250000
 * need 4 write back data context.
 *
 * */
TEST_F(FftsPlusDataContextST, invalidate_01) {
  ge::NodePtr b;
  vector<uint32_t> axes = {0};
  ComputeGraphPtr graph;
  CreateGraph05_x(b, graph, axes);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_successor_num(2);
  a_aic_aiv->add_successor_list(1);
  a_aic_aiv->add_successor_list(2);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();

  auto c_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  c_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto c_aic_aiv = c_context->mutable_aic_aiv_ctx();

  invalidate_.burst_len_ = 250000;

  CacheManager cm;
  cm.SetCacheOperation(*graph);
  Status ret = invalidate_.GenManualDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 7);
  for (size_t i = 3; i <= 5; i++) {
    EXPECT_EQ(ffts_plus_task_def->ffts_plus_ctx_[i].context_type(), RT_HW_CTX_TYPE_INVALIDATE_DATA);
    auto invalidate_context = ffts_plus_task_def->ffts_plus_ctx_[i].data_ctx();
    EXPECT_EQ(invalidate_context.thread_id(), 0);
    EXPECT_EQ(invalidate_context.addr_base(), 20000);
    EXPECT_EQ(invalidate_context.addr_offset(), 250000 * (i - 3));
    EXPECT_EQ(invalidate_context.non_tail_len_inner(), 250000);
    EXPECT_EQ(invalidate_context.non_tail_num_inner(), 1);
    EXPECT_EQ(invalidate_context.non_tail_num_outter(), 1);
    EXPECT_EQ(invalidate_context.non_tail_stride_inner(), 912384);
    EXPECT_EQ(invalidate_context.non_tail_stride_outter(), 912384);

    EXPECT_EQ(invalidate_context.tail_len_inner(), 250000);
    EXPECT_EQ(invalidate_context.tail_num_inner(), 1);
    EXPECT_EQ(invalidate_context.tail_num_outter(), 1);
    EXPECT_EQ(invalidate_context.tail_stride_inner(), 912384);
    EXPECT_EQ(invalidate_context.tail_stride_outter(), 912384);
  }
  EXPECT_EQ(ffts_plus_task_def->ffts_plus_ctx_[6].context_type(), RT_HW_CTX_TYPE_INVALIDATE_DATA);
  auto invalidate_context = ffts_plus_task_def->ffts_plus_ctx_[6].data_ctx();
  EXPECT_EQ(invalidate_context.thread_id(), 0);
  EXPECT_EQ(invalidate_context.addr_base(), 20000);
  EXPECT_EQ(invalidate_context.addr_offset(), 250000 * 3);
  EXPECT_EQ(invalidate_context.non_tail_len_inner(), 912384 - 750000);
  EXPECT_EQ(invalidate_context.non_tail_num_inner(), 1);
  EXPECT_EQ(invalidate_context.non_tail_num_outter(), 1);
  EXPECT_EQ(invalidate_context.non_tail_stride_inner(), 912384);
  EXPECT_EQ(invalidate_context.non_tail_stride_outter(), 912384);

  EXPECT_EQ(invalidate_context.tail_len_inner(), 912384 - 750000);
  EXPECT_EQ(invalidate_context.tail_num_inner(), 1);
  EXPECT_EQ(invalidate_context.tail_num_outter(), 1);
  EXPECT_EQ(invalidate_context.tail_stride_inner(), 912384);
  EXPECT_EQ(invalidate_context.tail_stride_outter(), 912384);

  a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[0].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);
  EXPECT_EQ(a_aic_aiv->successor_num(), 2);
  EXPECT_EQ(a_aic_aiv->successor_list(0), 1);
  EXPECT_EQ(a_aic_aiv->successor_list(1), 2);

  b_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[1].mutable_aic_aiv_ctx();
  EXPECT_EQ(b_aic_aiv->src_slot_size(), 0);
  EXPECT_EQ(b_aic_aiv->successor_num(), 4);
  EXPECT_EQ(b_aic_aiv->successor_list(0), 3);
  EXPECT_EQ(b_aic_aiv->successor_list(1), 4);
  EXPECT_EQ(b_aic_aiv->successor_list(2), 5);
  EXPECT_EQ(b_aic_aiv->successor_list(3), 6);

  c_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[2].mutable_aic_aiv_ctx();
  EXPECT_EQ(c_aic_aiv->src_slot_size(), 0);
  EXPECT_EQ(c_aic_aiv->successor_num(), 4);
  EXPECT_EQ(c_aic_aiv->successor_list(0), 3);
  EXPECT_EQ(b_aic_aiv->successor_list(1), 4);
  EXPECT_EQ(b_aic_aiv->successor_list(2), 5);
  EXPECT_EQ(b_aic_aiv->successor_list(3), 6);
}

/* A -----> C
 *  \-----> B
 * Shape is conitnuous.
 *
 * A and B have the same thread scope id 1 and C have scope id 2.
 * split axis {288}
 * Burst len is 1000000
 * need 1 write back data context.
 * */
TEST_F(FftsPlusDataContextST, invalidate_01_2) {
  ge::NodePtr b;
  ComputeGraphPtr graph;
  vector<uint32_t> axes = {0};
  CreateGraph05_x(b, graph, axes);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_successor_num(2);
  a_aic_aiv->add_successor_list(1);
  a_aic_aiv->add_successor_list(2);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();

  auto c_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  c_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto c_aic_aiv = c_context->mutable_aic_aiv_ctx();

  invalidate_.burst_len_ = 1000000;

  CacheManager cm;
  cm.SetCacheOperation(*graph);
  Status ret = invalidate_.GenManualDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 4);

  EXPECT_EQ(ffts_plus_task_def->ffts_plus_ctx_[3].context_type(), RT_HW_CTX_TYPE_INVALIDATE_DATA);
  auto invalidate_context = ffts_plus_task_def->ffts_plus_ctx_[3].data_ctx();
  EXPECT_EQ(invalidate_context.thread_id(), 0);
  EXPECT_EQ(invalidate_context.addr_base(), 20000);
  EXPECT_EQ(invalidate_context.addr_offset(), 0);
  EXPECT_EQ(invalidate_context.non_tail_len_inner(), 912384);
  EXPECT_EQ(invalidate_context.non_tail_num_inner(), 1);
  EXPECT_EQ(invalidate_context.non_tail_num_outter(), 1);
  EXPECT_EQ(invalidate_context.non_tail_stride_inner(), 912384);
  EXPECT_EQ(invalidate_context.non_tail_stride_outter(), 912384);

  EXPECT_EQ(invalidate_context.tail_len_inner(), 912384);
  EXPECT_EQ(invalidate_context.tail_num_inner(), 1);
  EXPECT_EQ(invalidate_context.tail_num_outter(), 1);
  EXPECT_EQ(invalidate_context.tail_stride_inner(), 912384);
  EXPECT_EQ(invalidate_context.tail_stride_outter(), 912384);

  a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[0].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);
  EXPECT_EQ(a_aic_aiv->successor_num(), 2);
  EXPECT_EQ(a_aic_aiv->successor_list(0), 1);
  EXPECT_EQ(a_aic_aiv->successor_list(1), 2);

  b_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[1].mutable_aic_aiv_ctx();
  EXPECT_EQ(b_aic_aiv->src_slot_size(), 0);
  EXPECT_EQ(b_aic_aiv->successor_num(), 1);
  EXPECT_EQ(b_aic_aiv->successor_list(0), 3);

  c_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[2].mutable_aic_aiv_ctx();
  EXPECT_EQ(c_aic_aiv->src_slot_size(), 0);
  EXPECT_EQ(c_aic_aiv->successor_num(), 1);
  EXPECT_EQ(c_aic_aiv->successor_list(0), 3);
}

/* A -----> C
 *  \-----> B
 * Shape is conitnuous. auto threading
 *
 * A and B have the same thread scope id 1 and C have scope id 2.
 * split axis {288}
 * Burst len is 1000000
 * need 1 write back data context.
 * */
TEST_F(FftsPlusDataContextST, auto_threading_invalidate_01_2) {
  ge::NodePtr b;
  ComputeGraphPtr graph;
  vector<uint32_t> axes = {0};
  CreateAutoThreadGraph05_x(b, graph, axes);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  GenerateABCAutoThreadingTaskDef(ffts_plus_task_def);

  invalidate_auto_.burst_len_ = 1000000;

  CacheManager cm;
  cm.SetCacheOperation(*graph);
  Status ret = invalidate_auto_.GenAutoDataCtxDef(b, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 26);

  // window_num 0, invalid
  EXPECT_EQ(ffts_plus_task_def->ffts_plus_ctx_[22].context_type(), RT_HW_CTX_TYPE_INVALIDATE_DATA);
  auto invalidate_context = ffts_plus_task_def->ffts_plus_ctx_[22].data_ctx();
  EXPECT_EQ(invalidate_context.thread_id(), 0);
  EXPECT_EQ(invalidate_context.addr_base(), 20000);
  EXPECT_EQ(invalidate_context.addr_offset(), 912384);
  EXPECT_EQ(invalidate_context.non_tail_len_inner(), 912384);
  EXPECT_EQ(invalidate_context.non_tail_num_inner(), 1);
  EXPECT_EQ(invalidate_context.non_tail_num_outter(), 1);
  EXPECT_EQ(invalidate_context.non_tail_stride_inner(), 3649536);
  EXPECT_EQ(invalidate_context.non_tail_stride_outter(), 3649536);

  EXPECT_EQ(invalidate_context.tail_len_inner(), 912384);
  EXPECT_EQ(invalidate_context.tail_num_inner(), 1);
  EXPECT_EQ(invalidate_context.tail_num_outter(), 1);
  EXPECT_EQ(invalidate_context.tail_stride_inner(), 3649536);
  EXPECT_EQ(invalidate_context.tail_stride_outter(), 3649536);

  // window_num 1, invalid
  EXPECT_EQ(ffts_plus_task_def->ffts_plus_ctx_[23].context_type(), RT_HW_CTX_TYPE_INVALIDATE_DATA);
  invalidate_context = ffts_plus_task_def->ffts_plus_ctx_[23].data_ctx();
  EXPECT_EQ(invalidate_context.thread_id(), 1);
  EXPECT_EQ(invalidate_context.addr_base(), 20000);
  EXPECT_EQ(invalidate_context.addr_offset(), 912384);  // no_tail thread offset
  EXPECT_EQ(invalidate_context.non_tail_len_inner(), 912384);
  EXPECT_EQ(invalidate_context.non_tail_num_inner(), 1);
  EXPECT_EQ(invalidate_context.non_tail_num_outter(), 1);
  EXPECT_EQ(invalidate_context.non_tail_stride_inner(), 3649536);
  EXPECT_EQ(invalidate_context.non_tail_stride_outter(), 3649536);

  EXPECT_EQ(invalidate_context.tail_len_inner(), 912384);
  EXPECT_EQ(invalidate_context.tail_num_inner(), 1);
  EXPECT_EQ(invalidate_context.tail_num_outter(), 1);
  EXPECT_EQ(invalidate_context.tail_stride_inner(), 3649536);
  EXPECT_EQ(invalidate_context.tail_stride_outter(), 3649536);

  auto a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[5].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);
  EXPECT_EQ(a_aic_aiv->successor_num(), 2);
  EXPECT_EQ(a_aic_aiv->successor_list(0), 9);
  EXPECT_EQ(a_aic_aiv->successor_list(1), 13);

  auto b_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[9].mutable_aic_aiv_ctx();
  EXPECT_EQ(b_aic_aiv->src_slot_size(), 0);
  EXPECT_EQ(b_aic_aiv->successor_num(), 2);
  EXPECT_EQ(b_aic_aiv->successor_list(0), 17);
  EXPECT_EQ(b_aic_aiv->successor_list(1), 22);

  auto c_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[13].mutable_aic_aiv_ctx();
  EXPECT_EQ(c_aic_aiv->src_slot_size(), 0);
  EXPECT_EQ(c_aic_aiv->successor_num(), 2);
  EXPECT_EQ(c_aic_aiv->successor_list(0), 17);
  EXPECT_EQ(c_aic_aiv->successor_list(1), 22);
}

/* A -----> PhonyConcat ----> C
 *  \-----> B
 * Shape is conitnuous.
 *
 * A and B and C have the same thread scope id 1.
 * split axis {288}
 * Burst len is 1000000
 * A need 1 write back data context.
 * B and C's successor lists need to be updated.
 * */
TEST_F(FftsPlusDataContextST, invalidate_01_3) {
  ge::NodePtr a;
  ComputeGraphPtr graph;
  vector<uint32_t> axes = {0};
  CreateGraph04_x_1(a, graph, axes, 1);
  domi::TaskDef task_def;
  domi::FftsPlusTaskDef *ffts_plus_task_def = task_def.mutable_ffts_plus_task();
  auto a_context = ffts_plus_task_def->add_ffts_plus_ctx(); // a's context
  a_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto a_aic_aiv = a_context->mutable_aic_aiv_ctx();
  a_aic_aiv->set_successor_num(2);
  a_aic_aiv->add_successor_list(1);
  a_aic_aiv->add_successor_list(2);

  auto b_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  b_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto b_aic_aiv = b_context->mutable_aic_aiv_ctx();

  auto c_context = ffts_plus_task_def->add_ffts_plus_ctx(); // b's context
  c_context->set_context_type(RT_HW_CTX_TYPE_AIV);
  auto c_aic_aiv = c_context->mutable_aic_aiv_ctx();

  invalidate_.burst_len_ = 1000000;

  CacheManager cm;
  cm.SetCacheOperation(*graph);
  Status ret = invalidate_.GenManualDataCtxDef(a, ffts_plus_task_def);
  ASSERT_EQ(ffts::SUCCESS, ret);
  ASSERT_EQ(ffts_plus_task_def->ffts_plus_ctx_size(), 4);

  EXPECT_EQ(ffts_plus_task_def->ffts_plus_ctx_[3].context_type(), RT_HW_CTX_TYPE_INVALIDATE_DATA);
  auto invalidate_context = ffts_plus_task_def->ffts_plus_ctx_[3].data_ctx();
  EXPECT_EQ(invalidate_context.thread_id(), 0);
  EXPECT_EQ(invalidate_context.addr_base(), 20000);
  EXPECT_EQ(invalidate_context.addr_offset(), 0);
  EXPECT_EQ(invalidate_context.non_tail_len_inner(), 912384);
  EXPECT_EQ(invalidate_context.non_tail_num_inner(), 1);
  EXPECT_EQ(invalidate_context.non_tail_num_outter(), 1);
  EXPECT_EQ(invalidate_context.non_tail_stride_inner(), 912384);
  EXPECT_EQ(invalidate_context.non_tail_stride_outter(), 912384);

  EXPECT_EQ(invalidate_context.tail_len_inner(), 912384);
  EXPECT_EQ(invalidate_context.tail_num_inner(), 1);
  EXPECT_EQ(invalidate_context.tail_num_outter(), 1);
  EXPECT_EQ(invalidate_context.tail_stride_inner(), 912384);
  EXPECT_EQ(invalidate_context.tail_stride_outter(), 912384);

  a_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[0].mutable_aic_aiv_ctx();
  EXPECT_EQ(a_aic_aiv->src_slot_size(), 0);
  EXPECT_EQ(a_aic_aiv->successor_num(), 2);
  EXPECT_EQ(a_aic_aiv->successor_list(0), 1);
  EXPECT_EQ(a_aic_aiv->successor_list(1), 2);

  b_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[1].mutable_aic_aiv_ctx();
  EXPECT_EQ(b_aic_aiv->src_slot_size(), 0);
  EXPECT_EQ(b_aic_aiv->successor_num(), 1);
  EXPECT_EQ(b_aic_aiv->successor_list(0), 3);

  c_aic_aiv = ffts_plus_task_def->ffts_plus_ctx_[2].mutable_aic_aiv_ctx();
  EXPECT_EQ(c_aic_aiv->src_slot_size(), 0);
  EXPECT_EQ(c_aic_aiv->successor_num(), 1);
  EXPECT_EQ(c_aic_aiv->successor_list(0), 3);
}