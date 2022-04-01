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


#define protected public
#define private public
#include "common/util/op_info_util.h"
#include "common/configuration.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v100_not_requant_fusion_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/v200_not_requant_fusion_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/bias_optimize_quant_rollback/avgpool_quant_process_fusion_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/bias_optimize_quant_rollback/conv2d_quant_process_fusion_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/bias_optimize_quant_rollback/deconvolution_quant_process_fusion_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/bias_optimize_quant_rollback/dwconv2d_quant_process_fusion_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/bias_optimize_quant_rollback/fc_quant_process_fusion_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/bias_optimize_quant_rollback/matmulv2_quant_process_fusion_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/bias_optimize_quant_rollback/pooling_quant_process_fusion_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/bias_optimize_quant_rollback/batch_matmulv2_quant_process_fusion_pass.h"
#include "common/pass_manager.h"
#undef protected
#undef private

using namespace std;
using namespace ge;
using namespace fe;

#define KERNEL_NUM  2

    /**
  * @ingroup dnn
  * @brief mode of convolution
  */
typedef enum tag_cc_convolution_mode
{
    CC_CONV_CONVOLUTION = 0,            /**< math convolution */
    CC_CONV_CROSS_CORRELATION,          /**< cross-correlation convolution */
    CC_CONV_DECONVOLUTION,              /**< deconvolution, also named transposed convolution*/
    CC_CONV_MODE_DEPTHWISE,             /**< depthwise convolution*/
    CC_CONV_MODE_RESERVED
} ccConvolutionMode_t;

/**
  * @ingroup dnn
  * @brief mode of padding
  */
typedef enum tag_cc_padding_mode
{
    CC_PADDING_CEIL = 0,             /**< Default padding mode, same with caffe, same with MxNet full mode */
    CC_PADDING_DIRECTASSIGN,         /**< Same with caffe2 default padding mode: NOTSET */
    CC_PADDING_VALID,                /**< VALID padding mode , same with tensorflow VALID mode, same with MxNet valid */
    CC_PADDING_SAME,                 /**< Padding values of 0 are always used */
    CC_PADDING_CEIL_NEW,             /*new ceil,use for backward compatibility*/
    CC_PADDING_VALID_NEW,            /*new valid,use for backward compatibility*/
    CC_PADDING_SAME_NEW,             /*new same,use for backward compatibility*/
    CC_PADDING_RESERVED
} ccPaddingMode_t;

namespace fe {

class UTEST_quant_norequant_eltwise_fusion_pass : public testing::Test {
public:
    std::string DATA_TYPE = "Data";

protected:
    void SetUp()
    {
    }
    void TearDown()
    {

    }

protected:
    /*
 * [original graph]
 *
 * --->cube--->
 *
 * [processed graph]
 *
 *                 offset_const     bias_const
 *                       \              \
 *                        v              v
 * weight_const--->ascendweightquant--->cube--->
 *
 * */
    void InitConvOp(ComputeGraph &graph, NodePtr cube, int cnt)
    {
      // 创建weight_const
      OpDescPtr op_desc_weight_const = std::make_shared<OpDesc>("weight_const_" + std::to_string(cnt), "Const");
      vector<int64_t> dim_weight_const(4, 2);
      dim_weight_const[0] = KERNEL_NUM;
      GeShape weight_const_shape(dim_weight_const);
      GeTensorDesc out_desc_weight_const(weight_const_shape);
      op_desc_weight_const->AddOutputDesc(out_desc_weight_const);

      // 创建offset_const
      OpDescPtr op_desc_offset_const = std::make_shared<OpDesc>("offset_const_" + std::to_string(cnt), "Const");
      vector<int64_t> dim_offset_const = {1,1,1,2};
      GeShape offset_const_shape(dim_offset_const);
      GeTensorDesc out_desc_offset_const(offset_const_shape);
      op_desc_offset_const->AddOutputDesc(out_desc_offset_const);

      // 创建AscendWeightQuant
      OpDescPtr op_desc_awq = std::make_shared<OpDesc>("awq_" + std::to_string(cnt), "AscendWeightDequant");
      op_desc_awq->AddInputDesc(0, out_desc_weight_const);
      op_desc_awq->AddInputDesc(1, out_desc_offset_const);
      op_desc_awq->AddOutputDesc(out_desc_weight_const);

      // 创建bias_const
      OpDescPtr op_desc_bias_const = std::make_shared<OpDesc>("bias_const_" + std::to_string(cnt), "Const");
      op_desc_bias_const->AddOutputDesc(out_desc_weight_const);

      // 添加node
      NodePtr node_weight_const = graph.AddNode(op_desc_weight_const);
      NodePtr node_offset_const = graph.AddNode(op_desc_offset_const);
      NodePtr node_bias_const = graph.AddNode(op_desc_bias_const);
      NodePtr node_awq = graph.AddNode(op_desc_awq);

      // 构建边
      GraphUtils::AddEdge(node_weight_const->GetOutDataAnchor(0), node_awq->GetInDataAnchor(0));
      GraphUtils::AddEdge(node_offset_const->GetOutDataAnchor(0), node_awq->GetInDataAnchor(1));
      GraphUtils::AddEdge(node_awq->GetOutDataAnchor(0), cube->GetInDataAnchor(1));
      GraphUtils::AddEdge(node_bias_const->GetOutDataAnchor(0), cube->GetInDataAnchor(2));

      // 设置attr
      AttrUtils::SetInt(cube->GetOpDesc(), CONV_ATTR_NAME_MODE, CC_CONV_CONVOLUTION);
      AttrUtils::SetInt(cube->GetOpDesc(), CONV_ATTR_NAME_GROUP, 1);
      AttrUtils::SetInt(cube->GetOpDesc(), CONV_ATTR_NAME_PAD_MODE, CC_PADDING_VALID);
      AttrUtils::SetInt(cube->GetOpDesc(), CONV_ATTR_NAME_ALGO, -1);

      vector<int64_t> pad(4, 1);
      AttrUtils::SetListInt(cube->GetOpDesc(), CONV_ATTR_NAME_PAD, pad);

      vector<int64_t> stride(2, 2);
      AttrUtils::SetListInt(cube->GetOpDesc(), CONV_ATTR_NAME_STRIDE, stride);
    }


    void InitInputOp(NodePtr node)
    {
        //初始化卷积算子
        int8_t sample_conv_weight[KERNEL_NUM][20][2][2]=
                    {
                         {
                            {{1,2},{3,4}},
                            {{4,3},{2,1}},
                            {{1,2},{3,4}},
                            {{4,3},{2,1}},
                            {{1,2},{3,4}},
                            {{4,3},{2,1}},
                            {{1,2},{3,4}},
                            {{4,3},{2,1}},
                            {{1,2},{3,4}},
                            {{4,3},{2,1}},
                            {{1,2},{3,4}},
                            {{4,3},{2,1}},
                            {{1,2},{3,4}},
                            {{4,3},{2,1}},
                            {{1,2},{3,4}},
                            {{4,3},{2,1}},
                            {{1,2},{3,4}},
                            {{4,3},{2,1}},
                            {{1,2},{3,4}},
                            {{4,3},{2,1}}
                        },
                        {
                            {{2,1},{4,3}},
                            {{3,4},{1,2}},
                            {{2,1},{4,3}},
                            {{3,4},{1,2}},
                            {{2,1},{4,3}},
                            {{3,4},{1,2}},
                            {{2,1},{4,3}},
                            {{3,4},{1,2}},
                            {{2,1},{4,3}},
                            {{3,4},{1,2}},
                            {{2,1},{4,3}},
                            {{3,4},{1,2}},
                            {{2,1},{4,3}},
                            {{3,4},{1,2}},
                            {{2,1},{4,3}},
                            {{3,4},{1,2}},
                            {{2,1},{4,3}},
                            {{3,4},{1,2}},
                            {{2,1},{4,3}},
                            {{3,4},{1,2}}
                        }
                    };

        int32_t sample_conv_bias[KERNEL_NUM] =
                    {
                        1,3
                    };
        vector<GeTensorPtr> conv_weights = OpDescUtils::MutableWeights(node);

        vector<int64_t> dim(4, 2);
        dim[0] = KERNEL_NUM;
        dim[1] = 20;
        GeShape shape(dim);
        GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);
        TensorUtils::SetDataOffset(out_desc, 0);

        GeTensorPtr filter = std::make_shared<ge::GeTensor>(out_desc, (uint8_t *)sample_conv_weight, KERNEL_NUM * 2 * 2 * 2 * sizeof(int8_t));

        vector<int64_t> dim_bias(2, 1);
        dim_bias[1] = KERNEL_NUM;
        GeTensorDesc out_desc_bias(shape);
        TensorUtils::SetDataOffset(out_desc_bias, 0);
        GeTensorPtr bias = std::make_shared<ge::GeTensor>(out_desc_bias, (uint8_t *)sample_conv_bias, 2 * sizeof(int32_t));

        conv_weights.push_back(filter);
        conv_weights.push_back(bias);
        OpDescUtils::SetWeights(node, conv_weights);

        AttrUtils::SetInt(node->GetOpDesc(), CONV_ATTR_NAME_MODE, CC_CONV_CONVOLUTION);
        AttrUtils::SetInt(node->GetOpDesc(), CONV_ATTR_NAME_GROUP, 1);
        AttrUtils::SetInt(node->GetOpDesc(), CONV_ATTR_NAME_PAD_MODE, CC_PADDING_VALID);
        AttrUtils::SetInt(node->GetOpDesc(), CONV_ATTR_NAME_ALGO, -1);

        vector<int64_t> pad(4, 1);
        AttrUtils::SetListInt(node->GetOpDesc(), CONV_ATTR_NAME_PAD, pad);

        vector<int64_t> stride(2, 2);
        AttrUtils::SetListInt(node->GetOpDesc(), CONV_ATTR_NAME_STRIDE, stride);

    }


    void InitQuantOp(NodePtr node)
    {
        //初始化Bn算子OpDef

        AttrUtils::SetFloat(node->GetOpDesc(), "scale", 1.1);
        AttrUtils::SetFloat(node->GetOpDesc(), "offset", 1.2);

    }

    void InitDequantOp(NodePtr node)
    {
        //初始化Scale算子OpDef

        uint64_t sample_deq_scale[KERNEL_NUM] = {0x00001103392BCD31,
                                                 0x000022043717AB06};
        vector<GeTensorPtr> scale_weights = OpDescUtils::MutableWeights(node);

        vector<int64_t> dim{KERNEL_NUM};
        GeShape shape(dim);
        GeTensorDesc out_desc(shape);
        TensorUtils::SetDataOffset(out_desc, 0);

        GeTensorPtr scale_weight = std::make_shared<ge::GeTensor>(out_desc, (uint8_t *)sample_deq_scale, KERNEL_NUM * sizeof(uint64_t));

        scale_weights.push_back(scale_weight);
        OpDescUtils::SetWeights(node, scale_weights);

    }

    void InitDequantOpBias(NodePtr node)
    {
        //初始化Scale算子OpDef
        
        uint64_t sample_deq_scale[KERNEL_NUM] = {0x00000000392BCD31,
                                                 0x000000003717AB06};

        vector<GeTensorPtr> scale_weights = OpDescUtils::MutableWeights(node);

        vector<int64_t> dim{KERNEL_NUM};
        GeShape shape(dim);
        GeTensorDesc out_desc(shape);
        TensorUtils::SetDataOffset(out_desc, 0);

        GeTensorPtr scale_weight = std::make_shared<ge::GeTensor>(out_desc, (uint8_t *)sample_deq_scale, KERNEL_NUM * sizeof(uint64_t));

        scale_weights.push_back(scale_weight);
        OpDescUtils::SetWeights(node, scale_weights);

    }

    static void DumpGraph(const ge::ComputeGraphPtr graph, string graph_name)
    {
      printf("start to dump graph %s...\n", graph_name.c_str());
      for(ge::NodePtr node : graph->GetAllNodes()) {
        printf("node name = %s.\n", node->GetName().c_str());
        for (ge::OutDataAnchorPtr anchor : node->GetAllOutDataAnchors()) {
          for (ge::InDataAnchorPtr peer_in_anchor : anchor->GetPeerInDataAnchors()) {
            printf("    node name = %s[%d], out data node name = %s[%d].\n",
                   node->GetName().c_str(),
                   anchor->GetIdx(),
                   peer_in_anchor->GetOwnerNode()->GetName().c_str(),
                   peer_in_anchor->GetIdx());
          }
        }
        if (node->GetOutControlAnchor() != nullptr) {
          for (ge::InControlAnchorPtr peer_in_anchor : node->GetOutControlAnchor()->GetPeerInControlAnchors()) {
            printf("    node name = %s, out control node name = %s.\n", node->GetName().c_str(),
                   peer_in_anchor->GetOwnerNode()->GetName().c_str());
          }
        }
      }

      return;
    }
};

TEST_F(UTEST_quant_norequant_eltwise_fusion_pass, quant_norequant_eltwise_pattern_success){
    //(void)setenv("DUMP_GE_GRAPH", "2", 2);
    Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    //创建待融合网络
    OpDescPtr op_desc_01 = std::make_shared<OpDesc>("data1", DATA_TYPE);
    OpDescPtr op_desc_a1 = std::make_shared<OpDesc>("A1", "AscendQuant");
    OpDescPtr op_desc_b1 = std::make_shared<OpDesc>("B1", "Conv2D");
    OpDescPtr op_desc_c1 = std::make_shared<OpDesc>("C1", "AscendDequant");
    OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");

    OpDescPtr op_desc_02 = std::make_shared<OpDesc>("data2", DATA_TYPE);
    OpDescPtr op_desc_a2 = std::make_shared<OpDesc>("A2", "AscendQuant");
    OpDescPtr op_desc_b2 = std::make_shared<OpDesc>("B2", "Conv2D");
    OpDescPtr op_desc_c2 = std::make_shared<OpDesc>("C2", "AscendDequant");

    OpDescPtr op_desc_e = std::make_shared<OpDesc>("E", "Eltwise");
    OpDescPtr op_desc_r = std::make_shared<OpDesc>("R", "Relu");

    OpDescPtr op_desc_q = std::make_shared<OpDesc>("Q", "AscendQuant");
    OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "Conv2D");
    //add descriptor
    vector<int64_t> dim{16, 20, 16, 16};
    GeShape shape(dim);
    GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

    op_desc_01->AddOutputDesc(out_desc);
    op_desc_a1->AddInputDesc(out_desc);
    op_desc_a1->AddOutputDesc(out_desc);
    op_desc_b1->AddInputDesc(out_desc);
    // weight input of op_desc_b1
    op_desc_b1->AddInputDesc(out_desc);
    // bias input of op_desc_b1
    op_desc_b1->AddInputDesc(out_desc);
    op_desc_b1->AddOutputDesc(out_desc);
    op_desc_c1->AddInputDesc(out_desc);
    op_desc_c1->AddOutputDesc(out_desc);
    op_desc_d->AddInputDesc(out_desc);
    op_desc_d->AddOutputDesc(out_desc);

    op_desc_02->AddOutputDesc(out_desc);
    op_desc_a2->AddInputDesc(out_desc);
    op_desc_a2->AddOutputDesc(out_desc);
    op_desc_b2->AddInputDesc(out_desc);
    // weight input of op_desc_b2
    op_desc_b2->AddInputDesc(out_desc);
    // bias input of op_desc_b2
    op_desc_b2->AddInputDesc(out_desc);
    op_desc_b2->AddOutputDesc(out_desc);
    op_desc_c2->AddInputDesc(out_desc);
    op_desc_c2->AddOutputDesc(out_desc);

    op_desc_e->AddInputDesc(out_desc);
    op_desc_e->AddInputDesc(out_desc);
    op_desc_e->AddOutputDesc(out_desc);
    op_desc_r->AddInputDesc(out_desc);
    op_desc_r->AddOutputDesc(out_desc);
    op_desc_q->AddInputDesc(out_desc);
    op_desc_q->AddOutputDesc(out_desc);

    op_desc_c->AddInputDesc(out_desc);
    // weight input of op_desc_c
    op_desc_c->AddInputDesc(out_desc);
    // bias input of op_desc_c
    op_desc_c->AddInputDesc(out_desc);
    op_desc_c->AddOutputDesc(out_desc);

    //添加Node
    NodePtr node_01 = graph->AddNode(op_desc_01);
    NodePtr node_a1 = graph->AddNode(op_desc_a1);
    NodePtr node_b1 = graph->AddNode(op_desc_b1);
    NodePtr node_c1 = graph->AddNode(op_desc_c1);
    NodePtr node_d = graph->AddNode(op_desc_d);

    NodePtr node_02 = graph->AddNode(op_desc_02);
    NodePtr node_a2 = graph->AddNode(op_desc_a2);
    NodePtr node_b2 = graph->AddNode(op_desc_b2);
    NodePtr node_c2 = graph->AddNode(op_desc_c2);

    NodePtr node_e = graph->AddNode(op_desc_e);
    NodePtr node_r = graph->AddNode(op_desc_r);
    NodePtr node_q = graph->AddNode(op_desc_q);

    NodePtr node_c = graph->AddNode(op_desc_c);

    //网络初始化
    InitQuantOp(node_a1);
    InitQuantOp(node_a2);
    InitQuantOp(node_q);
    InitConvOp(*graph, node_b1, 0);
    InitConvOp(*graph, node_b2, 1);
    InitConvOp(*graph, node_c, 2);
    InitDequantOp(node_c1);
    InitDequantOp(node_c2);
  ge::AttrUtils::SetStr(node_c1->GetOpDesc(), STR_QUANT_MODE, STR_QUANT_HIGH_PERFORMANCE);
  ge::AttrUtils::SetStr(node_c2->GetOpDesc(), STR_QUANT_MODE, STR_QUANT_HIGH_PERFORMANCE);

    //构建边
    GraphUtils::AddEdge(node_01->GetOutDataAnchor(0),
                        node_a1->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_a1->GetOutDataAnchor(0),
                        node_b1->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_b1->GetOutDataAnchor(0),
                        node_c1->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_c1->GetOutDataAnchor(0),
                        node_d->GetInDataAnchor(0));

    GraphUtils::AddEdge(node_02->GetOutDataAnchor(0),
                        node_a2->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_a2->GetOutDataAnchor(0),
                        node_b2->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_b2->GetOutDataAnchor(0),
                        node_c2->GetInDataAnchor(0));

    GraphUtils::AddEdge(node_d->GetOutDataAnchor(0),
                        node_e->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_c2->GetOutDataAnchor(0),
                        node_e->GetInDataAnchor(1));
    GraphUtils::AddEdge(node_e->GetOutDataAnchor(0),
                        node_r->GetInDataAnchor(0));

    GraphUtils::AddEdge(node_r->GetOutDataAnchor(0),
                        node_q->GetInDataAnchor(0));

    GraphUtils::AddEdge(node_q->GetOutDataAnchor(0),
                        node_c->GetInDataAnchor(0));

    //GraphUtils::DumpGEGraphToOnnx(*graph, "before_quant_norequant_eltwise");
    //执行融合
    AvgPoolQuantProcessFusionPass pass1;
    Conv2DQuantProcessFusionPass pass2;
    DeConvQuantProcessFusionPass pass3;
    DWConv2DQuantProcessFusionPass pass4;
    FCQuantProcessFusionPass pass5;
    MatmulV2QuantProcessFusionPass pass6;
    PoolingQuantProcessFusionPass pass7;
    V200NotRequantFusionPass pass8;
    BatchMatmulV2QuantProcessFusionPass pass9;
    vector<GraphPass*> passes = {&pass1, &pass2, &pass3, &pass4, &pass5, &pass6, &pass7, &pass8, &pass9};
    Status status = PassManager::Run(*graph, passes);
    EXPECT_EQ(fe::SUCCESS, status);
    //GraphUtils::DumpGEGraphToOnnx(*graph, "after_quant_norequant_eltwise");

    int32_t op_num = graph->GetDirectNode().size();
    printf("op num %d", op_num);
    EXPECT_EQ(op_num, 27);
    Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
    //unsetenv("DUMP_GE_GRAPH");
}

TEST_F(UTEST_quant_norequant_eltwise_fusion_pass, quant_norequant_double_eltwise_pattern_success){
    //(void)setenv("DUMP_GE_GRAPH", "2", 2);
    Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    //创建待融合网络
    OpDescPtr op_desc_01 = std::make_shared<OpDesc>("data1", DATA_TYPE);
    OpDescPtr op_desc_a1 = std::make_shared<OpDesc>("A1", "AscendQuant");
    OpDescPtr op_desc_b1 = std::make_shared<OpDesc>("B1", "Conv2D");
    OpDescPtr op_desc_c1 = std::make_shared<OpDesc>("C1", "AscendDequant");
    OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");

    OpDescPtr op_desc_02 = std::make_shared<OpDesc>("data2", DATA_TYPE);
    OpDescPtr op_desc_a2 = std::make_shared<OpDesc>("A2", "AscendQuant");
    OpDescPtr op_desc_b2 = std::make_shared<OpDesc>("B2", "Conv2D");
    OpDescPtr op_desc_c2 = std::make_shared<OpDesc>("C2", "AscendDequant");

    OpDescPtr op_desc_e = std::make_shared<OpDesc>("E", "Eltwise");
    OpDescPtr op_desc_r = std::make_shared<OpDesc>("R", "Relu");

    OpDescPtr op_desc_a3 = std::make_shared<OpDesc>("A3", "AscendQuant");
    OpDescPtr op_desc_b3 = std::make_shared<OpDesc>("B3", "Conv2D");
    OpDescPtr op_desc_c3 = std::make_shared<OpDesc>("C3", "AscendDequant");

    OpDescPtr op_desc_e2 = std::make_shared<OpDesc>("E1", "Eltwise");
    OpDescPtr op_desc_r2 = std::make_shared<OpDesc>("R1", "Relu");

    OpDescPtr op_desc_q = std::make_shared<OpDesc>("Q", "AscendQuant");
    OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "Conv2D");
    //add descriptor
    vector<int64_t> dim{16, 20, 16, 16};
    GeShape shape(dim);
    GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

    op_desc_01->AddOutputDesc(out_desc);
    op_desc_a1->AddInputDesc(out_desc);
    op_desc_a1->AddOutputDesc(out_desc);
    op_desc_b1->AddInputDesc(out_desc);
    // weight input of op_desc_b1
    op_desc_b1->AddInputDesc(out_desc);
    // bias input of op_desc_b1
    op_desc_b1->AddInputDesc(out_desc);
    op_desc_b1->AddOutputDesc(out_desc);
    op_desc_c1->AddInputDesc(out_desc);
    op_desc_c1->AddOutputDesc(out_desc);
    op_desc_d->AddInputDesc(out_desc);
    op_desc_d->AddOutputDesc(out_desc);

    op_desc_02->AddOutputDesc(out_desc);
    op_desc_a2->AddInputDesc(out_desc);
    op_desc_a2->AddOutputDesc(out_desc);
    op_desc_b2->AddInputDesc(out_desc);
    // weight input of op_desc_b2
    op_desc_b2->AddInputDesc(out_desc);
    // bias input of op_desc_b2
    op_desc_b2->AddInputDesc(out_desc);
    op_desc_b2->AddOutputDesc(out_desc);
    op_desc_c2->AddInputDesc(out_desc);
    op_desc_c2->AddOutputDesc(out_desc);

    op_desc_e->AddInputDesc(out_desc);
    op_desc_e->AddInputDesc(out_desc);
    op_desc_e->AddOutputDesc(out_desc);
    op_desc_r->AddInputDesc(out_desc);
    op_desc_r->AddOutputDesc(out_desc);

    op_desc_a3->AddInputDesc(out_desc);
    op_desc_a3->AddOutputDesc(out_desc);
    op_desc_b3->AddInputDesc(out_desc);
    // weight input of op_desc_b3
    op_desc_b3->AddInputDesc(out_desc);
    // bias input of op_desc_b3
    op_desc_b3->AddInputDesc(out_desc);
    op_desc_b3->AddOutputDesc(out_desc);
    op_desc_c3->AddInputDesc(out_desc);
    op_desc_c3->AddOutputDesc(out_desc);

    op_desc_e2->AddInputDesc(out_desc);
    op_desc_e2->AddInputDesc(out_desc);
    op_desc_e2->AddOutputDesc(out_desc);
    op_desc_r2->AddInputDesc(out_desc);
    op_desc_r2->AddOutputDesc(out_desc);

    op_desc_q->AddInputDesc(out_desc);
    op_desc_q->AddOutputDesc(out_desc);

    op_desc_c->AddInputDesc(out_desc);

    //添加Node
    NodePtr node_01 = graph->AddNode(op_desc_01);
    NodePtr node_a1 = graph->AddNode(op_desc_a1);
    NodePtr node_b1 = graph->AddNode(op_desc_b1);
    NodePtr node_c1 = graph->AddNode(op_desc_c1);
    NodePtr node_d = graph->AddNode(op_desc_d);

    NodePtr node_02 = graph->AddNode(op_desc_02);
    NodePtr node_a2 = graph->AddNode(op_desc_a2);
    NodePtr node_b2 = graph->AddNode(op_desc_b2);
    NodePtr node_c2 = graph->AddNode(op_desc_c2);

    NodePtr node_e = graph->AddNode(op_desc_e);
    NodePtr node_r = graph->AddNode(op_desc_r);

    NodePtr node_a3 = graph->AddNode(op_desc_a3);
    NodePtr node_b3 = graph->AddNode(op_desc_b3);
    NodePtr node_c3 = graph->AddNode(op_desc_c3);

    NodePtr node_e2 = graph->AddNode(op_desc_e2);
    NodePtr node_r2 = graph->AddNode(op_desc_r2);
    NodePtr node_q = graph->AddNode(op_desc_q);

    NodePtr node_c = graph->AddNode(op_desc_c);

    //网络初始化
    InitQuantOp(node_a1);
    InitQuantOp(node_a2);
    InitQuantOp(node_a3);
    InitQuantOp(node_q);
    InitConvOp(*graph, node_b1, 0);
    InitConvOp(*graph, node_b2, 1);
    InitConvOp(*graph, node_b3, 2);
    InitConvOp(*graph, node_c, 3);
    InitDequantOp(node_c1);
    InitDequantOp(node_c2);
    InitDequantOp(node_c3);
    ge::AttrUtils::SetStr(node_c1->GetOpDesc(), STR_QUANT_MODE, STR_QUANT_HIGH_PERFORMANCE);
    ge::AttrUtils::SetStr(node_c2->GetOpDesc(), STR_QUANT_MODE, STR_QUANT_HIGH_PERFORMANCE);
    ge::AttrUtils::SetStr(node_c3->GetOpDesc(), STR_QUANT_MODE, STR_QUANT_HIGH_PERFORMANCE);

    //构建边
    GraphUtils::AddEdge(node_01->GetOutDataAnchor(0),
                        node_a1->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_a1->GetOutDataAnchor(0),
                        node_b1->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_b1->GetOutDataAnchor(0),
                        node_c1->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_c1->GetOutDataAnchor(0),
                        node_d->GetInDataAnchor(0));

    GraphUtils::AddEdge(node_02->GetOutDataAnchor(0),
                        node_a2->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_a2->GetOutDataAnchor(0),
                        node_b2->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_b2->GetOutDataAnchor(0),
                        node_c2->GetInDataAnchor(0));

    GraphUtils::AddEdge(node_d->GetOutDataAnchor(0),
                        node_e->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_c2->GetOutDataAnchor(0),
                        node_e->GetInDataAnchor(1));
    GraphUtils::AddEdge(node_e->GetOutDataAnchor(0),
                        node_r->GetInDataAnchor(0));

    GraphUtils::AddEdge(node_r->GetOutDataAnchor(0),
                        node_e2->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_r->GetOutDataAnchor(0),
                        node_a3->GetInDataAnchor(0));

    GraphUtils::AddEdge(node_a3->GetOutDataAnchor(0),
                        node_b3->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_b3->GetOutDataAnchor(0),
                        node_c3->GetInDataAnchor(0));

    GraphUtils::AddEdge(node_c3->GetOutDataAnchor(0),
                        node_e2->GetInDataAnchor(1));
    GraphUtils::AddEdge(node_e2->GetOutDataAnchor(0),
                        node_r2->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_r2->GetOutDataAnchor(0),
                        node_q->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_q->GetOutDataAnchor(0),
                        node_c->GetInDataAnchor(0));

    //GraphUtils::DumpGEGraphToOnnx(*graph, "before_quant_norequant_eltwise");
    //执行融合
    Conv2DQuantProcessFusionPass pass2;
    V200NotRequantFusionPass pass8;
    vector<GraphPass*> passes = {&pass2, &pass8};
    DumpGraph(graph, "before notrequant pattern2 fusion");
    Status status = PassManager::Run(*graph, passes);
    EXPECT_EQ(fe::SUCCESS, status);
    DumpGraph(graph, "after notrequant pattern2 fusion");
    int32_t dequant_s16_num = 0;
    int32_t requant_s16_num = 0;
    for (auto node : graph->GetDirectNode()) {
      if (node->GetType() == "AscendDequantS16")
        dequant_s16_num++;
      if (node->GetType() == "AscendRequantS16")
        requant_s16_num++;
      printf("\nOp name is %s, type is %s.", node->GetName().c_str(), node->GetType().c_str());
    }
    EXPECT_EQ(dequant_s16_num, 3);
    EXPECT_EQ(requant_s16_num, 2);
    //GraphUtils::DumpGEGraphToOnnx(*graph, "after_quant_norequant_eltwise");

    int32_t op_num = graph->GetDirectNode().size();
    printf("op num %d", op_num);
    EXPECT_EQ(op_num, 37);
    Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
    //unsetenv("DUMP_GE_GRAPH");
}

TEST_F(UTEST_quant_norequant_eltwise_fusion_pass, quant_norequant_eltwise_pattern_tf_success){
  //(void)setenv("DUMP_GE_GRAPH", "2", 2);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  //创建待融合网络
  OpDescPtr op_desc_01 = std::make_shared<OpDesc>("data1", DATA_TYPE);
  OpDescPtr op_desc_a1 = std::make_shared<OpDesc>("A1", "AscendQuant");
  OpDescPtr op_desc_b1 = std::make_shared<OpDesc>("B1", "Conv2D");
  OpDescPtr op_desc_c1 = std::make_shared<OpDesc>("C1", "AscendDequant");
  OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");

  OpDescPtr op_desc_02 = std::make_shared<OpDesc>("data2", DATA_TYPE);
  OpDescPtr op_desc_a2 = std::make_shared<OpDesc>("A2", "AscendQuant");
  OpDescPtr op_desc_b2 = std::make_shared<OpDesc>("B2", "Conv2D");
  OpDescPtr op_desc_c2 = std::make_shared<OpDesc>("C2", "AscendDequant");

  OpDescPtr op_desc_e = std::make_shared<OpDesc>("E", "Eltwise");
  OpDescPtr op_desc_r = std::make_shared<OpDesc>("R", "Relu");

  OpDescPtr op_desc_q = std::make_shared<OpDesc>("Q", "AscendQuant");
  OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "Conv2D");
  //add descriptor
  vector<int64_t> dim{16, 20, 16, 16};
  GeShape shape(dim);
GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

  op_desc_01->AddOutputDesc(out_desc);
  op_desc_a1->AddInputDesc(out_desc);
  op_desc_a1->AddOutputDesc(out_desc);
  op_desc_b1->AddInputDesc(out_desc);
  // weight input of op_desc_b1
  op_desc_b1->AddInputDesc(out_desc);
  // bias input of op_desc_b1
  op_desc_b1->AddInputDesc(out_desc);
  op_desc_b1->AddOutputDesc(out_desc);
  op_desc_c1->AddInputDesc(out_desc);
  op_desc_c1->AddOutputDesc(out_desc);
  op_desc_d->AddInputDesc(out_desc);
  op_desc_d->AddOutputDesc(out_desc);

  op_desc_02->AddOutputDesc(out_desc);
  op_desc_a2->AddInputDesc(out_desc);
  op_desc_a2->AddOutputDesc(out_desc);
  op_desc_b2->AddInputDesc(out_desc);
  // weight input of op_desc_b2
  op_desc_b2->AddInputDesc(out_desc);
  // bias input of op_desc_b2
  op_desc_b2->AddInputDesc(out_desc);
  op_desc_b2->AddOutputDesc(out_desc);
  op_desc_c2->AddInputDesc(out_desc);
  op_desc_c2->AddOutputDesc(out_desc);

  op_desc_e->AddInputDesc(out_desc);
  op_desc_e->AddInputDesc(out_desc);
  op_desc_e->AddOutputDesc(out_desc);
  op_desc_r->AddInputDesc(out_desc);
  op_desc_r->AddOutputDesc(out_desc);
  op_desc_q->AddInputDesc(out_desc);
  op_desc_q->AddOutputDesc(out_desc);

  op_desc_c->AddInputDesc(out_desc);
  // weight input of op_desc_c
  op_desc_c->AddInputDesc(out_desc);
  // bias input of op_desc_c
  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddOutputDesc(out_desc);

  ge::AttrUtils::SetInt(op_desc_a1, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_b1, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_c1, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_c1, "platformTF_N", 1);
  ge::AttrUtils::SetInt(op_desc_d, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_a2, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_b2, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_c2, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_c2, "platformTF_N", 1);
  ge::AttrUtils::SetInt(op_desc_e, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_r, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_q, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_c, "platform", 1);
  vector<uint32_t> offset_w_data = {1, 1};
  ge::AttrUtils::SetListInt(op_desc_c, "platformTF_offsetW", offset_w_data);
  ge::AttrUtils::SetListInt(op_desc_b1, "platformTF_offsetW", offset_w_data);
  ge::AttrUtils::SetListInt(op_desc_b2, "platformTF_offsetW", offset_w_data);

  //添加Node
  NodePtr node_01 = graph->AddNode(op_desc_01);
  NodePtr node_a1 = graph->AddNode(op_desc_a1);
  NodePtr node_b1 = graph->AddNode(op_desc_b1);
  NodePtr node_c1 = graph->AddNode(op_desc_c1);
  NodePtr node_d = graph->AddNode(op_desc_d);

  NodePtr node_02 = graph->AddNode(op_desc_02);
  NodePtr node_a2 = graph->AddNode(op_desc_a2);
  NodePtr node_b2 = graph->AddNode(op_desc_b2);
  NodePtr node_c2 = graph->AddNode(op_desc_c2);

  NodePtr node_e = graph->AddNode(op_desc_e);
  NodePtr node_r = graph->AddNode(op_desc_r);
  NodePtr node_q = graph->AddNode(op_desc_q);

  NodePtr node_c = graph->AddNode(op_desc_c);

  //网络初始化
  InitQuantOp(node_a1);
  InitQuantOp(node_a2);
  InitQuantOp(node_q);
  InitConvOp(*graph, node_b1, 0);
  InitConvOp(*graph, node_b2, 1);
  InitConvOp(*graph, node_c, 2);
  InitDequantOp(node_c1);
  InitDequantOp(node_c2);
ge::AttrUtils::SetStr(node_c1->GetOpDesc(), STR_QUANT_MODE, STR_QUANT_HIGH_PERFORMANCE);
ge::AttrUtils::SetStr(node_c2->GetOpDesc(), STR_QUANT_MODE, STR_QUANT_HIGH_PERFORMANCE);

  //构建边
  GraphUtils::AddEdge(node_01->GetOutDataAnchor(0),
                      node_a1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a1->GetOutDataAnchor(0),
                      node_b1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_b1->GetOutDataAnchor(0),
                      node_c1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_c1->GetOutDataAnchor(0),
                      node_d->GetInDataAnchor(0));

  GraphUtils::AddEdge(node_02->GetOutDataAnchor(0),
                      node_a2->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a2->GetOutDataAnchor(0),
                      node_b2->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_b2->GetOutDataAnchor(0),
                      node_c2->GetInDataAnchor(0));

  GraphUtils::AddEdge(node_d->GetOutDataAnchor(0),
                      node_e->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_c2->GetOutDataAnchor(0),
                      node_e->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_e->GetOutDataAnchor(0),
                      node_r->GetInDataAnchor(0));

  GraphUtils::AddEdge(node_r->GetOutDataAnchor(0),
                      node_q->GetInDataAnchor(0));

  GraphUtils::AddEdge(node_q->GetOutDataAnchor(0),
                      node_c->GetInDataAnchor(0));

  //GraphUtils::DumpGEGraphToOnnx(*graph, "before_quant_norequant_eltwise");
  //执行融合
    AvgPoolQuantProcessFusionPass pass1;
    Conv2DQuantProcessFusionPass pass2;
    DeConvQuantProcessFusionPass pass3;
    DWConv2DQuantProcessFusionPass pass4;
    FCQuantProcessFusionPass pass5;
    MatmulV2QuantProcessFusionPass pass6;
    PoolingQuantProcessFusionPass pass7;
    V200NotRequantFusionPass pass8;
    BatchMatmulV2QuantProcessFusionPass pass9;
    vector<GraphPass*> passes = {&pass1, &pass2, &pass3, &pass4, &pass5, &pass6, &pass7, &pass8, &pass9};
  Status status = PassManager::Run(*graph, passes);
  EXPECT_EQ(fe::SUCCESS, status);
  //GraphUtils::DumpGEGraphToOnnx(*graph, "after_quant_norequant_eltwise");

  int32_t op_num = graph->GetDirectNode().size();
  printf("op num %d", op_num);
  EXPECT_EQ(op_num, 27);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  //unsetenv("DUMP_GE_GRAPH");
}

TEST_F(UTEST_quant_norequant_eltwise_fusion_pass, quant_norequant_eltwise_pattern_tf_fail1){
  //(void)setenv("DUMP_GE_GRAPH", "2", 2);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  //创建待融合网络
  OpDescPtr op_desc_01 = std::make_shared<OpDesc>("data1", DATA_TYPE);
  OpDescPtr op_desc_a1 = std::make_shared<OpDesc>("A1", "AscendQuant");
  OpDescPtr op_desc_b1 = std::make_shared<OpDesc>("B1", "Conv2D");
  OpDescPtr op_desc_c1 = std::make_shared<OpDesc>("C1", "AscendDequant");
  OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");

  OpDescPtr op_desc_02 = std::make_shared<OpDesc>("data2", DATA_TYPE);
  OpDescPtr op_desc_a2 = std::make_shared<OpDesc>("A2", "AscendQuant");
  OpDescPtr op_desc_b2 = std::make_shared<OpDesc>("B2", "Conv2D");
  OpDescPtr op_desc_c2 = std::make_shared<OpDesc>("C2", "AscendDequant");

  OpDescPtr op_desc_e = std::make_shared<OpDesc>("E", "Eltwise");
  OpDescPtr op_desc_r = std::make_shared<OpDesc>("R", "Relu");

  OpDescPtr op_desc_q = std::make_shared<OpDesc>("Q", "AscendQuant");
  OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "Conv2D");
  //add descriptor
  vector<int64_t> dim{16, 20, 16, 16};
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

  op_desc_01->AddOutputDesc(out_desc);
  op_desc_a1->AddInputDesc(out_desc);
  op_desc_a1->AddOutputDesc(out_desc);
  op_desc_b1->AddInputDesc(out_desc);
  // weight input of op_desc_b1
  op_desc_b1->AddInputDesc(out_desc);
  // bias input of op_desc_b1
  op_desc_b1->AddInputDesc(out_desc);
  op_desc_b1->AddOutputDesc(out_desc);
  op_desc_c1->AddInputDesc(out_desc);
  op_desc_c1->AddOutputDesc(out_desc);
  op_desc_d->AddInputDesc(out_desc);
  op_desc_d->AddOutputDesc(out_desc);

  op_desc_02->AddOutputDesc(out_desc);
  op_desc_a2->AddInputDesc(out_desc);
  op_desc_a2->AddOutputDesc(out_desc);
  op_desc_b2->AddInputDesc(out_desc);
  // weight input of op_desc_b2
  op_desc_b2->AddInputDesc(out_desc);
  // bias input of op_desc_b2
  op_desc_b2->AddInputDesc(out_desc);
  op_desc_b2->AddOutputDesc(out_desc);
  op_desc_c2->AddInputDesc(out_desc);
  op_desc_c2->AddOutputDesc(out_desc);

  op_desc_e->AddInputDesc(out_desc);
  op_desc_e->AddInputDesc(out_desc);
  op_desc_e->AddOutputDesc(out_desc);
  op_desc_r->AddInputDesc(out_desc);
  op_desc_r->AddOutputDesc(out_desc);
  op_desc_q->AddInputDesc(out_desc);
  op_desc_q->AddOutputDesc(out_desc);

  op_desc_c->AddInputDesc(out_desc);
  // weight input of op_desc_c
  op_desc_c->AddInputDesc(out_desc);
  // bias input of op_desc_c
  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddOutputDesc(out_desc);

  ge::AttrUtils::SetInt(op_desc_a1, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_b1, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_c1, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_c1, "platformTF_N", 1);
  ge::AttrUtils::SetInt(op_desc_d, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_a2, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_b2, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_c2, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_c2, "platformTF_N", 2);
  ge::AttrUtils::SetInt(op_desc_e, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_r, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_q, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_c, "platform", 1);
  vector<uint32_t> offset_w_data = {1, 1};
  ge::AttrUtils::SetListInt(op_desc_c, "platformTF_offsetW", offset_w_data);
  ge::AttrUtils::SetListInt(op_desc_b1, "platformTF_offsetW", offset_w_data);
  ge::AttrUtils::SetListInt(op_desc_b2, "platformTF_offsetW", offset_w_data);

  //添加Node
  NodePtr node_01 = graph->AddNode(op_desc_01);
  NodePtr node_a1 = graph->AddNode(op_desc_a1);
  NodePtr node_b1 = graph->AddNode(op_desc_b1);
  NodePtr node_c1 = graph->AddNode(op_desc_c1);
  NodePtr node_d = graph->AddNode(op_desc_d);

  NodePtr node_02 = graph->AddNode(op_desc_02);
  NodePtr node_a2 = graph->AddNode(op_desc_a2);
  NodePtr node_b2 = graph->AddNode(op_desc_b2);
  NodePtr node_c2 = graph->AddNode(op_desc_c2);

  NodePtr node_e = graph->AddNode(op_desc_e);
  NodePtr node_r = graph->AddNode(op_desc_r);
  NodePtr node_q = graph->AddNode(op_desc_q);

  NodePtr node_c = graph->AddNode(op_desc_c);

  //网络初始化
  InitQuantOp(node_a1);
  InitQuantOp(node_a2);
  InitQuantOp(node_q);
  InitConvOp(*graph, node_b1, 0);
  InitConvOp(*graph, node_b2, 1);
  InitConvOp(*graph, node_c, 2);
  InitDequantOp(node_c1);
  InitDequantOp(node_c2);
  ge::AttrUtils::SetStr(node_c1->GetOpDesc(), STR_QUANT_MODE, STR_QUANT_HIGH_PERFORMANCE);
  ge::AttrUtils::SetStr(node_c2->GetOpDesc(), STR_QUANT_MODE, STR_QUANT_HIGH_PERFORMANCE);
  //构建边
  GraphUtils::AddEdge(node_01->GetOutDataAnchor(0),
                      node_a1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a1->GetOutDataAnchor(0),
                      node_b1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_b1->GetOutDataAnchor(0),
                      node_c1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_c1->GetOutDataAnchor(0),
                      node_d->GetInDataAnchor(0));

  GraphUtils::AddEdge(node_02->GetOutDataAnchor(0),
                      node_a2->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a2->GetOutDataAnchor(0),
                      node_b2->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_b2->GetOutDataAnchor(0),
                      node_c2->GetInDataAnchor(0));

  GraphUtils::AddEdge(node_d->GetOutDataAnchor(0),
                      node_e->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_c2->GetOutDataAnchor(0),
                      node_e->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_e->GetOutDataAnchor(0),
                      node_r->GetInDataAnchor(0));

  GraphUtils::AddEdge(node_r->GetOutDataAnchor(0),
                      node_q->GetInDataAnchor(0));

  GraphUtils::AddEdge(node_q->GetOutDataAnchor(0),
                      node_c->GetInDataAnchor(0));

  //GraphUtils::DumpGEGraphToOnnx(*graph, "before_quant_norequant_eltwise");
  //执行融合
  AvgPoolQuantProcessFusionPass pass1;
  Conv2DQuantProcessFusionPass pass2;
  DeConvQuantProcessFusionPass pass3;
  DWConv2DQuantProcessFusionPass pass4;
  FCQuantProcessFusionPass pass5;
  MatmulV2QuantProcessFusionPass pass6;
  PoolingQuantProcessFusionPass pass7;
  V200NotRequantFusionPass pass8;
  BatchMatmulV2QuantProcessFusionPass pass9;
  vector<GraphPass*> passes = {&pass1, &pass2, &pass3, &pass4, &pass5, &pass6, &pass7, &pass8, &pass9};
  Status status = PassManager::Run(*graph, passes);
  EXPECT_EQ(fe::SUCCESS, status);
  //GraphUtils::DumpGEGraphToOnnx(*graph, "after_quant_norequant_eltwise");

  int32_t op_num = graph->GetDirectNode().size();
  printf("op num %d", op_num);
  EXPECT_EQ(op_num, 27);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  //unsetenv("DUMP_GE_GRAPH");
}

TEST_F(UTEST_quant_norequant_eltwise_fusion_pass, quant_norequant_eltwise_pattern_tf2){
  //(void)setenv("DUMP_GE_GRAPH", "2", 2);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  //创建待融合网络
  OpDescPtr op_desc_01 = std::make_shared<OpDesc>("data1", DATA_TYPE);
  OpDescPtr op_desc_a1 = std::make_shared<OpDesc>("A1", "AscendQuant");
  OpDescPtr op_desc_b1 = std::make_shared<OpDesc>("B1", "Conv2D");
  OpDescPtr op_desc_c1 = std::make_shared<OpDesc>("C1", "AscendDequant");
  OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");

  OpDescPtr op_desc_02 = std::make_shared<OpDesc>("data2", DATA_TYPE);
  OpDescPtr op_desc_a2 = std::make_shared<OpDesc>("A2", "AscendQuant");
  OpDescPtr op_desc_b2 = std::make_shared<OpDesc>("B2", "Conv2D");
  OpDescPtr op_desc_c2 = std::make_shared<OpDesc>("C2", "AscendDequant");

  OpDescPtr op_desc_e = std::make_shared<OpDesc>("E", "Eltwise");
  OpDescPtr op_desc_r = std::make_shared<OpDesc>("R", "Relu");

  OpDescPtr op_desc_q = std::make_shared<OpDesc>("Q", "AscendQuant");
  OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "Conv2D");
  //add descriptor
  vector<int64_t> dim{16, 20, 16, 16};
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

  op_desc_01->AddOutputDesc(out_desc);
  op_desc_a1->AddInputDesc(out_desc);
  op_desc_a1->AddOutputDesc(out_desc);
  op_desc_b1->AddInputDesc(out_desc);
  // weight input of op_desc_b1
  op_desc_b1->AddInputDesc(out_desc);
  // bias input of op_desc_b1
  op_desc_b1->AddInputDesc(out_desc);
  op_desc_b1->AddOutputDesc(out_desc);
  op_desc_c1->AddInputDesc(out_desc);
  op_desc_c1->AddOutputDesc(out_desc);
  op_desc_d->AddInputDesc(out_desc);
  op_desc_d->AddOutputDesc(out_desc);

  op_desc_02->AddOutputDesc(out_desc);
  op_desc_a2->AddInputDesc(out_desc);
  op_desc_a2->AddOutputDesc(out_desc);
  op_desc_b2->AddInputDesc(out_desc);
  // weight input of op_desc_b2
  op_desc_b2->AddInputDesc(out_desc);
  // bias input of op_desc_b2
  op_desc_b2->AddInputDesc(out_desc);
  op_desc_b2->AddOutputDesc(out_desc);
  op_desc_c2->AddInputDesc(out_desc);
  op_desc_c2->AddOutputDesc(out_desc);

  op_desc_e->AddInputDesc(out_desc);
  op_desc_e->AddInputDesc(out_desc);
  op_desc_e->AddOutputDesc(out_desc);
  op_desc_r->AddInputDesc(out_desc);
  op_desc_r->AddOutputDesc(out_desc);
  op_desc_q->AddInputDesc(out_desc);
  op_desc_q->AddOutputDesc(out_desc);

  op_desc_c->AddInputDesc(out_desc);
  // weight input of op_desc_c
  op_desc_c->AddInputDesc(out_desc);
  // bias input of op_desc_c
  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddOutputDesc(out_desc);

  ge::AttrUtils::SetInt(op_desc_a1, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_b1, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_c1, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_c1, "platformTF_N", 1);
  ge::AttrUtils::SetInt(op_desc_d, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_a2, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_b2, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_c2, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_c2, "platformTF_N", 1);
  ge::AttrUtils::SetInt(op_desc_e, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_r, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_q, "platform", 1);
  ge::AttrUtils::SetInt(op_desc_c, "platform", 1);
  vector<uint32_t> offset_w_data = {1, 1};
  ge::AttrUtils::SetListInt(op_desc_c, "platformTF_offsetW", offset_w_data);

  //添加Node
  NodePtr node_01 = graph->AddNode(op_desc_01);
  NodePtr node_a1 = graph->AddNode(op_desc_a1);
  NodePtr node_b1 = graph->AddNode(op_desc_b1);
  NodePtr node_c1 = graph->AddNode(op_desc_c1);
  NodePtr node_d = graph->AddNode(op_desc_d);

  NodePtr node_02 = graph->AddNode(op_desc_02);
  NodePtr node_a2 = graph->AddNode(op_desc_a2);
  NodePtr node_b2 = graph->AddNode(op_desc_b2);
  NodePtr node_c2 = graph->AddNode(op_desc_c2);

  NodePtr node_e = graph->AddNode(op_desc_e);
  NodePtr node_r = graph->AddNode(op_desc_r);
  NodePtr node_q = graph->AddNode(op_desc_q);

  NodePtr node_c = graph->AddNode(op_desc_c);

  //网络初始化
  InitQuantOp(node_a1);
  InitQuantOp(node_a2);
  InitQuantOp(node_q);
  InitConvOp(*graph, node_b1, 0);
  InitConvOp(*graph, node_b2, 1);
  InitConvOp(*graph, node_c, 2);
  InitDequantOp(node_c1);
  InitDequantOp(node_c2);
  ge::AttrUtils::SetStr(node_c1->GetOpDesc(), STR_QUANT_MODE, STR_QUANT_HIGH_PERFORMANCE);
  ge::AttrUtils::SetStr(node_c2->GetOpDesc(), STR_QUANT_MODE, STR_QUANT_HIGH_PERFORMANCE);
  //构建边
  GraphUtils::AddEdge(node_01->GetOutDataAnchor(0),
                      node_a1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a1->GetOutDataAnchor(0),
                      node_b1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_b1->GetOutDataAnchor(0),
                      node_c1->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_c1->GetOutDataAnchor(0),
                      node_d->GetInDataAnchor(0));

  GraphUtils::AddEdge(node_02->GetOutDataAnchor(0),
                      node_a2->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a2->GetOutDataAnchor(0),
                      node_b2->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_b2->GetOutDataAnchor(0),
                      node_c2->GetInDataAnchor(0));

  GraphUtils::AddEdge(node_d->GetOutDataAnchor(0),
                      node_e->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_c2->GetOutDataAnchor(0),
                      node_e->GetInDataAnchor(1));
  GraphUtils::AddEdge(node_e->GetOutDataAnchor(0),
                      node_r->GetInDataAnchor(0));

  GraphUtils::AddEdge(node_r->GetOutDataAnchor(0),
                      node_q->GetInDataAnchor(0));

  GraphUtils::AddEdge(node_q->GetOutDataAnchor(0),
                      node_c->GetInDataAnchor(0));

  //GraphUtils::DumpGEGraphToOnnx(*graph, "before_quant_norequant_eltwise");
  //执行融合
  AvgPoolQuantProcessFusionPass pass1;
  Conv2DQuantProcessFusionPass pass2;
  DeConvQuantProcessFusionPass pass3;
  DWConv2DQuantProcessFusionPass pass4;
  FCQuantProcessFusionPass pass5;
  MatmulV2QuantProcessFusionPass pass6;
  PoolingQuantProcessFusionPass pass7;
  V200NotRequantFusionPass pass8;
  BatchMatmulV2QuantProcessFusionPass pass9;
  vector<GraphPass*> passes = {&pass1, &pass2, &pass3, &pass4, &pass5, &pass6, &pass7, &pass8, &pass9};
  Status status = PassManager::Run(*graph, passes);
  EXPECT_EQ(fe::SUCCESS, status);
  //GraphUtils::DumpGEGraphToOnnx(*graph, "after_quant_norequant_eltwise");

  int32_t op_num = graph->GetDirectNode().size();
  printf("op num %d", op_num);
  EXPECT_EQ(op_num, 27);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  //unsetenv("DUMP_GE_GRAPH");
}
}

