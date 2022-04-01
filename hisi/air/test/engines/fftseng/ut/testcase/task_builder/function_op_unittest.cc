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

#define protected public
#define private public
#include "optimizer/graph_optimizer/fftsplus_graph_optimizer.h"
#include "common/graph_comm.h"
#include "common/sgt_slice_type.h"
#include "inc/ffts_log.h"
#include "inc/ffts_error_codes.h"
#include "inc/ffts_type.h"
#include "inc/graph/utils/graph_utils.h"
#include "../../../graph_constructor/graph_constructor.h"
#include "graph/debug/ge_attr_define.h"

#undef private
#undef protected

using namespace std;
using namespace ffts;
using namespace ge;

using SubGraphNodeMap = map <uint32_t, vector<ge::NodePtr>>;
using GraphOptimizerPtr = shared_ptr<FFTSPlusGraphOptimizer>;

class UTEST_trans_sub_graph_to_function_op : public testing::Test
{
protected:
	void SetUp()
	{
		graph_optimizer_ptr_ = make_shared<FFTSPlusGraphOptimizer>();
		graph_optimizer_ptr_->graph_comm_ptr_ = make_shared<fe::GraphComm>("engineName");
		graph_optimizer_ptr_->graph_comm_ptr_->Initialize();
	}
	void TearDown()
	{

	}

	/*
		batchnorm
			|
		  relu
		    |
		 expm_op
		    |
		   sqrt
	*/
	static void CreateGraph(ComputeGraphPtr graph)
	{
		OpDescPtr bn_op = make_shared<OpDesc>("batchnormal", "BatchNorm");
		OpDescPtr relu_op = make_shared<OpDesc>("relu", "Activation");
		OpDescPtr expm_op = make_shared<OpDesc>("expm1", "Expm1");
		OpDescPtr sqrt_op = make_shared<OpDesc>("sqrt", "Sqrt");

		// add descriptor
		vector<int64_t> dims = { 1, 2, 3, 4 };
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
		in_desc2.SetFormat(FORMAT_HWCN);
		in_desc2.SetDataType(DT_FLOAT16);
		relu_op->AddInputDesc("x", in_desc2);

		GeTensorDesc out_desc2(shape);
		out_desc2.SetFormat(FORMAT_HWCN);
		out_desc2.SetDataType(DT_FLOAT16);
		relu_op->AddOutputDesc("y", out_desc2);

		GeTensorDesc in_desc3(shape);
		in_desc3.SetFormat(FORMAT_HWCN);
		in_desc3.SetDataType(DT_FLOAT16);
		expm_op->AddInputDesc("x", in_desc3);

		GeTensorDesc out_desc3(shape);
		out_desc3.SetFormat(FORMAT_NHWC);
		out_desc3.SetDataType(DT_FLOAT16);
		expm_op->AddOutputDesc("y", out_desc3);

		GeTensorDesc in_desc4(shape);
		in_desc4.SetFormat(FORMAT_NHWC);
		in_desc4.SetDataType(DT_FLOAT16);
		sqrt_op->AddInputDesc("x", in_desc4);

		GeTensorDesc out_desc4(shape);
		out_desc4.SetFormat(FORMAT_NHWC);
		out_desc4.SetDataType(DT_FLOAT16);
		sqrt_op->AddOutputDesc("y", out_desc4);

		/*AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));
		AttrUtils::SetInt(relu_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));
		AttrUtils::SetInt(expm_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));
		AttrUtils::SetInt(sqrt_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));*/

		NodePtr bn_node = graph->AddNode(bn_op);
		NodePtr relu_node = graph->AddNode(relu_op);
		NodePtr expm_node = graph->AddNode(expm_op);
		NodePtr sqrt_node = graph->AddNode(sqrt_op);

		GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
		GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0), expm_node->GetInDataAnchor(0));
		GraphUtils::AddEdge(expm_node->GetOutDataAnchor(0), sqrt_node->GetInDataAnchor(0));
	}
  static void CreateGraph1(ComputeGraphPtr graph)
  {
    OpDescPtr bn_op = make_shared<OpDesc>("batchnormal", "BatchNorm");
    // add descriptor
    vector<int64_t> dims = { 1, 2, 3, 4 };
    GeShape shape(dims);
    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_FRACTAL_Z);
    in_desc1.SetDataType(DT_FLOAT16);
    bn_op->AddInputDesc("x", in_desc1);

    std::string tsmp_ptr = "{\"cutType\":[],\"dependencies\":[],\"input_tensor_slice\":[[[{\"higher\":3,\"lower\":0},{\"higher\":1,\"lower\":0},{\"higher\":200,\"lower\":0},{\"higher\":200,\"lower\":0},{\"higher\":16,\"lower\":0}],[{\"higher\":2,\"lower\":0},{\"higher\":1,\"lower\":0},{\"higher\":200,\"lower\":0},{\"higher\":200,\"lower\":0},{\"higher\":16,\"lower\":0}]]],\"is_input_node_of_thread_scope\":false,\"is_output_node_of_thread_scope\":false,\"node_num_in_thread_scope\":32679,\"oriInputTensorShape\":[[[3,200,200,16],[2,200,200,16]]],\"oriOutputTensorShape\":[[[5,200,200,16]]],\"original_node\":\"\",\"output_tensor_slice\":[[[{\"higher\":5,\"lower\":0},{\"higher\":1,\"lower\":0},{\"higher\":200,\"lower\":0},{\"higher\":200,\"lower\":0},{\"higher\":16,\"lower\":0}]]],\"parallel_window_size\":2,\"slice_instance_num\":1,\"threadMode\":false,\"thread_id\":0,\"thread_scopeId\":2}";
    AttrUtils::SetStr(bn_op, "_sgt_json_info", tsmp_ptr);
    NodePtr bn_node = graph->AddNode(bn_op);
  }
public:
	GraphOptimizerPtr graph_optimizer_ptr_;
};


TEST_F(UTEST_trans_sub_graph_to_function_op, create_function_op)
{
	ComputeGraphPtr graph = make_shared<ComputeGraph>("test");
	CreateGraph(graph);
	for (auto &node : graph->GetDirectNode()) {
		auto op_desc_ptr = node->GetOpDesc();
		if (op_desc_ptr->GetName() == "relu" || op_desc_ptr->GetName() == "expm1") {
			AttrUtils::SetInt(op_desc_ptr, kThreadScopeId, 1);
		}
	}
	SubGraphNodeMap node_map;
	Status ret = graph_optimizer_ptr_->GetSubGraphNodes(*graph, node_map);
	EXPECT_EQ(ret, ffts::SUCCESS);
	map<uint32_t, vector<NodePtr>>::iterator it = node_map.begin();
	vector<NodePtr> node_vec = it->second;
	OpDescPtr op_desc_ptr = graph_optimizer_ptr_->CreateFunctionOp(node_vec);
	string func_op_name = op_desc_ptr->GetName();
	bool flag = false;
	if (func_op_name == "reluexpm1") {
		flag = true;
	}
	EXPECT_EQ(flag, true);
}

TEST_F(UTEST_trans_sub_graph_to_function_op, trans_single_sub_graph)
{
	ComputeGraphPtr graph = make_shared<ComputeGraph>("test");
	CreateGraph(graph);
	ge::ComputeGraphPtr src_graph = make_shared<ComputeGraph>("test_src_graph");
	ge::ComputeGraphPtr par_graph = make_shared<ComputeGraph>("test_par_graph");
	src_graph->SetParentGraph(par_graph);
	(void)graph->SetExtAttr("part_src_graph", src_graph);

	string session_graph_id = "-1_8";
	AttrUtils::SetStr(graph, ATTR_NAME_SESSION_GRAPH_ID, session_graph_id);
	for (auto &node : graph->GetDirectNode()) {
		auto op_desc_ptr = node->GetOpDesc();
		if (op_desc_ptr->GetName() == "relu" || op_desc_ptr->GetName() == "expm1") {
			AttrUtils::SetInt(op_desc_ptr, kThreadScopeId, 1);
		}
	}
	SubGraphNodeMap node_map;
	Status ret = graph_optimizer_ptr_->GetSubGraphNodes(*graph, node_map);
	EXPECT_EQ(ret, ffts::SUCCESS);
	map<uint32_t, vector<NodePtr>>::iterator it = node_map.begin();
	vector<NodePtr> node_vec = it->second;
	ret = graph_optimizer_ptr_->TransSingleSubGraph(*graph, node_vec);
	EXPECT_EQ(ret, ffts::SUCCESS);
	NodePtr nodeptr = graph->FindNode("relu");
	EXPECT_EQ(nodeptr, nullptr);
	nodeptr = graph->FindNode("expm1");
	EXPECT_EQ(nodeptr, nullptr);
}

TEST_F(UTEST_trans_sub_graph_to_function_op, test_sub_judge_function)
{
  ComputeGraphPtr graph = make_shared<ComputeGraph>("test");
  CreateGraph1(graph);
  vector<vector<vector<DimRange>>> inputTensorSlice;
  vector<vector<vector<DimRange>>> outputTensorSlice;
  vector<vector<DimRange>> threadSlice;
  uint32_t inputNum = 0;
  vector<DimRange> inputSlice;

  DimRange dimRange;
  dimRange.lower = 0;
  dimRange.higher = 1;
  inputSlice.push_back(dimRange);

  dimRange.lower = 0;
  dimRange.higher = 2;
  inputSlice.push_back(dimRange);

  dimRange.lower = 0;
  dimRange.higher = 3;
  inputSlice.push_back(dimRange);

  dimRange.lower = 0;
  dimRange.higher = 4;
  inputSlice.push_back(dimRange);

  threadSlice.push_back(inputSlice);

  inputTensorSlice.push_back(threadSlice);
  outputTensorSlice.push_back(threadSlice);
  graph_optimizer_ptr_->GetSliceInfo(*graph);
  for (auto &node : graph->GetDirectNode()) {
    graph_optimizer_ptr_->JudgeThreadTensorAlignedAndAlarm(node, inputTensorSlice, true);
    graph_optimizer_ptr_->JudgeThreadTensorAlignedAndAlarm(node, outputTensorSlice, false);
  }
}
