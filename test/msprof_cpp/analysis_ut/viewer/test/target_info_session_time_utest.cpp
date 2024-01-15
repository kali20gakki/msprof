/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : target_info_session_time.cpp
 * Description        : TargetInfoNpuProcesser UT
 * Author             : msprof team
 * Creation Date      : 2024/01/11
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/target_info_session_time_processer.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Viewer::Database;
using namespace Parser::Environment;
using namespace Analysis::Utils;

using TimeDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t>>;

const std::string TARGET_INFO_SESSION_TIME_DIR = "./target_info_session_time";
const std::string REPORT = "report.db";
const std::string DB_PATH = File::PathJoin({TARGET_INFO_SESSION_TIME_DIR, REPORT});

class TargetInfoSessionTimeProcesserUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(TARGET_INFO_SESSION_TIME_DIR));
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(TARGET_INFO_SESSION_TIME_DIR, 0));
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

TEST_F(TargetInfoSessionTimeProcesserUTest, TestRunShouldReturnTrueWhenProcesserRunSuccess)
{
    std::set<std::string> profPath = {"PROF_0", "PROF_1", "PROF_2", "PROF_3", "PROF_4", "PROF_5", "PROF_6", "PROF_7"};
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
    nlohmann::json record4 = {
            {"startCollectionTimeBegin", "1701069324326713"},
            {"endCollectionTimeEnd", "1701069338060030"},
            {"startClockMonotonicRaw", "36471085681930"},
    };
    nlohmann::json record5 = {
            {"startCollectionTimeBegin", "1701069324572460"},
            {"endCollectionTimeEnd", "1701069338066763"},
            {"startClockMonotonicRaw", "36471331422600"},
    };
    nlohmann::json record6 = {
            {"startCollectionTimeBegin", "1701069324436050"},
            {"endCollectionTimeEnd", "1701069338050917"},
            {"startClockMonotonicRaw", "36471195011820"},
    };
    nlohmann::json record7 = {
            {"startCollectionTimeBegin", "1701069324072301"},
            {"endCollectionTimeEnd", "1701069338087785"},
            {"startClockMonotonicRaw", "36470831276580"},
    };
    Utils::ProfTimeRecord repectRecord{1701069323446942000, 1701069338159976000, UINT64_MAX};
    MOCKER_CPP(&Context::GetInfoByDeviceId)
        .stubs()
        .will(returnValue(record0))
        .then(returnValue(record1))
        .then(returnValue(record2))
        .then(returnValue(record3))
        .then(returnValue(record4))
        .then(returnValue(record5))
        .then(returnValue(record6))
        .then(returnValue(record7));
    auto processer = TargetInfoSessionTimeProcesser(DB_PATH, profPath);
    EXPECT_TRUE(processer.Run());
    EXPECT_EQ(repectRecord.startTimeNs, processer.record_.startTimeNs);
    EXPECT_EQ(repectRecord.endTimeNs, processer.record_.endTimeNs);
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();

    auto dbRunner = MakeShared<DBRunner>(DB_PATH);
    TimeDataFormat checkData;
    TimeDataFormat expectData = {
            {repectRecord.startTimeNs, repectRecord.endTimeNs, Analysis::Utils::TIME_BASE_OFFSET_NS},
    };
    std::string sqlStr = "SELECT startTimeNs, endTimeNs, baseTimeNs FROM " + TABLE_NAME_TARGET_INFO_SESSION_TIME;
    if (dbRunner != nullptr) {
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectData, checkData);
    }
}

TEST_F(TargetInfoSessionTimeProcesserUTest, TestRunShouldReturnFalseWhenGetTimeRecordFail)
{
    std::set<std::string> profPath = {"PROF_0", "PROF_1", "PROF_2", "PROF_3", "PROF_4", "PROF_5", "PROF_6", "PROF_7"};
    MOCKER_CPP(&Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(false));
    auto processer = TargetInfoSessionTimeProcesser(DB_PATH, profPath);
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
}

