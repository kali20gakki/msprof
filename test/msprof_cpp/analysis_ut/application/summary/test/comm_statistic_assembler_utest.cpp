/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : comm_statistic_assembler_utest.cpp
 * Description        : comm_statistic_assembler UT
 * Author             : msprof team
 * Creation Date      : 2025/5/24
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/summary/comm_statistic_assembler.h"
#include "analysis/csrc/application/summary/summary_constant.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/hccl_statistic_data.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"

using namespace Analysis::Application;
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Viewer::Database;

namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./communication_statistic_test";
const std::string PROF_PATH = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string RESULT_PATH = File::PathJoin({PROF_PATH, OUTPUT_PATH});
}

class CommStatisticAssemblerUTest : public testing::Test {
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

static std::vector<HcclStatisticData> GenerateCommStatisticData()
{
    std::vector<HcclStatisticData> res;
    HcclStatisticData data;
    data.deviceId = 0; // deviceId 0
    data.opType = "hcom_send_"; // opType hcom_send_
    data.count = "384"; // count 384
    data.totalTime = 5412825240; // totalTime 5412825240
    data.min = 219280; // min 219280
    data.avg = 14095899.0625; // avg 14095899.0625
    data.max = 42146680; // max 42146680
    data.ratio = 45.69861; // ratio 45.69861
    res.push_back(data);
    data.opType = "hcom_allGather_"; // opType hcom_allGather_
    data.count = "3202"; // count 3202
    data.totalTime = 771407100; // totalTime 771407100
    data.min = 35960; // min 35960
    data.avg = 240914.1474; // avg 240914.1474
    data.max = 706500; // max 706500
    data.ratio = 6.5127; // ratio 6.5127
    res.push_back(data);
    return res;
}

TEST_F(CommStatisticAssemblerUTest, ShouldReturnTrueWhenDataNotExist)
{
    CommStatisticAssembler assembler(PROCESSOR_NAME_COMM_STATISTIC, PROF_PATH);
    DataInventory dataInventory;
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {COMM_STATISTIC_NAME}, {});
    EXPECT_EQ(0ul, files.size());
}

TEST_F(CommStatisticAssemblerUTest, ShouldReturnTrueWhenCommStatisticExist)
{
    DataInventory dataInventory;
    std::shared_ptr<std::vector<HcclStatisticData>> commStatisticDataS;
    auto commStatisticData = GenerateCommStatisticData();
    MAKE_SHARED_NO_OPERATION(commStatisticDataS, std::vector<HcclStatisticData>, commStatisticData);

    dataInventory.Inject(commStatisticDataS);

    CommStatisticAssembler assembler(PROCESSOR_NAME_COMM_STATISTIC, PROF_PATH);
    EXPECT_TRUE(assembler.Run(dataInventory));
    auto files = File::GetOriginData(RESULT_PATH, {COMM_STATISTIC_NAME}, {});
    EXPECT_EQ(1ul, files.size());
    FileReader reader(files.back());
    std::vector<std::string> res;
    EXPECT_EQ(Analysis::ANALYSIS_OK, reader.ReadText(res));
    EXPECT_EQ(3ul, res.size());
    std::string expectHeader{"Device_id,OP Type,Count,Total Time(us),Min Time(us),Avg Time(us),Max Time(us),Ratio(%)"};
    EXPECT_EQ(expectHeader, res[0]); // 0 the header result
    std::string expectRowOne{"0,hcom_send_,384,5412825240,219280,14095899.062,42146680,45.699"};
    EXPECT_EQ(expectRowOne, res[1]); // 1 the first result
    std::string expectRowTwo{"0,hcom_allGather_,3202,771407100,35960,240914.147,706500,6.513"};
    EXPECT_EQ(expectRowTwo, res[2]); // 2 the second
}

TEST_F(CommStatisticAssemblerUTest, ShouldReturnFalseWhenDataExistAndResEmpty)
{
    using ResDataFormat = std::vector<std::vector<std::string>>;
    CommStatisticAssembler assembler(PROCESSOR_NAME_COMM_STATISTIC, PROF_PATH);
    DataInventory dataInventory;
    std::shared_ptr<std::vector<HcclStatisticData>> commStatisticDataS;
    auto commStatisticData = GenerateCommStatisticData();
    MAKE_SHARED_NO_OPERATION(commStatisticDataS, std::vector<HcclStatisticData>, commStatisticData);
    dataInventory.Inject(commStatisticDataS);

    MOCKER_CPP(&ResDataFormat::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(assembler.Run(dataInventory));
    MOCKER_CPP(&ResDataFormat::empty).reset();
}