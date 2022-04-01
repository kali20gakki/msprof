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

#define protected public
#define private public
#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_comm/l2_fusion_comm.h"
#include "common/configuration.h"
#include "adapter/common/op_store_adapter_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#undef private
#undef protected

#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <stdio.h>
#include "runtime/base.h"
#include "graph/ge_tensor.h"
#include "graph/compute_graph.h"
#include "graph/op_desc.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/attr_utils.h"
#include "fusion_manager/fusion_manager.h"
#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_optimizer.h"

#define protected public
#define private public
#include "graph_optimizer/fe_graph_optimizer.h"
#include "common/configuration.h"
// #include "all_ops.h"
#include "common/util/op_info_util.h"
#include <adapter/tbe_adapter/tbe_op_store_adapter.h>
#include "ops_kernel_store/fe_ops_kernel_info_store.h"
#include "ops_store/ops_kernel_manager.h"
#include <fusion_rule_manager/fusion_rule_data/fusion_rule_pattern.h>

#include "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_parser/l2_fusion_parser.h"

#undef protected
#undef private

using namespace fe;
using namespace ge;
static const std::string OPDESC_SRC_NAME = "opdesc_src_name";
static const std::string OPDESC_DST_NAME = "opdesc_dst_name";

using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;

typedef struct
{
    string targetname;
    uint32_t index;
} desc_info;

class L2_OPTIMIZER_ST : public testing::Test {
protected:
    FEOpsKernelInfoStorePtr op_info_store_in_l2_;
    OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
    std::vector<FEOpsStoreInfo> cfg_info_in_l2_;
    RefRelationsPtr reflection_builder_ptr_;

protected:
    void SetUp()
    {
        reflection_builder_ptr_ = std::make_shared<ge::RefRelations>();
        op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
        op_info_store_in_l2_ = std::make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_);
        FEOpsStoreInfo TIK_CUSTOM_OPINFO_STUB  = {
            1,
            "tik-custom",
            EN_IMPL_CUSTOM_TIK,
            "./air/test/engines/nneng/st/stub/fe_config/tik_custom_opinfo",
            ""
        };
        FEOpsStoreInfo TBE_CUSTOM_OPINFO_STUB = {
            2,
            "tbe-custom",
            EN_IMPL_CUSTOM_TBE,
            "./air/test/engines/nneng/st/stub/fe_config/tbe_custom_opinfo",
            ""
        };
        FEOpsStoreInfo TIK_OPINFO_STUB = {
            5,
            "tik-builtin",
            EN_IMPL_HW_TIK,
            "./air/test/engines/nneng/st/stub/fe_config/tik_opinfo",
            ""
        };
        FEOpsStoreInfo TBE_OPINFO_STUB = {
            6,
            "tbe-builtin",
            EN_IMPL_HW_TBE,
            "./air/test/engines/nneng/st/stub/fe_config/tbe_opinfo",
            ""
        };
        FEOpsStoreInfo RL_OPINFO_STUB = {
            7,
            "rl-builtin",
            EN_IMPL_RL,
            "./air/test/engines/nneng/st/stub/fe_config/rl_opinfo",
            ""
        };
        cfg_info_in_l2_.push_back(TIK_CUSTOM_OPINFO_STUB);
        cfg_info_in_l2_.push_back(TBE_CUSTOM_OPINFO_STUB);
        cfg_info_in_l2_.push_back(TIK_OPINFO_STUB);
        cfg_info_in_l2_.push_back(TBE_OPINFO_STUB);
        cfg_info_in_l2_.push_back(RL_OPINFO_STUB);
    }
    void TearDown()
    {

        std::cout << "L2 optimizer TearDown" << std::endl;
    }
};

void AllocScopeId(ge::OpDescPtr opdef, uint32_t scopeid)
{
    // ref SCOPE_ID_ATTR in scope_allocator.cc
    ge::AttrUtils::SetInt(opdef, SCOPE_ID_ATTR, scopeid);
}

ge::OpDescPtr CreateOpDef(string name, string type, vector<string> &srcname_list, vector<string> &dstname_list,
                          vector<ge::GeTensorDesc> &inputdesc_list, vector<ge::GeTensorDesc> &outputdesc_list)
{
    ge::OpDescPtr opdef = std::make_shared<ge::OpDesc>(name, type);

    uint32_t src_node_num = inputdesc_list.size();
    vector<bool> fusion_is_input_const_vector;
    for (uint32_t loop = 0; loop < src_node_num; loop++) {
        fusion_is_input_const_vector.push_back(false);
        opdef->AddInputDesc(inputdesc_list[loop]);
    }
    opdef->SetIsInputConst(fusion_is_input_const_vector);

    uint32_t dst_node_num = outputdesc_list.size();
    for (uint32_t loop = 0; loop < dst_node_num; loop++) {
        opdef->AddOutputDesc(outputdesc_list[loop]);
    }

    ge::AttrUtils::SetInt(opdef, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_HW_TBE));

    return opdef;
}

// fill shape, datatype and format in tensor_desc
void FillTensorDesc(ge::GeTensorDesc &tensor_desc, int64_t n, int64_t c, int64_t h, int64_t w, ge::DataType d_type,
                    ge::Format d_format)
{
    vector<int64_t> dim;
    dim.push_back(n);
    dim.push_back(c);
    dim.push_back(h);
    dim.push_back(w);
    ge::GeShape shape(dim);
    tensor_desc.SetShape(shape);
    tensor_desc.SetDataType(d_type);
    tensor_desc.SetFormat(d_format);
    return;
}

void GetSrcDstIndexL2(ge::NodePtr &src_op, ge::NodePtr &dst_op, map<string, vector<desc_info>> &src_map,
                      map<string, vector<desc_info>> &dst_map, int &src_index, int &dst_index)
{
    map<string, vector<desc_info>>::iterator it;

    // find src_op's output op -> dst_op's output index
    it = dst_map.find(src_op->GetName());
    if (it != dst_map.end()) {
        vector<desc_info> &vec1 = it->second;
        for (int loop = 0; loop < vec1.size(); loop++) {
            if (vec1[loop].targetname == dst_op->GetName()) {
                src_index = loop;
            }
        }
    } else {
        std::cout << "not be found in dst_map!" << std::endl;
    }

    // find dst_op's input op -> src_op's input index
    it = src_map.find(dst_op->GetName());
    if (it != src_map.end()) {
        vector<desc_info> &vec2 = it->second;
        for (int loop = 0; loop < vec2.size(); loop++) {
            if (vec2[loop].targetname == src_op->GetName()) {
                dst_index = loop;
            }
        }
    } else {
        std::cout << "not be found in src_map!" << std::endl;
    }
}

void CreateL2Graph(ge::ComputeGraphPtr model_graph, vector<ge::OpDescPtr> &op_list,
                   map<string, vector<desc_info>> &src_map, map<string, vector<desc_info>> &dst_map)
{
    uint32_t src_index = 0;
    uint32_t dst_index = 0;
    bool flag = false;

    std::cout << "graph added nodes as below: " << std::endl;
    uint32_t node_num = 0;
    for (auto opdef : op_list) {
        ge::NodePtr node = model_graph->AddNode(opdef);
        cout << "To be added: node[" << node_num << "]name = " << node->GetName() << endl;
        node_num++;
    }

    for (ge::OpDescPtr opdef : op_list) {
        ge::NodePtr src_node = model_graph->FindNode(opdef->GetName());
        map<string, vector<desc_info>>::iterator it;
        it = dst_map.find(opdef->GetName());
        ge::NodePtr dst_node;
        if (it != dst_map.end()) {
            vector<desc_info> &vec = it->second;

            // TODO!
            for (int32_t i = 0; i < vec.size(); i++) {
                dst_node = model_graph->FindNode(vec[i].targetname);
                int src_index = 0;
                int dst_index = 0;
                GetSrcDstIndexL2(src_node, dst_node, src_map, dst_map, src_index, dst_index);
                ge::GraphUtils::AddEdge(src_node->GetOutDataAnchor(src_index), dst_node->GetInDataAnchor(dst_index));
            }

        } else {
            std::cout << "srcNode not found in the dst_map!" << std::endl;
        }
    }
    // Dump graph
    std::cout << "Created graph linking as below: " << std::endl;
    model_graph->Dump();

    // 6. set input and output ddr addr
    uint32_t ddr_addr = 0;
    for (auto node : model_graph->GetDirectNode()) {
        ge::OpDescPtr opdef = node->GetOpDesc();
        string node_type = opdef->GetType();
        int32_t input_size = opdef->GetInputsSize();
        int32_t output_size = opdef->GetOutputsSize();

        vector<int64_t> input_list;

        for (int32_t loop = 0; loop < input_size; loop++) {
            input_list.push_back(ddr_addr++);
        }

        opdef->SetInputOffset(input_list);

        vector<int64_t> output_list;

        for (int32_t loop = 0; loop < output_size; loop++) {
            output_list.push_back(ddr_addr++);
        }

        opdef->SetOutputOffset(input_list);
    }

    return;
}

Status OptimizerConfigurationStub(Configuration *This, std::string& graph_file_path);
Status OptimizerConfigurationStubBiasAddSoftMaxGrad(Configuration *This, std::string& graph_file_path);

/************************
*conv1-->prelu1-->eltw-->prelu2-->conv3
*                   ^
*                   |
*                 conv2
*redu eltw relu�ں�
*************************/
static uint32_t CreateGraph_conv1_conv2_redu_eltw_relu_conv3(ge::ComputeGraphPtr &model_graph)
{
    map<string, vector<desc_info>> src_map;
    map<string, vector<desc_info>> dst_map;
    map<string, vector<desc_info>>::iterator it;
    desc_info dscinfo;
    vector<desc_info> desctmp;

    ge::OpDescPtr opdef;
    vector<ge::OpDescPtr> op_list;
    vector<string> srcname_list;
    vector<string> dstname_list;
    ge::GeTensorDesc input_desc;
    ge::GeTensorDesc input_desc2;
    ge::GeTensorDesc output_desc;
    vector<ge::GeTensorDesc> inputdesc_list;
    vector<ge::GeTensorDesc> outputdesc_list;

    // 1.creat src map
    desctmp.clear();
    src_map.insert(pair<string, vector<desc_info >> ("conv1", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("conv2", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("conv3", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("prelu1", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("eltw", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("prelu2", desctmp));

    // 2.creat dst map
    dst_map.insert(pair<string, vector<desc_info >> ("conv1", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("conv2", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("conv3", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("prelu1", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("eltw", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("prelu2", desctmp));

    // conv2
    FillTensorDesc(input_desc, 1, 16, 300, 300, ge::DT_FLOAT16, ge::FORMAT_NCHW);   // create input tensor info
    FillTensorDesc(output_desc, 1, 16, 150, 150, ge::DT_FLOAT16, ge::FORMAT_NCHW);  // creat output tensor info

    srcname_list.clear();
    dstname_list.clear();
    dstname_list.push_back("eltw");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("conv2", "Convolution", srcname_list, dstname_list, inputdesc_list, outputdesc_list);
    op_list.push_back(opdef);

    it = dst_map.find("conv2");
    vector<desc_info> &vec1 = it->second;
    dscinfo.targetname = "eltw";
    dscinfo.index = 0;
    vec1.push_back(dscinfo);

    // conv1
    FillTensorDesc(input_desc, 1, 16, 200, 200, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, 1, 16, 160, 160, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    dstname_list.clear();
    dstname_list.push_back("prelu1");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("conv1", "Convolution", srcname_list, dstname_list, inputdesc_list, outputdesc_list);
    op_list.push_back(opdef);

    it = dst_map.find("conv1");
    vector<desc_info> &vec2 = it->second;
    dscinfo.targetname = "prelu1";
    dscinfo.index = 0;
    vec2.push_back(dscinfo);

    // redu
    FillTensorDesc(input_desc, 1, 16, 160, 160, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, 1, 16, 100, 100, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("conv1");
    dstname_list.clear();
    dstname_list.push_back("eltw");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("prelu1", "PReLU", srcname_list, dstname_list, inputdesc_list, outputdesc_list);

    AllocScopeId(opdef, 100);
    op_list.push_back(opdef);

    it = dst_map.find("prelu1");
    vector<desc_info> &vec3 = it->second;
    dscinfo.targetname = "eltw";
    dscinfo.index = 0;
    vec3.push_back(dscinfo);

    it = src_map.find("prelu1");
    vector<desc_info> &vec4 = it->second;
    dscinfo.targetname = "conv1";
    dscinfo.index = 0;
    vec4.push_back(dscinfo);

    // eltw
    FillTensorDesc(input_desc, 1, 16, 150, 150, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(input_desc2, 1, 16, 100, 100, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, 1, 16, 101, 101, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("conv2");
    srcname_list.push_back("prelu1");
    dstname_list.clear();
    dstname_list.push_back("prelu2");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    inputdesc_list.push_back(input_desc2);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("eltw", "Eltwise", srcname_list, dstname_list, inputdesc_list, outputdesc_list);

    AllocScopeId(opdef, 100);
    op_list.push_back(opdef);

    it = dst_map.find("eltw");
    vector<desc_info> &vec5 = it->second;
    dscinfo.targetname = "prelu2";
    dscinfo.index = 0;
    vec5.push_back(dscinfo);

    it = src_map.find("eltw");
    vector<desc_info> &vec6 = it->second;
    dscinfo.targetname = "conv2";
    dscinfo.index = 0;
    vec6.push_back(dscinfo);
    dscinfo.targetname = "prelu1";
    dscinfo.index = 1;
    vec6.push_back(dscinfo);

    // prelu
    FillTensorDesc(input_desc, 1, 16, 101, 101, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, 1, 16, 55, 55, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("eltw");
    dstname_list.clear();
    dstname_list.push_back("conv3");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("prelu2", "PReLU", srcname_list, dstname_list, inputdesc_list, outputdesc_list);

    AllocScopeId(opdef, 100);
    op_list.push_back(opdef);

    it = dst_map.find("prelu2");
    vector<desc_info> &vec7 = it->second;
    dscinfo.targetname = "conv3";
    dscinfo.index = 0;
    vec7.push_back(dscinfo);

    it = src_map.find("prelu2");
    vector<desc_info> &vec8 = it->second;
    dscinfo.targetname = "eltw";
    dscinfo.index = 0;
    vec8.push_back(dscinfo);

    // conv3
    FillTensorDesc(input_desc, 1, 16, 55, 55, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, 1, 16, 50, 50, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("prelu2");
    dstname_list.clear();
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("conv3", "Convolution", srcname_list, dstname_list, inputdesc_list, outputdesc_list);
    op_list.push_back(opdef);

    it = src_map.find("conv3");
    vector<desc_info> &vec9 = it->second;
    dscinfo.targetname = "prelu2";
    dscinfo.index = 0;
    vec9.push_back(dscinfo);

    // invoke creat graph
    CreateL2Graph(model_graph, op_list, src_map, dst_map);
    return fe::SUCCESS;
}

/************************
*conv1-->prelu1-->eltw-->prelu2-->conv3
*                   ^
*                   |
*                 conv2
*************************/
static uint32_t CreateGraph_conv1_conv2_redu_eltw_relu_conv3_small(ge::ComputeGraphPtr &model_graph)
{
    map<string, vector<desc_info>> src_map;
    map<string, vector<desc_info>> dst_map;
    map<string, vector<desc_info>>::iterator it;
    desc_info dscinfo;
    vector<desc_info> desctmp;

    ge::OpDescPtr opdef;
    vector<ge::OpDescPtr> op_list;
    vector<string> srcname_list;
    vector<string> dstname_list;
    ge::GeTensorDesc input_desc;
    ge::GeTensorDesc input_desc2;
    ge::GeTensorDesc output_desc;
    vector<ge::GeTensorDesc> inputdesc_list;
    vector<ge::GeTensorDesc> outputdesc_list;

    desctmp.clear();
    src_map.insert(pair<string, vector<desc_info >> ("conv1", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("conv2", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("conv3", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("prelu1", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("eltw", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("prelu2", desctmp));

    dst_map.insert(pair<string, vector<desc_info >> ("conv1", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("conv2", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("conv3", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("prelu1", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("eltw", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("prelu2", desctmp));

    // conv2
    FillTensorDesc(input_desc, 1, 16, 32, 32, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, 1, 16, 32, 32, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    dstname_list.clear();
    dstname_list.push_back("eltw");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("conv2", "Convolution", srcname_list, dstname_list, inputdesc_list, outputdesc_list);
    op_list.push_back(opdef);

    it = dst_map.find("conv2");
    vector<desc_info> &vec1 = it->second;
    dscinfo.targetname = "eltw";
    dscinfo.index = 0;
    vec1.push_back(dscinfo);

    // conv1
    FillTensorDesc(input_desc, 1, 16, 32, 32, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, 1, 16, 32, 32, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    dstname_list.clear();
    dstname_list.push_back("prelu1");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("conv1", "Convolution", srcname_list, dstname_list, inputdesc_list, outputdesc_list);
    op_list.push_back(opdef);

    it = dst_map.find("conv1");
    vector<desc_info> &vec2 = it->second;
    dscinfo.targetname = "prelu1";
    dscinfo.index = 0;
    vec2.push_back(dscinfo);

    // redu
    FillTensorDesc(input_desc, 1, 16, 32, 32, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, 1, 16, 32, 32, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("conv1");
    dstname_list.clear();
    dstname_list.push_back("eltw");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("prelu1", "PReLU", srcname_list, dstname_list, inputdesc_list, outputdesc_list);

    AllocScopeId(opdef, 100);
    op_list.push_back(opdef);

    it = dst_map.find("prelu1");
    vector<desc_info> &vec3 = it->second;
    dscinfo.targetname = "eltw";
    dscinfo.index = 0;
    vec3.push_back(dscinfo);

    it = src_map.find("prelu1");
    vector<desc_info> &vec4 = it->second;
    dscinfo.targetname = "conv1";
    dscinfo.index = 0;
    vec4.push_back(dscinfo);

    // eltw
    FillTensorDesc(input_desc, 1, 16, 32, 32, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(input_desc2, 1, 16, 32, 32, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, 1, 16, 32, 32, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("conv2");
    srcname_list.push_back("prelu1");
    dstname_list.clear();
    dstname_list.push_back("prelu2");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    inputdesc_list.push_back(input_desc2);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("eltw", "Eltwise", srcname_list, dstname_list, inputdesc_list, outputdesc_list);

    AllocScopeId(opdef, 100);
    op_list.push_back(opdef);

    it = dst_map.find("eltw");
    vector<desc_info> &vec5 = it->second;
    dscinfo.targetname = "prelu2";
    dscinfo.index = 0;
    vec5.push_back(dscinfo);

    it = src_map.find("eltw");
    vector<desc_info> &vec6 = it->second;
    dscinfo.targetname = "conv2";
    dscinfo.index = 0;
    vec6.push_back(dscinfo);
    dscinfo.targetname = "prelu1";
    dscinfo.index = 1;
    vec6.push_back(dscinfo);

    // relu
    FillTensorDesc(input_desc, 1, 16, 32, 32, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, 1, 16, 32, 32, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("eltw");
    dstname_list.clear();
    dstname_list.push_back("conv3");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("prelu2", "PReLU", srcname_list, dstname_list, inputdesc_list, outputdesc_list);

    AllocScopeId(opdef, 100);
    op_list.push_back(opdef);

    it = dst_map.find("prelu2");
    vector<desc_info> &vec7 = it->second;
    dscinfo.targetname = "conv3";
    dscinfo.index = 0;
    vec7.push_back(dscinfo);

    it = src_map.find("prelu2");
    vector<desc_info> &vec8 = it->second;
    dscinfo.targetname = "eltw";
    dscinfo.index = 0;
    vec8.push_back(dscinfo);

    // conv3
    FillTensorDesc(input_desc, 1, 16, 32, 32, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, 1, 16, 32, 32, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("prelu2");
    dstname_list.clear();
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("conv3", "Convolution", srcname_list, dstname_list, inputdesc_list, outputdesc_list);
    op_list.push_back(opdef);

    it = src_map.find("conv3");
    vector<desc_info> &vec9 = it->second;
    dscinfo.targetname = "prelu2";
    dscinfo.index = 0;
    vec9.push_back(dscinfo);

    CreateL2Graph(model_graph, op_list, src_map, dst_map);
    return fe::SUCCESS;
}


/************************
*conv1-->prelu1-->eltw-->prelu2-->conv3
*                   ^
*                   |
*                 conv2
*redu eltw relu�ں�
*************************/
static uint32_t CreateGraph_conv1_conv2_redu_eltw_relu_conv3_splitBatch(ge::ComputeGraphPtr &model_graph)
{
    uint32_t batch_size = 32;
    map<string, vector<desc_info>> src_map;
    map<string, vector<desc_info>> dst_map;
    map<string, vector<desc_info>>::iterator it;
    desc_info dscinfo;
    vector<desc_info> desctmp;

    ge::OpDescPtr opdef;
    vector<ge::OpDescPtr> op_list;
    vector<string> srcname_list;
    vector<string> dstname_list;
    ge::GeTensorDesc input_desc;
    ge::GeTensorDesc input_desc2;
    ge::GeTensorDesc output_desc;
    vector<ge::GeTensorDesc> inputdesc_list;
    vector<ge::GeTensorDesc> outputdesc_list;

    desctmp.clear();
    src_map.insert(pair<string, vector<desc_info >> ("conv1", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("conv2", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("conv3", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("prelu1", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("eltw", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("prelu2", desctmp));

    dst_map.insert(pair<string, vector<desc_info >> ("conv1", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("conv2", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("conv3", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("prelu1", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("eltw", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("prelu2", desctmp));

    // conv2
    FillTensorDesc(input_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    dstname_list.clear();
    dstname_list.push_back("eltw");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("conv2", "Convolution", srcname_list, dstname_list, inputdesc_list, outputdesc_list);
    op_list.push_back(opdef);

    it = dst_map.find("conv2");
    vector<desc_info> &vec1 = it->second;
    dscinfo.targetname = "eltw";
    dscinfo.index = 0;
    vec1.push_back(dscinfo);

    // conv1
    FillTensorDesc(input_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    dstname_list.clear();
    dstname_list.push_back("prelu1");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("conv1", "Convolution", srcname_list, dstname_list, inputdesc_list, outputdesc_list);
    op_list.push_back(opdef);

    it = dst_map.find("conv1");
    vector<desc_info> &vec2 = it->second;
    dscinfo.targetname = "prelu1";
    dscinfo.index = 0;
    vec2.push_back(dscinfo);

    // redu
    FillTensorDesc(input_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("conv1");
    dstname_list.clear();
    dstname_list.push_back("eltw");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("prelu1", "PReLU", srcname_list, dstname_list, inputdesc_list, outputdesc_list);

    AllocScopeId(opdef, 100);
    op_list.push_back(opdef);

    it = dst_map.find("prelu1");
    vector<desc_info> &vec3 = it->second;
    dscinfo.targetname = "eltw";
    dscinfo.index = 0;
    vec3.push_back(dscinfo);

    it = src_map.find("prelu1");
    vector<desc_info> &vec4 = it->second;
    dscinfo.targetname = "conv1";
    dscinfo.index = 0;
    vec4.push_back(dscinfo);

    // eltw
    FillTensorDesc(input_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(input_desc2, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, batch_size, 16, 28, 28, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("conv2");
    srcname_list.push_back("prelu1");
    dstname_list.clear();
    dstname_list.push_back("prelu2");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    inputdesc_list.push_back(input_desc2);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("eltw", "Eltwise", srcname_list, dstname_list, inputdesc_list, outputdesc_list);

    AllocScopeId(opdef, 100);
    op_list.push_back(opdef);

    it = dst_map.find("eltw");
    vector<desc_info> &vec5 = it->second;
    dscinfo.targetname = "prelu2";
    dscinfo.index = 0;
    vec5.push_back(dscinfo);

    it = src_map.find("eltw");
    vector<desc_info> &vec6 = it->second;
    dscinfo.targetname = "conv2";
    dscinfo.index = 0;
    vec6.push_back(dscinfo);
    dscinfo.targetname = "prelu1";
    dscinfo.index = 1;
    vec6.push_back(dscinfo);

    // relu
    FillTensorDesc(input_desc, batch_size, 16, 28, 28, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, batch_size, 16, 28, 28, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("eltw");
    dstname_list.clear();
    dstname_list.push_back("conv3");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("prelu2", "PReLU", srcname_list, dstname_list, inputdesc_list, outputdesc_list);

    AllocScopeId(opdef, 100);
    op_list.push_back(opdef);

    it = dst_map.find("prelu2");
    vector<desc_info> &vec7 = it->second;
    dscinfo.targetname = "conv3";
    dscinfo.index = 0;
    vec7.push_back(dscinfo);

    it = src_map.find("prelu2");
    vector<desc_info> &vec8 = it->second;
    dscinfo.targetname = "eltw";
    dscinfo.index = 0;
    vec8.push_back(dscinfo);

    // conv3
    FillTensorDesc(input_desc, batch_size, 16, 28, 28, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, batch_size, 16, 28, 28, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("prelu2");
    dstname_list.clear();
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("conv3", "Convolution", srcname_list, dstname_list, inputdesc_list, outputdesc_list);
    op_list.push_back(opdef);

    it = src_map.find("conv3");
    vector<desc_info> &vec9 = it->second;
    dscinfo.targetname = "prelu2";
    dscinfo.index = 0;
    vec9.push_back(dscinfo);

    CreateL2Graph(model_graph, op_list, src_map, dst_map);
    return fe::SUCCESS;
}

/************************
*conv1-->prelu1-->eltw-->prelu2-->conv3
*                   ^
*                   |
*                 conv2
*redu eltw relu�ں�
*************************/
static uint32_t CreateGraph_conv1_conv2_redu_eltw_relu_conv3_splitBatch_Big(ge::ComputeGraphPtr &model_graph)
{
    uint32_t batch_size = 64;
    map<string, vector<desc_info>> src_map;
    map<string, vector<desc_info>> dst_map;
    map<string, vector<desc_info>>::iterator it;
    desc_info dscinfo;
    vector<desc_info> desctmp;

    ge::OpDescPtr opdef;
    vector<ge::OpDescPtr> op_list;
    vector<string> srcname_list;
    vector<string> dstname_list;
    ge::GeTensorDesc input_desc;
    ge::GeTensorDesc input_desc2;
    ge::GeTensorDesc output_desc;
    vector<ge::GeTensorDesc> inputdesc_list;
    vector<ge::GeTensorDesc> outputdesc_list;

    desctmp.clear();
    src_map.insert(pair<string, vector<desc_info >> ("conv1", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("conv2", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("conv3", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("prelu1", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("eltw", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("prelu2", desctmp));

    dst_map.insert(pair<string, vector<desc_info >> ("conv1", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("conv2", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("conv3", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("prelu1", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("eltw", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("prelu2", desctmp));

    // conv2
    FillTensorDesc(input_desc, batch_size, 64, 128, 128, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, batch_size, 64, 128, 128, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    dstname_list.clear();
    dstname_list.push_back("eltw");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("conv2", "Convolution", srcname_list, dstname_list, inputdesc_list, outputdesc_list);
    op_list.push_back(opdef);

    it = dst_map.find("conv2");
    vector<desc_info> &vec1 = it->second;
    dscinfo.targetname = "eltw";
    dscinfo.index = 0;
    vec1.push_back(dscinfo);

    // conv1
    FillTensorDesc(input_desc, batch_size, 64, 128, 128, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, batch_size, 64, 128, 128, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    dstname_list.clear();
    dstname_list.push_back("prelu1");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("conv1", "Convolution", srcname_list, dstname_list, inputdesc_list, outputdesc_list);
    op_list.push_back(opdef);

    it = dst_map.find("conv1");
    vector<desc_info> &vec2 = it->second;
    dscinfo.targetname = "prelu1";
    dscinfo.index = 0;
    vec2.push_back(dscinfo);

    // redu
    FillTensorDesc(input_desc, batch_size, 64, 128, 128, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, batch_size, 64, 128, 128, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("conv1");
    dstname_list.clear();
    dstname_list.push_back("eltw");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("prelu1", "PReLU", srcname_list, dstname_list, inputdesc_list, outputdesc_list);

    AllocScopeId(opdef, 100);
    op_list.push_back(opdef);

    it = dst_map.find("prelu1");
    vector<desc_info> &vec3 = it->second;
    dscinfo.targetname = "eltw";
    dscinfo.index = 0;
    vec3.push_back(dscinfo);

    it = src_map.find("prelu1");
    vector<desc_info> &vec4 = it->second;
    dscinfo.targetname = "conv1";
    dscinfo.index = 0;
    vec4.push_back(dscinfo);

    // eltw
    FillTensorDesc(input_desc, batch_size, 64, 128, 128, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(input_desc2, batch_size, 64, 128, 128, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, batch_size, 64, 128, 128, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("conv2");
    srcname_list.push_back("prelu1");
    dstname_list.clear();
    dstname_list.push_back("prelu2");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    inputdesc_list.push_back(input_desc2);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("eltw", "Eltwise", srcname_list, dstname_list, inputdesc_list, outputdesc_list);

    AllocScopeId(opdef, 100);
    op_list.push_back(opdef);

    it = dst_map.find("eltw");
    vector<desc_info> &vec5 = it->second;
    dscinfo.targetname = "prelu2";
    dscinfo.index = 0;
    vec5.push_back(dscinfo);

    it = src_map.find("eltw");
    vector<desc_info> &vec6 = it->second;
    dscinfo.targetname = "conv2";
    dscinfo.index = 0;
    vec6.push_back(dscinfo);
    dscinfo.targetname = "prelu1";
    dscinfo.index = 1;
    vec6.push_back(dscinfo);

    // relu
    FillTensorDesc(input_desc, batch_size, 64, 128, 128, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, batch_size, 64, 128, 128, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("eltw");
    dstname_list.clear();
    dstname_list.push_back("conv3");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("prelu2", "PReLU", srcname_list, dstname_list, inputdesc_list, outputdesc_list);

    AllocScopeId(opdef, 100);
    op_list.push_back(opdef);

    it = dst_map.find("prelu2");
    vector<desc_info> &vec7 = it->second;
    dscinfo.targetname = "conv3";
    dscinfo.index = 0;
    vec7.push_back(dscinfo);

    it = src_map.find("prelu2");
    vector<desc_info> &vec8 = it->second;
    dscinfo.targetname = "eltw";
    dscinfo.index = 0;
    vec8.push_back(dscinfo);

    // conv3
    FillTensorDesc(input_desc, batch_size, 64, 128, 128, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, batch_size, 64, 128, 128, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("prelu2");
    dstname_list.clear();
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("conv3", "Convolution", srcname_list, dstname_list, inputdesc_list, outputdesc_list);
    op_list.push_back(opdef);

    it = src_map.find("conv3");
    vector<desc_info> &vec9 = it->second;
    dscinfo.targetname = "prelu2";
    dscinfo.index = 0;
    vec9.push_back(dscinfo);

    CreateL2Graph(model_graph, op_list, src_map, dst_map);
    return fe::SUCCESS;
}

/************************
*tile1-->prelu1-->eltw-->fc-->conv3
*                   ^
*                   |
*                 conv2
*************************/
static uint32_t CreateGraph_conv1_conv2_redu_eltw_relu_conv3_special_nodes(ge::ComputeGraphPtr &model_graph)
{
    uint32_t batch_size = 2;
    map<string, vector<desc_info>> src_map;
    map<string, vector<desc_info>> dst_map;
    map<string, vector<desc_info>>::iterator it;
    desc_info dscinfo;
    vector<desc_info> desctmp;

    ge::OpDescPtr opdef;
    vector<ge::OpDescPtr> op_list;
    vector<string> srcname_list;
    vector<string> dstname_list;
    ge::GeTensorDesc input_desc;
    ge::GeTensorDesc input_desc2;
    ge::GeTensorDesc output_desc;
    vector<ge::GeTensorDesc> inputdesc_list;
    vector<ge::GeTensorDesc> outputdesc_list;

    desctmp.clear();
    src_map.insert(pair<string, vector<desc_info >> ("tile1", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("conv2", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("conv3", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("prelu1", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("eltw", desctmp));
    src_map.insert(pair<string, vector<desc_info >> ("fc", desctmp));

    dst_map.insert(pair<string, vector<desc_info >> ("tile1", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("conv2", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("conv3", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("prelu1", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("eltw", desctmp));
    dst_map.insert(pair<string, vector<desc_info >> ("fc", desctmp));

    // conv2
    FillTensorDesc(input_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    dstname_list.clear();
    dstname_list.push_back("eltw");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("conv2", "Convolution", srcname_list, dstname_list, inputdesc_list, outputdesc_list);
    op_list.push_back(opdef);

    it = dst_map.find("conv2");
    vector<desc_info> &vec1 = it->second;
    dscinfo.targetname = "eltw";
    dscinfo.index = 0;
    vec1.push_back(dscinfo);

    // tile1
    FillTensorDesc(input_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    dstname_list.clear();
    dstname_list.push_back("prelu1");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("tile1", "Tile", srcname_list, dstname_list, inputdesc_list, outputdesc_list);
    op_list.push_back(opdef);

    it = dst_map.find("tile1");
    vector<desc_info> &vec2 = it->second;
    dscinfo.targetname = "prelu1";
    dscinfo.index = 0;
    vec2.push_back(dscinfo);

    // prelu1
    FillTensorDesc(input_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("tile1");
    dstname_list.clear();
    dstname_list.push_back("eltw");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("prelu1", "PReLU", srcname_list, dstname_list, inputdesc_list, outputdesc_list);
    op_list.push_back(opdef);

    it = dst_map.find("prelu1");
    vector<desc_info> &vec3 = it->second;
    dscinfo.targetname = "eltw";
    dscinfo.index = 0;
    vec3.push_back(dscinfo);

    it = src_map.find("prelu1");
    vector<desc_info> &vec4 = it->second;
    dscinfo.targetname = "tile1";
    dscinfo.index = 0;
    vec4.push_back(dscinfo);

    // eltw
    FillTensorDesc(input_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(input_desc2, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("conv2");
    srcname_list.push_back("prelu1");
    dstname_list.clear();
    dstname_list.push_back("fc");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    inputdesc_list.push_back(input_desc2);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("eltw", "Eltwise", srcname_list, dstname_list, inputdesc_list, outputdesc_list);
    op_list.push_back(opdef);

    it = dst_map.find("eltw");
    vector<desc_info> &vec5 = it->second;
    dscinfo.targetname = "fc";
    dscinfo.index = 0;
    vec5.push_back(dscinfo);

    it = src_map.find("eltw");
    vector<desc_info> &vec6 = it->second;
    dscinfo.targetname = "conv2";
    dscinfo.index = 0;
    vec6.push_back(dscinfo);
    dscinfo.targetname = "prelu1";
    dscinfo.index = 1;
    vec6.push_back(dscinfo);

    // fc
    FillTensorDesc(input_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("eltw");
    dstname_list.clear();
    dstname_list.push_back("conv3");
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("fc", "FullConnection", srcname_list, dstname_list, inputdesc_list, outputdesc_list);

    AllocScopeId(opdef, 100);
    op_list.push_back(opdef);

    it = dst_map.find("fc");
    vector<desc_info> &vec7 = it->second;
    dscinfo.targetname = "conv3";
    dscinfo.index = 0;
    vec7.push_back(dscinfo);

    it = src_map.find("fc");
    vector<desc_info> &vec8 = it->second;
    dscinfo.targetname = "eltw";
    dscinfo.index = 0;
    vec8.push_back(dscinfo);

    // conv3
    FillTensorDesc(input_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);
    FillTensorDesc(output_desc, batch_size, 64, 56, 56, ge::DT_FLOAT16, ge::FORMAT_NCHW);

    srcname_list.clear();
    srcname_list.push_back("fc");
    dstname_list.clear();
    inputdesc_list.clear();
    inputdesc_list.push_back(input_desc);
    outputdesc_list.clear();
    outputdesc_list.push_back(output_desc);
    opdef = CreateOpDef("conv3", "Convolution", srcname_list, dstname_list, inputdesc_list, outputdesc_list);
    op_list.push_back(opdef);

    it = src_map.find("conv3");
    vector<desc_info> &vec9 = it->second;
    dscinfo.targetname = "fc";
    dscinfo.index = 0;
    vec9.push_back(dscinfo);

    CreateL2Graph(model_graph, op_list, src_map, dst_map);
    return fe::SUCCESS;
}


TEST_F(L2_OPTIMIZER_ST, test_conv1_conv2_relu_eltw_relu_conv3_split_batch)
{
    ge::ComputeGraphPtr modelgraph = std::make_shared<ge::ComputeGraph>("test");
    EXPECT_EQ(CreateGraph_conv1_conv2_redu_eltw_relu_conv3_splitBatch(modelgraph), fe::SUCCESS);

    Configuration &config_inst = Configuration::Instance(fe::VECTOR_CORE_NAME);
    map<string, string> options;
    string soc_version = "Ascend310";
    config_inst.Initialize(options, soc_version);
    //configure.SetNeedL2Fusion(true);
    L2Optimizer l2_opt(fe::VECTOR_CORE_NAME);
    rtStream_t streamId = new rtStream_t();
    EXPECT_EQ(fe::SUCCESS, l2_opt.GetL2DataAlloc((*modelgraph), 0, streamId));
    delete streamId;
}

TEST_F(L2_OPTIMIZER_ST, update_input_for_l2_fusion)
{
    Configuration &configure = Configuration::Instance(fe::AI_CORE_NAME);
    configure.buffer_fusion_mode_ = EN_L2_FUSION;
    ge::ComputeGraphPtr modelgraph = std::make_shared<ge::ComputeGraph>("test");
    EXPECT_EQ(CreateGraph_conv1_conv2_redu_eltw_relu_conv3_splitBatch_Big(modelgraph), fe::SUCCESS);

    L2Optimizer l2_opt(fe::AI_CORE_NAME);
    L2FusionInfoPtr L2Info = std::make_shared<TaskL2FusionInfo_t>();
    L2Info->node_name = "testName";
    vector<int64_t> input_vector = {1};
    vector<int64_t> output_vector = {1};
    for (auto &node : (*modelgraph).GetDirectNode()) {
        node->GetOpDesc()->SetExtAttr(ATTR_NAME_TASK_L2_FUSION_INFO_EXTEND_PTR, L2Info);
        node->GetOpDesc()->SetInputOffset(input_vector);
        node->GetOpDesc()->SetOutputOffset(output_vector);
    }

    EXPECT_EQ(fe::SUCCESS, l2_opt.UpdateInputForL2Fusion(*modelgraph));
    configure.buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;
    }