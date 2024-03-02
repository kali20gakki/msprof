/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccs_processor_utest.cpp
 * Description        : HCCSProcessor UT
 * Author             : msprof team
 * Creation Date      : 2024/03/02
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/hccs_processor.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Viewer::Database;
using namespace Parser::Environment;
using namespace Analysis::Utils;

// device_id, timestamp, txthroughput, rxthroughput
using HccsDataFormat = std::vector<std::tuple<uint32_t, double, double, double>>;
// deviceId, timestampNs, txThroughput, rxThroughput
using ProcessedDataFormat = std::vector<std::tuple<uint16_t, uint64_t, double, double>>;
// deviceId, timestampNs, txThroughput, rxThroughput
using QueryDataFormat = std::vector<std::tuple<uint32_t, uint64_t, double, double>>;

const std::string HCCS_DIR = "./hccs";
const std::string REPORT = "report.db";
const std::string DB_PATH = File::PathJoin({HCCS_DIR, REPORT});
const std::string PROF = File::PathJoin({HCCS_DIR, "PROF"});

const HccsDataFormat HCCS_DATA = {
    {1, 331124176903000, 280146.575, 0},
    {1, 331124196902780, 280146.903, 0},
    {1, 331124216902500, 280146.937, 0},
    {1, 331124236902500, 280146.26, 0},
};

class HCCSProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(HCCS_DIR));
        EXPECT_TRUE(File::CreateDir(PROF));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF, DEVICE_PREFIX + "0"})));
        EXPECT_TRUE(CreateHCCSDB(File::PathJoin({PROF, DEVICE_PREFIX + "0", SQLITE})));
    }

    static bool CreateHCCSDB(const std::string& sqlitePath)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));
        std::shared_ptr<HCCSDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, HCCSDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, File::PathJoin({sqlitePath, database->GetDBName()}));
        EXPECT_TRUE(dbRunner->CreateTable("HCCSEventsData", database->GetTableCols("HCCSEventsData")));
        EXPECT_TRUE(dbRunner->InsertData("HCCSEventsData", HCCS_DATA));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(HCCS_DIR, 0));
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

TEST_F(HCCSProcessorUTest, TestRunShouldReturnTrueWhenRunSuccess)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    auto processor = HCCSProcessor(DB_PATH, {PROF});
    EXPECT_TRUE(processor.Run());
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();

    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    QueryDataFormat checkData;
    uint16_t expectNum = HCCS_DATA.size();
    std::string sqlStr = "SELECT * FROM " + TABLE_NAME_HCCS;
    ASSERT_NE(dbRunner, nullptr);
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectNum, checkData.size());
}

TEST_F(HCCSProcessorUTest, TestProcessShouldReturnFalseWhenGetRecordOrCheckPathFailed)
{
    MOCKER_CPP(&TableProcessor::CheckPath).stubs().will(returnValue(CHECK_FAILED));
    auto processor1 = HCCSProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor1.Run());
    MOCKER_CPP(&TableProcessor::CheckPath).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    auto processor2 = HCCSProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor2.Run());
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
}

TEST_F(HCCSProcessorUTest, TestFormatDataShouldReturnFalseWhenFormatDataFailed)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    // dataFormat empty
    MOCKER_CPP(&HccsDataFormat::empty).stubs().will(returnValue(true));
    auto processor1 = HCCSProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor1.Run());
    MOCKER_CPP(&HccsDataFormat::empty).reset();

    // Reserve failed
    MOCKER_CPP(&ProcessedDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    auto processor2 = HCCSProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor2.Run());
    MOCKER_CPP(&ProcessedDataFormat::reserve).reset();

    // ProcessedDataFormat empty
    MOCKER_CPP(&ProcessedDataFormat::empty).stubs().will(returnValue(true));
    auto processor3 = HCCSProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor3.Run());
    MOCKER_CPP(&ProcessedDataFormat::empty).reset();

    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

