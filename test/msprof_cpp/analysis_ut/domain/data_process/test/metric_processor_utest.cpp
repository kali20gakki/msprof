// Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
/* ******************************************************************************
 * File Name          : metric_processor_utest.cpp
 * Description        : metric_processor UT
 * Author             : msprof team
 * Creation Date      : 2024/11/09
 * *****************************************************************************
 */

#include <algorithm>
#include <vector>
#include <set>

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/domain/data_process/ai_task/metric_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Viewer::Database;
using namespace Parser::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Domain;


const std::string METRIC_DIR = "./metric";
const std::string METRIC_SUMMARY_DB = "metric_summary.db";
const std::string PROF_PATH = File::PathJoin({METRIC_DIR, "./PROF_114514"});
const std::string DB_PATH = File::PathJoin({PROF_PATH, DEVICE_PREFIX + "0", SQLITE, METRIC_SUMMARY_DB});
const std::string TASK_BASED = "task-based";
const std::string SAMPLE_BASED = "sample-based";

using TempTaskFormat = std::vector<std::tuple<double, double, double, double, double, double, double, double, double,
    double, double, uint32_t, uint32_t, uint32_t, double, double, uint32_t>>;

const TempTaskFormat TASK_DATA = {
    {0.2, 0.1, 0.1, 318.36, 14135332, 197.9849315375118, 0.62189009780598, 75.1, 6668981, 0.87503179271316,
        0.01165155516263, 22, 68, 2344, 21203781495130.004, 21203812838010.004, 0},
    {0.3, 0.2, 0.2, 327.19, 14527061, 197.98020025179216, 0.60509245469541, 76.41, 6785309, 0.87503201991243,
        0.01145179976328, 22, 68, 2356, 21203781495130.004, 21203813146290.004, 0},
    {0.5, 0.1, 0.2, 313.94, 13939056, 197.98647121440646, 0.63065066960058, 61.91, 5497563, 0.8750522076782,
        0.01413426276334, 22, 68, 2357, 21203781495130.004, 21203813447710.004, 0},
    {0.8, 0.2, 0.4, 315.17, 13993558, 197.98357473417414, 0.62818026694855, 61.88, 5495101, 0.87502004421757,
        0.01414059541399, 22, 68, 2358, 21203781495130.004, 21203813750090.004, 0},
};

const TableColumns TASK_METRIC_SUMMARY = {
    {"mac_ratio", SQL_NUMERIC_TYPE},
    {"vec_ratio", SQL_NUMERIC_TYPE},
    {"mte2_ratio", SQL_NUMERIC_TYPE},
    {"aic_total_time", SQL_NUMERIC_TYPE},
    {"aic_total_cycles", SQL_NUMERIC_TYPE},
    {"aic_mac_time", SQL_NUMERIC_TYPE},
    {"aic_mac_ratio_extra", SQL_NUMERIC_TYPE},
    {"aiv_total_time", SQL_NUMERIC_TYPE},
    {"aiv_total_cycles", SQL_NUMERIC_TYPE},
    {"aiv_vec_time", SQL_NUMERIC_TYPE},
    {"aiv_vec_ratio", SQL_NUMERIC_TYPE},
    {"task_id", SQL_INTEGER_TYPE},
    {"stream_id", SQL_INTEGER_TYPE},
    {"subtask_id", SQL_INTEGER_TYPE},
    {"start_time", SQL_NUMERIC_TYPE},
    {"end_time", SQL_NUMERIC_TYPE},
    {"batch_id", SQL_INTEGER_TYPE},
};

class MetricProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(METRIC_DIR));
        EXPECT_TRUE(File::CreateDir(PROF_PATH));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH, DEVICE_PREFIX + "0"})));
        EXPECT_TRUE(CreateMetricSummaryDB(File::PathJoin({PROF_PATH, DEVICE_PREFIX + "0", SQLITE})));
    }

    static bool CreateMetricSummaryDB(const std::string& sqlitePath)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));
        std::shared_ptr<DBRunner> metricRunner;
        MAKE_SHARED_RETURN_VALUE(metricRunner, DBRunner, false, File::PathJoin({sqlitePath, "metric_summary.db"}));
        EXPECT_TRUE(metricRunner->CreateTable("MetricSummary", TASK_METRIC_SUMMARY));
        EXPECT_TRUE(metricRunner->InsertData("MetricSummary", TASK_DATA));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(METRIC_DIR, 0));
    }

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
};

TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnTrueWhenRunSuccess)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", TASK_BASED},
        {"platform_version", "5"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    DataInventory dataInventory;
    auto processor = MetricProcessor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnTrueWhenSampleBased)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", SAMPLE_BASED},
        {"platform_version", "5"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    DataInventory dataInventory;
    auto processor = MetricProcessor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnFALSEWhenGetMetricModeFailed)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", TASK_BASED},
        {"platform_version", "5"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    MOCKER_CPP(&Context::GetMetricMode).stubs().will(returnValue(false));
    DataInventory dataInventory;
    auto processor = MetricProcessor(PROF_PATH);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));

    MOCKER_CPP(&Context::GetMetricMode).reset();
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnFalseWhenCheckColumnsFailed)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", TASK_BASED},
        {"platform_version", "5"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));

    // GetAndCheckTableColumns false
    std::vector<std::string> emptyHeaders;
    MOCKER_CPP(&MetricProcessor::GetAndCheckTableColumns).stubs().will(returnValue(emptyHeaders));
    auto processor1 = MetricProcessor(PROF_PATH);
    DataInventory dataInventory;
    EXPECT_FALSE(processor1.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));

    MOCKER_CPP(&MetricProcessor::GetAndCheckTableColumns).reset();
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnFalseWhenUpdateColumnsFailed)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", TASK_BASED},
        {"platform_version", "5"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));

    // ModifySummaryHeaders failed
    std::vector<std::string> emptyHeaders;
    MOCKER_CPP(&MetricProcessor::ModifySummaryHeaders).stubs().will(returnValue(emptyHeaders));
    auto processor1 = MetricProcessor(PROF_PATH);
    DataInventory dataInventory;
    EXPECT_FALSE(processor1.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));

    MOCKER_CPP(&MetricProcessor::ModifySummaryHeaders).reset();
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnFalseWhenDeviceIDInvalid)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", TASK_BASED},
        {"platform_version", "5"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));

    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(UINT16_MAX));
    auto processor1 = MetricProcessor(PROF_PATH);
    DataInventory dataInventory;
    EXPECT_FALSE(processor1.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));

    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).reset();
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}
TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnFalseWhenCheckPathAndTableNotExist)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", TASK_BASED},
        {"platform_version", "5"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));

    DataInventory dataInventory;
    MOCKER_CPP(&MetricProcessor::CheckPathAndTable).stubs().will(returnValue(1));

    auto processor1 = MetricProcessor(PROF_PATH);
    EXPECT_TRUE(processor1.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));

    MOCKER_CPP(&MetricProcessor::CheckPathAndTable).reset();
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnFalseWhenCheckPathAndTableFailed)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", TASK_BASED},
        {"platform_version", "5"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));

    DataInventory dataInventory;
    const uint8_t CHECK_FAILED = 2;
    MOCKER_CPP(&MetricProcessor::CheckPathAndTable).stubs().will(returnValue(CHECK_FAILED));

    auto processor1 = MetricProcessor(PROF_PATH);
    EXPECT_FALSE(processor1.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));

    MOCKER_CPP(&MetricProcessor::CheckPathAndTable).reset();
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(MetricProcessorUTest, TestTaskRunShouldReturnTrueWhenNoMetricSummaryDB)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
        {"DeviceInfo", {{{"aic_frequency", "1000"}}}},
        {"ai_core_profiling_mode", TASK_BASED},
        {"platform_version", "5"},
    };
    EXPECT_TRUE(File::RemoveDir(File::PathJoin({PROF_PATH, DEVICE_PREFIX + "0"}), 0));

    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    DataInventory dataInventory;
    auto processor = MetricProcessor(PROF_PATH);
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_METRIC_SUMMARY));
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}
