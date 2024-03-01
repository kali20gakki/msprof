/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pmu_processor_utest.cpp
 * Description        : PmuProcessor UT
 * Author             : msprof team
 * Creation Date      : 2024/02/29
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/pmu_processor.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Viewer::Database;
using namespace Parser::Environment;
using namespace Analysis::Utils;

// Original Sample Timeline Format:aicore/ai_vector_core中的AICoreOriginalData
// timestamp, task_cyc, coreid
using OSTFormat = std::vector<std::tuple<uint64_t, std::string, uint32_t>>;
// Original Sample Summary Format:aicore/ai_vector_core中的MetricSummary
// metric, value, coreid
using OSSFormat = std::vector<std::tuple<std::string, double, uint32_t>>;

// Processed Sample Timeline Format
// deviceId, timestamp, totalCycle, usage, freq, coreId
using PSTFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, double, double, uint16_t>>;
// Processed Sample Summary Format
// deviceId, metric, value, coreId
using PSSFormat = std::vector<std::tuple<uint16_t, uint64_t, double, uint16_t>>;

const std::string PMU_DIR = "./pmu";
const std::string REPORT = "report.db";
const std::string DB_PATH = File::PathJoin({PMU_DIR, REPORT});
const std::string PROF = File::PathJoin({PMU_DIR, "PROF"});
const std::string TASK_BASED = "task-based";
const std::string SAMPLE_BASED = "sample-based";

const std::vector<std::tuple<uint64_t, uint32_t, uint64_t, uint32_t, std::string, std::string, std::string,
std::string, std::string, std::string, std::string, std::string, std::string>> SAMPLE_TIMELINE = {
    {1, 0, 88885812174450, 0, "0", "0", "0", "0", "0", "0", "0", "0", "0"},
    {1, 0, 88885812182000, 1, "0", "0", "0", "0", "0", "0", "0", "0", "0"},
    {1, 0, 88885812187440, 2, "0", "0", "0", "0", "0", "0", "0", "0", "0"},
    {1, 0, 88885822175760, 0, "303770", "42452", "116329", "27651", "86395", "162419", "35857", "13793", "347"},
    {1, 0, 88885822182560, 1, "306253", "42140", "115151", "29956", "86321", "155588", "35627", "14004", "304"},
    {1, 0, 88885822187760, 2, "249423", "36166", "74243", "24100", "61766", "116532", "31727", "10212", "251"},
};

const OSSFormat SAMPLE_SUMMARY = {
    {"total_time(ms)", 734.951, 0},
    {"vec_ratio", 0.12857727930161, 0},
    {"mac_ratio", 0.36937700608612, 0},
    {"total_time(ms)", 735.67, 1},
    {"vec_ratio", 0.12561746435222, 1},
    {"mac_ratio", 0.36563948509522, 1},
};

class PmuProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(PMU_DIR));
        EXPECT_TRUE(File::CreateDir(PROF));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF, DEVICE_PREFIX + "0"})));
        EXPECT_TRUE(CreatePmuDB(File::PathJoin({PROF, DEVICE_PREFIX + "0", SQLITE})));
    }

    static bool CreatePmuDB(const std::string& sqlitePath)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));
        std::shared_ptr<AicoreDB> aicDB;
        MAKE_SHARED0_RETURN_VALUE(aicDB, AicoreDB, false);
        std::shared_ptr<DBRunner> aicRunner;
        MAKE_SHARED_RETURN_VALUE(aicRunner, DBRunner, false, File::PathJoin({sqlitePath, "aicore_0.db"}));
        EXPECT_TRUE(aicRunner->CreateTable("AICoreOriginalData", aicDB->GetTableCols("AICoreOriginalData")));
        EXPECT_TRUE(aicRunner->InsertData("AICoreOriginalData", SAMPLE_TIMELINE));
        EXPECT_TRUE(aicRunner->CreateTable("MetricSummary", aicDB->GetTableCols("MetricSummary")));
        EXPECT_TRUE(aicRunner->InsertData("MetricSummary", SAMPLE_SUMMARY));

        std::shared_ptr<AiVectorCoreDB> aivDB;
        MAKE_SHARED0_RETURN_VALUE(aivDB, AiVectorCoreDB, false);
        std::shared_ptr<DBRunner> aivRunner;
        MAKE_SHARED_RETURN_VALUE(aivRunner, DBRunner, false, File::PathJoin({sqlitePath, "ai_vector_core_0.db"}));
        EXPECT_TRUE(aivRunner->CreateTable("AICoreOriginalData", aivDB->GetTableCols("AICoreOriginalData")));
        EXPECT_TRUE(aivRunner->InsertData("AICoreOriginalData", SAMPLE_TIMELINE));
        EXPECT_TRUE(aivRunner->CreateTable("MetricSummary", aivDB->GetTableCols("MetricSummary")));
        EXPECT_TRUE(aivRunner->InsertData("MetricSummary", SAMPLE_SUMMARY));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(PMU_DIR, 0));
    }

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        if (File::Exist(DB_PATH)) {
            EXPECT_TRUE(File::DeleteFile(DB_PATH));
        }
    }
};

TEST_F(PmuProcessorUTest, TestSampleRunShouldReturnTrueWhenRunSuccess)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", "sample-based"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    auto processor = PmuProcessor(DB_PATH, {PROF});
    EXPECT_TRUE(processor.Run());
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(PmuProcessorUTest, TestSampleBasedProcessShouldReturnFalseWhenTimelineOrSummaryFailed)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", "sample-based"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));

    // timeline GetProfTimeRecordInfo false + summary FormatSampleBasedSummaryData false
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    MOCKER_CPP(&PmuProcessor::FormatSampleBasedSummaryData).stubs().will(returnValue(false));
    auto processor1 = PmuProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor1.SampleBasedProcess(PROF));
    MOCKER_CPP(&PmuProcessor::FormatSampleBasedSummaryData).reset();
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();

    // timeline GetPmuFreq false
    MOCKER_CPP(&Context::GetPmuFreq).stubs().will(returnValue(false));
    auto processor2 = PmuProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor2.SampleBasedProcess(PROF));
    MOCKER_CPP(&Context::GetPmuFreq).reset();

    // timeline FormatSampleBasedTimelineData false
    MOCKER_CPP(&PmuProcessor::FormatSampleBasedTimelineData).stubs().will(returnValue(false));
    auto processor3 = PmuProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor3.SampleBasedProcess(PROF));
    MOCKER_CPP(&PmuProcessor::FormatSampleBasedTimelineData).reset();

    // SaveData false
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    auto processor4 = PmuProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor4.SampleBasedProcess(PROF));
    MOCKER_CPP(&DBRunner::CreateTable).reset();

    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(PmuProcessorUTest, TestSampleFormatShouldReturnFalseWhenFormatDataFailed)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", "sample-based"},
    };

    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    // dataFormat empty
    MOCKER_CPP(&OSSFormat::empty).stubs().will(returnValue(true));
    MOCKER_CPP(&OSTFormat::empty).stubs().will(returnValue(true));
    auto processor1 = PmuProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor1.Run());
    MOCKER_CPP(&OSTFormat::empty).reset();
    MOCKER_CPP(&OSSFormat::empty).reset();

    // Reserve failed
    MOCKER_CPP(&PSTFormat::reserve).stubs().will(throws(std::bad_alloc()));
    MOCKER_CPP(&PSSFormat::reserve).stubs().will(throws(std::bad_alloc()));
    auto processor2 = PmuProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor2.Run());
    MOCKER_CPP(&PSSFormat::reserve).reset();
    MOCKER_CPP(&PSTFormat::reserve).reset();

    // ProcessedDataFormat empty
    MOCKER_CPP(&PSTFormat::empty).stubs().will(returnValue(true));
    MOCKER_CPP(&PSSFormat::empty).stubs().will(returnValue(true));
    auto processor3 = PmuProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor3.Run());
    MOCKER_CPP(&PSSFormat::empty).reset();
    MOCKER_CPP(&PSTFormat::empty).reset();

    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

