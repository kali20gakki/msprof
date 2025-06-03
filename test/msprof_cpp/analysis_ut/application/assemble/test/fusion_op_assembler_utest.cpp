/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : fusion_op_assembler_utest.cpp
 * Description        : fusion_op_assembler UT
 * Author             : msprof team
 * Creation Date      : 2025/5/12
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/summary/fusion_op_assembler.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/application/summary/summary_constant.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain::Environment;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./fusion_op_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class FusionOpAssemblerUTest : public testing::Test {
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        GlobalMockObject::verify();
    }
    virtual void SetUp()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(RESULT_PATH));
    }
};

static std::vector<FusionOpInfo> GeneratefusionOpData()
{
    std::vector<FusionOpInfo> res;
    FusionOpInfo data;
    data.modelId = 1;
    data.fusionName = "FusedBatchNormV3_BNInferenceD";
    data.opNames = "FusedBatchNormV3";
    data.memoryInput = "10";
    data.memoryOutput = "20";
    data.memoryWeight = "1";
    data.memoryWorkspace = "0";
    data.memoryTotal = "50";
    res.push_back(data);
    return res;
}

static std::vector<ModelName> GenerateModelNameData()
{
    std::vector<ModelName> res;
    ModelName data;
    data.modelId = 1;
    data.modelName = "ge_default_20240924192820_1";
    res.push_back(data);
    return res;
}

static std::unordered_map<std::string, std::string> GenerateHashData()
{
    std::unordered_map<std::string, std::string> res;
    res.insert({"1", "ge_default_20240924192820_1"});
    return res;
}

TEST_F(FusionOpAssemblerUTest, ShouldReturnTrueWhenDataNotExist)
{
    FusionOpAssembler assembler(PROCESSOR_NAME_FUSION_OP, PROF_PATH);
    DataInventory dataInventory;
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"fusion_op"}, {});
    EXPECT_EQ(0ul, files.size());
}

TEST_F(FusionOpAssemblerUTest, ShouldReturnTrueWhenFusionOpExist)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<FusionOpInfo>> taskS;
    std::shared_ptr<std::vector<ModelName>> modelNameTaskS;
    std::shared_ptr<std::unordered_map<std::string, std::string>> hashTaskS;

    auto task = GeneratefusionOpData();
    auto modelNameTask = GenerateModelNameData();
    auto hashTask = GenerateHashData();

    MAKE_SHARED_NO_OPERATION(taskS, std::vector<FusionOpInfo>, task);
    MAKE_SHARED_NO_OPERATION(modelNameTaskS, std::vector<ModelName>, modelNameTask);
    hashTaskS = std::make_shared<std::unordered_map<std::string, std::string>>(hashTask);

    dataInventory.Inject(taskS);
    dataInventory.Inject(modelNameTaskS);
    dataInventory.Inject(hashTaskS);

    FusionOpAssembler assembler(PROCESSOR_NAME_FUSION_OP, PROF_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"fusion_op"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(2ul, res.size());
    std::string expectHeader{"Device_id,Model Name,Model ID,Fusion Op,Original Ops,Memory Input(KB),Memory Output(KB),"
            "Memory Weight(KB),Memory Workspace(KB),Memory Total(KB)"};
    EXPECT_EQ(expectHeader, res[0]);
    std::string expectRowOne{"host,ge_default_20240924192820_1,1,FusedBatchNormV3_BNInferenceD," \
                            "FusedBatchNormV3,10,20,1,0,50"};
    EXPECT_EQ(expectRowOne, res[1]);
}