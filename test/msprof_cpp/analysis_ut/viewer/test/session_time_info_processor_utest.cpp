/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : session_time_info_processor.cpp
 * Description        : SessionTimeInfoProcessor UT
 * Author             : msprof team
 * Creation Date      : 2024/01/11
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/session_time_info_processor.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Viewer::Database;
using namespace Parser::Environment;
using namespace Analysis::Utils;

using TimeDataFormat = std::vector<std::tuple<uint64_t, uint64_t>>;

const std::string SESSION_TIME_INFO_DIR = "./session_time_info";
const std::string MSPROF = "msprof.db";
const std::string DB_PATH = File::PathJoin({SESSION_TIME_INFO_DIR, MSPROF});

class SessionTimeInfoProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(SESSION_TIME_INFO_DIR));
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(SESSION_TIME_INFO_DIR, 0));
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

TEST_F(SessionTimeInfoProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    std::set<std::string> profPath = {"PROF_0", "PROF_1", "PROF_2", "PROF_3"};
    nlohmann::json record0 = {
        {"startCollectionTimeBegin", "1701069324370978"},
        {"endCollectionTimeEnd", "1701069338159976"},
        {"startClockMonotonicRaw", "36471129942580"},
    };
    nlohmann::json record1 = {
        {"startCollectionTimeBegin", "1701069324301676"},
        {"endCollectionTimeEnd", "1701069338059772"},
        {"startClockMonotonicRaw", "36471060644730"},
    };
    nlohmann::json record2 = {
        {"startCollectionTimeBegin", "1701069323446942"},
        {"endCollectionTimeEnd", "1701069338049987"},
        {"startClockMonotonicRaw", "36470205900460"},
    };
    nlohmann::json record3 = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
    };
    Utils::ProfTimeRecord expectRecord{1701069323446942000, 1701069338159976000, UINT64_MAX};
    MOCKER_CPP(&Context::GetInfoByDeviceId)
        .stubs()
        .will(returnValue(record0))
        .then(returnValue(record1))
        .then(returnValue(record2))
        .then(returnValue(record3));
    auto processor = SessionTimeInfoProcessor(DB_PATH, profPath);
    EXPECT_TRUE(processor.Run());
    EXPECT_EQ(expectRecord.startTimeNs, processor.record_.startTimeNs);
    EXPECT_EQ(expectRecord.endTimeNs, processor.record_.endTimeNs);
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    TimeDataFormat checkData;
    TimeDataFormat expectData = {{expectRecord.startTimeNs, expectRecord.endTimeNs}};
    std::string sqlStr = "SELECT startTimeNs, endTimeNs FROM " + TABLE_NAME_SESSION_TIME_INFO;
    ASSERT_NE(dbRunner, nullptr);
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectData, checkData);
}

TEST_F(SessionTimeInfoProcessorUTest, TestRunShouldReturnFalseWhenGetTimeRecordFail)
{
    std::set<std::string> profPath = {"PROF_0", "PROF_1", "PROF_2", "PROF_3", "PROF_4", "PROF_5", "PROF_6", "PROF_7"};
    MOCKER_CPP(&Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(false));
    auto processor = SessionTimeInfoProcessor(DB_PATH, profPath);
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
}

