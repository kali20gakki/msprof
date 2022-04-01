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
#include <memory>

#include "graph/anchor.h"
#include "graph/attr_value.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "omg/omg_inner_types.h"
#include "../passes/graph_builder_utils.h"

#define protected public
#define private public
#include "graph/build/memory/binary_block_mem_assigner.h"
#include "graph/build/memory/graph_mem_assigner.h"
#include "graph/build/memory/hybrid_mem_assigner.h"
#include "graph/build/memory/max_block_mem_assigner.h"
#include "graph/manager/graph_var_manager.h"
#undef protected
#undef private

using namespace std;
using namespace testing;
using namespace ge;
using domi::GetContext;

class UtestMemoryAssignerTest : public testing::Test {
 public:
  ge::OpDescPtr CreateOpWithWsSize(const string &name, int64_t wsByte, const string &type = "some",
                                   int64_t size = 1024) {
    ge::OpDescPtr op_def = std::make_shared<ge::OpDesc>(name, type);
    auto desc_temp_ptr = std::make_shared<ge::GeTensorDesc>();
    auto desc_temp = *desc_temp_ptr;

    TensorUtils::SetSize(desc_temp, size);
    op_def->AddInputDesc(desc_temp);
    op_def->AddOutputDesc(desc_temp);

    std::vector<int64_t> workspace_bytes;
    workspace_bytes.push_back(wsByte);
    op_def->SetWorkspaceBytes(workspace_bytes);
    return op_def;
  }
  ge::OpDescPtr CreateRefOpWithWsSize(const string &name, int64_t wsByte, const string &type = "some") {
    ge::OpDescPtr op_def = std::make_shared<ge::OpDesc>(name, type);
    auto desc_temp_ptr = std::make_shared<ge::GeTensorDesc>();
    auto desc_temp = *desc_temp_ptr;

    TensorUtils::SetSize(desc_temp, 1024);
    op_def->AddInputDesc(desc_temp);

    auto desc_output_ptr = std::make_shared<ge::GeTensorDesc>();
    auto desc_output = *desc_output_ptr;
    TensorUtils::SetSize(desc_output, 6500);
    ge::TensorUtils::SetReuseInput(desc_output, true);
    ge::TensorUtils::SetReuseInputIndex(desc_output, 0);
    op_def->AddOutputDesc(desc_output);

    std::vector<int64_t> workspace_bytes;
    workspace_bytes.push_back(wsByte);
    op_def->SetWorkspaceBytes(workspace_bytes);
    return op_def;
  }
  void MakeGraph(ge::ComputeGraphPtr &graph, const string &type = "some") {
    ge::OpDescPtr op_def_a = CreateOpWithWsSize("A", 6000, type);
    op_def_a->SetStreamId(0);
    ge::OpDescPtr op_def_b = CreateOpWithWsSize("B", 120000);
    op_def_b->SetStreamId(0);
    ge::OpDescPtr op_def_c = CreateOpWithWsSize("C", 16000);
    op_def_c->SetStreamId(1);
    ge::OpDescPtr op_def_d = CreateOpWithWsSize("D", 24000);
    op_def_d->SetStreamId(2);
    ge::OpDescPtr op_def_e = CreateOpWithWsSize("E", 24000);
    op_def_e->SetStreamId(3);
    ge::OpDescPtr op_def_f = CreateOpWithWsSize("F", 30000);
    op_def_f->SetStreamId(2);
    ge::OpDescPtr op_def_g = CreateOpWithWsSize("G", 32000);
    op_def_g->SetStreamId(3);
    ge::OpDescPtr op_def_h = CreateOpWithWsSize("H", 48000);
    op_def_h->SetStreamId(2);
    ge::OpDescPtr op_def_i = CreateOpWithWsSize("I", 60000);
    op_def_i->SetStreamId(2);
    ge::OpDescPtr op_def_j = CreateOpWithWsSize("J", 256000, NETOUTPUT);
    op_def_j->SetStreamId(3);

    // add node
    ge::NodePtr node_a = graph->AddNode(op_def_a);
    ge::NodePtr node_b = graph->AddNode(op_def_b);
    ge::NodePtr node_c = graph->AddNode(op_def_c);
    ge::NodePtr node_d = graph->AddNode(op_def_d);
    ge::NodePtr node_e = graph->AddNode(op_def_e);
    ge::NodePtr node_f = graph->AddNode(op_def_f);
    ge::NodePtr node_g = graph->AddNode(op_def_g);
    ge::NodePtr node_h = graph->AddNode(op_def_h);
    ge::NodePtr node_i = graph->AddNode(op_def_i);
    ge::NodePtr node_j = graph->AddNode(op_def_j);

    // add edge
    ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_e->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_g->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_d->GetOutDataAnchor(0), node_f->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_e->GetOutDataAnchor(0), node_g->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(node_f->GetOutDataAnchor(0), node_h->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_g->GetOutDataAnchor(0), node_j->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_h->GetOutDataAnchor(0), node_i->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_i->GetOutDataAnchor(0), node_j->GetInDataAnchor(1));

    GetContext().out_nodes_map["H"] = {0};
    GetContext().out_nodes_map["I"] = {0};
    GetContext().out_nodes_map["J"] = {0};
    graph->TopologicalSorting();
  }

  void MakeStreamGraph(ge::ComputeGraphPtr &graph, const string &type = "some") {
    ge::OpDescPtr op_def_a = CreateOpWithWsSize("A", 6000, type);
    op_def_a->SetStreamId(39);
    ge::OpDescPtr op_def_b = CreateOpWithWsSize("B", 120000);
    op_def_b->SetStreamId(39);
    ge::OpDescPtr op_def_c = CreateOpWithWsSize("C", 16000);
    op_def_c->SetStreamId(44);
    ge::OpDescPtr op_def_d = CreateOpWithWsSize("D", 24000);
    op_def_d->SetStreamId(44);
    ge::OpDescPtr op_def_e = CreateOpWithWsSize("E", 24000);
    op_def_e->SetStreamId(17);
    ge::OpDescPtr op_def_f = CreateOpWithWsSize("F", 30000);
    op_def_f->SetStreamId(6);
    ge::OpDescPtr op_def_g = CreateOpWithWsSize("G", 32000);
    op_def_g->SetStreamId(6);
    ge::OpDescPtr op_def_h = CreateOpWithWsSize("H", 48000);
    op_def_h->SetStreamId(42);

    // add node
    ge::NodePtr node_a = graph->AddNode(op_def_a);
    ge::NodePtr node_b = graph->AddNode(op_def_b);
    ge::NodePtr node_c = graph->AddNode(op_def_c);
    ge::NodePtr node_d = graph->AddNode(op_def_d);
    ge::NodePtr node_e = graph->AddNode(op_def_e);
    ge::NodePtr node_f = graph->AddNode(op_def_f);
    ge::NodePtr node_g = graph->AddNode(op_def_g);
    ge::NodePtr node_h = graph->AddNode(op_def_h);

    // a:39 ----b:39--h:42
    //   \       |   /
    //   c:44   f:6
    //     \     |
    //     d:44 g:6
    //        \  |
    //         e:17
    // add edge
    ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_h->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_f->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_d->GetOutDataAnchor(0), node_e->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_f->GetOutDataAnchor(0), node_g->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_g->GetOutDataAnchor(0), node_e->GetInDataAnchor(0));

    GetContext().out_nodes_map["H"] = {0};
    GetContext().out_nodes_map["I"] = {0};
    GetContext().out_nodes_map["J"] = {0};
    graph->TopologicalSorting();
  }

  void MakeReuseGraph(ge::ComputeGraphPtr graph) {
    ge::OpDescPtr op_def_a = CreateOpWithWsSize("A", 6000);
    ge::OpDescPtr op_def_b = CreateOpWithWsSize("B", 120000);
    ge::OpDescPtr op_def_c = CreateRefOpWithWsSize("C", 120000);
    ge::OpDescPtr op_def_d = std::make_shared<ge::OpDesc>("D", "CONSTANT");

    ge::NodePtr node_a = graph->AddNode(op_def_a);
    ge::NodePtr node_b = graph->AddNode(op_def_b);
    ge::NodePtr node_c = graph->AddNode(op_def_c);
    ge::NodePtr node_d = graph->AddNode(op_def_d);

    ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));
    GetContext().out_nodes_map["B"] = {0};
    GetContext().out_nodes_map["C"] = {0};
    graph->TopologicalSorting();
  }

  ComputeGraphPtr MakeCascadeContinuousMemoryGraph() {
    ge::ut::GraphBuilder builder("graph");
    auto data = builder.AddNode("data", "Data", 1, 1);
    auto addn1 = builder.AddNode("addn1", "AddN", 1, 1);
    auto addn2 = builder.AddNode("addn2", "AddN", 1, 1);
    auto addn3 = builder.AddNode("addn3", "AddN", 1, 1);
    auto concat1 = builder.AddNode("concat1", "Concat", 2, 1);
    auto concat2 = builder.AddNode("concat2", "Concat", 2, 1);
    auto netoutput = builder.AddNode("netoutput", "NetOutput", 2, 0);

    ge::AttrUtils::SetBool(concat1->GetOpDesc(), ATTR_NAME_NOPADDING_CONTINUOUS_INPUT, true);
    ge::AttrUtils::SetBool(concat1->GetOpDesc(), ATTR_NAME_CONTINUOUS_INPUT_ALLOC, true);
    ge::AttrUtils::SetBool(concat1->GetOpDesc(), ATTR_NAME_OUTPUT_REUSE_INPUT, true);

    ge::AttrUtils::SetBool(concat2->GetOpDesc(), ATTR_NAME_NOPADDING_CONTINUOUS_INPUT, true);
    ge::AttrUtils::SetBool(concat2->GetOpDesc(), ATTR_NAME_CONTINUOUS_INPUT_ALLOC, true);
    ge::AttrUtils::SetBool(concat2->GetOpDesc(), ATTR_NAME_OUTPUT_REUSE_INPUT, true);

    addn1->GetOpDesc()->SetOutputOffset({100});
    addn2->GetOpDesc()->SetOutputOffset({200});
    concat1->GetOpDesc()->SetOutputOffset({100});
    addn3->GetOpDesc()->SetOutputOffset({700});
    concat2->GetOpDesc()->SetOutputOffset({500});

    ge::AttrUtils::SetListInt(addn1->GetOpDesc(), ATTR_NAME_OUTPUT_OFFSET_FOR_BUFFER_FUSION, {100});
    ge::AttrUtils::SetListInt(addn2->GetOpDesc(), ATTR_NAME_OUTPUT_OFFSET_FOR_BUFFER_FUSION, {100});
    ge::AttrUtils::SetListInt(addn3->GetOpDesc(), ATTR_NAME_OUTPUT_OFFSET_FOR_BUFFER_FUSION, {100});
    ge::AttrUtils::SetListInt(concat1->GetOpDesc(), ATTR_NAME_OUTPUT_OFFSET_FOR_BUFFER_FUSION, {200});
    ge::AttrUtils::SetListInt(concat2->GetOpDesc(), ATTR_NAME_OUTPUT_OFFSET_FOR_BUFFER_FUSION, {300});


    builder.AddDataEdge(data, 0, addn1, 0);
    builder.AddDataEdge(data, 0, addn2, 0);
    builder.AddDataEdge(addn1, 0, concat1, 0);
    builder.AddDataEdge(addn2, 0, concat1, 1);
    builder.AddDataEdge(concat1, 0, concat2, 0);
    builder.AddDataEdge(addn3, 0, concat2, 1);

    return builder.GetGraph();
  }

  ComputeGraphPtr MakeRefNodeGraph() {
    ge::ut::GraphBuilder builder("graph");
    auto var_input = builder.AddNode("var", "Variable", 1, 1);
    auto const_input = builder.AddNode("const", "Const", 1, 1);
    auto assign = builder.AddNode("assgin", "Assign", 2, 1);
     // add link
    builder.AddDataEdge(var_input, 0, assign, 0);
    builder.AddDataEdge(const_input, 0, assign, 1);
    // set offset
    assign->GetOpDesc()->SetInputOffset({100, 0});
    assign->GetOpDesc()->SetOutputOffset({10000});
    var_input->GetOpDesc()->SetOutputOffset({10000});
    const_input->GetOpDesc()->SetOutputOffset({1000});
    // set mem type
    ge::AttrUtils::SetListInt(assign->GetOpDesc(), ATTR_NAME_INPUT_MEM_TYPE_LIST, {RT_MEMORY_HBM, RT_MEMORY_L1});
    // set ref
    auto output_tensordesc = assign->GetOpDesc()->MutableOutputDesc(0);
    ge::TensorUtils::SetReuseInput(*output_tensordesc, true);
    uint32_t reuse_input_index = 0;
    ge::TensorUtils::SetReuseInputIndex(*output_tensordesc, reuse_input_index);

    return builder.GetGraph();
  }

  ComputeGraphPtr MakeAtomicGraph() {
    ge::ut::GraphBuilder builder("test_graph");
    auto data = builder.AddNode("data", "Data", 1, 1, FORMAT_NCHW, DT_FLOAT, {8, 3, 224, 224});
    auto addn1 = builder.AddNode("addn1", "AddN", 1, 1, FORMAT_NCHW, DT_FLOAT, {1, 3, 224, 224});
    auto addn2 = builder.AddNode("addn2", "AddN", 1, 1, FORMAT_NCHW, DT_FLOAT, {8, 3, 224, 224});
    auto merge = builder.AddNode("merge", "Merge", 2, 1);
    auto netouput = builder.AddNode("netoutput", "NetOutput", 1, 0);
    vector<int64_t> output_list;
    output_list.emplace_back(0);
    auto addn1_output_desc = addn1->GetOpDesc()->GetOutputDesc(0);
    // 1*3*224*224*3
    int64_t addn1_output_size = 602112;
    ge::TensorUtils::SetSize(addn1_output_desc, addn1_output_size);
    addn1->GetOpDesc()->UpdateOutputDesc(0, addn1_output_desc);
    addn1->GetOpDesc()->SetOutputOffset(output_list);

    auto addn2_output_desc = addn2->GetOpDesc()->GetOutputDesc(0);
    //8*3*224*224*3
    int64_t addn2_output_size = 4816896;
    ge::TensorUtils::SetSize(addn2_output_desc, addn2_output_size);
    addn2->GetOpDesc()->UpdateOutputDesc(0, addn2_output_desc);
    addn2->GetOpDesc()->SetOutputOffset(output_list);
    merge->GetOpDesc()->SetOutputOffset(output_list);
    builder.AddDataEdge(data, 0, addn1, 0);
    builder.AddDataEdge(data, 0, addn2, 0);
    builder.AddDataEdge(addn1, 0, merge, 0);
    builder.AddDataEdge(addn2, 0, merge, 1);
    builder.AddDataEdge(merge, 0, netouput, 0);

    vector<int> atomic_output_index = {0};
    (void) ge::AttrUtils::SetBool(addn1->GetOpDesc(), ATOMIC_ATTR_IS_ATOMIC_NODE, true);
    (void) ge::AttrUtils::SetListInt(addn1->GetOpDesc(), ATOMIC_ATTR_OUTPUT_INDEX, atomic_output_index);
    auto atomic_clean = builder.AddNode("atomic_clean", "AtomicAddrClean", 0, 0);
    builder.AddControlEdge(atomic_clean, addn1);
    auto graph = builder.GetGraph();
    graph->SetInputSize(1);
    graph->SetInputsOrder({"data"});
    graph->AddInputNode(data);
    return graph;
  }

  ComputeGraphPtr MakeAtomicAndRefGraph() {
    ge::ut::GraphBuilder builder("test_graph");
    auto data = builder.AddNode("data", "Data", 1, 1, FORMAT_NCHW, DT_FLOAT, {8, 3, 224, 224});
    auto addn1 = builder.AddNode("addn1", "AddN", 1, 1, FORMAT_NCHW, DT_FLOAT, {1, 3, 224, 224});
    auto addn2 = builder.AddNode("addn2", "AddN", 1, 1, FORMAT_NCHW, DT_FLOAT, {8, 3, 224, 224});
    auto merge = builder.AddNode("merge", "Merge", 2, 1);
    auto netouput = builder.AddNode("netoutput", "NetOutput", 1, 0);
    vector<int64_t> output_list;
    output_list.emplace_back(0);
    auto addn1_output_desc = addn1->GetOpDesc()->GetOutputDesc(0);
    // 1*3*224*224*3
    int64_t addn1_output_size = 602112;
    ge::TensorUtils::SetSize(addn1_output_desc, addn1_output_size);
    addn1->GetOpDesc()->UpdateOutputDesc(0, addn1_output_desc);
    addn1->GetOpDesc()->SetOutputOffset(output_list);

    auto addn2_output_desc = addn2->GetOpDesc()->GetOutputDesc(0);
    //8*3*224*224*3
    int64_t addn2_output_size = 4816896;
    ge::TensorUtils::SetSize(addn2_output_desc, addn2_output_size);
    addn2->GetOpDesc()->UpdateOutputDesc(0, addn2_output_desc);
    addn2->GetOpDesc()->SetOutputOffset(output_list);
    merge->GetOpDesc()->SetOutputOffset(output_list);
    builder.AddDataEdge(data, 0, addn1, 0);
    builder.AddDataEdge(data, 0, addn2, 0);
    builder.AddDataEdge(addn1, 0, merge, 0);
    builder.AddDataEdge(addn2, 0, merge, 1);
    builder.AddDataEdge(merge, 0, netouput, 0);

    vector<int> atomic_output_index = {0};
    (void) ge::AttrUtils::SetBool(addn1->GetOpDesc(), ATOMIC_ATTR_IS_ATOMIC_NODE, true);
    (void) ge::AttrUtils::SetBool(addn1->GetOpDesc(), ATTR_NAME_REFERENCE, true);
    (void) ge::AttrUtils::SetListInt(addn1->GetOpDesc(), ATOMIC_ATTR_OUTPUT_INDEX, atomic_output_index);
    auto atomic_clean = builder.AddNode("atomic_clean", "AtomicAddrClean", 0, 0);
    builder.AddControlEdge(atomic_clean, addn1);
    auto graph = builder.GetGraph();
    graph->SetInputSize(1);
    graph->SetInputsOrder({"data"});
    graph->AddInputNode(data);
    return graph;
  }

  void MakeFftsReuseGraph(ge::ComputeGraphPtr graph, int32_t thread_scope_id_1 = kInvalidThreadScopeId,
                            int32_t thread_scope_id_2 = kInvalidThreadScopeId) {
    ge::OpDescPtr op_def_a = CreateOpWithWsSize("A", 512);
    ge::OpDescPtr op_def_b = CreateOpWithWsSize("B", 0);
    ge::OpDescPtr op_def_c = CreateOpWithWsSize("C", 512);
    ge::OpDescPtr op_def_d = CreateOpWithWsSize("D", 512);
    ge::OpDescPtr op_def_e = CreateOpWithWsSize("E", 0);
    ge::OpDescPtr op_def_f = CreateOpWithWsSize("F", 512, "some", 2048UL);
    ge::OpDescPtr op_def_g = CreateOpWithWsSize("G", 0);

    if (thread_scope_id_1 != kInvalidThreadScopeId) {
      (void)ge::AttrUtils::SetInt(op_def_a, ATTR_NAME_THREAD_SCOPE_ID, thread_scope_id_1);
      (void)ge::AttrUtils::SetInt(op_def_b, ATTR_NAME_THREAD_SCOPE_ID, thread_scope_id_1);
      (void)ge::AttrUtils::SetInt(op_def_c, ATTR_NAME_THREAD_SCOPE_ID, thread_scope_id_1);
    }

    if (thread_scope_id_2 != kInvalidThreadScopeId) {
      (void)ge::AttrUtils::SetInt(op_def_d, ATTR_NAME_THREAD_SCOPE_ID, thread_scope_id_2);
      (void)ge::AttrUtils::SetInt(op_def_e, ATTR_NAME_THREAD_SCOPE_ID, thread_scope_id_2);
      (void)ge::AttrUtils::SetInt(op_def_f, ATTR_NAME_THREAD_SCOPE_ID, thread_scope_id_2);
    }

    ge::NodePtr node_a = graph->AddNode(op_def_a);
    ge::NodePtr node_b = graph->AddNode(op_def_b);
    ge::NodePtr node_c = graph->AddNode(op_def_c);
    ge::NodePtr node_d = graph->AddNode(op_def_d);
    ge::NodePtr node_e = graph->AddNode(op_def_e);
    ge::NodePtr node_f = graph->AddNode(op_def_f);
    ge::NodePtr node_g = graph->AddNode(op_def_g);

    ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_d->GetOutDataAnchor(0), node_e->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_e->GetOutDataAnchor(0), node_f->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_f->GetOutDataAnchor(0), node_g->GetInDataAnchor(0));
    graph->TopologicalSorting();
  }

  void MakeSessionScopeReuseGraph(ge::ComputeGraphPtr graph) {
    ge::OpDescPtr op_def_a = CreateOpWithWsSize("A", 512);
    ge::OpDescPtr op_def_b = CreateOpWithWsSize("B", 0);
    ge::OpDescPtr op_def_c = CreateOpWithWsSize("C", 512);
    ge::OpDescPtr op_def_d = CreateOpWithWsSize("D", 512);
    ge::OpDescPtr op_def_e = CreateOpWithWsSize("E", 1024);
    ge::OpDescPtr op_def_f = CreateOpWithWsSize("F", 512, "some", 2048UL);
    ge::OpDescPtr op_def_g = CreateOpWithWsSize("G", 0);

    std::vector<int64_t> workspace_bytes;
    workspace_bytes.push_back(1024);
    workspace_bytes.push_back(512);
    op_def_c->SetWorkspaceBytes(workspace_bytes);
    vector<int32_t> workspace_no_reuse_scope = { 0 , 1 };
    (void)ge::AttrUtils::SetListInt(op_def_c, ATTR_NAME_WORKSPACE_MEMORY_NO_REUSE_SCOPE, workspace_no_reuse_scope);

    vector<int32_t> workspace_no_reuse_scope_e = { 1 };
    (void)ge::AttrUtils::SetListInt(op_def_e, ATTR_NAME_WORKSPACE_MEMORY_NO_REUSE_SCOPE, workspace_no_reuse_scope_e);

    ge::NodePtr node_a = graph->AddNode(op_def_a);
    ge::NodePtr node_b = graph->AddNode(op_def_b);
    ge::NodePtr node_c = graph->AddNode(op_def_c);
    ge::NodePtr node_d = graph->AddNode(op_def_d);
    ge::NodePtr node_e = graph->AddNode(op_def_e);
    ge::NodePtr node_f = graph->AddNode(op_def_f);
    ge::NodePtr node_g = graph->AddNode(op_def_g);

    ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_d->GetOutDataAnchor(0), node_e->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_e->GetOutDataAnchor(0), node_f->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_f->GetOutDataAnchor(0), node_g->GetInDataAnchor(0));
    graph->TopologicalSorting();
  }

 void MakeContinuousReuseGraph(ge::ComputeGraphPtr graph, bool nopading = false) {
    ge::OpDescPtr op_def_a = CreateOpWithWsSize("A", 512);
    ge::OpDescPtr op_def_b = CreateOpWithWsSize("B", 0);
    ge::OpDescPtr op_def_c = CreateOpWithWsSize("C", 512);
    ge::OpDescPtr op_def_d = CreateOpWithWsSize("D", 512);
    ge::OpDescPtr op_def_e = CreateOpWithWsSize("E", 1024);
    ge::OpDescPtr op_def_f = CreateOpWithWsSize("F", 512, "some", 2048UL);
    ge::OpDescPtr op_def_g = CreateOpWithWsSize("G", 0);

    if (nopading) {
      (void)ge::AttrUtils::SetBool(op_def_d, ATTR_NAME_NOPADDING_CONTINUOUS_INPUT, true);
      (void)ge::AttrUtils::SetBool(op_def_d, ATTR_NAME_NOPADDING_CONTINUOUS_OUTPUT, true);
      (void)ge::AttrUtils::SetBool(op_def_d, ATTR_NAME_OUTPUT_REUSE_INPUT, true);
      (void)ge::AttrUtils::SetInt(op_def_d, ATTR_NAME_REUSE_INPUT_ON_DIM_INDEX, 0);
    } else {
      (void)ge::AttrUtils::SetBool(op_def_d, ATTR_NAME_CONTINUOUS_INPUT, true);
      (void)ge::AttrUtils::SetBool(op_def_d, ATTR_NAME_CONTINUOUS_OUTPUT, true);
    }

    ge::NodePtr node_a = graph->AddNode(op_def_a);
    ge::NodePtr node_b = graph->AddNode(op_def_b);
    ge::NodePtr node_c = graph->AddNode(op_def_c);
    ge::NodePtr node_d = graph->AddNode(op_def_d);
    ge::NodePtr node_e = graph->AddNode(op_def_e);
    ge::NodePtr node_f = graph->AddNode(op_def_f);
    ge::NodePtr node_g = graph->AddNode(op_def_g);

    ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_d->GetOutDataAnchor(0), node_e->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_d->GetOutDataAnchor(0), node_f->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_d->GetOutDataAnchor(0), node_g->GetInDataAnchor(0));
    graph->TopologicalSorting();
  }

  void MakeMultiBatchReuseGraph(ge::ComputeGraphPtr graph) {
    ge::OpDescPtr op_def_a = CreateOpWithWsSize("A", 512);
    ge::OpDescPtr op_def_b = CreateOpWithWsSize("B", 0);
    ge::OpDescPtr op_def_c = CreateOpWithWsSize("C", 512);
    ge::OpDescPtr op_def_d = CreateOpWithWsSize("D", 512);
    ge::OpDescPtr op_def_e = CreateOpWithWsSize("E", 1024);
    ge::OpDescPtr op_def_f = CreateOpWithWsSize("F", 512, "some", 2048UL);
    ge::OpDescPtr op_def_g = CreateOpWithWsSize("G", 0);

   (void)ge::AttrUtils::SetStr(op_def_b, ATTR_NAME_BATCH_LABEL, "Batch_0");
   (void)ge::AttrUtils::SetStr(op_def_c, ATTR_NAME_BATCH_LABEL, "Batch_0");
   (void)ge::AttrUtils::SetStr(op_def_e, ATTR_NAME_BATCH_LABEL, "Batch_1");
   (void)ge::AttrUtils::SetStr(op_def_f, ATTR_NAME_BATCH_LABEL, "Batch_1");
   vector<int32_t> workspace_no_reuse_scope = { 1 };
   (void)ge::AttrUtils::SetListInt(op_def_c, ATTR_NAME_WORKSPACE_MEMORY_NO_REUSE_SCOPE, workspace_no_reuse_scope);
   (void)ge::AttrUtils::SetListInt(op_def_e, ATTR_NAME_WORKSPACE_MEMORY_NO_REUSE_SCOPE, workspace_no_reuse_scope);

    ge::NodePtr node_a = graph->AddNode(op_def_a);
    ge::NodePtr node_b = graph->AddNode(op_def_b);
    ge::NodePtr node_c = graph->AddNode(op_def_c);
    ge::NodePtr node_d = graph->AddNode(op_def_d);
    ge::NodePtr node_e = graph->AddNode(op_def_e);
    ge::NodePtr node_f = graph->AddNode(op_def_f);
    ge::NodePtr node_g = graph->AddNode(op_def_g);

    ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_e->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_e->GetOutDataAnchor(0), node_f->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_f->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(node_d->GetOutDataAnchor(0), node_g->GetInDataAnchor(0));
    graph->TopologicalSorting();
  }

 protected:
  void SetUp() {}

  void TearDown() { GetContext().out_nodes_map.clear(); }
};

namespace ge {

class MockBlockMemAssigner : public BlockMemAssigner {
 public:
  explicit MockBlockMemAssigner(ge::ComputeGraphPtr compute_graph, const std::map<std::string, std::string> &anchor_to_symbol, const std::map<std::string, std::list<NodeIndexIO>> &symbol_to_anchors) : BlockMemAssigner(compute_graph, anchor_to_symbol, symbol_to_anchors) {};

  virtual ~MockBlockMemAssigner(){};

  Status GetMemoryRanges(std::vector<int64_t> &ranges) override { return FAILED; }
};
}  // namespace ge

// when check GetMemoryRanges return fail, Assign return fail
TEST_F(UtestMemoryAssignerTest, Mock_block_mem_assigner_failed) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  MakeGraph(graph);
  std::map<std::string, std::string> anchor_to_symbol;
  std::map<std::string, std::list<NodeIndexIO>> symbol_to_anchors;
  EXPECT_EQ(GraphUtils::GetRefMapping(graph, symbol_to_anchors, anchor_to_symbol), GRAPH_SUCCESS);

  MockBlockMemAssigner mock_assigner(graph, anchor_to_symbol, symbol_to_anchors);
  EXPECT_EQ(mock_assigner.Assign(), FAILED);
}

TEST_F(UtestMemoryAssignerTest, graph_memory_assign_continuous_input) {
  ge::ComputeGraphPtr graph = MakeCascadeContinuousMemoryGraph();
  auto addn1 = graph->FindNode("addn1");
  auto addn2 = graph->FindNode("addn2");
  EXPECT_EQ(addn1->GetOpDesc()->GetOutputOffset()[0], 100);
  EXPECT_EQ(addn2->GetOpDesc()->GetOutputOffset()[0], 200);
  GraphMemoryAssigner memoryAssigner(graph);
  MemoryOffset memory_offset(RT_MEMORY_HBM, 0);
  memoryAssigner.memory_offset_.emplace(RT_MEMORY_HBM, memory_offset);
  EXPECT_EQ(memoryAssigner.ReAssignContinuousMemory(), GRAPH_SUCCESS);
  EXPECT_EQ(addn1->GetOpDesc()->GetOutputOffset()[0], 500);
  EXPECT_EQ(addn2->GetOpDesc()->GetOutputOffset()[0], 600);
}

TEST_F(UtestMemoryAssignerTest, block_memory_assign_nopading_continuous_memory) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  MakeContinuousReuseGraph(graph, true);
  HybridMemAssigner hybridMemAssigner(graph);
  ge::Status ret = hybridMemAssigner.Assign();
  size_t offset = 0;
  auto it = hybridMemAssigner.GetMemOffsets().find(RT_MEMORY_HBM);
  if (it != hybridMemAssigner.GetMemOffsets().end()) {
    offset = it->second;
  }

  EXPECT_EQ(offset, 8192);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestMemoryAssignerTest, block_memory_assign_nopading_continuous_memory_fail) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  MakeContinuousReuseGraph(graph, true);
  auto op_d = graph->FindNode("D");
  (void)ge::AttrUtils::SetInt(op_d->GetOpDesc(), ATTR_NAME_REUSE_INPUT_ON_DIM_INDEX, 3);
  GraphMemoryAssigner memoryAssigner(graph);
  MemoryOffset memory_offset(RT_MEMORY_HBM, 0);
  memoryAssigner.memory_offset_.emplace(RT_MEMORY_HBM, memory_offset);
  EXPECT_EQ(memoryAssigner.ReAssignContinuousMemory(), FAILED);
}

TEST_F(UtestMemoryAssignerTest, block_memory_assign_continuous_memory) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  MakeContinuousReuseGraph(graph);
  map<uint64_t, size_t> mem_offset;
  size_t zero_copy_mem_size = 0;
  MemoryAssigner memoryAssigner(graph);
  ge::Status ret = memoryAssigner.AssignMemory(mem_offset, zero_copy_mem_size);
  size_t offset = 0;
  auto it = mem_offset.find(RT_MEMORY_HBM);
  if (it != mem_offset.end()) {
    offset = it->second;
  }

  EXPECT_EQ(offset, 11264);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestMemoryAssignerTest, block_memory_assign_with_release_first_reuse_first_strategy) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  MakeGraph(graph);
  (void)AttrUtils::SetBool(graph, "_mem_release_first_reuse_first", true);
  map<uint64_t, size_t> mem_offset;
  size_t zero_copy_mem_size = 0;
  MemoryAssigner memoryAssigner(graph);
  EXPECT_EQ(memoryAssigner.AssignMemory(mem_offset, zero_copy_mem_size), SUCCESS);
}

TEST_F(UtestMemoryAssignerTest, graph_memory_set_last_used_attr) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  MakeGraph(graph);
  auto node_f = graph->FindNode("F");
  MemoryAssigner memory_assigner(graph);
  map<uint64_t, size_t> mem_offset;
  size_t zero_memory_size = 0;
  EXPECT_EQ(memory_assigner.AssignMemory(mem_offset, zero_memory_size), GRAPH_SUCCESS);

  bool flag = 0;
  (void) ge::AttrUtils::GetBool(node_f->GetOpDesc()->GetInputDesc(0), ATTR_NAME_IS_END_OF_INPUTMEM_LIFECYCLE, flag);
  EXPECT_EQ(flag, true);
}

TEST_F(UtestMemoryAssignerTest, graph_memory_assign_ref_var) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  MakeGraph(graph, VARIABLE);
  auto node_a = graph->FindNode("A");
  auto node_b = graph->FindNode("B");
  std::string value = "A";
  (void) ge::AttrUtils::SetStr(node_b->GetOpDesc()->MutableOutputDesc(0), REF_VAR_SRC_VAR_NAME, value);
  MemoryAssigner memory_assigner(graph);
  map<uint64_t, size_t> mem_offset;
  size_t zero_memory_size = 0;
  VarManager::Instance(0)->Init(0, 0, 0, 0);
  EXPECT_EQ(memory_assigner.AssignMemory(mem_offset, zero_memory_size), GRAPH_SUCCESS);

  EXPECT_EQ(node_b->GetOpDesc()->GetOutputOffset()[0], node_a->GetOpDesc()->GetOutputOffset()[0]);
}

TEST_F(UtestMemoryAssignerTest, graph_memory_assign_ref_var_not_found) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  MakeGraph(graph, VARIABLE);

  ge::ComputeGraphPtr sub_graph = std::make_shared<ge::ComputeGraph>("");
  MakeReuseGraph(sub_graph);
  graph->AddSubGraph(sub_graph);

  auto node_a = graph->FindNode("A");
  auto node_b = graph->FindNode("B");
  std::string value = "M";
  (void) ge::AttrUtils::SetStr(node_b->GetOpDesc()->MutableOutputDesc(0), REF_VAR_SRC_VAR_NAME, value);
  MemoryAssigner memory_assigner(graph);
  map<uint64_t, size_t> mem_offset;
  size_t zero_memory_size = 0;
  VarManager::Instance(0)->Init(0, 0, 0, 0);
  EXPECT_NE(memory_assigner.AssignMemory(mem_offset, zero_memory_size), GRAPH_SUCCESS);
}

TEST_F(UtestMemoryAssignerTest, graph_memory_assign_set_input_offset) {
  ge::ComputeGraphPtr graph = MakeRefNodeGraph();
  auto assgin = graph->FindNode("assgin");
  EXPECT_EQ(assgin->GetOpDesc()->GetOutputOffset()[0], 10000);
  EXPECT_EQ(assgin->GetOpDesc()->GetInputOffset()[0], 100);
  EXPECT_EQ(assgin->GetOpDesc()->GetInputOffset()[1], 0);
  GraphMemoryAssigner memoryAssigner(graph);
  MemoryOffset memory_offset(RT_MEMORY_HBM, 0);
  memoryAssigner.memory_offset_.emplace(RT_MEMORY_HBM, memory_offset);
  EXPECT_EQ(memoryAssigner.SetInputOffset(), GRAPH_SUCCESS);
  EXPECT_EQ(assgin->GetOpDesc()->GetOutputOffset()[0], 10100);
  EXPECT_EQ(assgin->GetOpDesc()->GetInputOffset()[0], 10100);
  EXPECT_EQ(assgin->GetOpDesc()->GetInputOffset()[1], 0);
  EXPECT_EQ(memoryAssigner.CheckOffset(), GRAPH_SUCCESS);
}

TEST_F(UtestMemoryAssignerTest, graph_memory_assign_check_inner_offset) {
  ge::ComputeGraphPtr graph = MakeRefNodeGraph();
  auto assign = graph->FindNode("assgin");
  auto op_desc = assign->GetOpDesc();
  int64_t inner_offset = 0;
  EXPECT_EQ(ge::AttrUtils::GetInt(op_desc->MutableInputDesc(0), ATTR_NAME_INNER_OFFSET, inner_offset), false);
  EXPECT_EQ(ge::AttrUtils::GetInt(op_desc->MutableInputDesc(1), ATTR_NAME_INNER_OFFSET, inner_offset), false);
  GraphMemoryAssigner memoryAssigner(graph);
  MemoryOffset memory_offset(RT_MEMORY_HBM, 0);
  memoryAssigner.memory_offset_.emplace(RT_MEMORY_HBM, memory_offset);
  EXPECT_EQ(memoryAssigner.SetInputOffset(), GRAPH_SUCCESS);
  EXPECT_EQ(ge::AttrUtils::GetInt(op_desc->MutableInputDesc(0), ATTR_NAME_INNER_OFFSET, inner_offset), true);
  EXPECT_EQ(inner_offset, 100);
  EXPECT_EQ(ge::AttrUtils::GetInt(op_desc->MutableOutputDesc(0), ATTR_NAME_INNER_OFFSET, inner_offset), true);
  EXPECT_EQ(inner_offset, 100);
  EXPECT_EQ(ge::AttrUtils::GetInt(op_desc->MutableInputDesc(1), ATTR_NAME_INNER_OFFSET, inner_offset), false);
}

TEST_F(UtestMemoryAssignerTest, graph_memory_assign_update_ref_op_offset_reverse) {
    ge::ut::GraphBuilder builder("graph");
    auto data_input = builder.AddNode("data", "Data", 1, 1);
    auto const_input = builder.AddNode("const", "Const", 1, 1);
    auto add = builder.AddNode("add", "Add", 2, 1);
     // add link
    builder.AddDataEdge(data_input, 0, add, 0);
    builder.AddDataEdge(const_input, 0, add, 1);
    // set ref
    uint32_t reuse_input_index = 0;
    auto output_tensordesc = data_input->GetOpDesc()->MutableOutputDesc(0);
    ge::TensorUtils::SetReuseInput(*output_tensordesc, true);
    ge::TensorUtils::SetReuseInputIndex(*output_tensordesc, reuse_input_index);
    auto output_tensordesc1 = add->GetOpDesc()->MutableOutputDesc(0);
    ge::TensorUtils::SetReuseInput(*output_tensordesc1, true);
    ge::TensorUtils::SetReuseInputIndex(*output_tensordesc1, reuse_input_index);
    ge::ComputeGraphPtr graph = builder.GetGraph();

    GraphMemoryAssigner memoryAssigner(graph);
    EXPECT_EQ(memoryAssigner.UpdateRefOpOffsetReverse(add), SUCCESS);
}

TEST_F(UtestMemoryAssignerTest, graph_memory_assign_var_input_ref_cascade_false) {
  ge::ut::GraphBuilder builder("graph");
  auto var = builder.AddNode("var", VARIABLE, 1, 1);
  auto broadcast = builder.AddNode("broadcast", HCOMBROADCAST, 1, 1);
  auto assign = builder.AddNode("assign", "Assign", 2, 1);
  // add link
  builder.AddDataEdge(var, 0, assign, 0);
  builder.AddDataEdge(var, 0, broadcast, 0);
  builder.AddDataEdge(broadcast, 0, assign, 1);

  int reuse_input_index = 0;
  auto broadcast_desc = broadcast->GetOpDesc()->MutableOutputDesc(0);
  ge::TensorUtils::SetReuseInput(*broadcast_desc, true);
  ge::TensorUtils::SetReuseInputIndex(*broadcast_desc, reuse_input_index);

  ge::ComputeGraphPtr graph = builder.GetGraph();

  GraphMemoryAssigner memory_assigner(graph);
  bool ref_cascade = memory_assigner.IsRefFromInputOpCascade(broadcast);
  EXPECT_EQ(ref_cascade, false);
  ref_cascade = memory_assigner.IsRefFromInputOpCascade(assign);
  EXPECT_EQ(ref_cascade, false);
  auto ret = memory_assigner.UpdateRefOpOffsetReverse(broadcast);
  EXPECT_EQ(ret, SUCCESS);
  ret = memory_assigner.UpdateRefOpOffsetReverse(assign);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestMemoryAssignerTest, graph_memory_assign_atomic_output_and_workspace) {
  ge::ut::GraphBuilder builder("graph");
  auto data_input = builder.AddNode("data", "Data", 1, 1);
  auto const_input = builder.AddNode("const", "Const", 1, 1);
  auto add = builder.AddNode("add", "Add", 2, 1);
  // add link
  builder.AddDataEdge(data_input, 0, add, 0);
  builder.AddDataEdge(const_input, 0, add, 1);
  ge::ComputeGraphPtr graph = builder.GetGraph();

  auto node = graph->FindNode("add");
  EXPECT_NE(node, nullptr);
  auto output_tensor_desc = node->GetOpDesc()->MutableOutputDesc(0);
  ge::TensorUtils::SetSize(*output_tensor_desc, 100);
  vector<int64_t> output_list = {0};
  node->GetOpDesc()->SetOutputOffset(output_list);
  vector<int64_t> workspace_list = {0};
  node->GetOpDesc()->SetWorkspace(workspace_list);
  vector<int64_t> atomic_output_index = {0};
  bool set_attr = ge::AttrUtils::SetListInt(node->GetOpDesc(), ATOMIC_ATTR_OUTPUT_INDEX, atomic_output_index);
  EXPECT_EQ(set_attr, true);

  map<string, map<int64_t, int64_t>> workspace_info;
  workspace_info["add"][0] = 100;
  set_attr = node->GetOpDesc()->SetExtAttr(EXT_ATTR_ATOMIC_WORKSPACE_INFO, workspace_info);
  EXPECT_EQ(set_attr, true);

  {
    bool is_fusion_node = false;
    set_attr = ge::AttrUtils::SetBool(node->GetOpDesc(), ATOMIC_ATTR_IS_FUSION_NODE, is_fusion_node);
    EXPECT_EQ(set_attr, true);

    GraphMemoryAssigner graph_memory_assigner(graph);
    graph_memory_assigner.memory_offset_.insert({RT_MEMORY_HBM, MemoryOffset(RT_MEMORY_HBM, 0)});
    vector<int64_t> mem_offset_end;
    Status ret = graph_memory_assigner.AssignAtomicOutputAndWorkspaceMemory(node, mem_offset_end);
    EXPECT_EQ(ret, SUCCESS);
    EXPECT_EQ(mem_offset_end.size(), 2);
    MemoryOffset mem_offset = graph_memory_assigner.memory_offset_.at(RT_MEMORY_HBM);
    EXPECT_EQ(mem_offset.mem_offset_, 1024);
  }

  {
    bool is_fusion_node = true;
    set_attr = ge::AttrUtils::SetBool(node->GetOpDesc(), ATOMIC_ATTR_IS_FUSION_NODE, is_fusion_node);
    EXPECT_EQ(set_attr, true);

    GraphMemoryAssigner graph_memory_assigner(graph);
    graph_memory_assigner.memory_offset_.insert({RT_MEMORY_HBM, MemoryOffset(RT_MEMORY_HBM, 0)});
    vector<int64_t> mem_offset_end;
    Status ret = graph_memory_assigner.AssignAtomicOutputAndWorkspaceMemory(node, mem_offset_end);
    EXPECT_EQ(ret, SUCCESS);
    EXPECT_EQ(mem_offset_end.size(), 2);
    MemoryOffset mem_offset = graph_memory_assigner.memory_offset_.at(RT_MEMORY_HBM);
    EXPECT_EQ(mem_offset.mem_offset_, 1024);
  }
}

TEST_F(UtestMemoryAssignerTest, Mock_ffts_reuse_no_functinon_op) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  MakeFftsReuseGraph(graph, kInvalidThreadScopeId, kInvalidThreadScopeId);
  HybridMemAssigner hybridMemAssigner(graph);
  ge::Status ret = hybridMemAssigner.Assign();
  size_t offset = 0;
  auto it = hybridMemAssigner.GetMemOffsets().find(RT_MEMORY_HBM);
  if (it != hybridMemAssigner.GetMemOffsets().end()) {
    offset = it->second;
  }
  EXPECT_EQ(offset, 5120);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestMemoryAssignerTest, Mock_ffts_reuse_two_functinon_op) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  MakeFftsReuseGraph(graph, 0, 1);
  HybridMemAssigner hybridMemAssigner(graph);
  ge::Status ret = hybridMemAssigner.Assign();
  size_t offset = 0;
  auto it = hybridMemAssigner.GetMemOffsets().find(RT_MEMORY_HBM);
  if (it != hybridMemAssigner.GetMemOffsets().end()) {
    offset = it->second;
  }
  EXPECT_EQ(offset, 6656);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestMemoryAssignerTest, Mock_ffts_reuse_one_functinon_op) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  MakeFftsReuseGraph(graph, 0, kInvalidThreadScopeId);
  HybridMemAssigner hybridMemAssigner(graph);
  ge::Status ret = hybridMemAssigner.Assign();
  size_t offset = 0;
  auto it = hybridMemAssigner.GetMemOffsets().find(RT_MEMORY_HBM);
  if (it != hybridMemAssigner.GetMemOffsets().end()) {
    offset = it->second;
  }
  EXPECT_EQ(offset, 5632);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestMemoryAssignerTest, one_session_scope_op) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  MakeSessionScopeReuseGraph(graph);
  HybridMemAssigner hybridMemAssigner(graph);
  ge::Status ret = hybridMemAssigner.Assign();
  size_t offset = 0;
  auto it = hybridMemAssigner.GetMemOffsets().find(RT_MEMORY_HBM);
  if (it != hybridMemAssigner.GetMemOffsets().end()) {
    offset = it->second;
  }

  auto mem_type_session_scope = (kSessionScopeMemory | RT_MEMORY_HBM);
  size_t session_scope_offset = 0;
  it = hybridMemAssigner.GetMemOffsets().find(mem_type_session_scope);
  if (it != hybridMemAssigner.GetMemOffsets().end()) {
    session_scope_offset = it->second;
  }
  EXPECT_EQ(offset, 5120);
  EXPECT_EQ(session_scope_offset, 1536);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestMemoryAssignerTest, multi_batch_reuse) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  MakeMultiBatchReuseGraph(graph);
  HybridMemAssigner hybridMemAssigner(graph);
  ge::Status ret = hybridMemAssigner.Assign();
  size_t offset = 0;
  auto it = hybridMemAssigner.GetMemOffsets().find(RT_MEMORY_HBM);
  if (it != hybridMemAssigner.GetMemOffsets().end()) {
    offset = it->second;
  }

  auto mem_type_session_scope = (kSessionScopeMemory | RT_MEMORY_HBM);
  size_t session_scope_offset = 0;
  it = hybridMemAssigner.GetMemOffsets().find(mem_type_session_scope);
  if (it != hybridMemAssigner.GetMemOffsets().end()) {
    session_scope_offset = it->second;
  }
  EXPECT_EQ(offset, 6656);
  EXPECT_EQ(session_scope_offset, 1536);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestMemoryAssignerTest, graph_memory_assign_atomic_node_assign_success) {
  ge::ComputeGraphPtr graph = MakeAtomicGraph();
  GraphMemoryAssigner memoryAssigner(graph);
  MemoryOffset memory_offset(RT_MEMORY_HBM, 0);
  memoryAssigner.memory_offset_.emplace(RT_MEMORY_HBM, memory_offset);
  map<string, map<NodePtr, vector<NodePtr>>> normal_atomic_nodes_map;
  map<string, vector<NodePtr>> connecting_output_atomic_nodes;
  EXPECT_EQ(memoryAssigner.FilterAtomicNodesForMemoryAssign(normal_atomic_nodes_map, connecting_output_atomic_nodes),
            SUCCESS);
}

TEST_F(UtestMemoryAssignerTest, graph_memory_assign_atomic_ref_node_assign_success) {
  ge::ComputeGraphPtr graph = MakeAtomicAndRefGraph();
  GraphMemoryAssigner memoryAssigner(graph);
  MemoryOffset memory_offset(RT_MEMORY_HBM, 0);
  memoryAssigner.memory_offset_.emplace(RT_MEMORY_HBM, memory_offset);
  map<string, map<NodePtr, vector<NodePtr>>> normal_atomic_nodes_map;
  map<string, vector<NodePtr>> connecting_output_atomic_nodes;
  EXPECT_EQ(memoryAssigner.FilterAtomicNodesForMemoryAssign(normal_atomic_nodes_map, connecting_output_atomic_nodes),
            SUCCESS);
}

TEST_F(UtestMemoryAssignerTest, graph_memory_assign_atomic_ref_node_assign_failed) {
  ge::ComputeGraphPtr graph = MakeAtomicAndRefGraph();
  auto addn1 = graph->FindNode("addn1");
  vector<int> atomic_out_index = {0, 1};
  ge::AttrUtils::SetListInt(addn1->GetOpDesc(), ATOMIC_ATTR_OUTPUT_INDEX, atomic_out_index);
  GraphMemoryAssigner memoryAssigner(graph);
  MemoryOffset memory_offset(RT_MEMORY_HBM, 0);
  memoryAssigner.memory_offset_.emplace(RT_MEMORY_HBM, memory_offset);
  map<string, map<NodePtr, vector<NodePtr>>> normal_atomic_nodes_map;
  map<string, vector<NodePtr>> connecting_output_atomic_nodes;
  EXPECT_EQ(memoryAssigner.FilterAtomicNodesForMemoryAssign(normal_atomic_nodes_map, connecting_output_atomic_nodes),
            PARAM_INVALID);
}

TEST_F(UtestMemoryAssignerTest, test_no_tiling_mem_assign) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  const map<string, string> anchor_to_symbol;
  const map<string, list<NodeIndexIO>> symbol_to_anchors;
  MockBlockMemAssigner block_mem_assigner(graph, anchor_to_symbol, symbol_to_anchors);
  
  OpDescPtr op_desc = std::make_shared<OpDesc>("where", "Where");
  GeTensorDesc tensor(GeShape(), FORMAT_ND, DT_INT64);
  op_desc->AddOutputDesc(tensor);
  NodePtr node = graph->AddNode(op_desc);
  vector<int64_t> ranges;
  auto mem_block = block_mem_assigner.ApplyOutDescMemory(node, 0, ranges);
  EXPECT_EQ(mem_block != nullptr, true);
}

TEST_F(UtestMemoryAssignerTest, test_no_tiling_mem_assign_ref) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");

  OpDescPtr op_desc = std::make_shared<OpDesc>("where", "Where");
  GeTensorDesc tensor(GeShape(), FORMAT_ND, DT_INT64);
  op_desc->AddOutputDesc(tensor);
  NodePtr node = graph->AddNode(op_desc);
  vector<int64_t> ranges;

  NodeIndexIO node_index_io(node, 0, kOut);
  std::string symbol = "where";
  const map<string, string> anchor_to_symbol = {std::make_pair(node_index_io.ToString(), symbol)};
  const map<string, list<NodeIndexIO>> symbol_to_anchors;
  MockBlockMemAssigner block_mem_assigner(graph, anchor_to_symbol, symbol_to_anchors);
  
  MemoryBlock *block = new MemoryBlock(512);
  block_mem_assigner.symbol_desc_blocks_[symbol] = block;
  auto mem_block = block_mem_assigner.ApplyOutDescMemory(node, 0, ranges);
  EXPECT_EQ(mem_block != nullptr, true);
}

TEST_F(UtestMemoryAssignerTest, test_input_desc_offset_update) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  GraphMemoryAssigner graph_mem_assigner(graph);

  OpDescPtr where_desc = std::make_shared<OpDesc>("where", WHERE);
  GeTensorDescPtr where_tensor = std::make_shared<GeTensorDesc>();
  ge::AttrUtils::SetInt(where_tensor, ATTR_NAME_TENSOR_DESC_MEM_OFFSET, 1024);
  where_desc->AddOutputDesc(where_tensor->Clone());
  NodePtr where = graph->AddNode(where_desc);

  OpDescPtr where1_desc = std::make_shared<OpDesc>("where", WHERE);
  GeTensorDescPtr where1_tensor = std::make_shared<GeTensorDesc>();
  ge::AttrUtils::SetBool(where1_tensor, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, true);
  where1_desc->AddInputDesc(where1_tensor->Clone());
  NodePtr where1 = graph->AddNode(where1_desc);

  GraphUtils::AddEdge(where->GetOutDataAnchor(0), where1->GetInDataAnchor(0));

  EXPECT_EQ(graph_mem_assigner.UpdateOpInputDescOffset(where1), SUCCESS);

  uint32_t offset = 0;
  auto tensor = where1_desc->GetInputDescPtr(0);
  ge::AttrUtils::GetInt(tensor, ATTR_NAME_TENSOR_DESC_MEM_OFFSET, offset);
  EXPECT_EQ(offset, 1024);
}

TEST_F(UtestMemoryAssignerTest, test_data_input_desc_offset_update) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  ComputeGraphPtr sub_graph = std::make_shared<ComputeGraph>("sub");
  sub_graph->SetParentGraph(graph);
  GraphMemoryAssigner graph_mem_assigner(graph);

  OpDescPtr p_desc = std::make_shared<OpDesc>("partitioned_call", PARTITIONEDCALL);
  GeTensorDescPtr p_tensor = std::make_shared<GeTensorDesc>();
  ge::AttrUtils::SetInt(p_tensor, ATTR_NAME_TENSOR_DESC_MEM_OFFSET, 1024);
  p_desc->AddInputDesc(p_tensor->Clone());
  NodePtr partitioned_call = graph->AddNode(p_desc);
  sub_graph->SetParentNode(partitioned_call);

  OpDescPtr sub_data_desc = std::make_shared<OpDesc>("sub_data", DATA);
  ge::AttrUtils::SetInt(sub_data_desc, ATTR_NAME_PARENT_NODE_INDEX, 0);
  GeTensorDescPtr sub_data_tensor = std::make_shared<GeTensorDesc>();
  ge::AttrUtils::SetBool(sub_data_tensor, ATTR_NAME_TENSOR_NO_TILING_MEM_TYPE, true);
  sub_data_desc->AddOutputDesc(sub_data_tensor->Clone());
  NodePtr sub_data = sub_graph->AddNode(sub_data_desc);

  EXPECT_EQ(graph_mem_assigner.UpdateOpInputDescOffset(sub_data), SUCCESS);

  uint32_t offset = 0;
  auto tensor = sub_data_desc->GetOutputDescPtr(0);
  ge::AttrUtils::GetInt(tensor, ATTR_NAME_TENSOR_DESC_MEM_OFFSET, offset);
  EXPECT_EQ(offset, 1024);
}

TEST_F(UtestMemoryAssignerTest, test_get_node_memory_type) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  GraphMemoryAssigner graph_mem_assigner(graph);

  OpDescPtr hcom_desc = std::make_shared<OpDesc>("broadcast", HCOMBROADCAST);
  GeTensorDescPtr hcom_tensor = std::make_shared<GeTensorDesc>();
  hcom_desc->AddOutputDesc(hcom_tensor->Clone());
  NodePtr hcom = graph->AddNode(hcom_desc);
  int64_t memory_type = RT_MEMORY_HBM;
  MemoryOffset offset(RT_MEMORY_HBM, 0);
  graph_mem_assigner.memory_offset_.emplace(RT_MEMORY_HBM, offset);
  std::vector<int64_t> mem_type_list = {RT_MEMORY_HBM};
  (void)ge::AttrUtils::SetListInt(hcom_desc, ATTR_NAME_OUTPUT_MEM_TYPE_LIST, mem_type_list);
  EXPECT_EQ(graph_mem_assigner.GetNodeMemoryType(hcom, memory_type, "input"), SUCCESS);
  EXPECT_EQ(graph_mem_assigner.GetNodeMemoryType(hcom, memory_type, "output"), SUCCESS);

  (void)ge::AttrUtils::SetListInt(hcom_desc, ATTR_NAME_INPUT_MEM_TYPE_LIST, mem_type_list);
  EXPECT_EQ(graph_mem_assigner.GetNodeMemoryType(hcom, memory_type, "input"), FAILED);
}

TEST_F(UtestMemoryAssignerTest, graph_memory_stream_reuse) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("");
  MakeStreamGraph(graph);
  MemoryAssigner memory_assigner(graph);
  map<uint64_t, size_t> mem_offset;
  size_t zero_memory_size = 0;
  EXPECT_EQ(memory_assigner.AssignMemory(mem_offset, zero_memory_size), GRAPH_SUCCESS);
}