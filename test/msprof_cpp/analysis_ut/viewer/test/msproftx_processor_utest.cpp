/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : msproftx_processor_utest.cpp
 * Description        : MsprofTxProcessor UT
 * Author             : msprof team
 * Creation Date      : 2024/05/20
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/msproftx_processor.h"
 
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/utils/file.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Parser::Environment;

// MsprofTx table data format
using MsprofTxDataFormat = std::vector<std::tuple<uint32_t, uint32_t, uint32_t,
        std::string, int32_t, uint64_t, uint64_t, uint64_t, int32_t, std::string>>;
// MsprofTxEx table data format
using MsprofTxExDataFormat = std::vector<std::tuple<uint32_t, uint32_t,
        std::string, uint64_t, uint64_t, uint64_t, std::string>>;
// startNs, endNs, eventType, rangeId, category, message, globalTid, endGlobalTid, domainId, connectionId
using QueryDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint32_t,
        uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint32_t, uint64_t>>;

const std::string DATA_DIR = "./msproftx";
const std::string MSPROF = "msprof.db";
const std::string DB_PATH = File::PathJoin({DATA_DIR, MSPROF});
const std::string PROF = File::PathJoin({DATA_DIR, "PROF"});

const MsprofTxDataFormat MSPROFTX_DATA = {
    {0, 0, 0, "marker", 0, 0, 19627611986845096, 19627611986845096, 0, "test"},
    {0, 0, 0, "push/pop", 0, 0, 19627611986854415, 19627611986854415, 0, "test"},
    {0, 0, 0, "start/end", 0, 0, 19627611986858995, 19627611993981237, 0, "test"},
};

const MsprofTxExDataFormat MSPROFTX_EX_DATA = {
    {0, 0, "marker_ex", 19627611986934385, 19627611986941564, 0, "test"}
};

class MsprofTxProcessorUtest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(DATA_DIR));
        EXPECT_TRUE(File::CreateDir(PROF));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF, "host"})));
        EXPECT_TRUE(CreateMsprofTxDB(File::PathJoin({PROF, "host", SQLITE})));
        EXPECT_TRUE(CreateMsprofTxExDB(File::PathJoin({PROF, "host", SQLITE})));
    }

    static bool CreateMsprofTxDB(const std::string& sqlitePath)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));
        std::shared_ptr<MsprofTxDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, MsprofTxDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, File::PathJoin({sqlitePath, database->GetDBName()}));
        EXPECT_TRUE(dbRunner->CreateTable("MsprofTx", database->GetTableCols("MsprofTx")));
        EXPECT_TRUE(dbRunner->InsertData("MsprofTx", MSPROFTX_DATA));
        return true;
    }

    static bool CreateMsprofTxExDB(const std::string& sqlitePath)
    {
        std::shared_ptr<MsprofTxDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, MsprofTxDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, File::PathJoin({sqlitePath, database->GetDBName()}));
        EXPECT_TRUE(dbRunner->CreateTable("MsprofTxEx", database->GetTableCols("MsprofTxEx")));
        EXPECT_TRUE(dbRunner->InsertData("MsprofTxEx", MSPROFTX_EX_DATA));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(DATA_DIR, 0));
    }

    virtual void SetUp()
    {
        nlohmann::json record = {
            {"hostCntvct", "484576969200418"}, {"pid", "10"},
            {"CPU", {{{"Frequency", ""}}}},
            {"startCollectionTimeBegin", "1701069323851824"},
            {"endCollectionTimeEnd", "1701069338041681"},
            {"startClockMonotonicRaw", "36470610791630"},
            {"hostMonotonic", "36471130547330"},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }

    virtual void TearDown()
    {
        if (File::Exist(DB_PATH)) {
            EXPECT_TRUE(File::DeleteFile(DB_PATH));
        }
        GlobalMockObject::verify();
    }
};

TEST_F(MsprofTxProcessorUtest, TestRunShouldReturnTrueWhenRunSuccess)
{
    auto processor = MsprofTxProcessor(DB_PATH, {PROF});
    EXPECT_TRUE(processor.Run());

    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    QueryDataFormat checkData;
    size_t expectNum = MSPROFTX_DATA.size() + MSPROFTX_EX_DATA.size();
    std::string sqlStr = "SELECT startNs, message FROM " + TABLE_NAME_MSTX;
    ASSERT_NE(dbRunner, nullptr);
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectNum, checkData.size());
}

 
TEST_F(MsprofTxProcessorUtest, TestProcessShouldReturnFalseWhenProcessFailed)
{
    MOCKER_CPP(&TableProcessor::CheckPath).stubs().will(returnValue(CHECK_FAILED));
    auto processor1 = MsprofTxProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor1.Run());
    MOCKER_CPP(&TableProcessor::CheckPath).reset();

    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(false));
    auto processor2 = MsprofTxProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor2.Run());
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    auto processor3 = MsprofTxProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor3.Run());
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
 
    MOCKER_CPP(&MsprofTxProcessor::FormatTxData).stubs().will(returnValue(false));
    auto processor4 = MsprofTxProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor4.Run());
    MOCKER_CPP(&MsprofTxProcessor::FormatTxData).reset();
}

