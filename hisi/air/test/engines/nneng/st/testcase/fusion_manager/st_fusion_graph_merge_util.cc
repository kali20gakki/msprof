#include <gtest/gtest.h>
#include <iostream>
#include <list>

#define protected public
#define private public
#include "common/configuration.h"
#include "common/fe_inner_attr_define.h"
#include "common/util/constants.h"
#include "graph/ge_tensor.h"
#include "graph/op_desc.h"
#include "graph/op_kernel_bin.h"
#include "graph/utils/graph_utils.h"
#include "graph_optimizer/ub_fusion/fusion_graph_merge/fusion_graph_merge.h"
#include "graph_optimizer/ub_fusion/automatic_buffer_fusion.h"
#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_optimizer.h"
#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_comm/l2_fusion_comm.h"
#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_parser/l2_fusion_parser.h"
#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_allocation/l2_fusion_allocation.h"

#undef private
#undef protected

using namespace std;
using namespace fe;
using namespace ge;

class fusion_graph_merge_util_st : public testing::Test {
public:
protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(fusion_graph_merge_util_st, set_l2_task_info_to_fusion_op) {
  auto graph_common = std::make_shared<GraphComm>("engineName");
  graph_common->Initialize();
  auto fusion_graph_merge_ptr =
      std::make_shared<FusionGraphMerge>("fusion_scope", graph_common);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  ge::OpDescPtr node1_op = std::make_shared<ge::OpDesc>("conv0","conv");
  ge::NodePtr node1 = std::make_shared<ge::Node>(node1_op, graph);
  Status ret = fusion_graph_merge_ptr->SetL2TaskInfoToFusionOp(node1);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(fusion_graph_merge_util_st, fuse_two_nodes) {
  std::shared_ptr<AutomaticBufferFusion> auto_buffer_fusion_ptr = std::make_shared<AutomaticBufferFusion>(nullptr);
  ComputeGraphPtr owner_graph = std::make_shared<ComputeGraph>("test");
  ge::OpDescPtr node1_op = std::make_shared<ge::OpDesc>("conv0","conv");
  ge::OpDescPtr node2_op = std::make_shared<ge::OpDesc>("conv1","conv");
  ge::NodePtr node1 = std::make_shared<ge::Node>(node1_op, owner_graph);
  ge::NodePtr node2 = std::make_shared<ge::Node>(node1_op, owner_graph);
  int64_t producer_scope_id = 0;
  int64_t consumer_scope_id = 0;
  Status ret = auto_buffer_fusion_ptr->FuseTwoNodes(node1, node2, producer_scope_id, consumer_scope_id);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(fusion_graph_merge_util_st, test_allocdata_standing_data_special)
{
    L2FusionAllocation l2fusionAlloc;
    k_l2_buffer_t l2;
    k_data_dependent_count_map count_map;
    k_l2_task_datas_map_t datas_map;
    k_l2_data_allocs_t standing_alloc_data;
    k_l2_datas_t converge_data;
    k_l2_task_data_allocs_map_t alloc_map;
    k_l2_datas_t output;
    uint64_t max_page = 63;
    uint32_t data_in_l2_id = 1;
    uint8_t priority = 5100;
    int64_t page_size = 1;
    int32_t page_num_left = 0;
    Status status = l2fusionAlloc.AllocateStandingDataSpecial(data_in_l2_id, priority, page_size, count_map,
                                                              standing_alloc_data, output,
                                                              page_num_left);
    EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(fusion_graph_merge_util_st, set_multi_kernel_output_offsets) {
  std::vector<int64_t> buffer_fusion_output_offset;
  std::vector<int64_t> save_pre_output_offset;
  buffer_fusion_output_offset.push_back(1);

  auto graph_common = std::make_shared<GraphComm>("engineName");
  graph_common->Initialize();
  auto fusion_graph_merge_ptr =
      std::make_shared<FusionGraphMerge>(SCOPE_ID_ATTR, graph_common);
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
  std::int64_t fusion_anchor_idx = 0;
  ge::OpDescPtr op_desc_src = make_shared<OpDesc>("sourceNode", "Conv");
  ge::OpDescPtr op_desc_fus = make_shared<OpDesc>("fusionNode", "Conv2D");
  ge::GeTensorDesc tensor_desc =
  ge::GeTensorDesc(ge::GeShape({4, 4}), FORMAT_ND, DT_FLOAT16);
  ge::AttrUtils::SetInt(tensor_desc, ge::ATTR_NAME_DATA_DUMP_ORIGIN_OUTPUT_INDEX,0);
  ge::AttrUtils::SetListInt(op_desc_fus, ge::ATTR_NAME_OUTPUT_OFFSET_FOR_BUFFER_FUSION, buffer_fusion_output_offset);
  op_desc_fus->AddOutputDesc(tensor_desc);
  op_desc_src->AddOutputDesc(tensor_desc);
  fusion_graph_merge_ptr->SetMultiKernelOutPutOffsets(op_desc_src, 2,
                                                      op_desc_fus, save_pre_output_offset);
  EXPECT_EQ(buffer_fusion_output_offset[1], 0);
}

TEST_F(fusion_graph_merge_util_st, update_output_ref_port_index) {
  std::vector<uint32_t> ref_port_index = {1};

  auto graph_common = std::make_shared<GraphComm>("engineName");
  graph_common->Initialize();
  auto fusion_graph_merge_ptr =
      std::make_shared<FusionGraphMerge>(SCOPE_ID_ATTR, graph_common);
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test_graph");
  std::int64_t fusion_anchor_idx = 0;
  ge::OpDescPtr op_desc_src = make_shared<OpDesc>("sourceNode", "Conv");
  ge::OpDescPtr op_desc_fus = make_shared<OpDesc>("fusionNode", "Conv2D");
  ge::GeTensorDesc tensor_desc_fus =
  ge::GeTensorDesc(ge::GeShape({4, 4}), FORMAT_ND, DT_FLOAT16);
  ge::GeTensorDesc tensor_desc_src =
  ge::GeTensorDesc(ge::GeShape({4, 4}), FORMAT_ND, DT_FLOAT16);
  ge::AttrUtils::SetListInt(tensor_desc_src, "ref_port_index", ref_port_index);
  op_desc_fus->AddOutputDesc(tensor_desc_fus);
  op_desc_fus->AddInputDesc(tensor_desc_fus);
  op_desc_fus->AddInputDesc(tensor_desc_fus);
  op_desc_fus->AddInputDesc(tensor_desc_fus);
  ge::AttrUtils::SetBool(op_desc_fus, ge::ATTR_NAME_REFERENCE, true);

  op_desc_src->AddOutputDesc(tensor_desc_src);
  fusion_graph_merge_ptr->UpdateOutputRefPortIndex(op_desc_src, op_desc_fus, 1);


  ge::AttrUtils::GetListInt(op_desc_src->MutableOutputDesc(0), "ref_port_index", ref_port_index);
  std::cout << "ref_port_index: " << ref_port_index[0] << std::endl;
  EXPECT_EQ(ref_port_index[0], 2);
}