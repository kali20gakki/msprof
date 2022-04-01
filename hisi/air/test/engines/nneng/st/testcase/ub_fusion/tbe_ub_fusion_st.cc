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

#include <map>
#include <stdlib.h>
#include <stdio.h>
#include <memory>
#include <string>
#include "gtest/gtest.h"

#define protected public
#define private public

#include "fusion_stub.hpp"
#include "common/scope_allocator.h"
#include "graph/utils/attr_utils.h"


#include "common/fusion_op_comm.h"
#include "common/graph_comm.h"
#include "graph_optimizer/ub_fusion/buffer_fusion.h"
#include "graph/op_kernel_bin.h"
#include "common/configuration.h"
#include "graph_optimizer/ub_fusion/automatic_buffer_fusion.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "ops_store/ops_kernel_manager.h"

using namespace std;
using namespace ge;
using namespace fe;

class UBFUSION_ST : public testing::Test {
 protected:
  static void SetUpTestCase()
  {
    std::cout << "fusion ST SetUp" << std::endl;
  }
  static void TearDownTestCase()
  {
    std::cout << "fusion ST TearDown" << std::endl;
  }

  virtual void SetUp() {
  }

  virtual void TearDown()
  {

  }
};
//namespace UB_FUSION {
const int LOOPTIMES = 10;
const string type_list[] = {"ElemWise", "CommReduce", "Opaque", "ElemWise", "ElemWise", "ElemWise"};
const string type_list1[] = {"ElemWise", "CommReduce", "Opaque", "Segment"};

void set_pattern(OpDescPtr OpDesc, string s)
{
  int i = rand() % (sizeof(type_list) / sizeof(type_list[0]));
  SetPattern(OpDesc, type_list[i]);
}

void set_pattern1(OpDescPtr OpDesc, string s)
{
  int i = rand() % (sizeof(type_list1) / sizeof(type_list1[0]));
  SetPattern(OpDesc, type_list1[i]);
}

void RunUbFusion(ComputeGraphPtr model_graph, string engine_name = fe::AI_CORE_NAME) {
  //  TEUBFUSION START
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
      std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr, nullptr);
  shared_ptr <BufferFusion> graph_builder(new BufferFusion(graph_comm_ptr,
                                                           fusion_pass_mgr_ptr, fusion_priority_mgr_ptr));
  graph_builder->SetEngineName(engine_name);

  graph_builder->MatchFusionPattern(*model_graph);
  auto auto_buffer_fusion_ptr = std::make_shared<AutomaticBufferFusion>(nullptr);
  auto_buffer_fusion_ptr->Run(*model_graph);
}

void check_result(ComputeGraphPtr model_graph, int i, set<set<string>>result)
{
  cout << "=======================graph=============================" << endl;
  PrintGraph(model_graph);
  cout << "=======================before fusion=============================" << endl;
  int64_t scope_id1 = 0;
  uint32_t id1 = 0;
  cout << "before fusion" << endl;
  for (NodePtr node: model_graph->GetAllNodes()) {
    cout << "name: " << node->GetName().c_str() << endl;
    if (ScopeAllocator::GetScopeAttr(node->GetOpDesc(), scope_id1)) {
      cout << "scopeID: " << scope_id1 << endl;
    } else {
      cout << "no scope ID" << endl;
    }
  }
  cout << "=======================debug info=============================" << endl;
  //  TEUBFUSION START

  RunUbFusion(model_graph);
  cout << "=======================after fusion " << i << " =============================" << endl;
  map<uint32_t, set<string>> res1;
  set<set<string>> res;
  for (NodePtr node: model_graph->GetAllNodes()) {
    string pattern = "";
    GetPattern(node->GetOpDesc(), pattern);
    if (ScopeAllocator::GetScopeAttr(node->GetOpDesc(), scope_id1)) {
      cout << "name: " << node->GetName().c_str() << "====scope_id: " << scope_id1 << "========type: "
           << pattern << endl;
      if (res1.find(scope_id1) != res1.end()) {
        ((res1.find(scope_id1))->second).insert(node->GetName());
      } else {
        set <string> s1 = {node->GetName()};
        res1[scope_id1] = s1;
      }
    } else {
      cout << "name: " << node->GetName().c_str() << "=====no scope ID" << "=======type: " << pattern
           << endl;
    }
  }
  for (auto const &x : res1) {
    res.insert(x.second);
  }
  if (res.size() == 0) {
    res = {{}};
  }
  if (res != result) {
    cout << res.size() << " size error " << i << endl;
  }
  EXPECT_EQ(true, res == result);
}
//}

/************************
*nodec1-->noder-->nodee-->nodeu-->nodec3
*                   ^
*                   |
*                 nodec2
*************************/
TEST_F(UBFUSION_ST, fusion_test_tefusion_nodec1_nodec2_noder_nodee_nodeu_nodec3)
{


  srand(1);
  vector<set <set <string>>> res{
      {{"nodec1", "noder"}, {"nodee", "nodeu"}},// 0
      {{"nodec1", "noder"}},// 1
      {{"nodec1", "noder"}, {"nodee", "nodeu", "nodec3"}},// 2
      {{"nodec1", "noder"}, {"nodeu", "nodec3"}},// 3
      {{}},// 4
      {{"nodec1", "noder"}, {"nodee", "nodeu", "nodec3"}},// 5
      {{"nodec1", "noder"}, {"nodee", "nodeu", "nodec3"}},// 6
      {{}},// 7
      {{"nodec1", "noder"}, {"nodee", "nodeu"}},// 8
      {{"nodeu",  "nodec3"}},// 9
  };

  for (int i = 0; i < 1; i++) {
    // initial node
    ComputeGraphPtr model_graph = std::make_shared<ComputeGraph>("modelgraph");

    map<string, vector<desc_info>> src_map;
    map<string, vector<desc_info>> dst_map;
    map<string, vector<desc_info>> ::iterator it;
    desc_info dsc_info;
    vector<desc_info> desc_tmp;

    OpDescPtr op_desc;
    vector<OpDescPtr> op_list;
    vector<string> src_name_list;
    vector<string> dst_name_list;
    GeTensorDesc input_desc;
    GeTensorDesc input_desc2;
    GeTensorDesc output_desc;
    vector<GeTensorDesc> input_desc_list;
    vector<GeTensorDesc> output_desc_list;

    desc_tmp.clear();
    src_map.insert(pair <string, vector<desc_info>> ("nodec1", desc_tmp));
    src_map.insert(pair <string, vector<desc_info>> ("nodec2", desc_tmp));
    src_map.insert(pair <string, vector<desc_info>> ("nodec3", desc_tmp));
    src_map.insert(pair <string, vector<desc_info>> ("noder", desc_tmp));
    src_map.insert(pair <string, vector<desc_info>> ("nodee", desc_tmp));
    src_map.insert(pair <string, vector<desc_info>> ("nodeu", desc_tmp));

    dst_map.insert(pair <string, vector<desc_info>> ("nodec1", desc_tmp));
    dst_map.insert(pair <string, vector<desc_info>> ("nodec2", desc_tmp));
    dst_map.insert(pair <string, vector<desc_info>> ("nodec3", desc_tmp));
    dst_map.insert(pair <string, vector<desc_info>> ("noder", desc_tmp));
    dst_map.insert(pair <string, vector<desc_info>> ("nodee", desc_tmp));
    dst_map.insert(pair <string, vector<desc_info>> ("nodeu", desc_tmp));

    // nodec2
    filltensordesc(input_desc, 1, 16, 300, 300, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 150, 150, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    dst_name_list.clear();
    dst_name_list.push_back("nodee");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    op_desc = CreateOpDefUbFusion("nodec2", "nodec", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(op_desc, OPDESC_DST_NAME, dst_name_list);
    op_list.push_back(op_desc);//
    SetAICoreOp(op_desc);
    SetTvmType(op_desc);
    int type;
    bool ret = ge::AttrUtils::GetInt(op_desc, "imply_type", type);
    cout << "bbbbbbbbbbbbbbbb type " << type << " findit "<< ret <<   endl;
    set_pattern(op_desc, "ElemWise");
    //ge::AttrUtils::SetListStr(op_desc, OPDESC_DST_NAME, dst_name_list);

    ge::AttrUtils::GetInt(op_desc, "imply_type", type);
    cout << "aaaaaaaaaaaaaa type " << type << endl;
    it = dst_map.find("nodec2");
    vector<desc_info> &vec1 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec1.push_back(dsc_info);

    // nodec1
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    dst_name_list.clear();
    dst_name_list.push_back("noder");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    op_desc = CreateOpDefUbFusion("nodec1", "nodec", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(op_desc, OPDESC_DST_NAME, dst_name_list);
    op_list.push_back(op_desc);
    SetAICoreOp(op_desc);
    SetTvmType(op_desc);
    set_pattern(op_desc, "ElemWise");
    it = dst_map.find("nodec1");
    vector<desc_info> &vec2 = it->second;
    dsc_info.targetname = "noder";
    dsc_info.index = 0;
    vec2.push_back(dsc_info);


    // noder
    filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 100, 100, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec1");
    dst_name_list.clear();
    dst_name_list.push_back("nodee");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    op_desc = CreateOpDefUbFusion("noder", "noder", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(op_desc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(op_desc, OPDESC_SRC_NAME, src_name_list);
    SetTvmType(op_desc);
    SetAICoreOp(op_desc);
    set_pattern(op_desc, "ElemWise");
    op_list.push_back(op_desc);

    it = dst_map.find("noder");
    vector<desc_info> &vec3 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec3.push_back(dsc_info);

    it = src_map.find("noder");
    vector<desc_info> &vec4 = it->second;
    dsc_info.targetname = "nodec1";
    dsc_info.index = 0;
    vec4.push_back(dsc_info);

    // nodee
    filltensordesc(input_desc, 1, 16, 150, 150, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(input_desc2, 1, 16, 100, 100, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 101, 101, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec2");
    src_name_list.push_back("noder");
    dst_name_list.clear();
    dst_name_list.push_back("nodeu");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    input_desc_list.push_back(input_desc2);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    op_desc = CreateOpDefUbFusion("nodee", "nodee", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(op_desc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(op_desc, OPDESC_SRC_NAME, src_name_list);
    SetTvmType(op_desc);
    SetAICoreOp(op_desc);
    set_pattern(op_desc, "ElemWise");
    op_list.push_back(op_desc);

    it = dst_map.find("nodee");
    vector<desc_info> &vec5 = it->second;
    dsc_info.targetname = "nodeu";
    dsc_info.index = 0;
    vec5.push_back(dsc_info);

    it = src_map.find("nodee");
    vector<desc_info> &vec6 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec6.push_back(dsc_info);
    dsc_info.targetname = "noder";
    dsc_info.index = 1;
    vec6.push_back(dsc_info);

    // nodeu
    filltensordesc(input_desc, 1, 16, 101, 101, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 55, 55, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodee");
    dst_name_list.clear();
    dst_name_list.push_back("nodec3");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    op_desc = CreateOpDefUbFusion("nodeu", "nodeu", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(op_desc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(op_desc, OPDESC_SRC_NAME, src_name_list);
    SetTvmType(op_desc);
    SetAICoreOp(op_desc);

    op_list.push_back(op_desc);

    it = dst_map.find("nodeu");
    vector<desc_info> &vec7 = it->second;
    dsc_info.targetname = "nodec3";
    dsc_info.index = 0;
    vec7.push_back(dsc_info);

    it = src_map.find("nodeu");
    vector<desc_info> &vec8 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec8.push_back(dsc_info);
    set_pattern(op_desc, "ElemWise");

    // nodec3
    filltensordesc(input_desc, 1, 16, 55, 55, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 50, 50, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodeu");
    dst_name_list.clear();
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    op_desc = CreateOpDefUbFusion("nodec3", "nodec", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(op_desc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(op_desc);
    SetAICoreOp(op_desc);
    SetTvmType(op_desc);

    it = src_map.find("nodec3");
    vector<desc_info> &vec9 = it->second;
    dsc_info.targetname = "nodeu";
    dsc_info.index = 0;
    vec9.push_back(dsc_info);
    string s;
    set_pattern(op_desc, "ElemWise");
    GetPattern(op_desc, s);
    cout << "=======================test pattern=============================" << endl;
    CreateModelGraph(model_graph, op_list, src_map, dst_map);
    check_result(model_graph, i, res[i]);
  }
}

/************************
*            nodec5     nodec4
*             |         ^
*             V         |
*  noder-->nodee-->multi_output-->nodec2
*    ^                             |
*    |                             V
*   nodec1                       nodec3
*************************/
TEST_F(UBFUSION_ST, fusion_test_tefusion_complex)
{

  srand(1);
  vector<set<set<string>>> res{
      {{"multiOutput", "nodec4"}, {"nodec1", "noder"}, {"nodec2", "nodec3"}}, // 0
      {{"nodec2", "nodec3"}}, // 1
      {{"nodec1", "noder", "nodee", "multiOutput", "nodec4"}}, // 2
      {{"nodec2", "nodec3"}}, // 3
      {{"nodee", "multiOutput"}, {"nodec2", "nodec3"}, {"nodec1", "noder"}}, // 4
      {{"nodec1", "noder"}, {"nodec5", "nodee"}}, // 5
      {{"nodee", "multiOutput"}, {"nodec1", "noder"}}, // 6
      {{"multiOutput", "nodec4", "nodec2"}}, // 7
      {{"nodee", "multiOutput"}, {"nodec2", "nodec3"}}, // 8
      {{"nodec5", "nodee"}}, // 9
  };
  for (int i = 0; i < LOOPTIMES; i++) {
    // initial node
    ComputeGraphPtr model_graph = std::make_shared<ComputeGraph>("modelgraph");

    map<string, vector<desc_info>> src_map;
    map<string, vector<desc_info>> dst_map;
    map<string, vector<desc_info>> ::iterator it;
    desc_info dsc_info;
    vector<desc_info> desc_tmp;
    desc_tmp.clear();
    src_map.insert(pair<string, vector<desc_info>> ("nodec1", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec2", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec3", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec4", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec5", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("noder", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodee", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("multiOutput", desc_tmp));

    dst_map.insert(pair<string, vector<desc_info>> ("nodec1", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec2", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec3", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec4", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec5", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("noder", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodee", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("multiOutput", desc_tmp));

    OpDescPtr OpDesc;
    vector<OpDescPtr> op_list;
    vector<string> src_name_list;
    vector<string> dst_name_list;
    GeTensorDesc input_desc;
    GeTensorDesc input_desc1;
    GeTensorDesc input_desc2;
    GeTensorDesc output_desc;
    GeTensorDesc output_desc1;
    GeTensorDesc output_desc2;
    vector<GeTensorDesc> input_desc_list;
    vector<GeTensorDesc> output_desc_list;

    // nodec1
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    dst_name_list.clear();
    dst_name_list.push_back("noder");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec1", "Data", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    op_list.push_back(OpDesc);

    it = dst_map.find("nodec1");
    vector<desc_info> &vec1 = it->second;
    dsc_info.targetname = "noder";
    dsc_info.index = 0;
    vec1.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern(OpDesc, "ElemWise");

    // nodec5
    filltensordesc(input_desc, 1, 16, 500, 500, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 170, 170, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    dst_name_list.clear();
    dst_name_list.push_back("nodee");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec5", "Data", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    op_list.push_back(OpDesc);

    it = dst_map.find("nodec5");
    vector<desc_info> &vec2 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec2.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern(OpDesc, "ElemWise");

    // noder
    filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 180, 180, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec1");
    dst_name_list.clear();
    dst_name_list.push_back("nodee");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("noder", "noder", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    SetTvmType(OpDesc);
    op_list.push_back(OpDesc);

    it = dst_map.find("noder");
    vector<desc_info> &vec3 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec3.push_back(dsc_info);

    it = src_map.find("noder");
    vector<desc_info> &vec4 = it->second;
    dsc_info.targetname = "nodec1";
    dsc_info.index = 0;
    vec4.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern(OpDesc, "ElemWise");

    // nodee
    filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 170, 170, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(input_desc2, 1, 16, 180, 180, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec5");
    src_name_list.push_back("noder");
    dst_name_list.clear();
    dst_name_list.push_back("multiOutput");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    input_desc_list.push_back(input_desc1);
    input_desc_list.push_back(input_desc2);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodee", "nodee", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);

    SetTvmType(OpDesc);
    op_list.push_back(OpDesc);

    it = src_map.find("nodee");
    vector<desc_info> &vec5 = it->second;
    dsc_info.targetname = "nodec5";
    dsc_info.index = 1;
    vec5.push_back(dsc_info);
    dsc_info.targetname = "noder";
    dsc_info.index = 2;
    vec5.push_back(dsc_info);

    it = dst_map.find("nodee");
    vector<desc_info> &vec6 = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec6.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern(OpDesc, "ElemWise");

    // multi_output
    filltensordesc(input_desc, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc1, 1, 16, 201, 201, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodee");
    dst_name_list.clear();
    dst_name_list.push_back("nodec2");
    dst_name_list.push_back("nodec4");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("multiOutput", "multiOutput", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);

    SetTvmType(OpDesc);
    op_list.push_back(OpDesc);

    it = dst_map.find("multiOutput");
    vector<desc_info> &vec7 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec7.push_back(dsc_info);

    dsc_info.targetname = "nodec4";
    dsc_info.index = 0;
    vec7.push_back(dsc_info);

    it = src_map.find("multiOutput");
    vector<desc_info> &vec8 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec8.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern(OpDesc, "ElemWise");

    // nodec2
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 207, 207, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("multiOutput");
    dst_name_list.clear();
    dst_name_list.push_back("nodec3");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec2", "nodec2", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec2");
    vector<desc_info> &vec9 = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec9.push_back(dsc_info);

    dsc_info.targetname = "nodec3";
    dsc_info.index = 1;
    vec7.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern(OpDesc, "ElemWise");

    // nodec3
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec2");
    dst_name_list.clear();
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec3", "nodec", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec3");
    vector<desc_info> &vec10 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec10.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern(OpDesc, "ElemWise");

    // nodec4
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 209, 209, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("multiOutput");
    dst_name_list.clear();
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec4", "nodec", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec4");
    vector<desc_info> &vec11 = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec11.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern(OpDesc, "ElemWise");

    CreateModelGraph(model_graph, op_list, src_map, dst_map);
    check_result(model_graph, i, res[i]);
  }
}

/************************
*nodec1-->noder-->nodee-->nodeu-->nodec3
*                   ^       |
*                   |       V
*                 nodec2   nodeu1
*************************/
TEST_F(UBFUSION_ST, for_coverage)
{
  // initial node
  ComputeGraphPtr model_graph = std::make_shared<ComputeGraph>("modelgraph");

  map<string, vector<desc_info>> src_map;
  map<string, vector<desc_info>> dst_map;
  map<string, vector<desc_info>>::iterator it;
  desc_info dsc_info;
  vector<desc_info> desc_tmp;
  desc_tmp.clear();
  src_map.insert(pair<string, vector<desc_info>> ("nodec1", desc_tmp));
  src_map.insert(pair<string, vector<desc_info>> ("nodec2", desc_tmp));
  src_map.insert(pair<string, vector<desc_info>> ("nodec3", desc_tmp));
  src_map.insert(pair<string, vector<desc_info>> ("nodec4", desc_tmp));
  src_map.insert(pair<string, vector<desc_info>> ("nodec5", desc_tmp));
  src_map.insert(pair<string, vector<desc_info>> ("noder", desc_tmp));
  src_map.insert(pair<string, vector<desc_info>> ("nodee", desc_tmp));
  src_map.insert(pair<string, vector<desc_info>> ("multiOutput", desc_tmp));

  dst_map.insert(pair<string, vector<desc_info>> ("nodec1", desc_tmp));
  dst_map.insert(pair<string, vector<desc_info>> ("nodec2", desc_tmp));
  dst_map.insert(pair<string, vector<desc_info>> ("nodec3", desc_tmp));
  dst_map.insert(pair<string, vector<desc_info>> ("nodec4", desc_tmp));
  dst_map.insert(pair<string, vector<desc_info>> ("nodec5", desc_tmp));
  dst_map.insert(pair<string, vector<desc_info>> ("noder", desc_tmp));
  dst_map.insert(pair<string, vector<desc_info>> ("nodee", desc_tmp));
  dst_map.insert(pair<string, vector<desc_info>> ("multiOutput", desc_tmp));

  OpDescPtr OpDesc;
  vector<OpDescPtr> op_list;
  vector<string> src_name_list;
  vector<string> dst_name_list;
  GeTensorDesc input_desc;
  GeTensorDesc input_desc1;
  GeTensorDesc input_desc2;
  GeTensorDesc output_desc;
  GeTensorDesc output_desc1;
  GeTensorDesc output_desc2;
  vector<GeTensorDesc> input_desc_list;
  vector<GeTensorDesc> output_desc_list;

  // nodec1
  filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
  filltensordesc(output_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
  src_name_list.clear();
  src_name_list.push_back("nodec5");
  dst_name_list.clear();
  dst_name_list.push_back("noder");
  input_desc_list.clear();
  input_desc_list.push_back(input_desc);
  output_desc_list.clear();
  output_desc_list.push_back(output_desc);
  OpDesc = CreateOpDefUbFusion("nodec1", "Data", src_name_list, dst_name_list, input_desc_list, output_desc_list);
  ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
  ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
  op_list.push_back(OpDesc);

  it = dst_map.find("nodec1");
  vector<desc_info> &vec1 = it->second;
  dsc_info.targetname = "noder";
  dsc_info.index = 0;
  vec1.push_back(dsc_info);
  SetTvmType(OpDesc);
  SetAICoreOp(OpDesc);
  SetPattern(OpDesc, "ElemWise");

  // nodec5
  filltensordesc(input_desc, 1, 16, 500, 500, DT_FLOAT16, FORMAT_NCHW);
  filltensordesc(output_desc, 1, 16, 170, 170, DT_FLOAT16, FORMAT_NCHW);
  src_name_list.clear();
  dst_name_list.clear();
  dst_name_list.push_back("nodec1");
  input_desc_list.clear();
  input_desc_list.push_back(input_desc);
  output_desc_list.clear();
  output_desc_list.push_back(output_desc);
  OpDesc = CreateOpDefUbFusion("nodec5", "Data", src_name_list, dst_name_list, input_desc_list, output_desc_list);
  ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
  op_list.push_back(OpDesc);

  it = dst_map.find("nodec5");
  vector<desc_info> &vec2 = it->second;
  dsc_info.targetname = "nodec1";
  dsc_info.index = 0;
  vec2.push_back(dsc_info);
  SetTvmType(OpDesc);
  SetAICoreOp(OpDesc);
  SetPattern(OpDesc, "ElemWise");

  // noder
  filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
  filltensordesc(output_desc, 1, 16, 180, 180, DT_FLOAT16, FORMAT_NCHW);
  src_name_list.clear();
  src_name_list.push_back("nodec1");
  dst_name_list.clear();
  dst_name_list.push_back("nodee");
  input_desc_list.clear();
  input_desc_list.push_back(input_desc);
  output_desc_list.clear();
  output_desc_list.push_back(output_desc);
  OpDesc = CreateOpDefUbFusion("noder", "noder", src_name_list, dst_name_list, input_desc_list, output_desc_list);
  ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
  ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
  SetTvmType(OpDesc);
  op_list.push_back(OpDesc);

  it = dst_map.find("noder");
  vector<desc_info> &vec3 = it->second;
  dsc_info.targetname = "nodee";
  dsc_info.index = 0;
  vec3.push_back(dsc_info);

  it = src_map.find("noder");
  vector<desc_info> &vec4 = it->second;
  dsc_info.targetname = "nodec1";
  dsc_info.index = 0;
  vec4.push_back(dsc_info);
  SetTvmType(OpDesc);
  SetAICoreOp(OpDesc);
  SetPattern(OpDesc, "ElemWise");

  // nodee
  filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
  filltensordesc(input_desc1, 1, 16, 170, 170, DT_FLOAT16, FORMAT_NCHW);
  filltensordesc(input_desc2, 1, 16, 180, 180, DT_FLOAT16, FORMAT_NCHW);
  filltensordesc(output_desc, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
  src_name_list.clear();
  src_name_list.push_back("noder");
  dst_name_list.clear();
  dst_name_list.push_back("multiOutput");
  input_desc_list.clear();
  input_desc_list.push_back(input_desc);
  input_desc_list.push_back(input_desc1);
  input_desc_list.push_back(input_desc2);
  output_desc_list.clear();
  output_desc_list.push_back(output_desc);
  OpDesc = CreateOpDefUbFusion("nodee", "nodee", src_name_list, dst_name_list, input_desc_list, output_desc_list);
  ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
  ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);

  SetTvmType(OpDesc);
  op_list.push_back(OpDesc);

  it = src_map.find("nodee");
  vector<desc_info> &vec5 = it->second;
  dsc_info.targetname = "noder";
  dsc_info.index = 2;
  vec5.push_back(dsc_info);

  it = dst_map.find("nodee");
  vector<desc_info> &vec6 = it->second;
  dsc_info.targetname = "multiOutput";
  dsc_info.index = 0;
  vec6.push_back(dsc_info);
  SetTvmType(OpDesc);
  SetAICoreOp(OpDesc);
  SetPattern(OpDesc, "ElemWise");

  // multi_output
  filltensordesc(input_desc, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
  filltensordesc(output_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
  filltensordesc(output_desc1, 1, 16, 201, 201, DT_FLOAT16, FORMAT_NCHW);
  src_name_list.clear();
  src_name_list.push_back("nodee");
  dst_name_list.clear();
  dst_name_list.push_back("nodec2");
  dst_name_list.push_back("nodec4");
  input_desc_list.clear();
  input_desc_list.push_back(input_desc);
  output_desc_list.clear();
  output_desc_list.push_back(output_desc);
  output_desc_list.push_back(output_desc1);
  OpDesc = CreateOpDefUbFusion("multiOutput", "multiOutput", src_name_list, dst_name_list, input_desc_list, output_desc_list);
  ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
  ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);

  SetTvmType(OpDesc);
  op_list.push_back(OpDesc);

  it = dst_map.find("multiOutput");
  vector<desc_info> &vec7 = it->second;
  dsc_info.targetname = "nodec2";
  dsc_info.index = 4;
  vec7.push_back(dsc_info);

  dsc_info.targetname = "nodec4";
  dsc_info.index = 0;
  vec7.push_back(dsc_info);

  it = src_map.find("multiOutput");
  vector<desc_info> &vec8 = it->second;
  dsc_info.targetname = "nodee";
  dsc_info.index = 1;
  vec8.push_back(dsc_info);

  SetTvmType(OpDesc);
  SetAICoreOp(OpDesc);
  set_pattern(OpDesc, "ElemWise");

  // nodec2
  filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
  filltensordesc(output_desc, 1, 16, 207, 207, DT_FLOAT16, FORMAT_NCHW);
  src_name_list.clear();
  src_name_list.push_back("multiOutput");
  dst_name_list.clear();
  dst_name_list.push_back("nodec3");
  input_desc_list.clear();
  input_desc_list.push_back(input_desc);
  output_desc_list.clear();
  output_desc_list.push_back(output_desc);
  OpDesc = CreateOpDefUbFusion("nodec2", "nodec2", src_name_list, dst_name_list, input_desc_list, output_desc_list);
  ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
  ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
  op_list.push_back(OpDesc);

  it = src_map.find("nodec2");
  vector<desc_info> &vec9 = it->second;
  dsc_info.targetname = "multiOutput";
  dsc_info.index = 0;
  vec9.push_back(dsc_info);

  dsc_info.targetname = "nodec3";
  dsc_info.index = 1;
  vec7.push_back(dsc_info);


  SetTvmType(OpDesc);
  SetAICoreOp(OpDesc);
  set_pattern(OpDesc, "ElemWise");

  // nodec3
  filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16, FORMAT_NCHW);
  filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16, FORMAT_NCHW);
  src_name_list.clear();
  src_name_list.push_back("nodec2");
  dst_name_list.clear();
  input_desc_list.clear();
  input_desc_list.push_back(input_desc);
  output_desc_list.clear();
  output_desc_list.push_back(output_desc);
  OpDesc = CreateOpDefUbFusion("nodec3", "nodec", src_name_list, dst_name_list, input_desc_list, output_desc_list);
  ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
  op_list.push_back(OpDesc);

  it = src_map.find("nodec3");
  vector<desc_info> &vec10 = it->second;
  dsc_info.targetname = "nodec2";
  dsc_info.index = 0;
  vec10.push_back(dsc_info);
  SetTvmType(OpDesc);
  SetAICoreOp(OpDesc);

  // nodec4
  filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16, FORMAT_NCHW);
  filltensordesc(output_desc, 1, 16, 209, 209, DT_FLOAT16, FORMAT_NCHW);
  src_name_list.clear();
  src_name_list.push_back("multiOutput");
  dst_name_list.clear();
  input_desc_list.clear();
  input_desc_list.push_back(input_desc);
  output_desc_list.clear();
  output_desc_list.push_back(output_desc);
  OpDesc = CreateOpDefUbFusion("nodec4", "nodec", src_name_list, dst_name_list, input_desc_list, output_desc_list);
  ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
  op_list.push_back(OpDesc);

  it = src_map.find("nodec4");
  vector<desc_info> &vec11 = it->second;
  dsc_info.targetname = "multiOutput";
  dsc_info.index = 0;
  vec11.push_back(dsc_info);
  SetTvmType(OpDesc);
  SetAICoreOp(OpDesc);
  set_pattern(OpDesc, "Elem");

  cout << "=======================test pattern=============================" << endl;

  CreateModelGraph(model_graph, op_list, src_map, dst_map);
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
      std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr, nullptr);
  shared_ptr <BufferFusion> graph_builder(new BufferFusion(graph_comm_ptr,
                                                           fusion_pass_mgr_ptr, fusion_priority_mgr_ptr));
  graph_builder->SetEngineName(fe::AI_CORE_NAME);
  graph_builder->MatchFusionPattern(*model_graph);
  cout << "=======================graph=============================" << endl;
  PrintGraph(model_graph);
  int64_t scope_id1 = 0;
  for (NodePtr node: model_graph->GetAllNodes()) {
    string pattern = "";
    GetPattern(node->GetOpDesc(), pattern);
    if (ScopeAllocator::GetScopeAttr(node->GetOpDesc(), scope_id1)) {
      cout << "name: " << node->GetName().c_str() << "====scope_id: " << scope_id1 << "========type: "
           << pattern << endl;
    } else {
      cout << "name: " << node->GetName().c_str() << "=====no scope ID" << "=======type: " << pattern
           << endl;
    }
  }
}

/*****************************************************
*            nodec5     nodec4--------->nodec6
*             |          ^                |
*             V          |                V
*  noder-->nodee-->multi_output->nodec7->nodec2->nodec3
*   ^
*   |
*  nodec1
******************************************************/
TEST_F(UBFUSION_ST, fusion_test_tefusion_bifurcated)
{

  srand(1);
  vector<set<set<string>>> res{
      {{"nodec1", "noder"}, {"nodec5", "nodee"}, {"nodec7", "nodec2"}, {"nodec4", "nodec6"}}, // 0
      {{"nodee", "multiOutput"}, {"nodec1", "noder"}, {"nodec4", "nodec6"}, {"nodec7", "nodec2"}}, // 1
      {{"nodee", "multiOutput"}, {"nodec1", "noder"}, {"nodec4", "nodec6"}, {"nodec7", "nodec2"}}, // 2
      {{"nodec1", "noder"}, {"nodec5", "nodee"}, {"nodec2", "nodec3"}, {"nodec4", "nodec6"}}, // 3
      {{"nodee", "multiOutput"}, {"nodec2", "nodec3"}, {"nodec1", "noder"}, {"nodec4", "nodec6"}}, // 4
      {{"nodec1", "noder"}, {"nodec5", "nodee"}, {"nodec7", "nodec2"}, {"nodec4", "nodec6"}}, // 5
      {{"nodec1", "noder"}, {"nodec5", "nodee"}, {"nodec7", "nodec2"}, {"nodec4", "nodec6"}}, // 6
      {{"nodec1", "noder"}, {"nodec5", "nodee"}, {"nodec7", "nodec2"}, {"nodec4", "nodec6"}}, // 7
      {{"nodec1", "noder", "nodee", "multiOutput", "nodec7", "nodec4", "nodec6", "nodec2"}}, // 8
      {{"nodec1", "noder", "nodee", "multiOutput", "nodec7", "nodec4", "nodec6", "nodec2", "nodec3"}}, // 9
  };
  for (int i = 0; i < LOOPTIMES; i++) {
    // initial node
    ComputeGraphPtr model_graph = std::make_shared<ComputeGraph>("modelgraph");

    map<string, vector<desc_info>> src_map;
    map<string, vector<desc_info>> dst_map;
    map<string, vector<desc_info>> ::iterator it;
    desc_info dsc_info;
    vector<desc_info> desc_tmp;
    desc_tmp.clear();
    src_map.insert(pair<string, vector<desc_info>> ("nodec1", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec2", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec3", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec4", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec5", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec6", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec7", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("noder", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodee", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("multiOutput", desc_tmp));

    dst_map.insert(pair<string, vector<desc_info>>("nodec1", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>>("nodec2", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>>("nodec3", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>>("nodec4", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>>("nodec5", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>>("nodec6", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>>("nodec7", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>>("noder", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>>("nodee", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>>("multiOutput", desc_tmp));

    OpDescPtr OpDesc;
    vector<OpDescPtr> op_list;
    vector<string> src_name_list;
    vector<string> dst_name_list;
    GeTensorDesc input_desc;
    GeTensorDesc input_desc1;
    GeTensorDesc input_desc2;
    GeTensorDesc output_desc;
    GeTensorDesc output_desc1;
    GeTensorDesc output_desc2;
    vector<GeTensorDesc> input_desc_list;
    vector<GeTensorDesc> output_desc_list;

    // nodec1
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    dst_name_list.clear();
    dst_name_list.push_back("noder");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec1", "Data", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    op_list.push_back(OpDesc);

    it = dst_map.find("nodec1");
    vector<desc_info> &vec1 = it->second;
    dsc_info.targetname = "noder";
    dsc_info.index = 0;
    vec1.push_back(dsc_info);


    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec5
    filltensordesc(input_desc, 1, 16, 500, 500, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 170, 170, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    dst_name_list.clear();
    dst_name_list.push_back("nodee");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec5", "Data", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    op_list.push_back(OpDesc);

    it = dst_map.find("nodec5");
    vector<desc_info> &vec2 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec2.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // noder
    filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 180, 180, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec1");
    dst_name_list.clear();
    dst_name_list.push_back("nodee");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("noder", "cov", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);


    it = dst_map.find("noder");
    vector<desc_info> &vec3 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec3.push_back(dsc_info);

    it = src_map.find("noder");
    vector<desc_info> &vec4 = it->second;
    dsc_info.targetname = "nodec1";
    dsc_info.index = 0;
    vec4.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodee
    filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 170, 170, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec5");
    src_name_list.push_back("noder");
    dst_name_list.clear();
    dst_name_list.push_back("multiOutput");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    input_desc_list.push_back(input_desc1);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodee", "nodee", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);

    op_list.push_back(OpDesc);

    it = src_map.find("nodee");
    vector<desc_info> &vec5 = it->second;
    dsc_info.targetname = "nodec5";
    dsc_info.index = 0;
    vec5.push_back(dsc_info);

    dsc_info.targetname = "noder";
    dsc_info.index = 1;
    vec5.push_back(dsc_info);

    it = dst_map.find("nodee");
    vector<desc_info> &vec6 = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec6.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // multi_output
    filltensordesc(input_desc, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodee");
    dst_name_list.clear();
    dst_name_list.push_back("nodec7");
    dst_name_list.push_back("nodec4");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("multiOutput", "multiOutput", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);

    op_list.push_back(OpDesc);

    it = dst_map.find("multiOutput");
    vector<desc_info> &vec7 = it->second;
    dsc_info.targetname = "nodec7";
    dsc_info.index = 0;
    vec7.push_back(dsc_info);
    dsc_info.targetname = "nodec4";
    dsc_info.index = 0;
    vec7.push_back(dsc_info);

    it = src_map.find("multiOutput");
    vector<desc_info> &vec8 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec8.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern1(OpDesc, "ElemWise");

    // nodec2
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 207, 207, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec6");
    src_name_list.push_back("nodec7");
    dst_name_list.clear();
    dst_name_list.push_back("nodec3");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    input_desc_list.push_back(input_desc1);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec2", "nodec2", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec2");
    vector<desc_info> &vec9 = it->second;
    dsc_info.targetname = "nodec6";
    dsc_info.index = 0;
    vec9.push_back(dsc_info);
    dsc_info.targetname = "nodec7";
    dsc_info.index = 1;
    vec9.push_back(dsc_info);

    it = dst_map.find("nodec2");
    vector<desc_info> &vec19 = it->second;
    dsc_info.targetname = "nodec3";
    dsc_info.index = 0;
    vec19.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec6
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 207, 207, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec4");
    dst_name_list.clear();
    dst_name_list.push_back("nodec2");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec6", "nodec6", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec6");
    vector<desc_info> &vec96 = it->second;
    dsc_info.targetname = "nodec4";
    dsc_info.index = 0;
    vec96.push_back(dsc_info);

    it = dst_map.find("nodec6");
    vector<desc_info> &vec199 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec199.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec3
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec2");
    dst_name_list.clear();
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec3", "nodec", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec3");
    vector<desc_info> &vec10 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec10.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern1(OpDesc, "ElemWise");

    // nodec4
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 209, 209, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("multiOutput");
    dst_name_list.clear();
    dst_name_list.push_back("nodec6");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec4", "nodec", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec4");
    vector<desc_info> &vec11 = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec11.push_back(dsc_info);

    it = dst_map.find("nodec4");
    vector<desc_info> &vec100 = it->second;
    dsc_info.targetname = "nodec6";
    dsc_info.index = 0;
    vec100.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec7
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 209, 209, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("multiOutput");
    dst_name_list.clear();
    dst_name_list.push_back("nodec2");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec7", "nodec", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec7");
    vector<desc_info> &vec17 = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec17.push_back(dsc_info);

    it = dst_map.find("nodec7");
    vector<desc_info> &vec101 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec101.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    CreateModelGraph(model_graph, op_list, src_map, dst_map);

    check_result(model_graph, i, res[i]);
  }
}

/*******************************************************************************
*            nodec5     nodec4--------->nodec6           nodec11------->nodec12
*             |          ^                |                 ^              |
*             V          |                V                 |              V
*  noder-->nodee-->multi_output->nodec7->nodec2->nodec3->nodec8->nodec9->nodec10
*   ^
*   |
*  nodec1
*****************************************************************************/
TEST_F(UBFUSION_ST, fusion_test_tefusion_bifurcated_with_multicircle)
{

  srand(1);
  vector<set<set<string>>> res{
      {{"nodec8", "nodec9", "nodec11", "nodec12"}, {"nodec2", "nodec3"}, {"nodec1", "noder"},
       {"nodec5", "nodee"}}, // 0
      {{"nodec8", "nodec9", "nodec11", "nodec12"}, {"nodee", "multiOutput"}, {"nodec1", "noder"}, {"nodec4", "nodec6"},
       {"nodec7", "nodec2"}}, // 1
      {{"nodec8", "nodec9", "nodec11", "nodec12"}, {"nodee", "multiOutput"}, {"nodec1", "noder"},
       {"nodec7", "nodec6", "nodec2"}}, // 2
      {{"nodec8", "nodec9", "nodec11", "nodec12"}, {"nodec1", "noder"}, {"nodec7", "nodec2", "nodec6"},
       {"nodec5", "nodee"}}, // 3
      {{"nodec1", "noder", "nodee", "multiOutput", "nodec7", "nodec4", "nodec6", "nodec2", "nodec3", "nodec8"},
       {"nodec11", "nodec12"}}, // 4
      {{"nodec8", "nodec9", "nodec11", "nodec12", "nodec10"}, {"nodec4", "nodec6"}, {"nodec1", "noder"},
       {"nodec7", "nodec2"}, {"nodec5", "nodee"}}, // 5
      {{"nodec8", "nodec9", "nodec11", "nodec12"}, {"nodec1", "noder"}, {"nodec7", "nodec2", "nodec6"},
       {"nodec5", "nodee"}}, // 6
      {{"nodec8", "nodec9", "nodec11", "nodec12"}, {"nodec2", "nodec3"}, {"nodec1", "noder"}, {"nodec5", "nodee"}}, // 7
      {{"nodec8", "nodec9", "nodec11", "nodec12"}, {"nodec2", "nodec3"}, {"nodec1", "noder"}, {"nodec5", "nodee"}}, // 8
      {{"nodec8", "nodec9", "nodec11", "nodec12", "nodec10"}, {"nodee", "multiOutput"}, {"nodec1", "noder"},
       {"nodec7", "nodec6", "nodec2"}}, // 9
  };
  for (int i = 0; i < LOOPTIMES; i++) {
    // initial node
    ComputeGraphPtr model_graph = std::make_shared<ComputeGraph>("modelgraph");

    map<string, vector<desc_info>> src_map;
    map<string, vector<desc_info>> dst_map;
    map<string, vector<desc_info>>::iterator it;
    desc_info dsc_info;
    vector<desc_info> desc_tmp;
    desc_tmp.clear();
    src_map.insert(pair<string, vector<desc_info>> ("nodec1", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec2", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec3", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec4", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec5", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec6", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec7", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec8", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec9", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec10", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec11", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec12", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("noder", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodee", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("multiOutput",
                                                   desc_tmp));

    dst_map.insert(pair<string, vector<desc_info>> ("nodec1", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec2", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec3", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec4", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec5", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec6", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec7", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec8", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec9", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec10", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec11", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec12", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("noder", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodee", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("multiOutput",
                                                   desc_tmp));

    OpDescPtr OpDesc;
    vector<OpDescPtr> op_list;
    vector<string> src_name_list;
    vector<string> dst_name_list;
    GeTensorDesc input_desc;
    GeTensorDesc input_desc1;
    GeTensorDesc input_desc2;
    GeTensorDesc output_desc;
    GeTensorDesc output_desc1;
    GeTensorDesc output_desc2;
    vector<GeTensorDesc> input_desc_list;
    vector<GeTensorDesc> output_desc_list;

    // nodec1
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 160, 160, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    dst_name_list.clear();
    dst_name_list.push_back("noder");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec1", "Data", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    op_list.push_back(OpDesc);

    it = dst_map.find("nodec1");
    vector<desc_info> &vec1 = it->second;
    dsc_info.targetname = "noder";
    dsc_info.index = 0;
    vec1.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec5
    filltensordesc(input_desc, 1, 16, 500, 500, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 170, 170, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    dst_name_list.clear();
    dst_name_list.push_back("nodee");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec5", "Data", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    op_list.push_back(OpDesc);

    it = dst_map.find("nodec5");
    vector<desc_info> &vec2 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec2.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // noder
    filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 180, 180, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec1");
    dst_name_list.clear();
    dst_name_list.push_back("nodee");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("noder", "cov", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = dst_map.find("noder");
    vector<desc_info> &vec3 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec3.push_back(dsc_info);

    it = src_map.find("noder");
    vector<desc_info> &vec4 = it->second;
    dsc_info.targetname = "nodec1";
    dsc_info.index = 0;
    vec4.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodee
    filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 170, 170, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec5");
    src_name_list.push_back("noder");
    dst_name_list.clear();
    dst_name_list.push_back("multiOutput");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    input_desc_list.push_back(input_desc1);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodee", "nodee", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);

    op_list.push_back(OpDesc);

    it = src_map.find("nodee");
    vector<desc_info> &vec5 = it->second;
    dsc_info.targetname = "nodec5";
    dsc_info.index = 0;
    vec5.push_back(dsc_info);

    dsc_info.targetname = "noder";
    dsc_info.index = 1;
    vec5.push_back(dsc_info);

    it = dst_map.find("nodee");
    vector<desc_info> &vec6 = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec6.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // multi_output
    filltensordesc(input_desc, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodee");
    dst_name_list.clear();
    dst_name_list.push_back("nodec7");
    dst_name_list.push_back("nodec4");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("multiOutput", "multiOutput", src_name_list,
                                 dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);

    op_list.push_back(OpDesc);

    it = dst_map.find("multiOutput");
    vector<desc_info> &vec7 = it->second;
    dsc_info.targetname = "nodec7";
    dsc_info.index = 0;
    vec7.push_back(dsc_info);
    dsc_info.targetname = "nodec4";
    dsc_info.index = 0;
    vec7.push_back(dsc_info);

    it = src_map.find("multiOutput");
    vector<desc_info> &vec8 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec8.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern1(OpDesc, "ElemWise");

    // nodec2
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 200, 200, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 207, 207, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec6");
    src_name_list.push_back("nodec7");
    dst_name_list.clear();
    dst_name_list.push_back("nodec3");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    input_desc_list.push_back(input_desc1);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec2", "nodec2", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec2");
    vector<desc_info> &vec9 = it->second;
    dsc_info.targetname = "nodec6";
    dsc_info.index = 0;
    vec9.push_back(dsc_info);
    dsc_info.targetname = "nodec7";
    dsc_info.index = 1;
    vec9.push_back(dsc_info);

    it = dst_map.find("nodec2");
    vector<desc_info> &vec19 = it->second;
    dsc_info.targetname = "nodec3";
    dsc_info.index = 0;
    vec19.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec10
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 200, 200, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 207, 207, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec9");
    src_name_list.push_back("nodec12");
    dst_name_list.clear();
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    input_desc_list.push_back(input_desc1);
    output_desc_list.clear();
    OpDesc = CreateOpDefUbFusion("nodec10", "nodec2", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec10");
    vector<desc_info> &vec910 = it->second;
    dsc_info.targetname = "nodec9";
    dsc_info.index = 0;
    vec910.push_back(dsc_info);
    dsc_info.targetname = "nodec12";
    dsc_info.index = 1;
    vec910.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern1(OpDesc, "ElemWise");

    // nodec6
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 207, 207, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec4");
    dst_name_list.clear();
    dst_name_list.push_back("nodec2");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec6", "nodec6", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec6");
    vector<desc_info> &vec96 = it->second;
    dsc_info.targetname = "nodec4";
    dsc_info.index = 0;
    vec96.push_back(dsc_info);

    it = dst_map.find("nodec6");
    vector<desc_info> &vec199 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec199.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec3
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec2");
    dst_name_list.clear();
    dst_name_list.push_back("nodec8");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec3", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec3");
    vector<desc_info> &vec103 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec103.push_back(dsc_info);

    it = dst_map.find("nodec3");
    vector<desc_info> &vec193 = it->second;
    dsc_info.targetname = "nodec8";
    dsc_info.index = 0;
    vec193.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern1(OpDesc, "ElemWise");

    // nodec11
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec8");
    dst_name_list.clear();
    dst_name_list.push_back("nodec12");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec11", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec11");
    vector<desc_info> &vec1011 = it->second;
    dsc_info.targetname = "nodec8";
    dsc_info.index = 0;
    vec1011.push_back(dsc_info);

    it = dst_map.find("nodec11");
    vector<desc_info> &vec1911 = it->second;
    dsc_info.targetname = "nodec12";
    dsc_info.index = 0;
    vec1911.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec12
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec11");
    dst_name_list.clear();
    dst_name_list.push_back("nodec10");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec12", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec12");
    vector<desc_info> &vec1012 = it->second;
    dsc_info.targetname = "nodec11";
    dsc_info.index = 0;
    vec1012.push_back(dsc_info);

    it = dst_map.find("nodec12");
    vector<desc_info> &vec1912 = it->second;
    dsc_info.targetname = "nodec10";
    dsc_info.index = 0;
    vec1912.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec9
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec8");
    dst_name_list.clear();
    dst_name_list.push_back("nodec10");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec9", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec9");
    vector<desc_info> &vec1909 = it->second;
    dsc_info.targetname = "nodec8";
    dsc_info.index = 0;
    vec1909.push_back(dsc_info);

    it = dst_map.find("nodec9");
    vector<desc_info> &vec1999 = it->second;
    dsc_info.targetname = "nodec10";
    dsc_info.index = 0;
    vec1999.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec8
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec3");
    dst_name_list.clear();
    dst_name_list.push_back("nodec9");
    dst_name_list.push_back("nodec11");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec8", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec8");
    vector<desc_info> &vec108 = it->second;
    dsc_info.targetname = "nodec3";
    dsc_info.index = 0;
    vec108.push_back(dsc_info);

    it = dst_map.find("nodec8");
    vector<desc_info> &vec1938 = it->second;
    dsc_info.targetname = "nodec9";
    dsc_info.index = 0;
    vec1938.push_back(dsc_info);

    dsc_info.targetname = "nodec11";
    dsc_info.index = 0;
    vec1938.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec4
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 209, 209, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("multiOutput");
    dst_name_list.clear();
    dst_name_list.push_back("nodec6");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec4", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec4");
    vector<desc_info> &vec11 = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec11.push_back(dsc_info);

    it = dst_map.find("nodec4");
    vector<desc_info> &vec100 = it->second;
    dsc_info.targetname = "nodec6";
    dsc_info.index = 0;
    vec100.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern1(OpDesc, "ElemWise");

    // nodec7
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 209, 209, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("multiOutput");
    dst_name_list.clear();
    dst_name_list.push_back("nodec2");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec7", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec7");
    vector<desc_info> &vec17 = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec17.push_back(dsc_info);

    it = dst_map.find("nodec7");
    vector<desc_info> &vec101 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec101.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    CreateModelGraph(model_graph, op_list, src_map, dst_map);

    check_result(model_graph, i, res[i]);

  }
}

/***************************************************************************
 *                                 nodec11------->--nodec12
 *                                   ^                |
 *                                   |                V
 *            nodec5     nodec4--->nodec8->nodec9->nodec10---->nodec6
 *             |          ^                                       |
 *             V          |                                       V
 *  noder-->nodee-->multi_output->nodec7------------------------->nodec2--->nodec3
 *   ^
 *   |
 *  nodec1
*****************************************************************************/
TEST_F(UBFUSION_ST, fusion_test_tefusion_bifurcated_with_circle_nesting)
{

  srand(1);
  vector<set<set<string>>> res{
      {{"nodec8", "nodec9", "nodec11", "nodec12"}, {"nodec2", "nodec3"}, {"nodec1", "noder"}, {"nodec5", "nodee"}}, // 0
      {{"nodec4", "nodec8", "nodec9", "nodec11", "nodec12"}, {"nodee", "multiOutput"}, {"nodec1", "noder"},
       {"nodec7", "nodec6", "nodec2"}}, // 1
      {{"nodec8", "nodec9", "nodec11", "nodec12"}, {"nodee", "multiOutput"}, {"nodec10", "nodec6"}, {"nodec1", "noder"},
       {"nodec7", "nodec2"}}, // 2
      {{"nodec8", "nodec9", "nodec11", "nodec12"}, {"nodec1", "noder"}, {"nodec7", "nodec2", "nodec6"},
       {"nodec5", "nodee"}}, // 3
      {{"nodec1", "noder", "nodee", "multiOutput", "nodec7", "nodec4", "nodec8", "nodec11", "nodec12"},
       {"nodec2", "nodec3"}}, // 4
      {{"nodec8", "nodec9", "nodec11", "nodec12", "nodec10", "nodec6", "nodec2"}, {"nodec1", "noder"},
       {"nodec5", "nodee"}}, // 5
      {{"nodec8", "nodec9", "nodec11", "nodec12"}, {"nodec1", "noder"}, {"nodec7", "nodec2", "nodec6"},
       {"nodec5", "nodee"}}, // 6
      {{"nodec8", "nodec9", "nodec11", "nodec12"}, {"nodec2", "nodec3"}, {"nodec1", "noder"}, {"nodec5", "nodee"}}, // 7
      {{"nodec8", "nodec9", "nodec11", "nodec12"}, {"nodec2", "nodec3"}, {"nodec1", "noder"}, {"nodec5", "nodee"}}, // 8
      {{"nodec8", "nodec9", "nodec11", "nodec12", "nodec10", "nodec6", "nodec2"}, {"nodee", "multiOutput"},
       {"nodec1", "noder"}}, // 9
  };


  for (int i = 0; i < LOOPTIMES; i++) {
    // initial node
    ComputeGraphPtr model_graph = std::make_shared<ComputeGraph>("modelgraph");

    map<string, vector<desc_info>> src_map;
    map<string, vector<desc_info>> dst_map;
    map<string, vector<desc_info>>::iterator it;
    desc_info dsc_info;
    vector<desc_info> desc_tmp;
    desc_tmp.clear();

    src_map.insert(pair<string, vector<desc_info>> ("nodec1", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec2", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec3", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec4", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec5", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec6", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec7", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec8", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec9", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec10", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec11", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec12", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("noder", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodee", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("multiOutput", desc_tmp));

    dst_map.insert(pair<string, vector<desc_info>> ("nodec1", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec2", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec3", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec4", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec5", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec6", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec7", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec8", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec9", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec10", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec11", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec12", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("noder", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodee", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("multiOutput", desc_tmp));

    OpDescPtr OpDesc;
    vector<OpDescPtr> op_list;
    vector<string> src_name_list;
    vector<string> dst_name_list;
    GeTensorDesc input_desc;
    GeTensorDesc input_desc1;
    GeTensorDesc input_desc2;
    GeTensorDesc output_desc;
    GeTensorDesc output_desc1;
    GeTensorDesc output_desc2;
    vector<GeTensorDesc> input_desc_list;
    vector<GeTensorDesc> output_desc_list;

    // nodec1
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 160, 160, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    dst_name_list.clear();
    dst_name_list.push_back("noder");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec1", "Data", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    op_list.push_back(OpDesc);

    it = dst_map.find("nodec1");
    vector<desc_info> &vec1 = it->second;
    dsc_info.targetname = "noder";
    dsc_info.index = 0;
    vec1.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec5
    filltensordesc(input_desc, 1, 16, 500, 500, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 170, 170, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    dst_name_list.clear();
    dst_name_list.push_back("nodee");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec5", "Data", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    op_list.push_back(OpDesc);

    it = dst_map.find("nodec5");
    vector<desc_info> &vec2 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec2.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // noder
    filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 180, 180, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec1");
    dst_name_list.clear();
    dst_name_list.push_back("nodee");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("noder", "cov", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);


    it = dst_map.find("noder");
    vector<desc_info> &vec3 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec3.push_back(dsc_info);

    it = src_map.find("noder");
    vector<desc_info> &vec4 = it->second;
    dsc_info.targetname = "nodec1";
    dsc_info.index = 0;
    vec4.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodee
    filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 170, 170, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 190, 190, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec5");
    src_name_list.push_back("noder");
    dst_name_list.clear();
    dst_name_list.push_back("multiOutput");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    input_desc_list.push_back(input_desc1);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodee", "nodee", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);

    op_list.push_back(OpDesc);

    it = src_map.find("nodee");
    vector<desc_info> &vec5 = it->second;
    dsc_info.targetname = "nodec5";
    dsc_info.index = 0;
    vec5.push_back(dsc_info);

    dsc_info.targetname = "noder";
    dsc_info.index = 1;
    vec5.push_back(dsc_info);

    it = dst_map.find("nodee");
    vector<desc_info> &vec6 = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec6.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // multi_output
    filltensordesc(input_desc, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodee");
    dst_name_list.clear();
    dst_name_list.push_back("nodec7");
    dst_name_list.push_back("nodec4");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("multiOutput", "multiOutput", src_name_list,
                                 dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);

    op_list.push_back(OpDesc);

    it = dst_map.find("multiOutput");
    vector<desc_info> &vec7 = it->second;
    dsc_info.targetname = "nodec7";
    dsc_info.index = 0;
    vec7.push_back(dsc_info);
    dsc_info.targetname = "nodec4";
    dsc_info.index = 0;
    vec7.push_back(dsc_info);

    it = src_map.find("multiOutput");
    vector<desc_info> &vec8 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec8.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern1(OpDesc, "ElemWise");

    // nodec2
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 200, 200, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 207, 207, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec6");
    src_name_list.push_back("nodec7");
    dst_name_list.clear();
    dst_name_list.push_back("nodec3");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    input_desc_list.push_back(input_desc1);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec2", "nodec2", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec2");
    vector<desc_info> &vec9 = it->second;
    dsc_info.targetname = "nodec6";
    dsc_info.index = 0;
    vec9.push_back(dsc_info);
    dsc_info.targetname = "nodec7";
    dsc_info.index = 1;
    vec9.push_back(dsc_info);

    it = dst_map.find("nodec2");
    vector<desc_info> &vec19 = it->second;
    dsc_info.targetname = "nodec3";
    dsc_info.index = 0;
    vec19.push_back(dsc_info);


    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec10
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 200, 200, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 207, 207, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec9");
    src_name_list.push_back("nodec12");
    dst_name_list.clear();
    dst_name_list.push_back("nodec6");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    input_desc_list.push_back(input_desc1);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec10", "nodec2", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec10");
    vector<desc_info> &vec910 = it->second;
    dsc_info.targetname = "nodec9";
    dsc_info.index = 0;
    vec910.push_back(dsc_info);
    dsc_info.targetname = "nodec12";
    dsc_info.index = 1;
    vec910.push_back(dsc_info);

    it = dst_map.find("nodec10");
    vector<desc_info> &vec1990 = it->second;
    dsc_info.targetname = "nodec6";
    dsc_info.index = 0;
    vec1990.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern1(OpDesc, "ElemWise");

    // nodec6
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 207, 207, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec10");
    dst_name_list.clear();
    dst_name_list.push_back("nodec2");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec6", "nodec6", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec6");
    vector<desc_info> &vec96 = it->second;
    dsc_info.targetname = "nodec10";
    dsc_info.index = 0;
    vec96.push_back(dsc_info);

    it = dst_map.find("nodec6");
    vector<desc_info> &vec199 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec199.push_back(dsc_info);


    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec3
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec2");
    dst_name_list.clear();
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec3", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec3");
    vector<desc_info> &vec103 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec103.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);

    set_pattern1(OpDesc, "ElemWise");

    // nodec11
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec8");
    dst_name_list.clear();
    dst_name_list.push_back("nodec12");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec11", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec11");
    vector<desc_info> &vec1011 = it->second;
    dsc_info.targetname = "nodec8";
    dsc_info.index = 0;
    vec1011.push_back(dsc_info);

    it = dst_map.find("nodec11");
    vector<desc_info> &vec1911 = it->second;
    dsc_info.targetname = "nodec12";
    dsc_info.index = 0;
    vec1911.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec12
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec11");
    dst_name_list.clear();
    dst_name_list.push_back("nodec10");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec12", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec12");
    vector<desc_info> &vec1012 = it->second;
    dsc_info.targetname = "nodec11";
    dsc_info.index = 0;
    vec1012.push_back(dsc_info);

    it = dst_map.find("nodec12");
    vector<desc_info> &vec1912 = it->second;
    dsc_info.targetname = "nodec10";
    dsc_info.index = 0;
    vec1912.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec9
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec8");
    dst_name_list.clear();
    dst_name_list.push_back("nodec10");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec9", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec9");
    vector<desc_info> &vec1909 = it->second;
    dsc_info.targetname = "nodec8";
    dsc_info.index = 0;
    vec1909.push_back(dsc_info);

    it = dst_map.find("nodec9");
    vector<desc_info> &vec1999 = it->second;
    dsc_info.targetname = "nodec10";
    dsc_info.index = 0;
    vec1999.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec8
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec4");
    dst_name_list.clear();
    dst_name_list.push_back("nodec9");
    dst_name_list.push_back("nodec11");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec8", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec8");
    vector<desc_info> &vec108 = it->second;
    dsc_info.targetname = "nodec4";
    dsc_info.index = 0;
    vec108.push_back(dsc_info);

    it = dst_map.find("nodec8");
    vector<desc_info> &vec1938 = it->second;
    dsc_info.targetname = "nodec9";
    dsc_info.index = 0;
    vec1938.push_back(dsc_info);

    dsc_info.targetname = "nodec11";
    dsc_info.index = 0;
    vec1938.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec4
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 209, 209, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("multiOutput");
    dst_name_list.clear();
    dst_name_list.push_back("nodec8");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec4", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec4");
    vector<desc_info> &vec11 = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec11.push_back(dsc_info);

    it = dst_map.find("nodec4");
    vector<desc_info> &vec100 = it->second;
    dsc_info.targetname = "nodec8";
    dsc_info.index = 0;
    vec100.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern1(OpDesc, "ElemWise");

    // nodec7
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 209, 209, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("multiOutput");
    dst_name_list.clear();
    dst_name_list.push_back("nodec2");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec7", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec7");
    vector<desc_info> &vec17 = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec17.push_back(dsc_info);

    it = dst_map.find("nodec7");
    vector<desc_info> &vec101 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec101.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    CreateModelGraph(model_graph, op_list, src_map, dst_map);

    check_result(model_graph, i, res[i]);
  }
}

/************************
*            nodec5 -> nodec4
*             ^         |
*             |         V
*  noder-->nodee-->multi_output-->nodec2
*   ^                   |
*   |                   V
*  nodec1            nodec3
*************************/
TEST_F(UBFUSION_ST, fusion_test_tefusion_close_circle)
{
  srand(1);
  vector<set<set<string>>> res{
      {{"nodec1", "noder", "nodee", "nodec5", "nodec4", "multiOutput", "nodec2", "nodec3"}}, // 0
  };
  for (int i = 0; i < 1; i++) {
    // initial node
    ComputeGraphPtr model_graph = std::make_shared<ComputeGraph>("modelgraph");

    map<string, vector<desc_info>> src_map;
    map<string, vector<desc_info>> dst_map;
    map<string, vector<desc_info>> ::iterator it;
    desc_info dsc_info;
    vector<desc_info> desc_tmp;
    desc_tmp.clear();
    src_map.insert(pair<string, vector<desc_info>> ("nodec1", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec2", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec3", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec4", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec5", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("noder", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodee", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("multiOutput", desc_tmp));

    dst_map.insert(pair<string, vector<desc_info>> ("nodec1", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec2", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec3", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec4", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec5", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("noder", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodee", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("multiOutput", desc_tmp));

    OpDescPtr OpDesc;
    vector<OpDescPtr> op_list;
    vector<string> src_name_list;
    vector<string> dst_name_list;
    GeTensorDesc input_desc;
    GeTensorDesc input_desc1;
    GeTensorDesc input_desc2;
    GeTensorDesc output_desc;
    GeTensorDesc output_desc1;
    GeTensorDesc output_desc2;
    vector<GeTensorDesc> input_desc_list;
    vector<GeTensorDesc> output_desc_list;

    // nodec1
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    dst_name_list.clear();
    dst_name_list.push_back("noder");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec1", "Data", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    op_list.push_back(OpDesc);

    it = dst_map.find("nodec1");
    vector<desc_info> &vec1 = it->second;
    dsc_info.targetname = "noder";
    dsc_info.index = 0;
    vec1.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec5
    filltensordesc(input_desc, 1, 16, 500, 500, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 170, 170, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodee");
    dst_name_list.clear();
    dst_name_list.push_back("nodec4");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec5", "Data", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = dst_map.find("nodec5");
    vector<desc_info> &vec2 = it->second;
    dsc_info.targetname = "nodec4";
    dsc_info.index = 0;
    vec2.push_back(dsc_info);

    it = src_map.find("nodec5");
    vector<desc_info> &vec4e = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec4e.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // noder
    filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 180, 180, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec1");
    dst_name_list.clear();
    dst_name_list.push_back("nodee");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("noder", "noder", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = dst_map.find("noder");
    vector<desc_info> &vec3 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec3.push_back(dsc_info);

    it = src_map.find("noder");
    vector<desc_info> &vec4 = it->second;
    dsc_info.targetname = "nodec1";
    dsc_info.index = 0;
    vec4.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodee
    filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 170, 170, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(input_desc2, 1, 16, 180, 180, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("noder");
    dst_name_list.clear();
    dst_name_list.push_back("multiOutput");
    dst_name_list.push_back("nodec5");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodee", "nodee", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);

    op_list.push_back(OpDesc);

    it = src_map.find("nodee");
    vector<desc_info> &vec5 = it->second;
    dsc_info.targetname = "noder";
    dsc_info.index = 0;
    vec5.push_back(dsc_info);

    it = dst_map.find("nodee");
    vector<desc_info> &vec6 = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec6.push_back(dsc_info);

    dsc_info.targetname = "nodec5";
    dsc_info.index = 0;
    vec6.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // multi_output
    filltensordesc(input_desc, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodee");
    src_name_list.push_back("nodec4");
    dst_name_list.clear();
    dst_name_list.push_back("nodec2");
    dst_name_list.push_back("nodec3");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    input_desc_list.push_back(input_desc1);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("multiOutput", "multiOutput", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = dst_map.find("multiOutput");
    vector<desc_info> &vec7 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec7.push_back(dsc_info);

    dsc_info.targetname = "nodec3";
    dsc_info.index = 0;
    vec7.push_back(dsc_info);

    it = src_map.find("multiOutput");
    vector<desc_info> &vec8 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec8.push_back(dsc_info);

    dsc_info.targetname = "nodec4";
    dsc_info.index = 1;
    vec8.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec2
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 207, 207, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("multiOutput");
    dst_name_list.clear();
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec2", "nodec2", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec2");
    vector<desc_info> &vec9 = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec9.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec3
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("multiOutput");
    dst_name_list.clear();
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec3", "nodec", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec3");
    vector<desc_info> &vec10 = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec10.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec4
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 209, 209, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec5");
    dst_name_list.clear();
    dst_name_list.push_back("multiOutput");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec4", "nodec", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec4");
    vector<desc_info> &vec11 = it->second;
    dsc_info.targetname = "nodec5";
    dsc_info.index = 0;
    vec11.push_back(dsc_info);

    it = dst_map.find("nodec4");
    vector<desc_info> &vec1c = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec1c.push_back(dsc_info);


    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    CreateModelGraph(model_graph, op_list, src_map, dst_map);
    check_result(model_graph, i, res[i]);
  }
}

/************************
*
*  noder-->nodec1-->nodec2-->nodec3-->nodec4
*
*************************/
TEST_F(UBFUSION_ST, fusion_test_tefusion_conv_fusion) {

  srand(1);
  vector<set<set<string>>> res{
      {{"noder", "nodec1", "nodec2", "nodec3"}},// 0
  };
  for (int i = 0; i < 1; i++) {
    // initial node
    ComputeGraphPtr model_graph = std::make_shared<ComputeGraph>("modelgraph");

    map<string, vector<desc_info>> src_map;
    map<string, vector<desc_info>> dst_map;
    map<string, vector<desc_info>>::iterator it;
    desc_info dsc_info;
    vector<desc_info> desc_tmp;
    desc_tmp.clear();
    src_map.insert(pair<string, vector<desc_info>>("noder", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>>("nodec1", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>>("nodec2", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>>("nodec3", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>>("nodec4", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>>("nodec5", desc_tmp));

    dst_map.insert(pair<string, vector<desc_info>>("noder", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>>("nodec1", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>>("nodec2", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>>("nodec3", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>>("nodec4", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>>("nodec5", desc_tmp));

    OpDescPtr OpDesc;
    vector<OpDescPtr> op_list;
    vector<string> src_name_list;
    vector<string> dst_name_list;
    GeTensorDesc input_desc;
    GeTensorDesc input_desc1;
    GeTensorDesc input_desc2;
    GeTensorDesc output_desc;
    GeTensorDesc output_desc1;
    GeTensorDesc output_desc2;
    vector<GeTensorDesc> input_desc_list;
    vector<GeTensorDesc> output_desc_list;

    // noder
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    dst_name_list.clear();
    dst_name_list.push_back("nodec1");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("noder", "Data", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    op_list.push_back(OpDesc);

    it = dst_map.find("noder");
    vector<desc_info> &vec1 = it->second;
    dsc_info.targetname = "nodec1";
    dsc_info.index = 0;
    vec1.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "Convolution");

    // nodec1
    filltensordesc(input_desc, 1, 16, 500, 500, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 170, 170, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("noder");
    dst_name_list.clear();
    dst_name_list.push_back("nodec2");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec1", "Eltwise", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec1");
    vector<desc_info> &vec1r = it->second;
    dsc_info.targetname = "noder";
    dsc_info.index = 0;
    vec1r.push_back(dsc_info);

    it = dst_map.find("nodec1");
    vector<desc_info> &vec2 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec2.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec2
    filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 180, 180, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec1");
    dst_name_list.clear();
    dst_name_list.push_back("nodec3");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec2", "Eltwise", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec2");
    vector<desc_info> &vec2r = it->second;
    dsc_info.targetname = "nodec1";
    dsc_info.index = 0;
    vec2r.push_back(dsc_info);

    it = dst_map.find("nodec2");
    vector<desc_info> &vec3 = it->second;
    dsc_info.targetname = "nodec3";
    dsc_info.index = 0;
    vec3.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec3
    filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 170, 170, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(input_desc2, 1, 16, 180, 180, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec2");
    dst_name_list.clear();
    dst_name_list.push_back("nodec4");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec3", "Eltwise", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);

    op_list.push_back(OpDesc);

    it = src_map.find("nodec3");
    vector<desc_info> &vec5 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec5.push_back(dsc_info);

    it = dst_map.find("nodec3");
    vector<desc_info> &vec34 = it->second;
    dsc_info.targetname = "nodec4";
    dsc_info.index = 0;
    vec34.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec4
    filltensordesc(input_desc, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec3");
    dst_name_list.clear();
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    OpDesc = CreateOpDefUbFusion("nodec4", "nodec4", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec4");
    vector<desc_info> &vec8 = it->second;
    dsc_info.targetname = "nodec3";
    dsc_info.index = 0;
    vec8.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "aipp");

    // nodec5
    filltensordesc(input_desc, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec4");
    dst_name_list.clear();
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    OpDesc = CreateOpDefUbFusion("nodec5", "nodec5", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec5");
    vector<desc_info> &vec9 = it->second;
    dsc_info.targetname = "nodec4";
    dsc_info.index = 0;
    vec9.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "aipp");

    CreateModelGraph(model_graph, op_list, src_map, dst_map);
    check_result(model_graph, i, res[i]);
  }
}

/************************
*
*  noder-->nodec1-->nodec2-->nodec3-->nodec4
*
*************************/
TEST_F(UBFUSION_ST, fusion_test_tefusion_conv_fusion2)
{

  srand(1);
  vector<set<set<string>>> res {
      {{"noder", "nodec1", "nodec2", "nodec3"}},// 0
  };
  for (int i = 0; i < 1; i++) {
    // initial node
    ComputeGraphPtr model_graph = std::make_shared<ComputeGraph>("modelgraph");

    map<string, vector<desc_info>> src_map;
    map<string, vector<desc_info>> dst_map;
    map<string, vector<desc_info>>::iterator it;
    desc_info dsc_info;
    vector<desc_info> desc_tmp;
    desc_tmp.clear();
    src_map.insert(pair<string, vector<desc_info>>("noder", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>>("nodec1", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>>("nodec2", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>>("nodec3", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>>("nodec4", desc_tmp));

    dst_map.insert(pair<string, vector<desc_info>>("noder", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>>("nodec1", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>>("nodec2", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>>("nodec3", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>>("nodec4", desc_tmp));

    OpDescPtr OpDesc;
    vector<OpDescPtr> op_list;
    vector<string> src_name_list;
    vector<string> dst_name_list;
    GeTensorDesc input_desc;
    GeTensorDesc input_desc1;
    GeTensorDesc input_desc2;
    GeTensorDesc output_desc;
    GeTensorDesc output_desc1;
    GeTensorDesc output_desc2;
    vector<GeTensorDesc> input_desc_list;
    vector<GeTensorDesc> output_desc_list;

    // noder
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    dst_name_list.clear();
    dst_name_list.push_back("nodec1");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("noder", "Data", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    op_list.push_back(OpDesc);

    it = dst_map.find("noder");
    vector<desc_info> &vec1 = it->second;
    dsc_info.targetname = "nodec1";
    dsc_info.index = 0;
    vec1.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "Convolution");

    // nodec1
    filltensordesc(input_desc, 1, 16, 500, 500, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 170, 170, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("noder");
    dst_name_list.clear();
    dst_name_list.push_back("nodec2");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec1", "nodec1", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec1");
    vector<desc_info> &vec1r = it->second;
    dsc_info.targetname = "noder";
    dsc_info.index = 0;
    vec1r.push_back(dsc_info);

    it = dst_map.find("nodec1");
    vector<desc_info> &vec2 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec2.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec2
    filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 180, 180, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec1");
    dst_name_list.clear();
    dst_name_list.push_back("nodec3");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec2", "nodec2", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec2");
    vector<desc_info> &vec2r = it->second;
    dsc_info.targetname = "nodec1";
    dsc_info.index = 0;
    vec2r.push_back(dsc_info);

    it = dst_map.find("nodec2");
    vector<desc_info> &vec3 = it->second;
    dsc_info.targetname = "nodec3";
    dsc_info.index = 0;
    vec3.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec3
    filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 170, 170, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(input_desc2, 1, 16, 180, 180, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec2");
    dst_name_list.clear();
    dst_name_list.push_back("nodec4");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec3", "nodec3", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);

    op_list.push_back(OpDesc);

    it = src_map.find("nodec3");
    vector<desc_info> &vec5 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec5.push_back(dsc_info);

    it = dst_map.find("nodec3");
    vector<desc_info> &vec34 = it->second;
    dsc_info.targetname = "nodec4";
    dsc_info.index = 0;
    vec34.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec4
    filltensordesc(input_desc, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec3");
    dst_name_list.clear();
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    OpDesc = CreateOpDefUbFusion("nodec4", "nodec4", src_name_list, dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec4");
    vector<desc_info> &vec8 = it->second;
    dsc_info.targetname = "nodec3";
    dsc_info.index = 0;
    vec8.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    CreateModelGraph(model_graph, op_list, src_map, dst_map);

    std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
    graph_comm_ptr->Initialize();
    std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
    std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
        std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr, nullptr);
    std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr = std::make_shared<BufferFusion>(graph_comm_ptr,
                                                                                        fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);


    // find sub-graphs that match UB fusion pattern
    //ComputeGraphPtr origin_graph_ptr = std::make_shared<ComputeGraph>(*model_graph);
    sub_graph_optimizer_ptr->MatchFusionPatternFromGraph(*model_graph);

    // create fused Graph, and merge matched sub-graphs into fusion ops
    sub_graph_optimizer_ptr->BuildFusionGraph(*model_graph);

    uint32_t id = 0;

    cerr << endl;
    cerr << "UB fusion befre" << endl;
    for (auto node : model_graph->GetDirectNode())
    {
      cerr << " id:" << id << endl;
      uint32_t scope_id = 0;
      cerr << "opdef : " << node->GetType() << endl;
      if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id))
      {
        cerr << "scope id : " << scope_id << endl;
      }
      id++;

    }
    id = 0;
    cerr << endl;
    cerr << "UB fusion result" << endl;
    for (auto node : model_graph->GetDirectNode())
    {
      cerr << " id:" << id << endl;
      uint32_t scope_id = 0;
      cerr << "opdef : " << node->GetType() << endl;
      if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id))
      {
        cerr << "scope id : " << scope_id << endl;
      }
      id++;

    }
  }
}

/***************************************************************************
 *                                 nodec11------->--nodec12
 *                                   ^                |
 *                                   |                V
 *            nodec5     nodec4--->nodec8->nodec9->nodec10---->nodec6
 *             |          ^                                       |
 *             V          |                                       V
 *  noder-->nodee-->multi_output->nodec7------------------------->nodec2--->nodec3
 *   ^
 *   |
 *  nodec1
*****************************************************************************/
TEST_F(UBFUSION_ST, fusion_test_tefusion_bifurcated_with_circle_nesting2)
{

  srand(1);
  vector<set<set<string>>> res{
      {{"nodec1", "noder"}, {"nodee",   "multiOutput"}, {"nodec4", "nodec8"}, {"nodec2",  "nodec3"}, {"nodec11", "nodec12"}},// 0
      {{"nodec1", "noder"}, {"nodee",   "multiOutput"}, {"nodec4", "nodec8"}, {"nodec10",  "nodec6"}, {"nodec11", "nodec12"}},// 1
      {{"nodec1", "noder"}, {"nodee",   "multiOutput"}, {"nodec4", "nodec8"}, {"nodec10",  "nodec6"}, {"nodec11", "nodec12"}},// 2
      {{"nodec1", "noder"}, {"nodec10",  "nodec6"}, {"nodec11", "nodec12"}, {"nodec2",  "nodec3"}},// 3
      {{"nodec1", "noder"}, {"nodee",   "multiOutput"}, {"nodec4", "nodec8"}, {"nodec2",  "nodec3"}, {"nodec11", "nodec12"}},// 4
      {{"nodec1", "noder"}, {"nodee",   "multiOutput"}, {"nodec4", "nodec8"}, {"nodec2",  "nodec3"}, {"nodec11", "nodec12"}, {"nodec10",  "nodec6"}},// 5
      {{"nodec1", "noder"}, {"nodec12", "nodec11"},     {"nodec4", "nodec8"}},// 6
      {{"nodec1", "noder"}, {"nodee",   "multiOutput"}, {"nodec10",  "nodec6"}, {"nodec2",  "nodec3"}, {"nodec11", "nodec12"}},// 7
      {{"nodec1", "noder"}, {"nodec3",  "nodec2"},      {"nodec4", "nodec8"}, {"nodec11", "nodec12"}},// 8
      {{"nodec1", "noder"}, {"nodee",   "multiOutput"}, {"nodec10",  "nodec6"}, {"nodec2",  "nodec3"}, {"nodec11", "nodec12"}},// 9
  };

  for (int i = 0; i < 1; i++) {
    // initial node
    ComputeGraphPtr model_graph = std::make_shared<ComputeGraph>("modelgraph");

    map<string, vector<desc_info>> src_map;
    map<string, vector<desc_info>> dst_map;
    map<string, vector<desc_info>>::iterator it;
    desc_info dsc_info;
    vector<desc_info> desc_tmp;
    desc_tmp.clear();

    src_map.insert(pair<string, vector<desc_info>> ("nodec1", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec2", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec3", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec4", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec5", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec6", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec7", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec8", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec9", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec10", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec11", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodec12", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("noder", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("nodee", desc_tmp));
    src_map.insert(pair<string, vector<desc_info>> ("multiOutput", desc_tmp));

    dst_map.insert(pair<string, vector<desc_info>> ("nodec1", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec2", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec3", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec4", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec5", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec6", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec7", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec8", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec9", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec10", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec11", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodec12", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("noder", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("nodee", desc_tmp));
    dst_map.insert(pair<string, vector<desc_info>> ("multiOutput", desc_tmp));

    OpDescPtr OpDesc;
    vector<OpDescPtr> op_list;
    vector<string> src_name_list;
    vector<string> dst_name_list;
    GeTensorDesc input_desc;
    GeTensorDesc input_desc1;
    GeTensorDesc input_desc2;
    GeTensorDesc output_desc;
    GeTensorDesc output_desc1;
    GeTensorDesc output_desc2;
    vector<GeTensorDesc> input_desc_list;
    vector<GeTensorDesc> output_desc_list;

    // nodec1
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 160, 160, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    dst_name_list.clear();
    dst_name_list.push_back("noder");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec1", "Data", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    op_list.push_back(OpDesc);

    it = dst_map.find("nodec1");
    vector<desc_info> &vec1 = it->second;
    dsc_info.targetname = "noder";
    dsc_info.index = 0;
    vec1.push_back(dsc_info);


    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec5
    filltensordesc(input_desc, 1, 16, 500, 500, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 170, 170, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    dst_name_list.clear();
    dst_name_list.push_back("nodee");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec5", "Data", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    op_list.push_back(OpDesc);

    it = dst_map.find("nodec5");
    vector<desc_info> &vec2 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec2.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // noder
    filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 180, 180, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec1");
    dst_name_list.clear();
    dst_name_list.push_back("nodee");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("noder", "cov", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);


    it = dst_map.find("noder");
    vector<desc_info> &vec3 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec3.push_back(dsc_info);

    it = src_map.find("noder");
    vector<desc_info> &vec4 = it->second;
    dsc_info.targetname = "nodec1";
    dsc_info.index = 0;
    vec4.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodee
    filltensordesc(input_desc, 1, 16, 160, 160, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 170, 170, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 190, 190, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec5");
    src_name_list.push_back("noder");
    dst_name_list.clear();
    dst_name_list.push_back("multiOutput");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    input_desc_list.push_back(input_desc1);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodee", "nodee", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);

    op_list.push_back(OpDesc);

    it = src_map.find("nodee");
    vector<desc_info> &vec5 = it->second;
    dsc_info.targetname = "nodec5";
    dsc_info.index = 0;
    vec5.push_back(dsc_info);

    dsc_info.targetname = "noder";
    dsc_info.index = 1;
    vec5.push_back(dsc_info);

    it = dst_map.find("nodee");
    vector<desc_info> &vec6 = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec6.push_back(dsc_info);
    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // multi_output
    filltensordesc(input_desc, 1, 16, 190, 190, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 200, 200, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodee");
    dst_name_list.clear();
    dst_name_list.push_back("nodec7");
    dst_name_list.push_back("nodec4");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("multiOutput", "multiOutput", src_name_list,
                                 dst_name_list, input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);

    op_list.push_back(OpDesc);

    it = dst_map.find("multiOutput");
    vector<desc_info> &vec7 = it->second;
    dsc_info.targetname = "nodec7";
    dsc_info.index = 0;
    vec7.push_back(dsc_info);
    dsc_info.targetname = "nodec4";
    dsc_info.index = 0;
    vec7.push_back(dsc_info);

    it = src_map.find("multiOutput");
    vector<desc_info> &vec8 = it->second;
    dsc_info.targetname = "nodee";
    dsc_info.index = 0;
    vec8.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern1(OpDesc, "ElemWise");

    // nodec2
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 200, 200, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 207, 207, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec6");
    src_name_list.push_back("nodec7");
    dst_name_list.clear();
    dst_name_list.push_back("nodec3");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    input_desc_list.push_back(input_desc1);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec2", "nodec2", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec2");
    vector<desc_info> &vec9 = it->second;
    dsc_info.targetname = "nodec6";
    dsc_info.index = 0;
    vec9.push_back(dsc_info);
    dsc_info.targetname = "nodec7";
    dsc_info.index = 1;
    vec9.push_back(dsc_info);

    it = dst_map.find("nodec2");
    vector<desc_info> &vec19 = it->second;
    dsc_info.targetname = "nodec3";
    dsc_info.index = 0;
    vec19.push_back(dsc_info);


    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec10
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(input_desc1, 1, 16, 200, 200, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 207, 207, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec9");
    src_name_list.push_back("nodec12");
    dst_name_list.clear();
    dst_name_list.push_back("nodec6");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    input_desc_list.push_back(input_desc1);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec10", "nodec2", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec10");
    vector<desc_info> &vec910 = it->second;
    dsc_info.targetname = "nodec9";
    dsc_info.index = 0;
    vec910.push_back(dsc_info);
    dsc_info.targetname = "nodec12";
    dsc_info.index = 1;
    vec910.push_back(dsc_info);

    it = dst_map.find("nodec10");
    vector<desc_info> &vec1990 = it->second;
    dsc_info.targetname = "nodec6";
    dsc_info.index = 0;
    vec1990.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern1(OpDesc, "ElemWise");

    // nodec6
    filltensordesc(input_desc, 1, 16, 200, 200, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 207, 207, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec10");
    dst_name_list.clear();
    dst_name_list.push_back("nodec2");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec6", "nodec6", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec6");
    vector<desc_info> &vec96 = it->second;
    dsc_info.targetname = "nodec10";
    dsc_info.index = 0;
    vec96.push_back(dsc_info);

    it = dst_map.find("nodec6");
    vector<desc_info> &vec199 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec199.push_back(dsc_info);


    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec3
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec2");
    dst_name_list.clear();
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec3", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec3");
    vector<desc_info> &vec103 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec103.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);

    set_pattern1(OpDesc, "ElemWise");

    // nodec11
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec8");
    dst_name_list.clear();
    dst_name_list.push_back("nodec12");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec11", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec11");
    vector<desc_info> &vec1011 = it->second;
    dsc_info.targetname = "nodec8";
    dsc_info.index = 0;
    vec1011.push_back(dsc_info);

    it = dst_map.find("nodec11");
    vector<desc_info> &vec1911 = it->second;
    dsc_info.targetname = "nodec12";
    dsc_info.index = 0;
    vec1911.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec12
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec11");
    dst_name_list.clear();
    dst_name_list.push_back("nodec10");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec12", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec12");
    vector<desc_info> &vec1012 = it->second;
    dsc_info.targetname = "nodec11";
    dsc_info.index = 0;
    vec1012.push_back(dsc_info);

    it = dst_map.find("nodec12");
    vector<desc_info> &vec1912 = it->second;
    dsc_info.targetname = "nodec10";
    dsc_info.index = 0;
    vec1912.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec9
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec8");
    dst_name_list.clear();
    dst_name_list.push_back("nodec10");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec9", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec9");
    vector<desc_info> &vec1909 = it->second;
    dsc_info.targetname = "nodec8";
    dsc_info.index = 0;
    vec1909.push_back(dsc_info);

    it = dst_map.find("nodec9");
    vector<desc_info> &vec1999 = it->second;
    dsc_info.targetname = "nodec10";
    dsc_info.index = 0;
    vec1999.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec8
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16, FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 208, 208, DT_FLOAT16, FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("nodec4");
    dst_name_list.clear();
    dst_name_list.push_back("nodec9");
    dst_name_list.push_back("nodec11");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec8", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec8");
    vector<desc_info> &vec108 = it->second;
    dsc_info.targetname = "nodec4";
    dsc_info.index = 0;
    vec108.push_back(dsc_info);

    it = dst_map.find("nodec8");
    vector<desc_info> &vec1938 = it->second;
    dsc_info.targetname = "nodec9";
    dsc_info.index = 0;
    vec1938.push_back(dsc_info);

    dsc_info.targetname = "nodec11";
    dsc_info.index = 0;
    vec1938.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    // nodec4
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 209, 209, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("multiOutput");
    dst_name_list.clear();
    dst_name_list.push_back("nodec8");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec4", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec4");
    vector<desc_info> &vec11 = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec11.push_back(dsc_info);

    it = dst_map.find("nodec4");
    vector<desc_info> &vec100 = it->second;
    dsc_info.targetname = "nodec8";
    dsc_info.index = 0;
    vec100.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    set_pattern1(OpDesc, "ElemWise");

    // nodec7
    filltensordesc(input_desc, 1, 16, 201, 201, DT_FLOAT16,
                   FORMAT_NCHW);
    filltensordesc(output_desc, 1, 16, 209, 209, DT_FLOAT16,
                   FORMAT_NCHW);
    src_name_list.clear();
    src_name_list.push_back("multiOutput");
    dst_name_list.clear();
    dst_name_list.push_back("nodec2");
    input_desc_list.clear();
    input_desc_list.push_back(input_desc);
    output_desc_list.clear();
    output_desc_list.push_back(output_desc);
    OpDesc = CreateOpDefUbFusion("nodec7", "nodec", src_name_list, dst_name_list,
                                 input_desc_list, output_desc_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_DST_NAME, dst_name_list);
    ge::AttrUtils::SetListStr(OpDesc, OPDESC_SRC_NAME, src_name_list);
    op_list.push_back(OpDesc);

    it = src_map.find("nodec7");
    vector<desc_info> &vec17 = it->second;
    dsc_info.targetname = "multiOutput";
    dsc_info.index = 0;
    vec17.push_back(dsc_info);

    it = dst_map.find("nodec7");
    vector<desc_info> &vec101 = it->second;
    dsc_info.targetname = "nodec2";
    dsc_info.index = 0;
    vec101.push_back(dsc_info);

    SetTvmType(OpDesc);
    SetAICoreOp(OpDesc);
    SetPattern(OpDesc, "ElemWise");

    CreateModelGraph(model_graph, op_list, src_map, dst_map);

    std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
    graph_comm_ptr->Initialize();
    std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
    std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
        std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr, nullptr);
    std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
            std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);

    // find sub-graphs that match UB fusion pattern
    //ComputeGraphPtr origin_graph_ptr = std::make_shared<ComputeGraph>(*model_graph);
    sub_graph_optimizer_ptr->MatchFusionPatternFromGraph(*model_graph);

    // create fused Graph, and merge matched sub-graphs into fusion ops
    sub_graph_optimizer_ptr->BuildFusionGraph(*model_graph);

    uint32_t id = 0;

    cerr << endl;
    cerr << "UB fusion befre" << endl;
    for (auto node : model_graph->GetDirectNode())
    {
      cerr << " id:" << id << endl;
      uint32_t scope_id = 0;
      cerr << "opdef : " << node->GetType() << endl;
      if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id))
      {
        cerr << "scope id : " << scope_id << endl;
      }
      id++;

    }
    id = 0;
    cerr << endl;
    cerr << "UB fusion result" << endl;
    for (auto node : model_graph->GetDirectNode())
    {
      cerr << " id:" << id << endl;
      uint32_t scope_id = 0;
      cerr << "opdef : " << node->GetType() << endl;
      if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id))
      {
        cerr << "scope id : " << scope_id << endl;
      }
      id++;
    }
  }
//}
}

namespace st {
class BuiltBufferFusionPassTest : public BufferFusionPassBase {
 public:
  std::vector<BufferFusionPattern *> DefinePatterns() override {
    return vector<BufferFusionPattern *>{};
  }
};
}

TEST_F(UBFUSION_ST, register_buitin_buffer_fusion_pass)
{
  BufferFusionPassType  type = BUFFER_FUSION_PASS_TYPE_RESERVED;
  REGISTER_BUFFER_FUSION_PASS("BuiltBufferFusionPassTest", type, st::BuiltBufferFusionPassTest);
  size_t size = BufferFusionPassRegistry::GetInstance().GetCreateFnByType(type).size();
  EXPECT_EQ(0, size);
  type = BUILT_IN_AI_CORE_BUFFER_FUSION_PASS;
  REGISTER_BUFFER_FUSION_PASS("", type, st::BuiltBufferFusionPassTest);
  size = BufferFusionPassRegistry::GetInstance().GetCreateFnByType(type).size();
  EXPECT_EQ(0, size);

  REGISTER_BUFFER_FUSION_PASS("test", type, st::BuiltBufferFusionPassTest);
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
      std::make_shared<FusionPriorityManager>("engineName", fusion_pass_mgr_ptr,nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
      std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);
  auto graph = std::make_shared<ComputeGraph>("test");
  std::shared_ptr<ConnectivityMatrix> connectivity = ConnectivityMatrix::Generate(*graph);
  Status status = sub_graph_optimizer_ptr->RunRegisterBufferFusionPass(*graph, type);
  EXPECT_EQ(fe::SUCCESS, status);
}


TEST_F(UBFUSION_ST, create_original_fusion_op_graph) {
  vector<int64_t> dim = {4, -1, 1, 4};
  GeShape shape(dim);
  GeTensorDesc tenosr_desc(shape);
  OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Relu");
  relu_op->AddInputDesc(tenosr_desc);
  relu_op->AddOutputDesc(tenosr_desc);
  relu_op->AddOutputDesc(tenosr_desc);
  ge::AttrUtils::SetBool(relu_op, ATTR_NAME_UNKNOWN_SHAPE, true);

  OpDescPtr relu_op1 = std::make_shared<OpDesc>("relu1", "Relu");
  relu_op1->AddInputDesc(tenosr_desc);
  relu_op1->AddOutputDesc(tenosr_desc);
  relu_op1->AddOutputDesc(tenosr_desc);
  ge::AttrUtils::SetBool(relu_op1, ATTR_NAME_UNKNOWN_SHAPE, true);

  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  ge::NodePtr relu_node1 = graph_out->AddNode(relu_op1);
  ge::NodePtr relu_node = graph_out->AddNode(relu_op);

  std::map<ge::AnchorPtr, ge::AnchorPtr> out_anchor_map;
  out_anchor_map.emplace(relu_node1->GetOutDataAnchor(0), relu_node->GetOutDataAnchor(0));
  out_anchor_map.emplace(relu_node1->GetOutDataAnchor(1), relu_node->GetOutDataAnchor(1));
  std::map<ge::NodePtr, std::map<ge::AnchorPtr, ge::AnchorPtr>> fusion_op_output_anchors_map;
  fusion_op_output_anchors_map.emplace(relu_node1, out_anchor_map);
  relu_op->SetExtAttr("OutEdgeAnchorMap", fusion_op_output_anchors_map);

  vector<ge::NodePtr> fus_nodelist;
  fus_nodelist.push_back(relu_node1);

  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>(AI_CORE_NAME);
  std::unique_ptr<FusionGraphMerge> ub_fusion_graph_merge_ptr_ =
          std::unique_ptr<FusionGraphMerge>(new (std::nothrow) UBFusionGraphMerge(SCOPE_ID_ATTR, graph_comm_ptr));
  ub_fusion_graph_merge_ptr_->CreateOriginalFusionOpGraph(relu_node, fus_nodelist);
}

using BufferFusionFn = BufferFusionPassBase *(*)();

class TestBufferFusionPass : public BufferFusionPassBase {
 protected:

  vector<BufferFusionPattern *> DefinePatterns() override {
    return {};
  };

};

fe::BufferFusionPassBase *BufferFunc() {
  return new(std::nothrow) TestBufferFusionPass();
}

void RegisterBufferFusionFunc(BufferFusionFn create_fn) {
  BufferFusionPassRegistry::GetInstance().RegisterPass(CUSTOM_AI_CORE_BUFFER_FUSION_PASS, "CUSTOM_PASS1", create_fn);
  BufferFusionPassRegistry::GetInstance().RegisterPass(CUSTOM_AI_CORE_BUFFER_FUSION_PASS, "CUSTOM_PASS2", create_fn);
  BufferFusionPassRegistry::GetInstance().RegisterPass(CUSTOM_AI_CORE_BUFFER_FUSION_PASS, "CUSTOM_PASS3", create_fn);

  BufferFusionPassRegistry::GetInstance().RegisterPass(
      CUSTOM_VECTOR_CORE_BUFFER_FUSION_PASS, "CUSTOM_PASS4", create_fn);
  BufferFusionPassRegistry::GetInstance().RegisterPass(
      CUSTOM_VECTOR_CORE_BUFFER_FUSION_PASS, "CUSTOM_PASS5", create_fn);

  BufferFusionPassRegistry::GetInstance().RegisterPass(
      BUILT_IN_AI_CORE_BUFFER_FUSION_PASS, "BUILT_IN_PASS1", create_fn);
  BufferFusionPassRegistry::GetInstance().RegisterPass(
      BUILT_IN_AI_CORE_BUFFER_FUSION_PASS, "BUILT_IN_PASS2", create_fn);

  BufferFusionPassRegistry::GetInstance().RegisterPass(
      BUILT_IN_VECTOR_CORE_BUFFER_FUSION_PASS, "BUILT_IN_PASS3", create_fn);
  BufferFusionPassRegistry::GetInstance().RegisterPass(
      BUILT_IN_VECTOR_CORE_BUFFER_FUSION_PASS, "BUILT_IN_PASS4", create_fn);


}

TEST_F(UBFUSION_ST, converage_09) {
  auto op_store_adapter_manager = std::make_shared<OpStoreAdapterManager>();
  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  op_store_adapter_manager->map_all_op_store_adapter_.emplace(
      std::make_pair("tbe_op_adapter", tbe_adapter_ptr));
  std::map<std::string, std::string> options;
  auto fe_ops_kernel_info_store = make_shared<fe::FEOpsKernelInfoStore>(
      op_store_adapter_manager);
  FEOpsStoreInfo heavy_op_info{
      6,
      "tbe-builtin",
      EN_IMPL_HW_TBE,
      "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/fusion_rule_manager",
      "",
      false,
      false};

  vector<FEOpsStoreInfo> store_info;
  store_info.emplace_back(heavy_op_info);
  Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
  OpsKernelManager::Instance(AI_CORE_NAME).Finalize();

  string file_path =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_rule_parser/cycle_detection.json";
  fe_ops_kernel_info_store->Initialize(options);
  auto fusion_rule_mgr = std::make_shared<FusionRuleManager>(fe_ops_kernel_info_store);

  auto fusion_pass_mgr = std::make_shared<FusionPassManager>();
  auto fusion_priority_mgr =
      std::make_shared<FusionPriorityManager>(AI_CORE_NAME, fusion_pass_mgr, fusion_rule_mgr);
  auto fusion_priority_mgr_vec =
      std::make_shared<FusionPriorityManager>(VECTOR_CORE_NAME, fusion_pass_mgr, fusion_rule_mgr);

  Configuration::Instance(fe::AI_CORE_NAME).lib_path_ =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_config_manager/builtin_config3/";
  Configuration::Instance(fe::AI_CORE_NAME).custom_fusion_config_file_ =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_config_manager/custom_config/fusion_config3.json";
  fusion_priority_mgr->fusion_config_parser_ptr_ = std::make_shared<FusionConfigParser>(fe::AI_CORE_NAME);
  fusion_priority_mgr->fusion_config_parser_ptr_->ParseFusionConfigFile();

  Configuration::Instance(fe::VECTOR_CORE_NAME).lib_path_ =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_config_manager/builtin_config3/";
  Configuration::Instance(fe::VECTOR_CORE_NAME).custom_fusion_config_file_ =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_config_manager/custom_config/fusion_config3.json";
  fusion_priority_mgr_vec->fusion_config_parser_ptr_ = std::make_shared<FusionConfigParser>(fe::VECTOR_CORE_NAME);
  fusion_priority_mgr_vec->fusion_config_parser_ptr_->ParseFusionConfigFile();

  RegisterBufferFusionFunc(BufferFunc);
  EXPECT_EQ(fe::SUCCESS, fusion_priority_mgr->SortBufferFusion());
  EXPECT_EQ(fe::SUCCESS, fusion_priority_mgr->SortBufferFusion());
  EXPECT_EQ(fe::SUCCESS, fusion_priority_mgr_vec->SortBufferFusion());
  auto &instance = BufferFusionPassRegistry::GetInstance();
  instance.~BufferFusionPassRegistry();
  OpsKernelManager::Instance(AI_CORE_NAME).Finalize();
}

TEST_F(UBFUSION_ST, change_scope_id_test) {
  int64_t old_scope_id = 0;
  int64_t new_scope_id = 2;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  ge::OpDescPtr op = std::make_shared<ge::OpDesc>("test", "Test");
  vector<int64_t> dims = {1, 2, 3};
  ge::GeShape shape = ge::GeShape(dims);
  ge::GeTensorDesc tensor(shape);
  op->AddInputDesc(tensor);
  op->AddOutputDesc(tensor);
  ge::NodePtr node1 = graph->AddNode(op);
  std::unordered_map<std::int64_t, ge::NodePtr> inner;
  std::unordered_map<int64_t, std::unordered_map<int64_t, ge::NodePtr>> scope_id_nodes_map_;
  inner.emplace(std::make_pair(0, node1));
  inner.emplace(std::make_pair(1, node1));
  inner.emplace(std::make_pair(2, node1));
  inner.emplace(std::make_pair(3, node1));
  inner.emplace(std::make_pair(4, node1));
  inner.emplace(std::make_pair(5, node1));
  inner.emplace(std::make_pair(6, node1));
  inner.emplace(std::make_pair(7, node1));
  inner.emplace(std::make_pair(8, node1));
  inner.emplace(std::make_pair(9, node1));
  inner.emplace(std::make_pair(10, node1));
  inner.emplace(std::make_pair(11, node1));
  inner.emplace(std::make_pair(12, node1));
  inner.emplace(std::make_pair(13, node1));
  inner.emplace(std::make_pair(14, node1));
  inner.emplace(std::make_pair(15, node1));
  std::shared_ptr<AutomaticBufferFusion> auto_buffer_fusion_ptr_ = std::make_shared<AutomaticBufferFusion>(nullptr);
  auto_buffer_fusion_ptr_->scope_id_nodes_map_.emplace(std::make_pair(0,inner));
  auto_buffer_fusion_ptr_->scope_id_nodes_map_.emplace(std::make_pair(2,inner));
  Status ret = auto_buffer_fusion_ptr_->ChangeScopeId(old_scope_id, new_scope_id);
  EXPECT_EQ(ret, GRAPH_OPTIMIZER_NOT_FUSE_TWO_SCOPE);
}

TEST_F(UBFUSION_ST, test_vector_core_buffer_fusion) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  RunUbFusion(graph, fe::VECTOR_CORE_NAME);
}
