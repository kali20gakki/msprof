/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : api_statistic_assembler_utest.cpp
 * Description        : api_statistic_assembler UT
 * Author             : msprof team
 * Creation Date      : 2025/5/12
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/summary/api_statistic_assembler.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"
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
const int LEVEL_NUM = 5000;
const int END_NUM = 5;
const int TIME_NUM = 4;
const std::string BASE_PATH = "./api_statistic_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class ApiStatisticAssemblerUTest : public testing::Test {
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

static std::vector<ApiData> GenerateApiData()
{
    std::vector<ApiData> res;
    ApiData data;
    data.level = LEVEL_NUM;
    data.end = END_NUM;
    data.apiName = "ContextCreate";
    data.timestamp = TIME_NUM;
    res.push_back(data);
    return res;
}

TEST_F(ApiStatisticAssemblerUTest, ShouldReturnTrueWhenDataNotExist)
{
    ApiStatisticAssembler assembler(PROCESSOR_NAME_API, PROF_PATH);
    DataInventory dataInventory;
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"api_statistic"}, {});
    EXPECT_EQ(0ul, files.size());
}

TEST_F(ApiStatisticAssemblerUTest, ShouldReturnTrueWhenApiStatisticExist)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<ApiData>> apiDataS;
    auto apiData = GenerateApiData();
    MAKE_SHARED_NO_OPERATION(apiDataS, std::vector<ApiData>, apiData);

    dataInventory.Inject(apiDataS);

    ApiStatisticAssembler assembler(PROCESSOR_NAME_API, PROF_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"api_statistic"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(2ul, res.size());
    std::string expectHeader{"Device_id,Level,API Name,Time(us),Count,Avg(us),Min(us),Max(us),Variance"};
    EXPECT_EQ(expectHeader, res[0]);
    std::string expectRowOne{"host,runtime,ContextCreate,0.001,1,0.001,0.001,0.001,0"};
    EXPECT_EQ(expectRowOne, res[1]);
}

TEST_F(ApiStatisticAssemblerUTest, ShouldReturnTrueWhenApiStatisticExistWithTwoLevel)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<ApiData>> apiDataS;
    auto apiData = GenerateApiData();
    const int timeNum = 1;
    const int levelNum = 10000;
    const int secondLine = 2;
    ApiData data;
    data.level = levelNum;
    data.end = END_NUM;
    data.apiName = "launch";
    data.timestamp = timeNum;
    apiData.emplace_back(data);
    MAKE_SHARED_NO_OPERATION(apiDataS, std::vector<ApiData>, apiData);

    dataInventory.Inject(apiDataS);

    ApiStatisticAssembler assembler(PROCESSOR_NAME_API, PROF_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"api_statistic"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(3ul, res.size());
    std::string expectHeader{"Device_id,Level,API Name,Time(us),Count,Avg(us),Min(us),Max(us),Variance"};
    EXPECT_EQ(expectHeader, res[0]);
    std::string expectRowOne{"host,runtime,ContextCreate,0.001,1,0.001,0.001,0.001,0"};
    EXPECT_EQ(expectRowOne, res[1]);
    std::string expectRowTwo{"host,node,launch,0.004,1,0.004,0.004,0.004,0"};
    EXPECT_EQ(expectRowTwo, res[secondLine]);
}

TEST_F(ApiStatisticAssemblerUTest, ShouldReturnTrueWhenApiStatisticExistWithSingleLevelAndOneLevelName)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<ApiData>> apiDataS;
    auto apiData = GenerateApiData();
    const int timeNum = 2;
    ApiData data;
    data.level = LEVEL_NUM;
    data.end = END_NUM;
    data.apiName = "ContextCreate";
    data.timestamp = timeNum;
    apiData.emplace_back(data);
    MAKE_SHARED_NO_OPERATION(apiDataS, std::vector<ApiData>, apiData);

    dataInventory.Inject(apiDataS);

    ApiStatisticAssembler assembler(PROCESSOR_NAME_API, PROF_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {"api_statistic"}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(2ul, res.size());
    std::string expectHeader{"Device_id,Level,API Name,Time(us),Count,Avg(us),Min(us),Max(us),Variance"};
    EXPECT_EQ(expectHeader, res[0]);
    std::string expectRowOne{"host,runtime,ContextCreate,0.004,2,0.002,0.001,0.003,0"};
    EXPECT_EQ(expectRowOne, res[1]);
}