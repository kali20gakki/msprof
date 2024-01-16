/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : api_processer_utest.cpp
 * Description        : ApiProcesser UT
 * Author             : msprof team
 * Creation Date      : 2024/01/09
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/api_processer.h"
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Viewer::Database;
using namespace Parser::Environment;
using namespace Analysis::Utils;

// struct_type, id, level, thread_id, item_id, start, end, connection_id
using ApiDataFormat = std::vector<std::tuple<std::string, std::string, std::string, uint32_t,
        std::string, uint64_t, uint64_t, uint64_t>>;
// start, end, level, globalTid, connectionId, name
using ProcessedDataFormat = std::vector<std::tuple<double, double, uint16_t,
        uint64_t, uint64_t, uint64_t>>;

const std::string API_DIR = "./api";
const std::string REPORT = "report.db";
const std::string DB_PATH = File::PathJoin({API_DIR, REPORT});
const std::string PROF0 = File::PathJoin({API_DIR, "PROF_0"});
const std::string PROF1 = File::PathJoin({API_DIR, "PROF_1"});
const std::string PROF2 = File::PathJoin({API_DIR, "PROF_2"});
const std::string PROF3 = File::PathJoin({API_DIR, "PROF_3"});
const std::string TABLE_NAME = "ApiData";
const std::set<std::string> PROF_PATHS = {PROF0, PROF1, PROF2, PROF3};

const ApiDataFormat API_DATA = {
    {"StreamSyncTaskFinish", "0", "runtime", 116, "0", 65177262396323, 65177262396323, 0},
    {"ACL_RTS", "aclrtSynchronizeStreamWithTimeout", "acl", 116, "0", 65177262395395, 65177262397115, 2},
    {"ACL_RTS", "aclrtMemcpy", "test", 116, "0", 65177262397274, 65177262404692, 4},
    {"launch", "0", "node", 116, "Index4", 65177262436161, 65177262437816, 24},
    {"ACL_OP", "aclCreateTensorDesc", "acl", 116, "0", 65177262448526, 65177262448545, 35},
    {"master", "0", "hccl", 6635, "hcom_allReduce_", 65177264896891, 65177264928192, 224},
    {"HOST_HCCL", "hcom_allReduce_", "acl", 6635, "hcom_allReduce_", 65177264940493, 65177264975485, 245},
    {"master", "0", "hccl", 6635, "hcom_allReduce_", 65177264940493, 65177264975485, 247},
    {"ACL_OTHERS", "262144", "acl", 116, "GraphOperation::Setup", 65177265026179, 65177265048078, 299},
};

class ApiProcesserUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(API_DIR));
        EXPECT_TRUE(File::CreateDir(PROF0));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF0, HOST})));
        EXPECT_TRUE(CreateAPIDB(File::PathJoin({PROF0, HOST, SQLITE}), API_DATA));
        EXPECT_TRUE(File::CreateDir(PROF1));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF1, HOST})));
        EXPECT_TRUE(CreateAPIDB(File::PathJoin({PROF1, HOST, SQLITE}), API_DATA));
        EXPECT_TRUE(File::CreateDir(PROF2));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF2, HOST})));
        EXPECT_TRUE(CreateAPIDB(File::PathJoin({PROF2, HOST, SQLITE}), API_DATA));
        EXPECT_TRUE(File::CreateDir(PROF3));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF3, HOST})));
        EXPECT_TRUE(CreateAPIDB(File::PathJoin({PROF3, HOST, SQLITE}), API_DATA));
    }

    static bool CreateAPIDB(const std::string& sqlitePath, ApiDataFormat data)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));
        std::shared_ptr<ApiEventDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, ApiEventDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, File::PathJoin({sqlitePath, database->GetDBName()}));
        EXPECT_TRUE(dbRunner->CreateTable(TABLE_NAME, database->GetTableCols("ApiEventData")));
        EXPECT_TRUE(dbRunner->InsertData(TABLE_NAME, data));
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(API_DIR, 0));
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

TEST_F(ApiProcesserUTest, TestRunShouldReturnTrueWhenProcesserRunSuccess)
{
    auto processer = ApiProcesser(DB_PATH, {PROF_PATHS});
    uint64_t pid0 = 10;
    uint64_t pid1 = 20;
    uint64_t pid2 = 30;
    uint64_t pid3 = 40;
    MOCKER_CPP(&Context::GetPidFromInfoJson)
        .stubs()
        .will(returnValue(pid0))
        .then(returnValue(pid1))
        .then(returnValue(pid2))
        .then(returnValue(pid3));
    nlohmann::json info = {
            {"cntvct", "65177261204177"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
            {"clock_monotonic_raw", "651599377155020"},
            {"startCollectionTimeBegin", "1701069324370978"},
            {"endCollectionTimeEnd", "1701069338159976"},
            {"startClockMonotonicRaw", "36471129942580"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId)
        .stubs()
        .will(returnValue(info));
    EXPECT_TRUE(processer.Run());
    MOCKER_CPP(&Context::GetPidFromInfoJson).reset();
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}

TEST_F(ApiProcesserUTest, TestRunShouldReturnFalseWhenProcesserFail)
{
    auto processer = ApiProcesser(DB_PATH, {PROF0});
    MOCKER_CPP(&Context::GetSyscntConversionParams)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();

    MOCKER_CPP(&Context::GetSyscntConversionParams)
    .stubs()
    .will(returnValue(true));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
}

TEST_F(ApiProcesserUTest, TestRunShouldReturnFalseWhenFormatDataFail)
{
    auto processer = ApiProcesser(DB_PATH, {PROF0});
    uint64_t pid = 10;
    MOCKER_CPP(&FileReader::Check)
    .stubs()
    .will(returnValue(false));
    MOCKER_CPP(&Context::GetPidFromInfoJson)
    .stubs()
    .will(returnValue(pid));
    nlohmann::json info = {
            {"cntvct", "65177261204177"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
            {"clock_monotonic_raw", "651599377155020"},
            {"startCollectionTimeBegin", "1701069324370978"},
            {"endCollectionTimeEnd", "1701069338159976"},
            {"startClockMonotonicRaw", "36471129942580"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId)
    .stubs()
    .will(returnValue(info));
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&FileReader::Check).reset();

    MOCKER_CPP(&ProcessedDataFormat::reserve)
    .stubs()
    .will(throws(std::bad_alloc()));
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&ProcessedDataFormat::reserve).reset();

    MOCKER_CPP(&ProcessedDataFormat::empty)
    .stubs()
    .will(returnValue(true));
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&ProcessedDataFormat::empty).reset();
    MOCKER_CPP(&Context::GetPidFromInfoJson).reset();
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}
