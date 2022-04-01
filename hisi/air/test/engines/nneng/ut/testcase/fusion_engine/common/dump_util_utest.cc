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
#include <string>
#include <vector>

#define protected public
#define private public
#include "graph_optimizer/fe_graph_optimizer.h"
#include "graph/utils/graph_utils.h"
#include "graph/op_kernel_bin.h"
#include "common/graph_comm.h"
#include "common/dump_util.h"
#include "common/util/op_info_util.h"
#include "tensor_engine/fusion_api.h"
#undef protected
#undef private

using namespace std;
using namespace fe;
using namespace ge;

class UTEST_dump_util_utest : public testing::Test
{
protected:
    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

TEST_F(UTEST_dump_util_utest, case_dump_op_info_tensor)
{
    std::vector<te::TbeOpParam> puts;
    te::TbeOpParam tbe_op_param;
    std::vector<te::TbeOpTensor> tensors;
    te::TbeOpTensor tbe_op_tensor;
    std::vector<int64_t> slice_shape = {1, 2, 3, 4};
    std::vector<int64_t> offset = {256};
    tbe_op_tensor.SetShape(slice_shape);
    tbe_op_tensor.SetOriginShape(slice_shape);
    tbe_op_tensor.SetSgtSliceShape(slice_shape);
    tbe_op_tensor.SetValidShape(slice_shape);
    tbe_op_tensor.SetSliceOffset(offset);
    tensors.push_back(tbe_op_tensor);
    tbe_op_param.SetTensors(tensors);
    puts.push_back(tbe_op_param);
    std::string debug_str;
    bool flag = false;
    DumpOpInfoTensor(puts, debug_str);
    if (!debug_str.empty()) {
        flag = true;
    }
    EXPECT_EQ(flag, true);
}

TEST_F(UTEST_dump_util_utest, dump_op_info)
{
    te::TbeOpInfo op_info = te::TbeOpInfo("", "", "", "", "");
    std::vector<te::TbeOpParam> puts;
    te::TbeOpParam tbe_op_param;
    std::vector<te::TbeOpTensor> tensors;
    te::TbeOpTensor tbe_op_tensor;
    std::vector<int64_t> slice_shape = {1, 2, 3, 4};
    std::vector<int64_t> offset = {256};
    tbe_op_tensor.SetShape(slice_shape);
    tbe_op_tensor.SetOriginShape(slice_shape);
    tbe_op_tensor.SetSgtSliceShape(slice_shape);
    tbe_op_tensor.SetValidShape(slice_shape);
    tbe_op_tensor.SetSliceOffset(offset);
    tensors.push_back(tbe_op_tensor);
    tbe_op_param.SetTensors(tensors);
    puts.push_back(tbe_op_param);
    op_info.SetInputs(puts);
    op_info.SetOutputs(puts);
    DumpOpInfo(op_info);
}

TEST_F(UTEST_dump_util_utest, dump_l1_info)
{
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    ge::OpDescPtr op_desc = std::make_shared<OpDesc>("test", "test");
    std::vector<int64_t> in_memory_type_list = {2, 2};
    std::vector<int64_t> out_memory_type_list = {2, 2};
    (void)ge::AttrUtils::SetListInt(op_desc, ge::ATTR_NAME_INPUT_MEM_TYPE_LIST, in_memory_type_list);
    (void)ge::AttrUtils::SetListInt(op_desc, ge::ATTR_NAME_OUTPUT_MEM_TYPE_LIST, out_memory_type_list);
    ToOpStructPtr lx_info = std::make_shared<ToOpStruct>();
    lx_info->slice_input_shape = {{2,2,2,2}};
    lx_info->slice_output_shape = {{2,2,2,2}};
    lx_info->slice_input_offset = {{2,2,2,2}};
    lx_info->slice_output_offset = {{2,2,2,2}};
    op_desc->SetExtAttr(ge::ATTR_NAME_L1_FUSION_EXTEND_PTR, lx_info);
    ge::NodePtr node = graph->AddNode(op_desc);
    DumpL1Attr(node.get());
}

TEST_F(UTEST_dump_util_utest, dump_l2_info)
{
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    ge::OpDescPtr op_desc = std::make_shared<OpDesc>("test", "test");
    std::vector<int64_t> in_memory_type_list = {2, 2};
    std::vector<int64_t> out_memory_type_list = {2, 2};
    (void)ge::AttrUtils::SetListInt(op_desc, ge::ATTR_NAME_INPUT_MEM_TYPE_LIST, in_memory_type_list);
    (void)ge::AttrUtils::SetListInt(op_desc, ge::ATTR_NAME_OUTPUT_MEM_TYPE_LIST, out_memory_type_list);
    ToOpStructPtr lx_info = std::make_shared<ToOpStruct>();
    lx_info->slice_input_shape = {{2,2,2,2}};
    lx_info->slice_output_shape = {{2,2,2,2}};
    lx_info->slice_input_offset = {{2,2,2,2}};
    lx_info->slice_output_offset = {{2,2,2,2}};
    op_desc->SetExtAttr(ATTR_NAME_L2_FUSION_EXTEND_PTR, lx_info);
    ge::NodePtr node = graph->AddNode(op_desc);
    DumpL2Attr(node.get());
}