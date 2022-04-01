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

#include <cstdio>
#include <map>
#include <memory>
#include "gtest/gtest.h"
#include "proto/om.pb.h"

#define protected public
#define private public
#include "common/graph_comm.h"
#include "common/pass_manager.h"
#include "common/configuration.h"
#include "common/util/op_info_util.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_tensor.h"
#include "graph/op_desc.h"
#include "graph/op_kernel_bin.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/ub_fusion/buffer_fusion.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_common_rules2_fusion_pass.h"

#undef protected
#undef private
using namespace std;
using namespace domi;
using namespace fe;
using namespace ge;

class UB_FUSION_ST_COMMON_RULES2 : public testing::Test {
public:
    using AttrDefMap = ::google::protobuf::Map<::std::string, AttrDef>;

protected:
    static void SetUpTestCase() {
      std::cout << "UB fusion SetUp" << std::endl;
    }

    static void TearDownTestCase() {
      std::cout <<"UB fusion TearDown" << std::endl;
    }

    virtual void SetUp() {}

    virtual void TearDown() {}

    static void SetPattern(const ge::OpDescPtr& op_desc, const string& op_type) {
      auto key_pattern = op_desc->GetName() + "_pattern";
      ge::AttrUtils::SetStr(op_desc, key_pattern, op_type);
    }

    static void SetTvmType(const ge::OpDescPtr& op_desc) {
      ge::AttrUtils::SetInt(op_desc, ge::ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM));
    }

/****************************************************
*
*       data       stride_read      filter      bias
*         \           \             /          /
*                     conv
*                      |
*      other_input -- dequant -- other_output
*                      |
*                  elem_wise1
*                      |
*                  elem_wise2
*                      |
*                  elem_wise3
*                      |
*                  elem_wise4
*                      |
*                  elem_wise5
*                      |
*                    quant
*                      |
*                 stride_write
*                      |
*                    output
*
****************************************************/
    static void BuildGraph1(const ComputeGraphPtr& graph) {
      vector<int64_t> dim(4, 4);
      GeShape shape(dim);
      GeTensorDesc tensor_desc(shape);

      /** data */
      OpDescPtr data = std::make_shared<OpDesc>("data", fe::DATA);
      SetPattern(data, fe::TBE_PATTERN_INPUT_NODE);
      SetTvmType(data);
      data->AddOutputDesc(tensor_desc);
      AttrUtils::SetInt(data, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr data_node = graph->AddNode(data);
      /** data */
      OpDescPtr data1 = std::make_shared<OpDesc>("data1", fe::DATA);
      SetPattern(data1, fe::TBE_PATTERN_INPUT_NODE);
      SetTvmType(data1);
      data->AddOutputDesc(tensor_desc);
      AttrUtils::SetInt(data1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr data1_node = graph->AddNode(data);
      /** stride_read */
      OpDescPtr stride_read = std::make_shared<OpDesc>("strideRead", fe::STRIDEDREAD);
      SetPattern(stride_read, fe::OP_PATTERN_STRIDED_READ);
      SetTvmType(stride_read);
      stride_read->AddInputDesc(tensor_desc);
      stride_read->AddOutputDesc(tensor_desc);
      AttrUtils::SetInt(stride_read, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr stride_read_node = graph->AddNode(stride_read);
      const char stride_read_tbe_bin[] = "strideRead_tbe_bin";
      vector<char> stride_readbuffer(stride_read_tbe_bin, stride_read_tbe_bin+strlen(stride_read_tbe_bin));
      ge::OpKernelBinPtr stride_read_tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
              stride_read_node->GetName(), std::move(stride_readbuffer));
      stride_read_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, stride_read_tbe_kernel_ptr);
      /** filter */
      OpDescPtr filter = std::make_shared<OpDesc>("filter", fe::DATA);
      SetPattern(filter, fe::TBE_PATTERN_INPUT_NODE);
      SetTvmType(filter);
      filter->AddOutputDesc(tensor_desc);
      AttrUtils::SetInt(filter, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr filter_node = graph->AddNode(filter);
      /** bias */
      OpDescPtr bias = std::make_shared<OpDesc>("bias", fe::DATA);
      SetPattern(bias, fe::TBE_PATTERN_INPUT_NODE);
      SetTvmType(bias);
      bias->AddOutputDesc(tensor_desc);
      AttrUtils::SetInt(bias, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr bias_node = graph->AddNode(bias);
      /** conv */
      OpDescPtr conv = std::make_shared<OpDesc>("conv", fe::CONV2D);
      SetPattern(conv, fe::OP_PATTERN_CONV);
      SetTvmType(conv);
      conv->AddInputDesc(tensor_desc);
      conv->AddInputDesc(tensor_desc);
      conv->AddInputDesc(tensor_desc);
      conv->AddInputDesc(tensor_desc);
      conv->AddOutputDesc(tensor_desc);
      AttrUtils::SetInt(conv, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr conv_node = graph->AddNode(conv);
      const char conv_tbe_bin[] = "conv_tbe_bin";
      vector<char> convbuffer(conv_tbe_bin, conv_tbe_bin+strlen(conv_tbe_bin));
      ge::OpKernelBinPtr conv_tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(
              conv_node->GetName(), std::move(convbuffer));
      conv_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, conv_tbe_kernel_ptr);
      /** other_input */
      OpDescPtr other_input = std::make_shared<OpDesc>("otherInput", fe::DATA);
      SetPattern(other_input, fe::TBE_PATTERN_INPUT_NODE);
      SetTvmType(other_input);
      other_input->AddOutputDesc(tensor_desc);
      AttrUtils::SetInt(other_input, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr other_input_node = graph->AddNode(other_input);
      /** other_output */
      OpDescPtr other_output = std::make_shared<OpDesc>("otherOutput", fe::DATA);
      SetPattern(other_output, fe::TBE_PATTERN_OUTPUT_NODE);
      SetTvmType(other_output);
      other_output->AddInputDesc(tensor_desc);
      AttrUtils::SetInt(other_output, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr other_output_node = graph->AddNode(other_output);
      /** dequant */
      OpDescPtr dequant = std::make_shared<OpDesc>("dequant", fe::ASCEND_DEQUANT);
      SetPattern(dequant, OP_PATTERN_DEQUANT);
      SetTvmType(dequant);
      dequant->AddInputDesc(tensor_desc);
      dequant->AddInputDesc(tensor_desc);
      dequant->AddOutputDesc(tensor_desc);
      AttrUtils::SetInt(dequant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr dequant_node = graph->AddNode(dequant);
      /** elem_wise1 */
      OpDescPtr elem_wise1 = std::make_shared<OpDesc>("elemWise1", fe::ELTWISE);
      SetPattern(elem_wise1, OP_PATTERN_ELEMWISE);
      SetTvmType(elem_wise1);
      elem_wise1->AddInputDesc(tensor_desc);
      elem_wise1->AddOutputDesc(tensor_desc);
      AttrUtils::SetInt(elem_wise1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr elem_wise1_node = graph->AddNode(elem_wise1);
      /** elem_wise2 */
      OpDescPtr elem_wise2 = std::make_shared<OpDesc>("elemWise2", fe::ELTWISE);
      SetPattern(elem_wise2, OP_PATTERN_ELEMWISE);
      SetTvmType(elem_wise2);
      elem_wise2->AddInputDesc(tensor_desc);
      elem_wise2->AddOutputDesc(tensor_desc);
      AttrUtils::SetInt(elem_wise2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr elem_wise2_node = graph->AddNode(elem_wise2);
      /** elem_wise3 */
      OpDescPtr elem_wise3 = std::make_shared<OpDesc>("elemWise3", fe::ELTWISE);
      SetPattern(elem_wise3, OP_PATTERN_ELEMWISE);
      SetTvmType(elem_wise3);
      elem_wise3->AddInputDesc(tensor_desc);
      elem_wise3->AddOutputDesc(tensor_desc);
      AttrUtils::SetInt(elem_wise3, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr elem_wise3_node = graph->AddNode(elem_wise3);
      /** elem_wise4 */
      OpDescPtr elem_wise4 = std::make_shared<OpDesc>("elemWise4", fe::ELTWISE);
      SetPattern(elem_wise4, OP_PATTERN_ELEMWISE);
      SetTvmType(elem_wise4);
      elem_wise4->AddInputDesc(tensor_desc);
      elem_wise4->AddOutputDesc(tensor_desc);
      AttrUtils::SetInt(elem_wise4, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr elem_wise4_node = graph->AddNode(elem_wise4);
      /** elem_wise5 */
      OpDescPtr elem_wise5 = std::make_shared<OpDesc>("elemWise5", fe::ELTWISE);
      SetPattern(elem_wise5, OP_PATTERN_ELEMWISE);
      SetTvmType(elem_wise5);
      elem_wise5->AddInputDesc(tensor_desc);
      elem_wise5->AddOutputDesc(tensor_desc);
      AttrUtils::SetInt(elem_wise5, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr elem_wise5_node = graph->AddNode(elem_wise5);
      /** quant */
      OpDescPtr quant = std::make_shared<OpDesc>("quant", fe::ASCEND_QUANT);
      SetPattern(quant, OP_PATTERN_QUANT);
      SetTvmType(quant);
      quant->AddInputDesc(tensor_desc);
      quant->AddOutputDesc(tensor_desc);
      AttrUtils::SetInt(quant, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr quant_node = graph->AddNode(quant);
      /** stride_write */
      OpDescPtr stride_write = std::make_shared<OpDesc>("strideWrite", fe::STRIDEDWRITE);
      SetPattern(stride_write, fe::OP_PATTERN_STRIDED_WRITE);
      SetTvmType(stride_write);
      stride_write->AddInputDesc(tensor_desc);
      stride_write->AddOutputDesc(tensor_desc);
      AttrUtils::SetInt(stride_write, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr stride_write_node = graph->AddNode(stride_write);
      /** output */
      OpDescPtr output = std::make_shared<OpDesc>("output", fe::DATA);
      SetPattern(output, fe::TBE_PATTERN_OUTPUT_NODE);
      SetTvmType(output);
      output->AddInputDesc(tensor_desc);
      AttrUtils::SetInt(output, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr output_node = graph->AddNode(output);

      GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
      GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), stride_read_node->GetInDataAnchor(0));
      GraphUtils::AddEdge(filter_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
      GraphUtils::AddEdge(stride_read_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(2));
      GraphUtils::AddEdge(bias_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(3));
      GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0), dequant_node->GetInDataAnchor(0));
      GraphUtils::AddEdge(other_input_node->GetOutDataAnchor(0), dequant_node->GetInDataAnchor(1));
      GraphUtils::AddEdge(dequant_node->GetOutDataAnchor(0),elem_wise1_node->GetInDataAnchor(0));
      GraphUtils::AddEdge(dequant_node->GetOutDataAnchor(0), other_output_node->GetInDataAnchor(0));
      GraphUtils::AddEdge(elem_wise1_node->GetOutDataAnchor(0), elem_wise2_node->GetInDataAnchor(0));
      GraphUtils::AddEdge(elem_wise2_node->GetOutDataAnchor(0), elem_wise3_node->GetInDataAnchor(0));
      GraphUtils::AddEdge(elem_wise3_node->GetOutDataAnchor(0), elem_wise4_node->GetInDataAnchor(0));
      GraphUtils::AddEdge(elem_wise4_node->GetOutDataAnchor(0), elem_wise5_node->GetInDataAnchor(0));
      GraphUtils::AddEdge(elem_wise5_node->GetOutDataAnchor(0), quant_node->GetInDataAnchor(0));
      GraphUtils::AddEdge(quant_node->GetOutDataAnchor(0), stride_write_node->GetInDataAnchor(0));
      GraphUtils::AddEdge(stride_write_node->GetOutDataAnchor(0), output_node->GetInDataAnchor(0));
    }

/****************************************************
*
*       data       stride_read      filter      bias
*         \           \             /          /
*                     conv
*                      |
*      other_input -- dequant
*                      |
*                  elem_wise1 -- other_output
*                      |
*                  elem_wise2
*                      |
*                  elem_wise3
*                      |
*                  elem_wise4
*                      |
*                  elem_wise5
*                      |
*                    quant
*                      |
*                 stride_write
*                      |
*                    output
*
****************************************************/
    static void BuildGraph2(const ComputeGraphPtr& graph) {
      BuildGraph1(graph);
      NodePtr dequant_node;
      NodePtr other_output_node;
      NodePtr elem_wise1_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "dequant") {
          dequant_node = node;
        }
        if (node->GetName() == "otherOutput") {
          other_output_node = node;
        }
        if (node->GetName() == "elemWise1") {
          elem_wise1_node = node;
        }
      }
      GraphUtils::RemoveEdge(dequant_node->GetOutDataAnchor(0), other_output_node->GetInDataAnchor(0));
      GraphUtils::AddEdge(elem_wise1_node->GetOutDataAnchor(0), other_output_node->GetInDataAnchor(0));
    }

/****************************************************
*
*       data       stride_read      filter      bias
*         \           \             /          /
*                     conv
*                      |
*      other_input -- dequant
*                      |
*                  elem_wise1
*                      |
*                  elem_wise2
*                      |
*                  elem_wise3 -- other_output
*                      |
*                  elem_wise4
*                      |
*                  elem_wise5
*                      |
*                    quant
*                      |
*                 stride_write
*                      |
*                    output
*
****************************************************/
    static void BuildGraph3(const ComputeGraphPtr& graph) {
      BuildGraph1(graph);
      NodePtr dequant_node;
      NodePtr other_output_node;
      NodePtr elem_wise3_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "dequant") {
          dequant_node = node;
        }
        if (node->GetName() == "otherOutput") {
          other_output_node = node;
        }
        if (node->GetName() == "elemWise3") {
          elem_wise3_node = node;
        }
      }
      GraphUtils::RemoveEdge(dequant_node->GetOutDataAnchor(0), other_output_node->GetInDataAnchor(0));
      GraphUtils::AddEdge(elem_wise3_node->GetOutDataAnchor(0), other_output_node->GetInDataAnchor(0));
    }

/****************************************************
*
*       data       stride_read      filter      bias
*         \           \             /          /
*                     conv
*                      |
*      other_input -- dequant
*                      |
*                  elem_wise1
*                      |
*                  elem_wise2
*                      |
*                  elem_wise3
*                      |
*                  elem_wise4
*                      |
*                  elem_wise5 -- other_output
*                      |
*                    quant
*                      |
*                 stride_write
*                      |
*                    output
*
****************************************************/
    static void BuildGraph4(const ComputeGraphPtr& graph) {
      BuildGraph1(graph);
      NodePtr dequant_node;
      NodePtr other_output_node;
      NodePtr elem_wise5_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "dequant") {
          dequant_node = node;
        }
        if (node->GetName() == "otherOutput") {
          other_output_node = node;
        }
        if (node->GetName() == "elemWise5") {
          elem_wise5_node = node;
        }
      }
      GraphUtils::RemoveEdge(dequant_node->GetOutDataAnchor(0), other_output_node->GetInDataAnchor(0));
      GraphUtils::AddEdge(elem_wise5_node->GetOutDataAnchor(0), other_output_node->GetInDataAnchor(0));
    }

/****************************************************
*
*       data       stride_read      filter      bias
*         \           \             /          /
*                     conv
*                      |
*      other_input -- dequant -- other_output
*                      |
*                  elem_wise1
*                      |
*                    quant
*                      |
*                 stride_write
*                      |
*                    output
*
****************************************************/
    static void BuildGraph5(const ComputeGraphPtr& graph) {
      BuildGraph1(graph);
      NodePtr elem_wise2_node;
      NodePtr elem_wise3_node;
      NodePtr elem_wise4_node;
      NodePtr elem_wise5_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "elemWise2") {
          elem_wise2_node = node;
        }
        if (node->GetName() == "elemWise3") {
          elem_wise3_node = node;
        }
        if (node->GetName() == "elemWise4") {
          elem_wise4_node = node;
        }
        if (node->GetName() == "elemWise5") {
          elem_wise5_node = node;
        }
      }
      graph->RemoveNode(elem_wise2_node);
      graph->RemoveNode(elem_wise3_node);
      graph->RemoveNode(elem_wise4_node);
      graph->RemoveNode(elem_wise5_node);
    }

/****************************************************
*
*       data       stride_read      filter      bias
*         \           \             /          /
*                     conv
*                      |
*      other_input -- dequant
*                      |
*                  elem_wise1 -- other_output
*                      |
*                    quant
*                      |
*                 stride_write
*                      |
*                    output
*
****************************************************/
    static void BuildGraph6(const ComputeGraphPtr& graph) {
      BuildGraph5(graph);
      NodePtr dequant_node;
      NodePtr other_output_node;
      NodePtr elem_wise1_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "dequant") {
          dequant_node = node;
        }
        if (node->GetName() == "otherOutput") {
          other_output_node = node;
        }
        if (node->GetName() == "elemWise1") {
          elem_wise1_node = node;
        }
      }
      GraphUtils::RemoveEdge(dequant_node->GetOutDataAnchor(0), other_output_node->GetInDataAnchor(0));
      GraphUtils::AddEdge(elem_wise1_node->GetOutDataAnchor(0), other_output_node->GetInDataAnchor(0));
    }

/****************************************************
*
*       data       stride_read      filter      bias
*         \           \             /          /
*                     conv
*                      |
*      other_input -- dequant -- other_output
*                      |
*                    quant
*                      |
*                 stride_write
*                      |
*                    output
*
****************************************************/
    static void BuildGraph7(const ComputeGraphPtr& graph) {
      BuildGraph5(graph);
      NodePtr elem_wise1_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "elemWise1") {
          elem_wise1_node = node;
        }
      }
      graph->RemoveNode(elem_wise1_node);
    }

/****************************************************
*
*       data       stride_read      filter      bias
*         \           \             /          /
*                     conv
*                      |
*                  elem_wise1 -- other_output
*                      |
*                  elem_wise2
*                      |
*                  elem_wise3
*                      |
*                  elem_wise4
*                      |
*                  elem_wise5
*                      |
*                    quant
*                      |
*                 stride_write
*                      |
*                    output
*
****************************************************/
    static void BuildGraph8(const ComputeGraphPtr& graph) {
      BuildGraph2(graph);
      NodePtr other_input_node;
      NodePtr dequant_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "otherInput") {
          other_input_node = node;
        }
        if (node->GetName() == "dequant") {
          dequant_node = node;
        }
      }
      graph->RemoveNode(other_input_node);
      graph->RemoveNode(dequant_node);
    }

/****************************************************
*
*           data           filter      bias
*             \             /          /
*                     conv
*                      |
*      other_input -- dequant -- other_output
*                      |
*                  elem_wise1
*                      |
*                  elem_wise2
*                      |
*                  elem_wise3
*                      |
*                  elem_wise4
*                      |
*                  elem_wise5
*                      |
*                    quant
*                      |
*                 stride_write
*                      |
*                    output
*
****************************************************/
    static void BuildGraph9(const ComputeGraphPtr& graph) {
      BuildGraph1(graph);
      NodePtr stride_read_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "strideRead") {
          stride_read_node = node;
        }
      }
      graph->RemoveNode(stride_read_node);
    }

/****************************************************
*
*       data       stride_read      filter      bias
*         \           \             /          /
*                     conv
*                      |
*      other_input -- dequant -- other_output
*                      |
*                  elem_wise1
*                      |
*                  elem_wise2
*                      |
*                  elem_wise3
*                      |
*                  elem_wise4
*                      |
*                  elem_wise5
*                      |
*                    quant
*                      |
*                    output
*
****************************************************/
    static void BuildGraph10(const ComputeGraphPtr& graph) {
      BuildGraph1(graph);
      NodePtr stride_write_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "strideWrite") {
          stride_write_node = node;
        }
      }
      graph->RemoveNode(stride_write_node);
    }

/****************************************************
*
*       data         filter      bias
*         \           \           /
*                     conv
*                      |
*      other_input -- dequant -- other_output
*                      |
*                  elem_wise1
*                      |
*                  elem_wise2
*                      |
*                  elem_wise3
*                      |
*                  elem_wise4
*                      |
*                  elem_wise5
*                      |
*                    quant
*                      |
*                    output
*
****************************************************/
    static void BuildGraph11(const ComputeGraphPtr& graph) {
      BuildGraph1(graph);
      NodePtr stride_read_node;
      NodePtr stride_write_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "strideRead") {
          stride_read_node = node;
        }
        if (node->GetName() == "strideWrite") {
          stride_write_node = node;
        }
      }
      graph->RemoveNode(stride_read_node);
      graph->RemoveNode(stride_write_node);
    }

/****************************************************
*
*       data         filter      bias
*         \           \           /
*                     conv
*                      |
*                  elem_wise1 -- other_output
*                      |
*                  elem_wise2
*                      |
*                  elem_wise3
*                      |
*                  elem_wise4
*                      |
*                  elem_wise5
*                      |
*                    quant
*                      |
*                  stride_write
*                      |
*                    output
*
****************************************************/
    static void BuildGraph12(const ComputeGraphPtr& graph) {
      BuildGraph8(graph);
      NodePtr stride_read_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "strideRead") {
          stride_read_node = node;
        }
      }
      graph->RemoveNode(stride_read_node);
    }

/****************************************************
*
*       data       stride_read      filter      bias
*         \           \             /          /
*                     conv
*                      |
*                  elem_wise1 -- other_output
*                      |
*                  elem_wise2
*                      |
*                  elem_wise3
*                      |
*                  elem_wise4
*                      |
*                  elem_wise5
*                      |
*                    quant
*                      |
*                    output
*
****************************************************/
    static void BuildGraph13(const ComputeGraphPtr& graph) {
      BuildGraph8(graph);
      NodePtr stride_write_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "strideWrite") {
          stride_write_node = node;
        }
      }
      graph->RemoveNode(stride_write_node);
    }

/****************************************************
*
*       data        filter      bias
*         \           \          /
*                     conv
*                      |
*                  elem_wise1 -- other_output
*                      |
*                  elem_wise2
*                      |
*                  elem_wise3
*                      |
*                  elem_wise4
*                      |
*                  elem_wise5
*                      |
*                    quant
*                      |
*                    output
*
****************************************************/
    static void BuildGraph14(const ComputeGraphPtr& graph) {
      BuildGraph13(graph);
      NodePtr stride_read_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "strideRead") {
          stride_read_node = node;
        }
      }
      graph->RemoveNode(stride_read_node);
    }

/****************************************************
*
*       data         filter      bias
*         \           \           /
*                     conv
*                      |
*      other_input -- dequant -- other_output
*                      |
*                    quant
*                      |
*                    output
*
****************************************************/
    static void BuildGraph15(const ComputeGraphPtr& graph) {
      BuildGraph7(graph);
      NodePtr stride_read_node;
      NodePtr stride_write_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "strideRead") {
          stride_read_node = node;
        }
        if (node->GetName() == "strideWrite") {
          stride_write_node = node;
        }
      }
      graph->RemoveNode(stride_read_node);
      graph->RemoveNode(stride_write_node);
    }

/****************************************************
*
*       data       stride_read      filter      bias
*         \           \             /          /
*                     conv
*                      |
*      other_input -- dequant -- other_output
*                      |
*                  elem_wise1
*                      |
*                  elem_wise2
*                      |
*                  elem_wise3
*                      |
*                  elem_wise4
*                      |
*                  elem_wise5 (not in whitelist)
*                      |
*                    quant
*                      |
*                 stride_write
*                      |
*                    output
*
****************************************************/
    static void BuildGraph16(const ComputeGraphPtr& graph) {
      BuildGraph1(graph);
      NodePtr elem_wise5_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "elemWise5") {
          elem_wise5_node = node;
        }
      }
      elem_wise5_node->GetOpDesc()->SetType("Abs");
    }

/****************************************************
*
*       data       stride_read      filter      bias
*         \           \             /          /
*                     conv
*                      |
*      other_input -- dequant -- other_output
*                      |
*                  elem_wise1
*                      |
*                  elem_wise2
*                      |
*                  elem_wise3
*                      |
*                  elem_wise4
*                      |
*                  elem_wise5 -- other_output1
*                      |
*                    quant
*                      |
*                 stride_write
*                      |
*                    output
*
****************************************************/
    static void BuildGraph17(const ComputeGraphPtr& graph) {
      BuildGraph1(graph);
      NodePtr elem_wise5_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "elemWise5") {
          elem_wise5_node = node;
        }
      }
      /** other_output1 */
      OpDescPtr other_output1 = std::make_shared<OpDesc>("otherOutput1", fe::DATA);
      SetPattern(other_output1, fe::TBE_PATTERN_OUTPUT_NODE);
      SetTvmType(other_output1);
      vector<int64_t> dim(4, 4);
      GeShape shape(dim);
      GeTensorDesc tensor_desc(shape);
      other_output1->AddInputDesc(tensor_desc);
      AttrUtils::SetInt(other_output1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr other_output_node1 = graph->AddNode(other_output1);
      GraphUtils::AddEdge(elem_wise5_node->GetOutDataAnchor(0), other_output_node1->GetInDataAnchor(0));
    }

/****************************************************
*
*       data       stride_read      filter      bias
*         \           \             /          /
*                     conv
*                      |
*      other_input -- dequant -- other_output
*                      |
*                  elem_wise1
*                      |
*                  elem_wise2
*                      |
*                  elem_wise3
*                      |
*                  elem_wise4
*                      |
*                  elem_wise5
*                      |
*                  elem_wise6
*                      |
*                    quant
*                      |
*                 stride_write
*                      |
*                    output
*
****************************************************/
    static void BuildGraph19(const ComputeGraphPtr& graph) {
      BuildGraph1(graph);
      NodePtr elem_wise5_node;
      NodePtr quant_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "elemWise5") {
          elem_wise5_node = node;
        }
        if (node->GetName() == "quant") {
          quant_node = node;
        }
      }
      /** elem_wise6 */
      OpDescPtr elem_wise6 = std::make_shared<OpDesc>("elemWise6", fe::ELTWISE);
      SetPattern(elem_wise6, OP_PATTERN_ELEMWISE);
      SetTvmType(elem_wise6);
      vector<int64_t> dim(4, 4);
      GeShape shape(dim);
      GeTensorDesc tensor_desc(shape);
      elem_wise6->AddInputDesc(tensor_desc);
      elem_wise6->AddOutputDesc(tensor_desc);
      AttrUtils::SetInt(elem_wise6, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr elem_wise6_node = graph->AddNode(elem_wise6);

      GraphUtils::RemoveEdge(elem_wise5_node->GetOutDataAnchor(0), quant_node->GetInDataAnchor(0));
      GraphUtils::AddEdge(elem_wise5_node->GetOutDataAnchor(0), elem_wise6_node->GetInDataAnchor(0));
      GraphUtils::AddEdge(elem_wise6_node->GetOutDataAnchor(0), quant_node->GetInDataAnchor(0));
    }

/****************************************************
*
*       data       stride_read      filter      bias
*         \           \             /          /
*                     conv
*                      |
*      other_input -- dequant -- read_select
*                      |
*                  elem_wise1
*                      |
*                  elem_wise2
*                      |
*                  elem_wise3
*                      |
*                  elem_wise4
*                      |
*                  elem_wise5
*                      |
*                  elem_wise6
*                      |
*                    quant
*                      |
*                 stride_write
*                      |
*                     cast
*
****************************************************/
    static void BuildGraph20(const ComputeGraphPtr& graph) {
      BuildGraph1(graph);
      NodePtr other_output_node;
      NodePtr output_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "otherOutput") {
          other_output_node = node;
        }
        if (node->GetName() == "output") {
          output_node = node;
        }
      }
      other_output_node->GetOpDesc()->SetName("readSelect");
      other_output_node->GetOpDesc()->SetType(fe::READ_SELECT);
      output_node->GetOpDesc()->SetName("cast");
      output_node->GetOpDesc()->SetType(fe::CAST);
    }

/****************************************************
*
*       data       stride_read      filter      bias
*         \           \             /          /
*                     conv
*                      |
*      other_input -- dequant -- other_output
*                      |
*                  elem_wise1
*                      |
*                  elem_wise2
*                      |
*                  elem_wise3
*                      |
*                  elem_wise4
*                      |
*                  elem_wise5
*                      |
*                    quant
*                      |
*                 stride_write
*                   |    \
*                output output1
*
****************************************************/
    static void BuildGraph21(const ComputeGraphPtr& graph) {
      BuildGraph1(graph);
      NodePtr stride_write_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "strideWrite") {
          stride_write_node = node;
        }
      }
      /** output1 */
      OpDescPtr output1 = std::make_shared<OpDesc>("output1", fe::DATA);
      SetPattern(output1, fe::TBE_PATTERN_OUTPUT_NODE);
      SetTvmType(output1);
      vector<int64_t> dim(4, 4);
      GeShape shape(dim);
      GeTensorDesc tensor_desc(shape);
      output1->AddInputDesc(tensor_desc);
      AttrUtils::SetInt(output1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr output1_node = graph->AddNode(output1);

      GraphUtils::AddEdge(stride_write_node->GetOutDataAnchor(0), output1_node->GetInDataAnchor(0));
    }

/****************************************************
*
*       data       stride_read      filter      bias
*         \           \             /          /
*                     conv
*                      |
*      other_input -- dequant -- other_output
*                      |
*                  elem_wise1
*                      |
*                  elem_wise2
*                      |
*                  elem_wise3
*                      |
*                  elem_wise4
*                      |
*                  elem_wise5
*                      |
*                    quant
*                   |    \
*                output output1
*
****************************************************/
    static void BuildGraph22(const ComputeGraphPtr& graph) {
      BuildGraph10(graph);
      NodePtr quant_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "quant") {
          quant_node = node;
        }
      }
      /** output1 */
      OpDescPtr output1 = std::make_shared<OpDesc>("output1", fe::DATA);
      SetPattern(output1, fe::TBE_PATTERN_OUTPUT_NODE);
      SetTvmType(output1);
      vector<int64_t> dim(4, 4);
      GeShape shape(dim);
      GeTensorDesc tensor_desc(shape);
      output1->AddInputDesc(tensor_desc);
      AttrUtils::SetInt(output1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr output1_node = graph->AddNode(output1);

      GraphUtils::AddEdge(quant_node->GetOutDataAnchor(0), output1_node->GetInDataAnchor(0));
    }

/****************************************************
*
*       data       stride_read      filter      bias
*         \           \             /          /
*                     conv
*                      |
*      other_input -- dequant -- other_output
*                      |
*                  elem_wise1
*                      |
*                  elem_wise2
*                      |
*                  elem_wise3
*                      |
*                  elem_wise4
*                      |
*                  elem_wise5
*                      |
*                    quant -- output1
*                      |
*                  stride_write
*                      |
*                    output
*
****************************************************/
    static void BuildGraph23(const ComputeGraphPtr& graph) {
      BuildGraph1(graph);
      NodePtr quant_node;
      for (const auto& node : graph->GetDirectNode()) {
        if (node->GetName() == "quant") {
          quant_node = node;
        }
      }
      /** output1 */
      OpDescPtr output1 = std::make_shared<OpDesc>("output1", fe::DATA);
      SetPattern(output1, fe::TBE_PATTERN_OUTPUT_NODE);
      SetTvmType(output1);
      vector<int64_t> dim(4, 4);
      GeShape shape(dim);
      GeTensorDesc tensor_desc(shape);
      output1->AddInputDesc(tensor_desc);
      AttrUtils::SetInt(output1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
      NodePtr output1_node = graph->AddNode(output1);

      GraphUtils::AddEdge(quant_node->GetOutDataAnchor(0), output1_node->GetInDataAnchor(0));
    }

};

static std::string GetBuiltInFusionConfigFilePathStubs() {
  std::string switch_priority_file_path = "./air/test/engines/nneng/st/stub/fe_config/fusion_config.json";

  return  switch_priority_file_path;
}

TEST_F(UB_FUSION_ST_COMMON_RULES2, tbe_common_rules2_last_node_stride_write_multi_out) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  BuildGraph21(graph);
  graph->TopologicalSorting();
  graph->Dump();
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>(fe::AI_CORE_NAME);
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
          std::make_shared<FusionPriorityManager>(fe::AI_CORE_NAME, fusion_pass_mgr_ptr, nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
          std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);
  uint32_t id = 0;

  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES2 UB fusion before" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id: " << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
         ", type: " << node->GetOpDesc()->GetType() << endl;
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id: " << scope_id << endl;
    }
    id++;
  }
  sub_graph_optimizer_ptr->engine_name_ = fe::AI_CORE_NAME;
  // find sub-graphs that match UB fusion pattern
  sub_graph_optimizer_ptr->MatchFusionPatternFromGraph(*graph);

  // create fused Graph, and merge matched sub-graphs into fusion ops
  sub_graph_optimizer_ptr->BuildFusionGraph(*graph);

  id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES2 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id: " << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
         ", type: " << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == fe::STRIDEDREAD &&
        node->GetName() == "strideReadconvdequantelemWise1elemWise2elemWise3elemWise4elemWise5quantstrideWrite") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id: " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);

}

TEST_F(UB_FUSION_ST_COMMON_RULES2, tbe_common_rules2_last_node_quant_multi_out_01) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  BuildGraph22(graph);
  graph->TopologicalSorting();
  graph->Dump();
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>(fe::AI_CORE_NAME);
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
          std::make_shared<FusionPriorityManager>(fe::AI_CORE_NAME, fusion_pass_mgr_ptr, nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
          std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);
  uint32_t id = 0;

  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES2 UB fusion before" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id: " << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
         ", type: " << node->GetOpDesc()->GetType() << endl;
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id: " << scope_id << endl;
    }
    id++;
  }
  sub_graph_optimizer_ptr->engine_name_ = fe::AI_CORE_NAME;
  // find sub-graphs that match UB fusion pattern
  sub_graph_optimizer_ptr->MatchFusionPatternFromGraph(*graph);

  // create fused Graph, and merge matched sub-graphs into fusion ops
  sub_graph_optimizer_ptr->BuildFusionGraph(*graph);

  id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES2 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id: " << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
         ", type: " << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == fe::STRIDEDREAD &&
        node->GetName() == "strideReadconvdequantelemWise1elemWise2elemWise3elemWise4elemWise5quant") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id: " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);

}

TEST_F(UB_FUSION_ST_COMMON_RULES2, tbe_common_rules2_last_node_quant_multi_out_02) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  BuildGraph23(graph);
  graph->TopologicalSorting();
  graph->Dump();
  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>(fe::AI_CORE_NAME);
  graph_comm_ptr->Initialize();
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr = std::make_shared<FusionPassManager>();
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr =
          std::make_shared<FusionPriorityManager>(fe::AI_CORE_NAME, fusion_pass_mgr_ptr, nullptr);
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr =
          std::make_shared<BufferFusion>(graph_comm_ptr, fusion_pass_mgr_ptr, fusion_priority_mgr_ptr);
  uint32_t id = 0;

  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES2 UB fusion before" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id: " << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
         ", type: " << node->GetOpDesc()->GetType() << endl;
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id: " << scope_id << endl;
    }
    id++;
  }
  sub_graph_optimizer_ptr->engine_name_ = fe::AI_CORE_NAME;
  // find sub-graphs that match UB fusion pattern
  sub_graph_optimizer_ptr->MatchFusionPatternFromGraph(*graph);

  // create fused Graph, and merge matched sub-graphs into fusion ops
  sub_graph_optimizer_ptr->BuildFusionGraph(*graph);

  id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES2 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id: " << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
         ", type: " << node->GetOpDesc()->GetType() << endl;
    if (node->GetOpDesc()->GetType() == fe::STRIDEDREAD &&
        node->GetName() == "strideReadconvdequantelemWise1elemWise2elemWise3elemWise4elemWise5quant") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id: " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);

}
