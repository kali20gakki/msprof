/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : stars_soc_parser_utest.cpp
 * Description        : stars_soc_parser ut用例
 * Author             : msprof team
 * Creation Date      : 2024/5/6
 * *****************************************************************************
 */

#include <gtest/gtest.h>
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/log/include/stars_soc_parser.h"
#include "analysis/csrc/domain/services/parser/parser_item/acsq_log_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item/ffts_plus_log_parser_item.h"
#include "analysis/csrc/domain/services/parser/parser_item/acc_pmu_parser_item.h"
#include "test/msprof_cpp/analysis_ut/domain/services/test/fake_generator.h"


namespace Analysis {
using namespace Analysis;
using namespace Analysis::Infra;
using namespace Analysis::Domain;
using namespace Analysis::Utils;

namespace {
const std::string SOC_LOG_PATH = "./soc_log";
const int ACC_PMU_SIZE = 2;
}

class StarsSocParserUtest : public testing::Test {
protected:
    void SetUp() override
    {
        GlobalMockObject::verify();
        EXPECT_TRUE(File::CreateDir(SOC_LOG_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({SOC_LOG_PATH, "data"})));
    }
    void TearDown() override
    {
        dataInventory_.RemoveRestData({});
        EXPECT_TRUE(File::RemoveDir(SOC_LOG_PATH, 0));
    }
    AcsqLog CreateAcsqLog(int funcType, int cnt, int taskId, int streamId, int timestamp)
    {
        AcsqLog log{};
        log.funcType = funcType;
        log.cnt = cnt;
        log.taskId = taskId;
        log.streamId = streamId;
        log.timestamp = timestamp;
        return log;
    }

    FftsPlusLog CreateFftsPlusLog(int funcType, int cnt, int taskId, int streamId, int timestamp)
    {
        FftsPlusLog log{};
        log.funcType = funcType;
        log.cnt = cnt;
        log.taskId = taskId;
        log.streamId = streamId;
        log.timestamp = timestamp;
        return log;
    }

    AccPmu CreateAccPmu(int funcType, int cnt, int accId, int timestamp, int ost)
    {
        AccPmu accPmu{};
        accPmu.cnt = cnt;
        accPmu.accId = accId;
        accPmu.funcType = funcType;
        accPmu.timestamp = timestamp;
        for (int i = 0; i < ACC_PMU_SIZE; ++i) {
            accPmu.bandwidth[i] = i * ost;
            accPmu.ost[i] = i * ost;
        }
        return accPmu;
    }

protected:
    DataInventory dataInventory_;
};


TEST_F(StarsSocParserUtest, ShouldReturnAcsqLogDataWhenParserRun)
{
    std::vector<int> expectTimestamp{100, 120, 100};
    StarsSocParser starsSocParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = SOC_LOG_PATH;
    std::vector<AcsqLog> log{CreateAcsqLog(0b000000, 1, 1, 1, 100), CreateAcsqLog(0b000001, 2, 1, 1, 120)};
    EXPECT_TRUE(WriteBin(log, File::PathJoin({SOC_LOG_PATH, "data"}), "stars_soc.data.0.slice_0"));
    std::vector<AccPmu> accPmu{CreateAccPmu(0b011010, 3, 10, 100, 200)};
    EXPECT_TRUE(WriteBin(accPmu, File::PathJoin({SOC_LOG_PATH, "data"}), "stars_soc.data.0.slice_0"));
    ASSERT_EQ(starsSocParser.Run(dataInventory_, context), Analysis::ANALYSIS_OK);
    auto logData = dataInventory_.GetPtr<std::vector<HalLogData>>();
    ASSERT_EQ(logData->size(), 3ul);
    for (int i = 0; i < logData->size(); i++) {
        ASSERT_EQ(expectTimestamp[i], logData->data()[i].hd.timestamp);
    }
}

TEST_F(StarsSocParserUtest, ShouldReturnFftsPlusLogDataWhenParserRun)
{
    std::vector<int> expectTimestamp{200, 220};
    StarsSocParser starsSocParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = SOC_LOG_PATH;
    std::vector<FftsPlusLog> log{CreateFftsPlusLog(0b100010, 1, 1, 1, 200), CreateFftsPlusLog(0b100011, 2, 1, 1, 220)};
    EXPECT_TRUE(WriteBin(log, File::PathJoin({SOC_LOG_PATH, "data"}), "stars_soc.data.0.slice_0"));
    ASSERT_EQ(starsSocParser.Run(dataInventory_, context), Analysis::ANALYSIS_OK);
    auto logData = dataInventory_.GetPtr<std::vector<HalLogData>>();
    ASSERT_EQ(logData->size(), 2ul);
    for (int i = 0; i < logData->size(); i++) {
        ASSERT_EQ(expectTimestamp[i], logData->data()[i].hd.timestamp);
    }
}

TEST_F(StarsSocParserUtest, ShouldReturnAcsqLogDataAndFftsPlusLogDataWhenParserRun)
{
    std::vector<int> expectTimestamp{100, 120, 200, 220};
    StarsSocParser starsSocParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = SOC_LOG_PATH;
    std::vector<AcsqLog> acsqLog{CreateAcsqLog(0b000000, 1, 1, 1, 100), CreateAcsqLog(0b000001, 2, 1, 1, 120)};
    EXPECT_TRUE(WriteBin(acsqLog, File::PathJoin({SOC_LOG_PATH, "data"}), "stars_soc.data.0.slice_0"));
    std::vector<FftsPlusLog> fftsLog{CreateFftsPlusLog(0b100010, 3, 1, 1, 200),
                                     CreateFftsPlusLog(0b100011, 4, 1, 1, 220)};
    EXPECT_TRUE(WriteBin(fftsLog, File::PathJoin({SOC_LOG_PATH, "data"}), "stars_soc.data.0.slice_0"));
    ASSERT_EQ(starsSocParser.Run(dataInventory_, context), Analysis::ANALYSIS_OK);
    auto logData = dataInventory_.GetPtr<std::vector<HalLogData>>();
    ASSERT_EQ(logData->size(), 4ul);
    for (int i = 0; i < logData->size(); i++) {
        ASSERT_EQ(expectTimestamp[i], logData->data()[i].hd.timestamp);
    }
}

TEST_F(StarsSocParserUtest, ShouldReturnAcsqLogDataAndFftsPlusLogDataWhenMultiDataAndParserRun)
{
    std::vector<int> expectTimestamp{100, 120, 200, 220, 100,
                                     120, 200, 220};
    StarsSocParser starsSocParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = SOC_LOG_PATH;
    std::vector<AcsqLog> acsqLog{CreateAcsqLog(0b000000, 1, 1, 1, 100), CreateAcsqLog(0b000001, 4, 1, 1, 120)};
    std::vector<AcsqLog> acsqLog2{CreateAcsqLog(0b000000, 5, 1, 1, 100), CreateAcsqLog(0b000001, 6, 1, 1, 120)};
    EXPECT_TRUE(WriteBin(acsqLog, File::PathJoin({SOC_LOG_PATH, "data"}), "stars_soc.data.0.slice_0"));
    EXPECT_TRUE(WriteBin(acsqLog2, File::PathJoin({SOC_LOG_PATH, "data"}), "stars_soc.data.0.slice_1"));
    std::vector<FftsPlusLog> fftsLog{CreateFftsPlusLog(0b100010, 3, 1, 1, 200),
                                     CreateFftsPlusLog(0b100011, 4, 1, 1, 220)};
    std::vector<FftsPlusLog> fftsLog2{CreateFftsPlusLog(0b100010, 7, 1, 1, 200),
                                     CreateFftsPlusLog(0b100011, 8, 1, 1, 220)};
    EXPECT_TRUE(WriteBin(fftsLog, File::PathJoin({SOC_LOG_PATH, "data"}), "stars_soc.data.0.slice_0"));
    EXPECT_TRUE(WriteBin(fftsLog2, File::PathJoin({SOC_LOG_PATH, "data"}), "stars_soc.data.0.slice_1"));
    ASSERT_EQ(starsSocParser.Run(dataInventory_, context), Analysis::ANALYSIS_OK);
    auto logData = dataInventory_.GetPtr<std::vector<HalLogData>>();
    ASSERT_EQ(logData->size(), 8ul);
    for (int i = 0; i < logData->size(); i++) {
        ASSERT_EQ(expectTimestamp[i], logData->data()[i].hd.timestamp);
    }
}

TEST_F(StarsSocParserUtest, ShouldReturnNoDataWhenPathError)
{
    StarsSocParser starsSocParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = "";
    ASSERT_EQ(Analysis::ANALYSIS_OK, starsSocParser.Run(dataInventory_, context));
    auto logData = dataInventory_.GetPtr<std::vector<HalLogData>>();
    ASSERT_EQ(0ul, logData->size());
}

TEST_F(StarsSocParserUtest, ShouldParseErrorWhenNumOfLogLessThanTwo)
{
    StarsSocParser starsSocParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = SOC_LOG_PATH;
    std::vector<AcsqLog> acsqLog{CreateAcsqLog(0b000000, 1, 1, 1, 100)};
    EXPECT_TRUE(WriteBin(acsqLog, File::PathJoin({SOC_LOG_PATH, "data"}), "stars_soc.data.0.slice_0"));
    ASSERT_EQ(Analysis::PARSER_PARSE_DATA_ERROR, starsSocParser.Run(dataInventory_, context));
    auto logData = dataInventory_.GetPtr<std::vector<HalLogData>>();
    ASSERT_EQ(1ul, logData->size());
}

TEST_F(StarsSocParserUtest, ShouldParseErrorWhenResizeException)
{
    StarsSocParser starsSocParser;
    DeviceContext context;
    context.deviceContextInfo.deviceFilePath = SOC_LOG_PATH;
    std::vector<FftsPlusLog> log{CreateFftsPlusLog(0b100010, 1, 1, 1, 200), CreateFftsPlusLog(0b100011, 2, 1, 1, 220)};
    EXPECT_TRUE(WriteBin(log, File::PathJoin({SOC_LOG_PATH, "data"}), "stars_soc.data.0.slice_0"));
    MOCKER_CPP(&std::vector<HalLogData>::resize, void(std::vector<HalLogData>::*)(size_t)).stubs()
        .will(throws(std::bad_alloc()));
    ASSERT_EQ(Analysis::PARSER_PARSE_DATA_ERROR, starsSocParser.Run(dataInventory_, context));
}

}
