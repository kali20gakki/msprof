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
#include <nn_calculation_ops.h>
#include <array_ops.h>

#define protected public
#define private public
#include "adapter/adapter_itf/task_builder_adapter.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "adapter/tbe_adapter/tbe_info/tbe_info_assembler.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/ge_context.h"
#include "./ge_local_context.h"

#include "ops_kernel_store/fe_ops_kernel_info_store.h"
#include "common/configuration.h"
#include "common/plugin_manager.h"
#include "common/util/op_info_util.h"
#include "ops_store/op_kernel_info.h"
#include "common/op_info_common.h"
#include "common/fe_log.h"
#include "common/aicore_util_types.h"
#include "ops_store/ops_kernel_manager.h"
#include "common/ffts_plus_type.h"
#undef protected
#undef private

using namespace std;
using namespace ge;
using namespace fe;
using namespace te;


class STEST_tbe_op_store_adapter : public testing::Test
{
protected:
    void SetUp()
    {
    }

    void TearDown()
    {
    }

        /*
 * batchnorm
 *    |
 *   relu
 */
    static void CreateGraph(ComputeGraphPtr graph) {
        OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
        OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Activation");

        // add descriptor
        vector<int64_t> dims = {288, 32, 16, 16};
        GeShape shape(dims);
        GeTensorDesc in_desc1(shape);
        in_desc1.SetFormat(FORMAT_FRACTAL_Z);
        in_desc1.SetDataType(DT_FLOAT16);
        bn_op->AddInputDesc("x", in_desc1);
        GeTensorDesc out_desc1(shape);
        out_desc1.SetFormat(FORMAT_NHWC);
        out_desc1.SetDataType(DT_FLOAT16);
        bn_op->AddOutputDesc("y", out_desc1);
        GeTensorDesc in_desc2(shape);
        in_desc2.SetFormat(FORMAT_NCHW);
        in_desc2.SetDataType(DT_FLOAT16);
        relu_op->AddInputDesc("x", in_desc2);
        GeTensorDesc out_desc2(shape);
        out_desc2.SetFormat(FORMAT_HWCN);
        out_desc2.SetDataType(DT_FLOAT16);
        relu_op->AddOutputDesc("y", out_desc2);
        ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));
        ge::AttrUtils::SetInt(relu_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));

        NodePtr bn_node = graph->AddNode(bn_op);
        NodePtr relu_node = graph->AddNode(relu_op);
        GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
    }
};

te::OpBuildResCode SgtTeFusionStub(std::vector<Node*> teGraphNode, OpDescPtr op_desc_ptr, const std::vector<ge::NodePtr>
        &to_be_del, uint64_t taskid, uint64_t tid, uint64_t sgt_thread_index, const std::string op_compile_strategy)
{
    string json_file_path = "./kernel_meta/";
    AttrUtils::SetStr(op_desc_ptr, "json_file_path", json_file_path);
    return te::OP_BUILD_SUCC;
}

te::OpBuildResCode SgtTeFusionStub2(std::vector<Node*> teGraphNode, OpDescPtr op_desc_ptr, const std::vector<ge::NodePtr>
        &to_be_del, uint64_t taskid, uint64_t tid, uint64_t sgt_thread_index, const std::string op_compile_strategy)
{
    string json_file_path = "./kernel_meta/";
    AttrUtils::SetStr(op_desc_ptr, "json_file_path", json_file_path);
    return te::OP_BUILD_FAIL;
}

te::OpBuildResCode TeFusionStub(std::vector<Node*> teGraphNode, OpDescPtr op_desc_ptr, const std::vector<ge::NodePtr>
        &to_be_del, uint64_t taskid, uint64_t tid, const std::string op_compile_strategy)
{
    string json_file_path = "./kernel_meta/";
    //OpDescPtr op_desc_ptr = output_node->GetOpDesc();
    AttrUtils::SetStr(op_desc_ptr, "json_file_path", json_file_path);
    return te::OP_BUILD_SUCC;
}

te::OpBuildResCode TeFusionStub3(std::vector<Node*> teGraphNode, OpDescPtr op_desc_ptr, const std::vector<ge::NodePtr> &to_be_del,
        uint64_t taskid, uint64_t tid, const std::string op_compile_strategy)
{
    string json_file_path = "";
    //OpDescPtr op_desc_ptr = output_node->GetOpDesc();
    AttrUtils::SetStr(op_desc_ptr, "json_file_path", json_file_path);
    return te::OP_BUILD_FAIL;
}

bool WaitAllFinishedStub(uint64_t tid, vector<te::FinComTask> &fin_task)
{
  te::FinComTask fin_com_task;
  fin_com_task.teNodeOpDesc = std::make_shared<ge::OpDesc>("OneOP", "");
  fin_com_task.taskId = GetAtomicId()-1;
  fin_com_task.status = 0;
  ge::AttrUtils::SetStr(fin_com_task.teNodeOpDesc, "json_file_path", "jsonFilePath");
  fin_task.push_back(fin_com_task);
  return true;
}

te::LX_QUERY_STATUS get_tbe_opinfo_stub(const te::TbeOpInfo &info, std::string &op_info) {
  return te::LX_QUERY_NOT_FOUND;
}

TEST_F(STEST_tbe_op_store_adapter, case_tbe_compiler_null_sgt_slice_op_success)
{
    TbeOpStoreAdapter compile_tbe_op;
    compile_tbe_op.support_parallel_compile = false;
    compile_tbe_op.TeFusion = TeFusionStub;
    compile_tbe_op.TeFusionV = SgtTeFusionStub;
    compile_tbe_op.WaitAllFinished = WaitAllFinishedStub;
    compile_tbe_op.GetOpInfo = get_tbe_opinfo_stub;
    ScopeNodeIdMap fusion_nodes_map;

    //1.create graph
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    vector<int64_t> dim_weight = {1, 3, 3, 3};
    GeShape shape_weight(dim_weight);
    GeTensorDesc weight_desc(shape_weight);
    vector<int64_t> dim_weight1 = {1, 3, 3, 3};
    GeShape shape_weight1(dim_weight1);
    GeTensorDesc weight_desc1(shape_weight1);
    OpDescPtr weight_op_desc1 = std::make_shared<OpDesc>("w1", fe::CONSTANT);
    OpDescPtr weight_op_desc2 = std::make_shared<OpDesc>("w2", fe::CONSTANT);
    weight_op_desc1->AddOutputDesc(weight_desc);
    weight_op_desc2->AddOutputDesc(weight_desc1);
    NodePtr Node1 = graph->AddNode(weight_op_desc1);
    NodePtr Node2 = graph->AddNode(weight_op_desc2);

    std::vector<ge::Node*> vector_node_ptr;
    vector_node_ptr.emplace_back(Node1.get());
    vector_node_ptr.emplace_back(Node2.get());
    fusion_nodes_map.insert(std::make_pair(0, vector_node_ptr));

    map<int64_t, std::string> json_file_map;
    json_file_map.emplace(make_pair(1, "xxxx1"));
    std::vector<ge::NodePtr> compile_failed_nodes;
    std::vector<ge::NodePtr> to_del_nodes;

    Status ret = compile_tbe_op.CompileMultiKernelSliceOp(fusion_nodes_map, json_file_map, compile_failed_nodes, to_del_nodes);
    EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_tbe_op_store_adapter, tbe_op_process_fail_pre_compile)
{
  TbeOpStoreAdapter compile_tbe_op;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  vector<int64_t> dim_weight = {1, 3, 3, 3};
  GeShape shape_weight(dim_weight);
  GeTensorDesc weight_desc(shape_weight);
  vector<int64_t> dim_weight1 = {1, 3, 3, 3};
  GeShape shape_weight1(dim_weight1);
  GeTensorDesc weight_desc1(shape_weight1);
  OpDescPtr weight_op_desc1 = std::make_shared<OpDesc>("w1", fe::CONSTANT);
  OpDescPtr weight_op_desc2 = std::make_shared<OpDesc>("w2", fe::CONSTANT);
  (void)ge::AttrUtils::SetBool(weight_op_desc2, NEED_RE_PRECOMPILE, true);
  weight_op_desc1->AddOutputDesc(weight_desc);
  weight_op_desc2->AddOutputDesc(weight_desc1);
  NodePtr Node1 = graph->AddNode(weight_op_desc1);
  NodePtr Node2 = graph->AddNode(weight_op_desc2);

  TbeOpStoreAdapter::CompileTaskPara task_para;
  task_para.task_num = 1;
  task_para.task_scope_id_map.insert(make_pair(1, 1));
  te::FinComTask failed_tasks;
  failed_tasks.taskId = 1;
  failed_tasks.graphId = 996;
  task_para.failed_tasks[failed_tasks.taskId] = failed_tasks;

  ScopeNodeIdMap fusion_nodes_map;
  std::vector<ge::Node*> vector_node_ptr;
  vector_node_ptr.emplace_back(Node2.get());
  fusion_nodes_map.insert(std::make_pair(1, vector_node_ptr));
  task_para.fusion_nodes_map = &fusion_nodes_map;
  Status ret = compile_tbe_op.ProcessFailPreCompTask(task_para);
  EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(STEST_tbe_op_store_adapter, tbe_op_process_succ_pre_comp_task) {
  TbeOpStoreAdapter compile_tbe_op;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  vector<int64_t> dim_weight = {1, 3, 3, 3};
  GeShape shape_weight(dim_weight);
  GeTensorDesc weight_desc(shape_weight);
  vector<int64_t> dim_weight1 = {1, 3, 3, 3};
  GeShape shape_weight1(dim_weight1);
  GeTensorDesc weight_desc1(shape_weight1);
  OpDescPtr weight_op_desc1 = std::make_shared<OpDesc>("w1", fe::CONSTANT);
  OpDescPtr weight_op_desc2 = std::make_shared<OpDesc>("w2", fe::CONSTANT);
  ge::AttrUtils::SetBool(weight_op_desc2, NEED_RE_PRECOMPILE, true);
  weight_op_desc1->AddOutputDesc(weight_desc);
  weight_op_desc2->AddOutputDesc(weight_desc1);
  NodePtr Node1 = graph->AddNode(weight_op_desc1);
  NodePtr Node2 = graph->AddNode(weight_op_desc2);

  TbeOpInfoPtr tbe_op_info_ptr = make_shared<te::TbeOpInfo>("","","","","");
  tbe_op_info_ptr->SetPattern("w2Pattern");
  TbeOpStoreAdapter::CompileTaskPara task_para;
  task_para.task_num = 1;
  task_para.task_node_map.insert(make_pair(1, Node2.get()));
  task_para.task_tbe_info_map.insert(make_pair(1, tbe_op_info_ptr));
  te::FinComTask fin_com_task;
  fin_com_task.taskId = 1;
  fin_com_task.graphId = 996;
  task_para.succ_tasks[fin_com_task.taskId] = fin_com_task;
  Status ret = compile_tbe_op.ProcessSuccPreCompTask(task_para);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_tbe_op_store_adapter, case_tbe_set_sgt_json_path_fail) {
    TbeOpStoreAdapter compile_tbe_op;
    compile_tbe_op.support_parallel_compile = true;
    compile_tbe_op.TeFusion = TeFusionStub3;
    compile_tbe_op.WaitAllFinished = WaitAllFinishedStub;
    compile_tbe_op.GetOpInfo = get_tbe_opinfo_stub;

    ge::OpDescPtr compile_op_desc = make_shared<ge::OpDesc>();
    string json_file_path;
    ge::AttrUtils::SetStr(compile_op_desc, "json_file_path", json_file_path);
    map<int64_t, std::string> json_file_map;
    json_file_map.emplace(make_pair(1, "xxxx1"));
    int scope_idx = 1;
    Status ret = compile_tbe_op.SetSgtOpJsonPath(compile_op_desc, json_file_map, scope_idx);
    EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(STEST_tbe_op_store_adapter, case_get_buffer_optimize_rollback_node_fail)
{
  TbeOpStoreAdapter compile_tbe_op;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  vector<int64_t> dim_weight = {1, 3, 3, 3};
  GeShape shape_weight(dim_weight);
  GeTensorDesc weight_desc(shape_weight);
  vector<int64_t> dim_weight1 = {1, 3, 3, 3};
  GeShape shape_weight1(dim_weight1);
  GeTensorDesc weight_desc1(shape_weight1);
  OpDescPtr weight_op_desc1 = std::make_shared<OpDesc>("w1", fe::CONSTANT);
  OpDescPtr weight_op_desc2 = std::make_shared<OpDesc>("w2", fe::CONSTANT);
  (void)ge::AttrUtils::SetBool(weight_op_desc2, NEED_RE_PRECOMPILE, true);
  weight_op_desc1->AddOutputDesc(weight_desc);
  weight_op_desc2->AddOutputDesc(weight_desc1);
  NodePtr Node1 = graph->AddNode(weight_op_desc1);
  NodePtr Node2 = graph->AddNode(weight_op_desc2);
  vector<ge::NodePtr> buff_fus_compile_failed_nodes;
  buff_fus_compile_failed_nodes.push_back(Node1);
  buff_fus_compile_failed_nodes.push_back(Node2);

  TbeOpStoreAdapter::CompileTaskPara task_para;
  task_para.task_num = 1;
  task_para.task_scope_id_map.insert(make_pair(2, 1));
  te::FinComTask failed_tasks;
  failed_tasks.taskId = 1;
  failed_tasks.graphId = 996;
  task_para.failed_tasks[failed_tasks.taskId] = failed_tasks;

  ScopeNodeIdMap fusion_nodes_map;
  std::vector<ge::Node*> vector_node_ptr;
  vector_node_ptr.emplace_back(Node2.get());
  fusion_nodes_map.insert(std::make_pair(1, vector_node_ptr));
  task_para.fusion_nodes_map = &fusion_nodes_map;
  vector<ge::NodePtr> l1_failed_nodes;
  Status ret = compile_tbe_op.ProcessLxFusionFailCompileTasks(task_para, l1_failed_nodes,
                                                              buff_fus_compile_failed_nodes);
  EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(STEST_tbe_op_store_adapter, save_ms_tune_error_msg)
{
  TbeOpStoreAdapter compile_tbe_op;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  vector<int64_t> dim_weight = {1, 3, 3, 3};
  GeShape shape_weight(dim_weight);
  GeTensorDesc weight_desc(shape_weight);
  vector<int64_t> dim_weight1 = {1, 3, 3, 3};
  GeShape shape_weight1(dim_weight1);
  GeTensorDesc weight_desc1(shape_weight1);
  OpDescPtr weight_op_desc1 = std::make_shared<OpDesc>("w1", fe::CONSTANT);
  OpDescPtr weight_op_desc2 = std::make_shared<OpDesc>("w2", fe::CONSTANT);
  (void)ge::AttrUtils::SetBool(weight_op_desc2, NEED_RE_PRECOMPILE, true);
  weight_op_desc1->AddOutputDesc(weight_desc);
  weight_op_desc2->AddOutputDesc(weight_desc1);
  NodePtr Node1 = graph->AddNode(weight_op_desc1);
  NodePtr Node2 = graph->AddNode(weight_op_desc2);

  TbeOpStoreAdapter::CompileTaskPara task_para;
  task_para.task_num = 1;
  task_para.task_scope_id_map.insert(make_pair(1, 1));
  te::FinComTask failed_tasks;
  failed_tasks.taskId = 1;
  failed_tasks.graphId = 996;
  task_para.failed_tasks[failed_tasks.taskId] = failed_tasks;

  ScopeNodeIdMap fusion_nodes_map;
  std::vector<ge::Node*> vector_node_ptr;
  vector_node_ptr.emplace_back(Node2.get());
  fusion_nodes_map.insert(std::make_pair(1, vector_node_ptr));
  task_para.fusion_nodes_map = &fusion_nodes_map;
  Status ret = fe::SUCCESS;
  compile_tbe_op.SaveMsTuneErrorMsg(task_para);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_tbe_op_store_adapter, case_retry_compile_fail_op_fail_1)
{
  TbeOpStoreAdapter compile_tbe_op;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  vector<int64_t> dim_weight = {1, 3, 3, 3};
  GeShape shape_weight(dim_weight);
  GeTensorDesc weight_desc(shape_weight);
  vector<int64_t> dim_weight1 = {1, 3, 3, 3};
  GeShape shape_weight1(dim_weight1);
  GeTensorDesc weight_desc1(shape_weight1);
  OpDescPtr weight_op_desc1 = std::make_shared<OpDesc>("w1", fe::CONSTANT);
  OpDescPtr weight_op_desc2 = std::make_shared<OpDesc>("w2", fe::CONSTANT);
  (void)ge::AttrUtils::SetBool(weight_op_desc2, NEED_RE_PRECOMPILE, true);
  weight_op_desc1->AddOutputDesc(weight_desc);
  weight_op_desc2->AddOutputDesc(weight_desc1);
  NodePtr Node1 = graph->AddNode(weight_op_desc1);
  NodePtr Node2 = graph->AddNode(weight_op_desc2);

  TbeOpStoreAdapter::CompileTaskPara task_para;
  task_para.task_num = 1;
  task_para.task_scope_id_map.insert(make_pair(1, 1));
  te::FinComTask failed_tasks;
  failed_tasks.taskId = 1;
  failed_tasks.graphId = 996;
  task_para.failed_tasks[failed_tasks.taskId] = failed_tasks;

  ScopeNodeIdMap fusion_nodes_map;
  std::vector<ge::Node*> vector_node_ptr;
  vector_node_ptr.emplace_back(Node2.get());
  fusion_nodes_map.insert(std::make_pair(1, vector_node_ptr));
  task_para.fusion_nodes_map = &fusion_nodes_map;
  Status ret = compile_tbe_op.RetryCompileFailOp(task_para);
  EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(STEST_tbe_op_store_adapter, tbe_op_process_fail_compile)
{
  TbeOpStoreAdapter compile_tbe_op;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  vector<int64_t> dim_weight = {1, 3, 3, 3};
  GeShape shape_weight(dim_weight);
  GeTensorDesc weight_desc(shape_weight);
  vector<int64_t> dim_weight1 = {1, 3, 3, 3};
  GeShape shape_weight1(dim_weight1);
  GeTensorDesc weight_desc1(shape_weight1);

  OpDescPtr weight_op_desc1 = std::make_shared<OpDesc>("w1", fe::CONSTANT);
  OpDescPtr weight_op_desc2 = std::make_shared<OpDesc>("w2", fe::CONSTANT);
  (void)ge::AttrUtils::SetBool(weight_op_desc2, NEED_RE_PRECOMPILE, true);
  weight_op_desc1->AddOutputDesc(weight_desc);
  weight_op_desc2->AddOutputDesc(weight_desc1);
  NodePtr Node1 = graph->AddNode(weight_op_desc1);
  NodePtr Node2 = graph->AddNode(weight_op_desc2);
  TbeOpStoreAdapter::CompileTaskPara task_para;
  task_para.task_num = 1;
  task_para.task_scope_id_map.insert(make_pair(1, 1));

  te::FinComTask failed_tasks;
  failed_tasks.taskId = 1;
  failed_tasks.graphId = 996;
  task_para.failed_tasks[failed_tasks.taskId] = failed_tasks;
  ScopeNodeIdMap fusion_nodes_map;
  std::vector<ge::Node*> vector_node_ptr;
  vector_node_ptr.emplace_back(Node2.get());
  fusion_nodes_map.insert(std::make_pair(1, vector_node_ptr));
  task_para.fusion_nodes_map = &fusion_nodes_map;

  Status ret = compile_tbe_op.ProcessFailCompileTask(task_para, CompileStrategy::COMPILE_STRATEGY_OP_SPEC);
  //3. result expected
  EXPECT_EQ(fe::FAILED, ret);
}

te::OpBuildResCode TeFusionStub1(std::vector<Node*> teGraphNode, OpDescPtr op_desc_ptr,
  const std::vector<ge::NodePtr>  &to_be_del, uint64_t taskid, uint64_t tid,
  const std::string op_compile_strategy)
{
    return te::OP_BUILD_SUCC;
}

te::OpBuildResCode FuzzBuildTbeOpStubSucc(uint64_t taskId, uint64_t graphId, ge::Node &node)
{
    return te::OP_BUILD_SUCC;
}

te::OpBuildResCode FuzzBuildTbeOpStubFail(uint64_t taskId, uint64_t graphId, ge::Node &node)
{
    return te::OP_BUILD_FAIL;
}

TEST_F(STEST_tbe_op_store_adapter, tbe_op_process_fail_compile_null_failed_tasks)
{
  TbeOpStoreAdapter compile_tbe_op;
  compile_tbe_op.TeFusion = TeFusionStub1;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  vector<int64_t> dim_weight = {1, 3, 3, 3};
  GeShape shape_weight(dim_weight);
  GeTensorDesc weight_desc(shape_weight);
  vector<int64_t> dim_weight1 = {1, 3, 3, 3};
  GeShape shape_weight1(dim_weight1);
  GeTensorDesc weight_desc1(shape_weight1);

  OpDescPtr weight_op_desc1 = std::make_shared<OpDesc>("w1", fe::CONSTANT);
  OpDescPtr weight_op_desc2 = std::make_shared<OpDesc>("w2", fe::CONSTANT);
  (void)ge::AttrUtils::SetBool(weight_op_desc2, NEED_RE_PRECOMPILE, true);
  weight_op_desc1->AddOutputDesc(weight_desc);
  weight_op_desc2->AddOutputDesc(weight_desc1);
  NodePtr Node1 = graph->AddNode(weight_op_desc1);
  NodePtr Node2 = graph->AddNode(weight_op_desc2);
  ge::AttrUtils::SetInt(Node2->GetOpDesc(), "_fe_imply_type", 1);
  TbeOpStoreAdapter::CompileTaskPara task_para;
  task_para.task_num = 1;
  task_para.task_scope_id_map.insert(make_pair(1, 1));

  te::FinComTask failed_tasks;
  failed_tasks.taskId = 1;
  failed_tasks.graphId = 996;
  task_para.failed_tasks[failed_tasks.taskId] = failed_tasks;
  ScopeNodeIdMap fusion_nodes_map;
  std::vector<ge::Node*> vector_node_ptr;
  vector_node_ptr.emplace_back(Node2.get());
  fusion_nodes_map.insert(std::make_pair(1, vector_node_ptr));
  task_para.fusion_nodes_map = &fusion_nodes_map;
  Status ret = compile_tbe_op.ProcessFailCompileTask(task_para, CompileStrategy::COMPILE_STRATEGY_OP_SPEC);
  //3. result expected
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_tbe_op_store_adapter, tbe_op_process_fail_compile_null_failed_tasks_fuzzy_succ)
{
  TbeOpStoreAdapter compile_tbe_op;
  compile_tbe_op.FuzzBuildTbeOp = FuzzBuildTbeOpStubSucc;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  vector<int64_t> dim_weight = {1, 3, 3, 3};
  GeShape shape_weight(dim_weight);
  GeTensorDesc weight_desc(shape_weight);
  vector<int64_t> dim_weight1 = {1, 3, 3, 3};
  GeShape shape_weight1(dim_weight1);
  GeTensorDesc weight_desc1(shape_weight1);

  OpDescPtr weight_op_desc1 = std::make_shared<OpDesc>("w1", fe::CONSTANT);
  OpDescPtr weight_op_desc2 = std::make_shared<OpDesc>("w2", fe::CONSTANT);
  (void)ge::AttrUtils::SetBool(weight_op_desc2, NEED_RE_PRECOMPILE, true);
  weight_op_desc1->AddOutputDesc(weight_desc);
  weight_op_desc2->AddOutputDesc(weight_desc1);
  NodePtr Node1 = graph->AddNode(weight_op_desc1);
  NodePtr Node2 = graph->AddNode(weight_op_desc2);
  ge::AttrUtils::SetInt(Node2->GetOpDesc(), "_fe_imply_type", 1);
  TbeOpStoreAdapter::CompileTaskPara task_para;
  task_para.task_num = 1;
  task_para.task_scope_id_map.insert(make_pair(1, 1));

  te::FinComTask failed_tasks;
  failed_tasks.taskId = 1;
  failed_tasks.graphId = 996;
  task_para.failed_tasks[failed_tasks.taskId] = failed_tasks;
  ScopeNodeIdMap fusion_nodes_map;
  std::vector<ge::Node*> vector_node_ptr;
  vector_node_ptr.emplace_back(Node2.get());
  fusion_nodes_map.insert(std::make_pair(1, vector_node_ptr));
  task_para.fusion_nodes_map = &fusion_nodes_map;

  std::map<std::string, std::string> options;
  options.insert(std::pair<std::string, std::string>("ge.shape_generalized_build_mode", SHAPE_GENERALIZED));
  ge::GetThreadLocalContext().SetGlobalOption(options);

  ge::AttrUtils::SetBool(Node1->GetOpDesc(), ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, true);
  ge::AttrUtils::SetBool(Node2->GetOpDesc(), ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, true);

  Status ret = compile_tbe_op.ProcessFailCompileTask(task_para, CompileStrategy::COMPILE_STRATEGY_OP_SPEC);

  std::map<std::string, std::string> options1;
  options.insert(std::pair<std::string, std::string>("ge.shape_generalized_build_mode", "shape_precise"));
  ge::GetThreadLocalContext().SetGlobalOption(options1);

  //3. result expected
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_tbe_op_store_adapter, tbe_op_process_fail_compile_null_failed_tasks_fuzzy_fail)
{
  TbeOpStoreAdapter compile_tbe_op;
  compile_tbe_op.FuzzBuildTbeOp = FuzzBuildTbeOpStubFail;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  vector<int64_t> dim_weight = {1, 3, 3, 3};
  GeShape shape_weight(dim_weight);
  GeTensorDesc weight_desc(shape_weight);
  vector<int64_t> dim_weight1 = {1, 3, 3, 3};
  GeShape shape_weight1(dim_weight1);
  GeTensorDesc weight_desc1(shape_weight1);

  OpDescPtr weight_op_desc1 = std::make_shared<OpDesc>("w1", fe::CONSTANT);
  OpDescPtr weight_op_desc2 = std::make_shared<OpDesc>("w2", fe::CONSTANT);
  (void)ge::AttrUtils::SetBool(weight_op_desc2, NEED_RE_PRECOMPILE, true);
  weight_op_desc1->AddOutputDesc(weight_desc);
  weight_op_desc2->AddOutputDesc(weight_desc1);
  NodePtr Node1 = graph->AddNode(weight_op_desc1);
  NodePtr Node2 = graph->AddNode(weight_op_desc2);
  ge::AttrUtils::SetInt(Node2->GetOpDesc(), "_fe_imply_type", 1);
  TbeOpStoreAdapter::CompileTaskPara task_para;
  task_para.task_num = 1;
  task_para.task_scope_id_map.insert(make_pair(1, 1));

  te::FinComTask failed_tasks;
  failed_tasks.taskId = 1;
  failed_tasks.graphId = 996;
  task_para.failed_tasks[failed_tasks.taskId] = failed_tasks;
  ScopeNodeIdMap fusion_nodes_map;
  std::vector<ge::Node*> vector_node_ptr;
  vector_node_ptr.emplace_back(Node2.get());
  fusion_nodes_map.insert(std::make_pair(1, vector_node_ptr));
  task_para.fusion_nodes_map = &fusion_nodes_map;

  std::map<std::string, std::string> options;
  options.insert(std::pair<std::string, std::string>("ge.shape_generalized_build_mode", SHAPE_GENERALIZED));
  ge::GetThreadLocalContext().SetGlobalOption(options);

  ge::AttrUtils::SetBool(Node1->GetOpDesc(), ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, true);
  ge::AttrUtils::SetBool(Node2->GetOpDesc(), ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, true);

  Status ret = compile_tbe_op.ProcessFailCompileTask(task_para, CompileStrategy::COMPILE_STRATEGY_OP_SPEC);

  std::map<std::string, std::string> options1;
  options.insert(std::pair<std::string, std::string>("ge.shape_generalized_build_mode", "shape_precise"));
  ge::GetThreadLocalContext().SetGlobalOption(options1);

  //3. result expected
  EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(STEST_tbe_op_store_adapter, case_get_sgt_slice_task_roll_back_node_fail_1)
{
  TbeOpStoreAdapter compile_tbe_op;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  vector<int64_t> dim_weight = {1, 3, 3, 3};
  GeShape shape_weight(dim_weight);
  GeTensorDesc weight_desc(shape_weight);
  vector<int64_t> dim_weight1 = {1, 3, 3, 3};
  GeShape shape_weight1(dim_weight1);
  GeTensorDesc weight_desc1(shape_weight1);
  OpDescPtr weight_op_desc1 = std::make_shared<OpDesc>("w1", fe::CONSTANT);
  OpDescPtr weight_op_desc2 = std::make_shared<OpDesc>("w2", fe::CONSTANT);
  (void)ge::AttrUtils::SetBool(weight_op_desc2, NEED_RE_PRECOMPILE, true);
  weight_op_desc1->AddOutputDesc(weight_desc);
  weight_op_desc2->AddOutputDesc(weight_desc1);
  NodePtr Node1 = graph->AddNode(weight_op_desc1);
  NodePtr Node2 = graph->AddNode(weight_op_desc2);
  vector<ge::NodePtr> need_rollback_nodes;
  need_rollback_nodes.push_back(Node1);
  need_rollback_nodes.push_back(Node2);

  TbeOpStoreAdapter::CompileTaskPara task_para;
  task_para.task_num = 1;
  task_para.task_scope_id_map.insert(make_pair(2, 1));
  te::FinComTask failed_tasks;
  failed_tasks.taskId = 1;
  failed_tasks.graphId = 996;
  task_para.failed_tasks[failed_tasks.taskId] = failed_tasks;

  Status ret = compile_tbe_op.GetSgtSliceTaskRollbackNode(task_para, need_rollback_nodes);
  EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(STEST_tbe_op_store_adapter, case_get_sgt_slice_task_roll_back_node_suc_1)
{
  TbeOpStoreAdapter compile_tbe_op;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  vector<int64_t> dim_weight = {1, 3, 3, 3};
  GeShape shape_weight(dim_weight);
  GeTensorDesc weight_desc(shape_weight);
  vector<int64_t> dim_weight1 = {1, 3, 3, 3};
  GeShape shape_weight1(dim_weight1);
  GeTensorDesc weight_desc1(shape_weight1);
  OpDescPtr weight_op_desc1 = std::make_shared<OpDesc>("w1", fe::CONSTANT);
  OpDescPtr weight_op_desc2 = std::make_shared<OpDesc>("w2", fe::CONSTANT);
  (void)ge::AttrUtils::SetBool(weight_op_desc2, NEED_RE_PRECOMPILE, true);
  weight_op_desc1->AddOutputDesc(weight_desc);
  weight_op_desc2->AddOutputDesc(weight_desc1);
  NodePtr Node1 = graph->AddNode(weight_op_desc1);
  NodePtr Node2 = graph->AddNode(weight_op_desc2);
  vector<ge::NodePtr> need_rollback_nodes;
  need_rollback_nodes.push_back(Node1);
  need_rollback_nodes.push_back(Node2);

  TbeOpStoreAdapter::CompileTaskPara task_para;
  task_para.task_num = 1;
  task_para.task_scope_id_map.insert(make_pair(1, 1));
  te::FinComTask failed_tasks;
  failed_tasks.taskId = 1;
  failed_tasks.graphId = 996;
  task_para.failed_tasks[failed_tasks.taskId] = failed_tasks;

  ScopeNodeIdMap fusion_nodes_map;
  std::vector<ge::Node*> vector_node_ptr;
  vector_node_ptr.emplace_back(Node2.get());
  fusion_nodes_map.insert(std::make_pair(1, vector_node_ptr));
  task_para.fusion_nodes_map = &fusion_nodes_map;
  Status ret = compile_tbe_op.GetSgtSliceTaskRollbackNode(task_para, need_rollback_nodes);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_tbe_op_store_adapter, case_get_sgt_slice_task_roll_back_node_suc_2)
{
  TbeOpStoreAdapter compile_tbe_op;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  vector<int64_t> dim_weight = {1, 3, 3, 3};
  GeShape shape_weight(dim_weight);
  GeTensorDesc weight_desc(shape_weight);
  vector<int64_t> dim_weight1 = {1, 3, 3, 3};
  GeShape shape_weight1(dim_weight1);
  GeTensorDesc weight_desc1(shape_weight1);
  OpDescPtr weight_op_desc1 = std::make_shared<OpDesc>("w1", fe::CONSTANT);
  OpDescPtr weight_op_desc2 = std::make_shared<OpDesc>("w2", fe::CONSTANT);
  (void)ge::AttrUtils::SetBool(weight_op_desc2, NEED_RE_PRECOMPILE, true);
  weight_op_desc1->AddOutputDesc(weight_desc);
  weight_op_desc2->AddOutputDesc(weight_desc1);
  NodePtr Node1 = graph->AddNode(weight_op_desc1);
  NodePtr Node2 = graph->AddNode(weight_op_desc2);
  vector<ge::NodePtr> need_rollback_nodes;
  need_rollback_nodes.push_back(Node1);
  need_rollback_nodes.push_back(Node2);

  TbeOpStoreAdapter::CompileTaskPara task_para;
  task_para.task_num = 1;
  task_para.task_scope_id_map.insert(make_pair(1, 1));
  te::FinComTask failed_tasks;
  failed_tasks.taskId = 1;
  failed_tasks.graphId = 996;
  task_para.failed_tasks[failed_tasks.taskId] = failed_tasks;
  vector<uint64_t> stim = {1, 2};
  task_para.scope_task_ids_map[1] = stim;
  ScopeNodeIdMap fusion_nodes_map;
  std::vector<ge::Node*> vector_node_ptr;
  vector_node_ptr.emplace_back(Node2.get());
  fusion_nodes_map.insert(std::make_pair(1, vector_node_ptr));
  task_para.fusion_nodes_map = &fusion_nodes_map;

  Status ret = compile_tbe_op.GetSgtSliceTaskRollbackNode(task_para, need_rollback_nodes);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_tbe_op_store_adapter, case_set_sgt_tensor_slice_info_to_nodes_fail)
{
  TbeOpStoreAdapter compile_tbe_op;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateGraph(graph);
  std::vector<ge::Node*> vector_node_ptr;
  ffts::ThreadSliceMapPtr tsmp_ptr;
  for (auto &node : graph->GetDirectNode()) {
    auto op_desc_ptr = node->GetOpDesc();

    if (op_desc_ptr->GetName() == "relu") {
        vector_node_ptr.emplace_back(node.get());
    }
  }

  Status ret = compile_tbe_op.SetSgtTensorSliceInfoToNodes(vector_node_ptr, 0);
  EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(STEST_tbe_op_store_adapter, case_set_sgt_tensor_slice_info_to_nodes_suc)
{
  TbeOpStoreAdapter compile_tbe_op;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateGraph(graph);
  std::vector<ge::Node*> vector_node_ptr;
  ffts::ThreadSliceMapPtr tsmp_ptr;
  for (auto &node : graph->GetDirectNode()) {
    auto op_desc_ptr = node->GetOpDesc();
    if (op_desc_ptr->GetName() == "relu") {
        AttrUtils::SetInt(node->GetOpDesc(), kThreadScopeId, 1);
        ffts::ThreadSliceMap subgraphInfo;
        vector<vector<vector<ffts::DimRange>>> inputTensorSlice;
        vector<vector<vector<ffts::DimRange>>> oriInputTensorSlice;
        for (size_t i = 0; i < 2; i++) {
            vector<int64_t> vec1 = {0, 288, 0, 32, 0, 16, 0, 16};
            vector<ffts::DimRange> vdr1;
            for (size_t j = 0; j < vec1.size() - 1;) {
                ffts::DimRange dr;
                dr.lower = vec1[j];
                dr.higher = vec1[j + 1];
                vdr1.push_back(dr);
                j = j + 2;
            }
            vector<vector<ffts::DimRange>> threadSlice;
            threadSlice.push_back(vdr1);
            vector<int64_t> vec2 = {0, 288, 0, 32, 0, 16, 0, 16};
            vector<ffts::DimRange> vdr2;
            for (size_t j = 0; j < vec2.size() - 1;) {
                ffts::DimRange dr;
                dr.lower = vec2[j];
                dr.higher = vec2[j + 1];
                vdr2.push_back(dr);
                j = j + 2;
            }
            vector<vector<ffts::DimRange>> oriThreadSlice;
            oriThreadSlice.push_back(vdr2);
            inputTensorSlice.push_back(threadSlice);
            oriInputTensorSlice.push_back(oriThreadSlice);
        }

        vector<vector<vector<ffts::DimRange>>> outputTensorSlice;
        vector<vector<vector<ffts::DimRange>>> oriOutputTensorSlice;
        for (size_t i = 0; i < 2; i++) {
            vector<int64_t> vec3 = {0, 288, 0, 32, 0, 16, 0, 16};
            vector<ffts::DimRange> vdr3;
            for (size_t j = 0; j < vec3.size() - 1;) {
                ffts::DimRange dr;
                dr.lower = vec3[j];
                dr.higher = vec3[j + 1];
                vdr3.push_back(dr);
                j = j + 2;
            }
            vector<vector<ffts::DimRange>> threadSlice;
            threadSlice.push_back(vdr3);
            vector<int64_t> vec4 = {0, 288, 0, 32, 0, 16, 0, 16};
            vector<ffts::DimRange> vdr4;
            for (size_t j = 0; j < vec4.size() - 1;) {
                ffts::DimRange dr;
                dr.lower = vec4[j];
                dr.higher = vec4[j + 1];
                vdr4.push_back(dr);
                j = j + 2;
            }
            vector<vector<ffts::DimRange>> oriThreadSlice;
            oriThreadSlice.push_back(vdr4);
            outputTensorSlice.push_back(threadSlice);
            oriOutputTensorSlice.push_back(oriThreadSlice);
        }

        subgraphInfo.input_tensor_slice = inputTensorSlice;
        subgraphInfo.ori_input_tensor_slice = oriInputTensorSlice;
        subgraphInfo.output_tensor_slice = outputTensorSlice;
        subgraphInfo.ori_output_tensor_slice = oriOutputTensorSlice;
        tsmp_ptr = make_shared<ffts::ThreadSliceMap>(subgraphInfo);
        op_desc_ptr->SetExtAttr(ffts::kAttrSgtStructInfo, tsmp_ptr);
        vector_node_ptr.emplace_back(node.get());
    }
  }

  Status ret = compile_tbe_op.SetSgtTensorSliceInfoToNodes(vector_node_ptr, 0);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_tbe_op_store_adapter, case_set_sgt_slice_task_to_te_fusion_suc)
{
  TbeOpStoreAdapter compile_tbe_op;
  compile_tbe_op.support_parallel_compile = false;
  compile_tbe_op.TeFusionV = SgtTeFusionStub;
  compile_tbe_op.WaitAllFinished = WaitAllFinishedStub;
  compile_tbe_op.GetOpInfo = get_tbe_opinfo_stub;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateGraph(graph);
  vector<ge::NodePtr> to_del_nodes;
  std::vector<ge::Node*> vector_node_ptr;
  ScopeNodeIdMap fusion_nodes_map;
  for (auto &node : graph->GetDirectNode()) {
    auto op_desc_ptr = node->GetOpDesc();
    if (op_desc_ptr->GetName() == "relu") {
        AttrUtils::SetInt(node->GetOpDesc(), kThreadScopeId, 1);
        ffts::ThreadSliceMap subgraphInfo;
        vector<vector<vector<ffts::DimRange>>> inputTensorSlice;
        vector<vector<vector<ffts::DimRange>>> oriInputTensorSlice;
        for (size_t i = 0; i < 2; i++) {
            vector<int64_t> vec1 = {0, 288, 0, 32, 0, 16, 0, 16};
            vector<ffts::DimRange> vdr1;
            for (size_t j = 0; j < vec1.size() - 1;) {
                ffts::DimRange dr;
                dr.lower = vec1[j];
                dr.higher = vec1[j + 1];
                vdr1.push_back(dr);
                j = j + 2;
            }
            vector<vector<ffts::DimRange>> threadSlice;
            threadSlice.push_back(vdr1);
            vector<int64_t> vec2 = {0, 288, 0, 32, 0, 16, 0, 16};
            vector<ffts::DimRange> vdr2;
            for (size_t j = 0; j < vec2.size() - 1;) {
                ffts::DimRange dr;
                dr.lower = vec2[j];
                dr.higher = vec2[j + 1];
                vdr2.push_back(dr);
                j = j + 2;
            }
            vector<vector<ffts::DimRange>> oriThreadSlice;
            oriThreadSlice.push_back(vdr2);
            inputTensorSlice.push_back(threadSlice);
            oriInputTensorSlice.push_back(oriThreadSlice);
        }

        vector<vector<vector<ffts::DimRange>>> outputTensorSlice;
        vector<vector<vector<ffts::DimRange>>> oriOutputTensorSlice;
        for (size_t i = 0; i < 2; i++) {
            vector<int64_t> vec3 = {0, 288, 0, 32, 0, 16, 0, 16};
            vector<ffts::DimRange> vdr3;
            for (size_t j = 0; j < vec3.size() - 1;) {
                ffts::DimRange dr;
                dr.lower = vec3[j];
                dr.higher = vec3[j + 1];
                vdr3.push_back(dr);
                j = j + 2;
            }
            vector<vector<ffts::DimRange>> threadSlice;
            threadSlice.push_back(vdr3);
            vector<int64_t> vec4 = {0, 288, 0, 32, 0, 16, 0, 16};
            vector<ffts::DimRange> vdr4;
            for (size_t j = 0; j < vec4.size() - 1;) {
                ffts::DimRange dr;
                dr.lower = vec4[j];
                dr.higher = vec4[j + 1];
                vdr4.push_back(dr);
                j = j + 2;
            }
            vector<vector<ffts::DimRange>> oriThreadSlice;
            oriThreadSlice.push_back(vdr4);
            outputTensorSlice.push_back(threadSlice);
            oriOutputTensorSlice.push_back(oriThreadSlice);
        }

        subgraphInfo.input_tensor_slice = inputTensorSlice;
        subgraphInfo.ori_input_tensor_slice = oriInputTensorSlice;
        subgraphInfo.output_tensor_slice = outputTensorSlice;
        subgraphInfo.ori_output_tensor_slice = oriOutputTensorSlice;
        subgraphInfo.thread_mode = 1;
        ffts::ThreadSliceMapPtr tsmp_ptr = make_shared<ffts::ThreadSliceMap>(subgraphInfo);
        node->GetOpDesc()->SetExtAttr("_sgt_struct_info", tsmp_ptr);
        vector_node_ptr.emplace_back(node.get());
    }
  }

  TbeOpStoreAdapter::CompileTaskPara task_para;
  fusion_nodes_map.insert(std::make_pair(1, vector_node_ptr));
  task_para.fusion_nodes_map = &fusion_nodes_map;
  task_para.task_num = 2;
  task_para.task_scope_id_map.insert(make_pair(1, 1));
  task_para.task_scope_id_map.insert(make_pair(2, 1));
  vector<uint64_t> stim = {1, 2};
  task_para.scope_task_ids_map[1] = stim;
  te::FinComTask succ_tasks;
  succ_tasks.taskId = 1;
  succ_tasks.graphId = 996;
  task_para.succ_tasks[succ_tasks.taskId] = succ_tasks;
  Status ret = compile_tbe_op.SetSgtSliceTaskToTeFusion(task_para, to_del_nodes);
  EXPECT_EQ(fe::SUCCESS, ret);
  ret = compile_tbe_op.SetTaskForOneScope(vector_node_ptr, 1, to_del_nodes, task_para,
                                          CompileStrategy::COMPILE_STRATEGY_NO_TUNE);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_tbe_op_store_adapter, case_set_sgt_slice_task_to_te_fusion_fail_1)
{
  TbeOpStoreAdapter compile_tbe_op;
  compile_tbe_op.support_parallel_compile = false;
  compile_tbe_op.TeFusionV = SgtTeFusionStub2;
  compile_tbe_op.WaitAllFinished = WaitAllFinishedStub;
  compile_tbe_op.GetOpInfo = get_tbe_opinfo_stub;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  CreateGraph(graph);
  vector<ge::NodePtr> to_del_nodes;
  std::vector<ge::Node*> vector_node_ptr;
  ScopeNodeIdMap fusion_nodes_map;
  for (auto &node : graph->GetDirectNode()) {
    auto op_desc_ptr = node->GetOpDesc();
    if (op_desc_ptr->GetName() == "relu") {
        AttrUtils::SetInt(node->GetOpDesc(), kThreadScopeId, 1);
        ffts::ThreadSliceMap subgraphInfo;
        vector<vector<vector<ffts::DimRange>>> inputTensorSlice;
        vector<vector<vector<ffts::DimRange>>> oriInputTensorSlice;
        for (size_t i = 0; i < 2; i++) {
            vector<int64_t> vec1 = {0, 288, 0, 32, 0, 16, 0, 16};
            vector<ffts::DimRange> vdr1;
            for (size_t j = 0; j < vec1.size() - 1;) {
                ffts::DimRange dr;
                dr.lower = vec1[j];
                dr.higher = vec1[j + 1];
                vdr1.push_back(dr);
                j = j + 2;
            }
            vector<vector<ffts::DimRange>> threadSlice;
            threadSlice.push_back(vdr1);

            vector<int64_t> vec2 = {0, 288, 0, 32, 0, 16, 0, 16};
            vector<ffts::DimRange> vdr2;
            for (size_t j = 0; j < vec2.size() - 1;) {
                ffts::DimRange dr;
                dr.lower = vec2[j];
                dr.higher = vec2[j + 1];
                vdr2.push_back(dr);
                j = j + 2;
            }
            vector<vector<ffts::DimRange>> oriThreadSlice;
            oriThreadSlice.push_back(vdr2);
            inputTensorSlice.push_back(threadSlice);
            oriInputTensorSlice.push_back(oriThreadSlice);
        }

        vector<vector<vector<ffts::DimRange>>> outputTensorSlice;
        vector<vector<vector<ffts::DimRange>>> oriOutputTensorSlice;
        for (size_t i = 0; i < 2; i++) {
            vector<int64_t> vec3 = {0, 288, 0, 32, 0, 16, 0, 16};
            vector<ffts::DimRange> vdr3;
            for (size_t j = 0; j < vec3.size() - 1;) {
                ffts::DimRange dr;
                dr.lower = vec3[j];
                dr.higher = vec3[j + 1];
                vdr3.push_back(dr);
                j = j + 2;
            }
            vector<vector<ffts::DimRange>> threadSlice;
            threadSlice.push_back(vdr3);

            vector<int64_t> vec4 = {0, 288, 0, 32, 0, 16, 0, 16};
            vector<ffts::DimRange> vdr4;
            for (size_t j = 0; j < vec4.size() - 1;) {
                ffts::DimRange dr;
                dr.lower = vec4[j];
                dr.higher = vec4[j + 1];
                vdr4.push_back(dr);
                j = j + 2;
            }
            vector<vector<ffts::DimRange>> oriThreadSlice;
            oriThreadSlice.push_back(vdr4);
            outputTensorSlice.push_back(threadSlice);
            oriOutputTensorSlice.push_back(oriThreadSlice);
        }

        subgraphInfo.input_tensor_slice = inputTensorSlice;
        subgraphInfo.ori_input_tensor_slice = oriInputTensorSlice;
        subgraphInfo.output_tensor_slice = outputTensorSlice;
        subgraphInfo.ori_output_tensor_slice = oriOutputTensorSlice;
        subgraphInfo.thread_mode = 1;
        ffts::ThreadSliceMapPtr tsmp_ptr = make_shared<ffts::ThreadSliceMap>(subgraphInfo);
        node->GetOpDesc()->SetExtAttr("_sgt_struct_info", tsmp_ptr);
        vector_node_ptr.emplace_back(node.get());
    }
  }

  TbeOpStoreAdapter::CompileTaskPara task_para;
  fusion_nodes_map.insert(std::make_pair(1, vector_node_ptr));
  task_para.fusion_nodes_map = &fusion_nodes_map;

  task_para.task_num = 2;
  task_para.task_scope_id_map.insert(make_pair(1, 1));
  task_para.task_scope_id_map.insert(make_pair(2, 1));
  vector<uint64_t> stim = {1, 2};
  task_para.scope_task_ids_map[1] = stim;

  te::FinComTask succ_tasks;
  succ_tasks.taskId = 1;
  succ_tasks.graphId = 996;
  task_para.succ_tasks[succ_tasks.taskId] = succ_tasks;
  Status ret = compile_tbe_op.SetSgtSliceTaskToTeFusion(task_para, to_del_nodes);
  EXPECT_EQ(fe::FAILED, ret);
}