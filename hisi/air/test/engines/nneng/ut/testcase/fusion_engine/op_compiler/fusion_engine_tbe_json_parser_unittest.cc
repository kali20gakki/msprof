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
#include <nlohmann/json.hpp>
#include "fcntl.h"

#define protected public
#define private public
#include <graph/tensor.h>
#include "graph_optimizer/op_compiler/tbe_json_parse.h"
#include "graph_optimizer/ffts/tbe_json_parse_impl.h"
#include "common/fe_utils.h"
#include "common/fe_log.h"
#include "graph/ge_tensor.h"
#undef protected
#undef private

using namespace std;
using namespace fe;
using namespace ge;
using namespace nlohmann;


static Status ParseParams(const google::protobuf::Message* op_src, ge::Operator& op_dest)
{
    return fe::SUCCESS;
}

static Status InferShapeAndType(vector<ge::TensorDesc>& v_output_desc)
{
    return fe::SUCCESS;
}

static Status UpdateOpDesc(ge::Operator&)
{
    return fe::SUCCESS;
}

static Status GetWorkspaceSize(const ge::Operator&, std::vector<int64_t>&)
{
    return fe::SUCCESS;
}


static Status BuildTeBin(string& json_file_path, string& bin_file_path)
{
    return fe::SUCCESS;
}

static Status BuildTeBin1(string json_file_name, string& json_file_path, string& bin_file_path)
{
    bin_file_path = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.o";
    json_file_path = "air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/" + json_file_name;
    return fe::SUCCESS;
}

static Status BuildTeBin2(string& json_file_path, string& bin_file_path)
{
    bin_file_path = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.o";
    json_file_path = "air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/cce_reductionLayer_1_10_float16__1_SUMSQ_1_with_so.json";
    return fe::SUCCESS;
}

static Status BuildTeBin5(string& json_file_path, string& bin_file_path)
{
    bin_file_path = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.o";
    json_file_path = "air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/cce_reduction_layer_1_10_float16__1_SUMSQ_1_0_null.json";
    return fe::SUCCESS;
}

static Status BuildTeBin6(string& json_file_path, string& bin_file_path)
{
    bin_file_path = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.o";
    json_file_path = "air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/cce_reduction_layer_1_10_float16__1_SUMSQ_1_0_error2.json";
    return fe::SUCCESS;
}

static Status BuildTeBin7(string& json_file_path, string& bin_file_path)
{
    bin_file_path = "./air/test/engines/nneng/stub/cce_reduction_layer_1_10_float16__1_SUMSQ_1_0_no_exist.o";
    json_file_path = "air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.json";
    return fe::SUCCESS;
}

static Status BuildTeBin8(string& json_file_path, string& bin_file_path)
{
    bin_file_path = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.o";
    json_file_path = "air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/cce_reduction_layer_1_10_float16__1_SUMSQ_1_0_error3.json";
    return fe::SUCCESS;
}

static Status BuildTeBin9(string& json_file_path, string& bin_file_path)
{
    bin_file_path = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.o";
    json_file_path = "air/test/engines/nneng/config/fe_config/atomic_test_core_type_wrong_string.json";
    return fe::SUCCESS;
}

static Status BuildTeBin10(string& json_file_path, string& bin_file_path)
{
    bin_file_path = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.o";
    json_file_path = "air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/cce_reduction_layer_1_10_float16__1_SUMSQ_1_0_error4.json";
    return fe::SUCCESS;
}

static Status BuildTeBin11(string& json_file_path, string& bin_file_path)
{
    bin_file_path = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.o";
    json_file_path = "air/test/engines/nneng/config/fe_config/atomic_test_core_type_wrong_type.json";
    return fe::SUCCESS;
}

static Status BuildTeBin12(string& json_file_path, string& bin_file_path)
{
    json_file_path = "air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/cce_reduction_layer_1_10_float16__1_SUMSQ_1_0_not_exist.json";
    return fe::SUCCESS;
}

static Status BuildTeBin13(string& json_file_path, string& bin_file_path)
{
    bin_file_path = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.o";
    json_file_path = "air/test/engines/nneng/config/fe_config/atomic_test_core_type_aiv.json";
    return fe::SUCCESS;
}

static Status BuildTeBin14(string& json_file_path, string& bin_file_path)
{
    bin_file_path = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.o";
    json_file_path = "air/test/engines/nneng/config/fe_config/atomic_test_parameters_no_workspace.json";
    return fe::SUCCESS;
}

static Status BuildTeBin15(string& json_file_path, string& bin_file_path)
{
    bin_file_path = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.o";
    json_file_path = "air/test/engines/nneng/config/fe_config/atomic_test_parameters_workspace_not_equal.json";
    return fe::SUCCESS;
}

static Status BuildTeBin16(string& json_file_path, string& bin_file_path)
{
    bin_file_path = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.o";
    json_file_path = "air/test/engines/nneng/config/fe_config/atomic_test_parameters_size_not_equal.json";
    return fe::SUCCESS;
}

static Status BuildTeBin17(string& json_file_path, string& bin_file_path)
{
    json_file_path = "air/test/engines/nneng/config/fe_config/atomic_test_false_parameters_true_weight.json";
    return fe::SUCCESS;
}


static Status BuildTeBin18(string& json_file_path, string& bin_file_path)
{
    json_file_path = "air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/te_conv2d_compress.json";
    return fe::SUCCESS;
}
static Status BuildTeBin19(string& json_file_path, string& bin_file_path)
{
  bin_file_path = "./air/test/engines/nneng/stub/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.o";
  json_file_path = "air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/cce_reduction_layer_1_10_float16__1_SUMSQ_1_0_error5.json";
  return fe::SUCCESS;
}
static Status BuildTeBinCoreTypeAIV(string& json_file_path, string& bin_file_path)
{
  json_file_path = "air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/te_conv2d_compress_core_type.json";
  return fe::SUCCESS;
}

static Status BuildTeBinCoreTypeAIVFail(string& json_file_path, string& bin_file_path)
{
  json_file_path = "air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/te_conv2d_compress_core_type_fail.json";
  return fe::SUCCESS;
}
class UTEST_FE_TBE_JSON_PARSER: public testing::Test
{
protected:
    void SetUp()
    {

    }

    void TearDown()
    {
    }
public:

};

TEST_F(UTEST_FE_TBE_JSON_PARSER, ParseTvmBlockDim_failed)
{
    OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>();
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_ptr);
    TbeJsonFileParse json_file_parse(*node);
    Status ret = json_file_parse.ParseTvmBlockDim();
    EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(UTEST_FE_TBE_JSON_PARSER, ParseBatchBindOnly_failed)
{
    OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>();
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_ptr);
    TbeJsonFileParse json_file_parse(*node);
    Status ret = json_file_parse.ParseBatchBindOnly();
    EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(UTEST_FE_TBE_JSON_PARSER, ParseTvmMagic_failed)
{
    OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>();
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_ptr);
    TbeJsonFileParse json_file_parse(*node);
    Status ret = json_file_parse.ParseTvmMagic();
    EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(UTEST_FE_TBE_JSON_PARSER, ParseTvmModeInArgsFirstField_failed)
{
    OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>();
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_ptr);
    TbeJsonFileParse json_file_parse(*node);
    Status ret = json_file_parse.ParseTvmModeInArgsFirstField();
    EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(UTEST_FE_TBE_JSON_PARSER, ParseTvmWorkSpace_failed)
{
    OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>();
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_ptr);
    TbeJsonFileParse json_file_parse(*node);
    Status ret = json_file_parse.ParseTvmWorkSpace();
    EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(UTEST_FE_TBE_JSON_PARSER, ParseTvmParameters_failed)
{
    OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>();
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_ptr);
    TbeJsonFileParse json_file_parse(*node);
    Status ret = json_file_parse.ParseTvmParameters();
    EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(UTEST_FE_TBE_JSON_PARSER, ParseConvCompressParameters_failed)
{
    OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>();
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_ptr);
    TbeJsonFileParse json_file_parse(*node);
    Status ret = json_file_parse.ParseConvCompressParameters();
    EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(UTEST_FE_TBE_JSON_PARSER, ParseWeightRepeat_failed)
{
    OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>();
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_ptr);
    TbeJsonFileParse json_file_parse(*node);
    Status ret = json_file_parse.ParseWeightRepeat();
    EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(UTEST_FE_TBE_JSON_PARSER, ParseOpParaSize_failed)
{
    OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>();
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_ptr);
    TbeJsonFileParse json_file_parse(*node);
    Status ret = json_file_parse.ParseOpParaSize();
    EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(UTEST_FE_TBE_JSON_PARSER, case_not_exist_bin_failed)
{
    OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>();
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_ptr);
    TbeJsonFileParse json_file_parse(*node);
    std::string json_file_path;
    std::string bin_file_path;
    BuildTeBin12(json_file_path, bin_file_path);
    Status ret = json_file_parse.PackageTvmJsonInfo(json_file_path, bin_file_path);
    EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(UTEST_FE_TBE_JSON_PARSER, case_json_format_error_failed)
{
    OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>();
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_ptr);
    TbeJsonFileParse json_file_parse(*node);
    string json_file_path;
    string bin_file_path;
    BuildTeBin10(json_file_path, bin_file_path);
    Status ret = json_file_parse.PackageTvmJsonInfo(json_file_path, bin_file_path);
    EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(UTEST_FE_TBE_JSON_PARSER, case_parse_tvm_core_type_fail_0)
{
    OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>();
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_ptr);
    TbeJsonFileParse json_file_parse(*node);
    string json_file_path;
    string bin_file_path;
    BuildTeBin9(json_file_path, bin_file_path);
    Status ret = json_file_parse.PackageTvmJsonInfo(json_file_path, bin_file_path);
    ret = json_file_parse.ParseTvmCoreType();
    EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(UTEST_FE_TBE_JSON_PARSER, case_parse_tvm_core_type_fail_1)
{
    OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>();
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_ptr);
    TbeJsonFileParse json_file_parse(*node);
    string json_file_path;
    string bin_file_path;
    BuildTeBin11(json_file_path, bin_file_path);
    Status ret = json_file_parse.PackageTvmJsonInfo(json_file_path, bin_file_path);
    ret = json_file_parse.ParseTvmCoreType();
    EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(UTEST_FE_TBE_JSON_PARSER, case_parse_tvm_core_type_suc)
{
    OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>();
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_ptr);
    TbeJsonFileParse json_file_parse(*node);
    string json_file_path;
    string bin_file_path;
    BuildTeBin13(json_file_path, bin_file_path);
    Status ret = json_file_parse.PackageTvmJsonInfo(json_file_path, bin_file_path);
    ret = json_file_parse.ParseTvmCoreType();
    EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(UTEST_FE_TBE_JSON_PARSER, case_parse_tvm_parameters_no_array)
{
    OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>();
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_ptr);
    TbeJsonFileParse json_file_parse(*node);
    string json_file_path;
    string bin_file_path;
    BuildTeBin17(json_file_path, bin_file_path);
    Status ret = json_file_parse.PackageTvmJsonInfo(json_file_path, bin_file_path);
    ret = json_file_parse.ParseTvmParameters();
    EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(UTEST_FE_TBE_JSON_PARSER, case_parse_weight_repeat)
{
    OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>();
    op_desc_ptr->SetName("node_name");
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_ptr);
    TbeJsonFileParse json_file_parse(*node);
    string json_file_path;
    string bin_file_path;
    BuildTeBin17(json_file_path, bin_file_path);
    Status ret = json_file_parse.PackageTvmJsonInfo(json_file_path, bin_file_path);
    ret = json_file_parse.ParseWeightRepeat();
    EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(UTEST_FE_TBE_JSON_PARSER, case_json_bin_error2)
{
    string file_name = "./air/test/engines/nneng/stub/emptyfile";
    vector<char> buffer;

    TbeJsonFileParseImpl tbe_json_file_parse_impl;
    Status ret = tbe_json_file_parse_impl.ReadBytesFromBinaryFile(file_name.c_str(), buffer);

    //3. result expected
    EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(UTEST_FE_TBE_JSON_PARSER, case_json_manual_thread_mix_aic_aiv)
{
    OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>("Sigmoid", "sigmoid");
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_ptr);
    TbeJsonFileParse json_file_parse(*node);
    string json_file_path = "./air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/te_sigmoid_9a43f1.json";
    string bin_file_path;
    Status ret = json_file_parse.PackageTvmJsonInfo(json_file_path, bin_file_path);
    EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(UTEST_FE_TBE_JSON_PARSER, case_json_manual_thread_mix_aic_aiv_fail)
{
    OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>("Sigmoid", "sigmoid");
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_ptr);
    TbeJsonFileParse json_file_parse(*node);
    string json_file_path = "./air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/te_sigmoid_9a43f144.json";
    string bin_file_path;
    Status ret = json_file_parse.PackageTvmJsonInfo(json_file_path, bin_file_path);
    EXPECT_EQ(ret, fe::FAILED);
}
TEST_F(UTEST_FE_TBE_JSON_PARSER, parse_params)
{
    ge::OpDescPtr op_desc_ = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
    vector<int64_t> dims = {1, 2, 3, 4};
    GeShape shape(dims);
    GeTensorDesc in_desc1(shape);
    in_desc1.SetFormat(FORMAT_NCHW);
    in_desc1.SetDataType(DT_FLOAT16);
    op_desc_->AddInputDesc("x", in_desc1);

    GeTensorDesc out_desc1(shape);
    out_desc1.SetFormat(FORMAT_HWCN);
    out_desc1.SetDataType(DT_FLOAT16);
    op_desc_->AddOutputDesc("y", out_desc1);
    op_desc_->SetWorkspaceBytes({32});
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr node = graph->AddNode(op_desc_);
    TbeJsonFileParse json_file_parse(*node);
    std::vector<int64_t> parameters_index = {0,1,1};
    Status ret = json_file_parse.SetAtomicInfo(parameters_index);
    EXPECT_EQ(ret, fe::SUCCESS);
}
TEST_F(UTEST_FE_TBE_JSON_PARSER, case_json_kbhit_format_error_failed)
{
  OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc_ptr);
  TbeJsonFileParse json_file_parse(*node);
  string json_file_path;
  string bin_file_path;
  BuildTeBin19(json_file_path, bin_file_path);
  Status ret = json_file_parse.PackageTvmJsonInfo(json_file_path, bin_file_path);
  EXPECT_EQ(ret, fe::FAILED);
}
TEST_F(UTEST_FE_TBE_JSON_PARSER, case_json_kbhit_format_error_failed1)
{
  OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc_ptr);
  TbeJsonFileParse json_file_parse(*node);
  Status ret = json_file_parse.ParseOpKBHitrate();
  EXPECT_EQ(ret, fe::FAILED);
}