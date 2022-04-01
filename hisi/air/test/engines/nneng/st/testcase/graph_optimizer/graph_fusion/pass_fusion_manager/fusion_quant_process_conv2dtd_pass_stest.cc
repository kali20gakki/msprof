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
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/bias_optimize_quant_rollback/conv2dtd_quant_process_fusion_pass.h"
#include "common/pass_manager.h"
#include "common/configuration.h"
#include "common/fe_log.h"

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

class STEST_quant_process_conv2dtd_pass : public testing::Test {
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
      GeTensorDesc out_desc_weight_const(weight_const_shape, ge::FORMAT_NCHW);
      op_desc_weight_const->AddOutputDesc(out_desc_weight_const);

      // 创建offset_const
      OpDescPtr op_desc_offset_const = std::make_shared<OpDesc>("offset_const_" + std::to_string(cnt), "Const");
      vector<int64_t> dim_offset_const = {1,1,1,2};
      GeShape offset_const_shape(dim_offset_const);
      GeTensorDesc out_desc_offset_const(offset_const_shape, ge::FORMAT_NCHW);
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
      AttrUtils::SetBool(cube->GetOpDesc(), "bias_term", true);

      vector<int64_t> pad(4, 1);
      AttrUtils::SetListInt(cube->GetOpDesc(), CONV_ATTR_NAME_PAD, pad);

      vector<int64_t> stride(2, 2);
      AttrUtils::SetListInt(cube->GetOpDesc(), CONV_ATTR_NAME_STRIDE, stride);
    }

void InitInputOpC20(NodePtr node)
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

        vector<GeTensorPtr> conv_weights = OpDescUtils::MutableWeights(node);

        vector<int64_t> dim(4, 2);
        dim[0] = KERNEL_NUM;
        dim[1] = 20;
        GeShape shape(dim);
        GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);
        TensorUtils::SetDataOffset(out_desc, 0);

        GeTensorPtr filter = std::make_shared<ge::GeTensor>(out_desc, (uint8_t *)sample_conv_weight, KERNEL_NUM * 20 * 2 * 2 * sizeof(int8_t));

        conv_weights.push_back(filter);
        OpDescUtils::SetWeights(node, conv_weights);
    }
    /*
     * [original graph]
     *
     * --->cube--->
     *
     * [processed graph]
     *
     *                 offset_const
     *                       \
     *                        v
     * weight_const--->ascendweightquant--->cube--->
     *
     * */
    void InitConvOpWithoutBias(ComputeGraph &graph, NodePtr cube, int cnt)
    {
      // 创建weight_const
      OpDescPtr op_desc_weight_const = std::make_shared<OpDesc>("weight_const_" + std::to_string(cnt), "Const");
      vector<int64_t> dim_weight_const(4, 2);
      dim_weight_const[0] = KERNEL_NUM;
      GeShape weight_const_shape(dim_weight_const);
      GeTensorDesc out_desc_weight_const(weight_const_shape, ge::FORMAT_NCHW);
      op_desc_weight_const->AddOutputDesc(out_desc_weight_const);

      // 创建offset_const
      OpDescPtr op_desc_offset_const = std::make_shared<OpDesc>("offset_const_" + std::to_string(cnt), "Const");
      vector<int64_t> dim_offset_const = {1,1,1,2};
      GeShape offset_const_shape(dim_offset_const);
      GeTensorDesc out_desc_offset_const(offset_const_shape, ge::FORMAT_NCHW);
      op_desc_offset_const->AddOutputDesc(out_desc_offset_const);

      // 创建AscendWeightQuant
      OpDescPtr op_desc_awq = std::make_shared<OpDesc>("awq_" + std::to_string(cnt), "AscendWeightDequant");
      op_desc_awq->AddInputDesc(0, out_desc_weight_const);
      op_desc_awq->AddInputDesc(1, out_desc_offset_const);
      op_desc_awq->AddOutputDesc(out_desc_weight_const);

      // 添加node
      NodePtr node_weight_const = graph.AddNode(op_desc_weight_const);
      NodePtr node_offset_const = graph.AddNode(op_desc_offset_const);
      NodePtr node_awq = graph.AddNode(op_desc_awq);

      // 构建边
      GraphUtils::AddEdge(node_weight_const->GetOutDataAnchor(0), node_awq->GetInDataAnchor(0));
      GraphUtils::AddEdge(node_offset_const->GetOutDataAnchor(0), node_awq->GetInDataAnchor(1));
      GraphUtils::AddEdge(node_awq->GetOutDataAnchor(0), cube->GetInDataAnchor(1));

      // 设置attr
      AttrUtils::SetInt(cube->GetOpDesc(), CONV_ATTR_NAME_MODE, CC_CONV_CONVOLUTION);
      AttrUtils::SetInt(cube->GetOpDesc(), CONV_ATTR_NAME_GROUP, 1);
      AttrUtils::SetInt(cube->GetOpDesc(), CONV_ATTR_NAME_PAD_MODE, CC_PADDING_VALID);
      AttrUtils::SetInt(cube->GetOpDesc(), CONV_ATTR_NAME_ALGO, -1);
      AttrUtils::SetBool(cube->GetOpDesc(), "bias_term", true);

      vector<int64_t> pad(4, 1);
      AttrUtils::SetListInt(cube->GetOpDesc(), CONV_ATTR_NAME_PAD, pad);

      vector<int64_t> stride(2, 2);
      AttrUtils::SetListInt(cube->GetOpDesc(), CONV_ATTR_NAME_STRIDE, stride);

    }
     void InitInnerProductOp(NodePtr node)
    {
        //初始化卷积算子
        
        int8_t sample_conv_weight[KERNEL_NUM][2] = {{1,2},{3,4}};

        int32_t sample_conv_bias[KERNEL_NUM] =
                    {
                        1,3
                    };
        vector<GeTensorPtr> conv_weights = OpDescUtils::MutableWeights(node);

        vector<int64_t> dim(2, 2);
        //vector<int64_t> dim(4, 0);
        dim[0] = KERNEL_NUM;
        GeShape shape(dim);
        GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);
        TensorUtils::SetDataOffset(out_desc, 0);

        GeTensorPtr filter = std::make_shared<ge::GeTensor>(out_desc, (uint8_t *)sample_conv_weight, KERNEL_NUM * 2 * sizeof(int8_t));

        vector<int64_t> dim_bias(2, 1);
        //vector<int64_t> dim_bias(2, 0);
        dim_bias[1] = KERNEL_NUM;
        GeTensorDesc out_desc_bias(shape, ge::FORMAT_NCHW);
        TensorUtils::SetDataOffset(out_desc_bias, 0);
        GeTensorPtr bias = std::make_shared<ge::GeTensor>(out_desc_bias, (uint8_t *)sample_conv_bias, 2 * sizeof(int32_t));

        conv_weights.push_back(filter);
        conv_weights.push_back(bias);
        OpDescUtils::SetWeights(node, conv_weights);

        AttrUtils::SetInt(node->GetOpDesc(), CONV_ATTR_NAME_MODE, CC_CONV_CONVOLUTION);
        AttrUtils::SetInt(node->GetOpDesc(), CONV_ATTR_NAME_GROUP, 1);
        AttrUtils::SetInt(node->GetOpDesc(), CONV_ATTR_NAME_PAD_MODE, CC_PADDING_VALID);
        AttrUtils::SetInt(node->GetOpDesc(), CONV_ATTR_NAME_ALGO, -1);
        AttrUtils::SetBool(node->GetOpDesc(), "bias_term", true);

        vector<int64_t> pad(4, 1);
        AttrUtils::SetListInt(node->GetOpDesc(), CONV_ATTR_NAME_PAD, pad);

        vector<int64_t> stride(2, 2);
        AttrUtils::SetListInt(node->GetOpDesc(), CONV_ATTR_NAME_STRIDE, stride);

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
        GeTensorDesc out_desc_bias(shape, ge::FORMAT_NCHW);
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
        
        uint64_t sample_deq_scale[KERNEL_NUM] = {0x00001100392BCD31,
                                                 0x000022003717AB06};
        
        
        
        vector<GeTensorPtr> scale_weights = OpDescUtils::MutableWeights(node);

        vector<int64_t> dim{KERNEL_NUM};
        GeShape shape(dim);
        GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);
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
        GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);
        TensorUtils::SetDataOffset(out_desc, 0);

        GeTensorPtr scale_weight = std::make_shared<ge::GeTensor>(out_desc, (uint8_t *)sample_deq_scale, KERNEL_NUM * sizeof(uint64_t));

        scale_weights.push_back(scale_weight);
        OpDescUtils::SetWeights(node, scale_weights);

    }
    

};

TEST_F(STEST_quant_process_conv2dtd_pass, quant_rollback_pattern_success){
    //(void)setenv("DUMP_GE_GRAPH", "2", 2);
    Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

    //创建待融合网络
    OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", DATA_TYPE);
    OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "AscendQuant");
    OpDescPtr op_desc_b = std::make_shared<OpDesc>("B", "Conv2DTransposeD");
    OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "AscendDequant");
    OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");

    //add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

    op_desc_0->AddOutputDesc(out_desc);
    op_desc_a->AddInputDesc(out_desc);
    op_desc_a->AddOutputDesc(out_desc);
    op_desc_b->AddInputDesc(out_desc);
    // weight input of op_desc_b
    op_desc_b->AddInputDesc(out_desc);
    // bias input of op_desc_b
    op_desc_b->AddInputDesc(out_desc);
    op_desc_b->AddOutputDesc(out_desc);
    op_desc_c->AddInputDesc(out_desc);
    op_desc_c->AddOutputDesc(out_desc);
    op_desc_d->AddInputDesc(out_desc);

    //添加Node
    NodePtr node_0 = graph->AddNode(op_desc_0);
    NodePtr node_a = graph->AddNode(op_desc_a);
    NodePtr node_b = graph->AddNode(op_desc_b);
    NodePtr node_c = graph->AddNode(op_desc_c);
    NodePtr node_d = graph->AddNode(op_desc_d);

    //网络初始化
    InitQuantOp(node_a);
    InitConvOp(*graph, node_b, 0);
    InitDequantOp(node_c);

    //构建边
    GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));

    //执行融合
    Conv2DTDQuantProcessFusionPass pass1;
    //ConvScaleFusionPass pass2;
    vector<GraphPass*> passes = {&pass1};
    Status status = PassManager::Run(*graph, passes);
    EXPECT_EQ(fe::SUCCESS, status);
    //GraphUtils::DumpGEGraphToOnnx(*graph, "after_quant_rollback");

    int32_t op_num = graph->GetDirectNode().size();
    EXPECT_EQ(op_num, 8);
    //unsetenv("DUMP_GE_GRAPH");
}

TEST_F(STEST_quant_process_conv2dtd_pass, quant_rollback_pattern_success2){
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  //创建待融合网络
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", DATA_TYPE);
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "AscendQuant");
  OpDescPtr op_desc_b = std::make_shared<OpDesc>("B", "Conv2DTransposeD");
  OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "AscendDequant");
  OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");
  OpDescPtr op_desc_e = std::make_shared<OpDesc>("E", "Pad");

  //add descriptor
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc);
  op_desc_a->AddOutputDesc(out_desc);
  op_desc_b->AddInputDesc(out_desc);
  // weight input of op_desc_b
  op_desc_b->AddInputDesc(out_desc);
  // bias input of op_desc_b
  op_desc_b->AddInputDesc(out_desc);
  op_desc_b->AddOutputDesc(out_desc);
  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddOutputDesc(out_desc);
  op_desc_d->AddInputDesc(out_desc);
  op_desc_e->AddInputDesc(out_desc);
  op_desc_e->AddOutputDesc(out_desc);

  //添加Node
  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_a = graph->AddNode(op_desc_a);
  NodePtr node_b = graph->AddNode(op_desc_b);
  NodePtr node_c = graph->AddNode(op_desc_c);
  NodePtr node_d = graph->AddNode(op_desc_d);
  NodePtr node_e = graph->AddNode(op_desc_e);

  //网络初始化
  InitQuantOp(node_a);
  InitConvOp(*graph, node_b, 0);
  InitDequantOp(node_c);

  //构建边
  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_e->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_e->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));

  size_t size_before = graph->GetDirectNode().size();
  FE_LOGI("Size before is %u", size_before);
  Conv2DTDQuantProcessFusionPass pass1;
  vector<GraphPass*> passes = {&pass1};
  Status status = PassManager::Run(*graph, passes);
  EXPECT_EQ(fe::NOT_CHANGED, status);
  size_t size_after = graph->GetDirectNode().size();
  FE_LOGI("Size after is %u", size_after);
  EXPECT_EQ(size_before, size_after);
}

TEST_F(STEST_quant_process_conv2dtd_pass, quant_bias_optimize_pattern_success){
    //(void)setenv("DUMP_GE_GRAPH", "2", 2);
    Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    //创建待融合网络
    OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", DATA_TYPE);
    OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "AscendQuant");
    OpDescPtr op_desc_b = std::make_shared<OpDesc>("B", "Conv2DTransposeD");
    OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "AscendDequant");
    OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");

    //add descriptor
    vector<int64_t> dim = {KERNEL_NUM, 20, 2, 2};
    GeShape shape(dim);
    GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

    op_desc_0->AddOutputDesc(out_desc);
    op_desc_a->AddInputDesc(out_desc);
    op_desc_a->AddOutputDesc(out_desc);
    op_desc_b->AddInputDesc(out_desc);
    // weight input of op_desc_b
    op_desc_b->AddInputDesc(out_desc);
    // bias input of op_desc_b
    op_desc_b->AddInputDesc(out_desc);
    op_desc_b->AddOutputDesc(out_desc);
    op_desc_c->AddInputDesc(out_desc);
    op_desc_c->AddOutputDesc(out_desc);
    op_desc_d->AddInputDesc(out_desc);

    //添加Node
    NodePtr node_0 = graph->AddNode(op_desc_0);
    NodePtr node_a = graph->AddNode(op_desc_a);
    NodePtr node_b = graph->AddNode(op_desc_b);
    NodePtr node_c = graph->AddNode(op_desc_c);
    NodePtr node_d = graph->AddNode(op_desc_d);



    //网络初始化
    InitInputOp(node_0);
    InitQuantOp(node_a);
    InitConvOp(*graph, node_b, 0);
    InitDequantOpBias(node_c);



     OpDescPtr op_desc = node_0->GetOpDesc();
     auto input_i = op_desc->GetInputDesc(0);
     ge::GeShape shape001 = input_i.GetShape();
     FE_LOGI("Input C shape %d, dim num %d, zxg_test_input_input001.",shape001.GetDim(1), shape001.GetDimNum());

    //构建边
    GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));

    //GraphUtils::DumpGEGraphToOnnx(*graph, "before_quant_bias_optimize");
    //执行融合
    Conv2DTDQuantProcessFusionPass pass1;
    vector<GraphPass*> passes = {&pass1};
    Status status = PassManager::Run(*graph, passes);
    EXPECT_EQ(fe::SUCCESS, status);
    //GraphUtils::DumpGEGraphToOnnx(*graph, "after_quant_bias_optimize");

    int32_t op_num = graph->GetDirectNode().size();
    EXPECT_EQ(op_num, 13);
}


TEST_F(STEST_quant_process_conv2dtd_pass, quant_bias_optimize_pattern_success_v200){
    //(void)setenv("DUMP_GE_GRAPH", "2", 2);
    Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    //创建待融合网络
    OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", DATA_TYPE);
    OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "AscendQuant");
    OpDescPtr op_desc_b = std::make_shared<OpDesc>("B", "Conv2DTransposeD");
    OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "AscendDequant");
    OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");

    //add descriptor
    vector<int64_t> dim = {KERNEL_NUM, 20, 2, 2};
    GeShape shape(dim);
    GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

    op_desc_0->AddOutputDesc(out_desc);
    op_desc_a->AddInputDesc(out_desc);
    op_desc_a->AddOutputDesc(out_desc);
    op_desc_b->AddInputDesc(out_desc);
    // weight input of op_desc_b
    op_desc_b->AddInputDesc(out_desc);
    // bias input of op_desc_b
    op_desc_b->AddInputDesc(out_desc);
    op_desc_b->AddOutputDesc(out_desc);
    op_desc_c->AddInputDesc(out_desc);
    op_desc_c->AddOutputDesc(out_desc);
    op_desc_d->AddInputDesc(out_desc);

    //添加Node
    NodePtr node_0 = graph->AddNode(op_desc_0);
    NodePtr node_a = graph->AddNode(op_desc_a);
    NodePtr node_b = graph->AddNode(op_desc_b);
    NodePtr node_c = graph->AddNode(op_desc_c);
    NodePtr node_d = graph->AddNode(op_desc_d);



    //网络初始化
    InitInputOp(node_0);
    InitQuantOp(node_a);
    InitConvOp(*graph, node_b, 0);
    InitDequantOpBias(node_c);



     OpDescPtr op_desc = node_0->GetOpDesc();
     auto input_i = op_desc->GetInputDesc(0);
     ge::GeShape shape001 = input_i.GetShape();
     FE_LOGI("Input C shape %d, dim num %d, zxg_test_input_input001.",shape001.GetDim(1), shape001.GetDimNum());

    //构建边
    GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));

    //GraphUtils::DumpGEGraphToOnnx(*graph, "before_quant_bias_optimize");
    //执行融合
    Conv2DTDQuantProcessFusionPass pass1;
    vector<GraphPass*> passes = {&pass1};
    Status status = PassManager::Run(*graph, passes);
    EXPECT_EQ(fe::SUCCESS, status);
    //GraphUtils::DumpGEGraphToOnnx(*graph, "after_quant_bias_optimize");

    int32_t op_num = graph->GetDirectNode().size();
    EXPECT_EQ(op_num, 13);
}

TEST_F(STEST_quant_process_conv2dtd_pass, quant_bias_optimize_pattern_conv2dtd_c20_without_bias_success){
    Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
    //(void)setenv("DUMP_GE_GRAPH", "2", 2);
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    //创建待融合网络
    OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", DATA_TYPE);
    OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "AscendQuant");
    OpDescPtr op_desc_b = std::make_shared<OpDesc>("B", "Conv2DTransposeD");
    OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "AscendDequant");
    OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");

    //add descriptor
    vector<int64_t> dim = {KERNEL_NUM, 20, 2, 2};
    GeShape shape(dim);
    GeTensorDesc out_desc(shape, ge::FORMAT_NCHW);

    op_desc_0->AddOutputDesc(out_desc);
    op_desc_a->AddInputDesc(out_desc);
    op_desc_a->AddOutputDesc(out_desc);
    op_desc_b->AddInputDesc(out_desc);
    // weight input of op_desc_b
    op_desc_b->AddInputDesc(out_desc);
    op_desc_b->AddOutputDesc(out_desc);
    op_desc_c->AddInputDesc(out_desc);
    op_desc_c->AddOutputDesc(out_desc);
    op_desc_d->AddInputDesc(out_desc);

    //添加Node
    NodePtr node_0 = graph->AddNode(op_desc_0);
    NodePtr node_a = graph->AddNode(op_desc_a);
    NodePtr node_b = graph->AddNode(op_desc_b);
    NodePtr node_c = graph->AddNode(op_desc_c);
    NodePtr node_d = graph->AddNode(op_desc_d);

    //网络初始化
    InitInputOpC20(node_0);
    InitQuantOp(node_a);
    InitConvOpWithoutBias(*graph, node_b, 0);
    InitDequantOpBias(node_c);

     OpDescPtr op_desc = node_0->GetOpDesc();
     auto input_i = op_desc->GetInputDesc(0);
     ge::GeShape shape001 = input_i.GetShape();
     FE_LOGI("Input C shape %d, dim num %d, zxg_test_input_input001.",shape001.GetDim(1), shape001.GetDimNum());

    //构建边
    GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
    GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));

    //GraphUtils::DumpGEGraphToOnnx(*graph, "before_quant_bias_optimize");
    //执行融合
    Conv2DTDQuantProcessFusionPass pass1;
    vector<GraphPass*> passes = {&pass1};
    Status status = PassManager::Run(*graph, passes);
    EXPECT_EQ(fe::SUCCESS, status);
    //GraphUtils::DumpGEGraphToOnnx(*graph, "after_quant_bias_optimize");

    int32_t op_num = graph->GetDirectNode().size();
    EXPECT_EQ(op_num, 12);
    NodePtr quant = node_0->GetOutAllNodes().at(0);
    EXPECT_EQ(quant->GetType(), "AscendQuant");
    NodePtr cube = quant->GetOutAllNodes().at(0);
    EXPECT_EQ(cube->GetType(), "Conv2DTransposeD");
    NodePtr dequant = cube->GetOutAllNodes().at(0);
    EXPECT_EQ(dequant->GetType(), "AscendDequant");
}

TEST_F(STEST_quant_process_conv2dtd_pass, quant_bias_optimize_unknown_shape){
  //(void)setenv("DUMP_GE_GRAPH", "2", 2);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  //创建待融合网络
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("data", DATA_TYPE);
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "AscendQuant");
  OpDescPtr op_desc_b = std::make_shared<OpDesc>("B", "Conv2DTransposeD");
  OpDescPtr op_desc_c = std::make_shared<OpDesc>("C", "AscendDequant");
  OpDescPtr op_desc_d = std::make_shared<OpDesc>("D", "Relu");

  //add descriptor
  vector<int64_t> dim = {KERNEL_NUM, -1, 2, 2};
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  out_desc.SetFormat(ge::FORMAT_NCHW);

  op_desc_0->AddOutputDesc(out_desc);
  op_desc_a->AddInputDesc(out_desc);
  op_desc_a->AddOutputDesc(out_desc);
  op_desc_b->AddInputDesc(out_desc);
  // weight input of op_desc_b
  op_desc_b->AddInputDesc(out_desc);
  // bias input of op_desc_b
  op_desc_b->AddInputDesc(out_desc);
  op_desc_b->AddOutputDesc(out_desc);
  op_desc_c->AddInputDesc(out_desc);
  op_desc_c->AddOutputDesc(out_desc);
  op_desc_d->AddInputDesc(out_desc);

  //添加Node
  NodePtr node_0 = graph->AddNode(op_desc_0);
  NodePtr node_a = graph->AddNode(op_desc_a);
  NodePtr node_b = graph->AddNode(op_desc_b);
  NodePtr node_c = graph->AddNode(op_desc_c);
  NodePtr node_d = graph->AddNode(op_desc_d);

  //网络初始化
  InitInputOp(node_0);
  InitQuantOp(node_a);
  InitConvOp(*graph, node_b, 0);
  InitDequantOpBias(node_c);

  OpDescPtr op_desc = node_0->GetOpDesc();
  auto input_i = op_desc->GetInputDesc(0);
  ge::GeShape shape001 = input_i.GetShape();
  FE_LOGI("Input C shape %d, dim num %d, zxg_test_input_input001.",shape001.GetDim(1), shape001.GetDimNum());

  //构建边
  GraphUtils::AddEdge(node_0->GetOutDataAnchor(0), node_a->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node_b->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_b->GetOutDataAnchor(0), node_c->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_c->GetOutDataAnchor(0), node_d->GetInDataAnchor(0));

  //GraphUtils::DumpGEGraphToOnnx(*graph, "before_quant_bias_optimize");
  //执行融合
  Conv2DTDQuantProcessFusionPass pass1;
  vector<GraphPass*> passes = {&pass1};
  Status status = PassManager::Run(*graph, passes);
  EXPECT_EQ(fe::SUCCESS, status);
  //GraphUtils::DumpGEGraphToOnnx(*graph, "after_quant_bias_optimize");
}

}


