/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : api_processor_utest.cpp
 * Description        : ApiProcessor UT
 * Author             : msprof team
 * Creation Date      : 2024/01/09
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "analysis/csrc/viewer/database/finals/api_processor.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/application/credential/id_pool.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Viewer::Database;
using namespace Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Application::Credential;

// struct_type, id, level, thread_id, item_id, start, end, connection_id
using ApiDataFormat = std::vector<std::tuple<std::string, std::string, std::string, uint32_t,
        std::string, uint64_t, uint64_t, uint64_t>>;
// start, end, type, globalTid, connectionId, name
using ProcessedDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint16_t,
        uint64_t, uint64_t, uint64_t>>;

// start, end, type, globalTid, connectionId, name
using QueryDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint32_t,
        uint64_t, uint64_t, uint64_t>>;

const std::string API_DIR = "./api";
const std::string MSPROF = "msprof.db";
const std::string DB_PATH = File::PathJoin({API_DIR, MSPROF});
const std::string PROF0 = File::PathJoin({API_DIR, "PROF_0"});
const std::string PROF1 = File::PathJoin({API_DIR, "PROF_1"});
const std::string PROF2 = File::PathJoin({API_DIR, "PROF_2"});
const std::string PROF3 = File::PathJoin({API_DIR, "PROF_3"});
const std::string TABLE_NAME = "ApiData";
const std::set<std::string> PROF_PATHS = {PROF0, PROF1, PROF2, PROF3};

const ApiDataFormat API_DATA = {
    {"StreamSyncTaskFinish", "0", "runtime", 116, "0", 65177262396323, 65177262396323, 1},
    {"ACL_RTS", "aclrtSynchronizeStreamWithTimeout", "acl", 116, "0", 65177262395395, 65177262397115, 2},
    {"ACL_RTS", "aclrtMemcpy", "test", 116, "0", 65177262397274, 65177262404692, 4},
    {"launch", "0", "node", 116, "Index4", 65177262436161, 65177262437816, 24},
    {"ACL_OP", "aclCreateTensorDesc", "acl", 116, "0", 65177262448526, 65177262448545, 35},
    {"master", "0", "hccl", 6635, "hcom_allReduce_", 65177264896891, 65177264928192, 224},
    {"HOST_HCCL", "hcom_allReduce_", "acl", 6635, "hcom_allReduce_", 65177264940493, 65177264975485, 245},
    {"master", "0", "hccl", 6635, "hcom_allReduce_", 65177264940493, 65177264975485, 247},
    {"ACL_OTHERS", "262144", "acl", 116, "GraphOperation::Setup", 65177265026179, 65177265048078, 299},
};
const uint16_t LEVEL_INDEX = 2;
const uint16_t TID_INDEX = 3;
const uint16_t CONNECTION_ID_INDEX = 7;

class ApiProcessorUTest : public testing::Test {
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
        EXPECT_TRUE(dbRunner->CreateTable(TABLE_NAME, database->GetTableCols(TABLE_NAME)));
        EXPECT_TRUE(dbRunner->InsertData(TABLE_NAME, data));
        return true;
    }

    static void TearDownTestCase()
    {
        Context::GetInstance().Clear();
        EXPECT_TRUE(File::RemoveDir(API_DIR, 0));
    }

    virtual void SetUp()
    {
        IdPool::GetInstance().Clear();
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069324370978"},
            {"endCollectionTimeEnd", "1701069338159976"},
            {"startClockMonotonicRaw", "36471129942580"},
            {"pid", "10"},
            {"hostCntvct", "65177261204177"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
            {"hostMonotonic", "651599377155020"},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }

    virtual void TearDown()
    {
        if (File::Exist(DB_PATH)) {
            EXPECT_TRUE(File::DeleteFile(DB_PATH));
        }
        IdPool::GetInstance().Clear();
        MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
    }
};

void CheckApiDataValid(const QueryDataFormat &checkData)
{
    // 业务角度，用于校验获取到的API表的数据
    uint64_t start;
    uint64_t end;
    uint16_t type;
    uint64_t tid;
    uint64_t connectionId;
    uint64_t name;
    const uint16_t moveCount = 32;
    for (uint16_t i = 0; i < checkData.size(); ++i) {
        auto index = i % API_DATA.size();
        std::tie(start, end, type, tid, connectionId, name) = checkData[i];
        // 开始小于结束时间
        EXPECT_LE(start, end);
        // 比对前后的level是否一致
        auto tempTypeIt = API_LEVEL_TABLE.find(std::get<LEVEL_INDEX>(API_DATA[index]));
        EXPECT_EQ(type, (tempTypeIt == API_LEVEL_TABLE.end()) ? UINT16_MAX : tempTypeIt->second);
        uint32_t oriTid = std::get<TID_INDEX>(API_DATA[index]);
        uint32_t oriConId = std::get<CONNECTION_ID_INDEX>(API_DATA[index]);
        // 分别校验获取到的tid和connectionId的低32位是否与原先保持一致。
        EXPECT_EQ((tid & 0xffffffff), oriTid);
        EXPECT_EQ((connectionId & 0xffffffff), oriConId);
    }
}

TEST_F(ApiProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    auto processor = ApiProcessor(DB_PATH, {PROF_PATHS});
    EXPECT_TRUE(processor.Run());
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    QueryDataFormat checkData;
    uint16_t expectNum = API_DATA.size() * PROF_PATHS.size();
    std::string sqlStr = "SELECT startNs, endNs, type, globalTid, connectionId, name FROM " + TABLE_NAME_CANN_API;
    ASSERT_NE(dbRunner, nullptr);
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectNum, checkData.size());
    const uint16_t expectStringIdNum = 11;
    EXPECT_EQ(IdPool::GetInstance().GetAllUint64Ids().size(), expectStringIdNum);
    CheckApiDataValid(checkData);
}

TEST_F(ApiProcessorUTest, TestRunShouldReturnFalseWhenProcessorFail)
{
    auto processor = ApiProcessor(DB_PATH, {PROF0});
    MOCKER_CPP(&Context::GetSyscntConversionParams)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();

    MOCKER_CPP(&Context::GetSyscntConversionParams)
    .stubs()
    .will(returnValue(true));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
}

TEST_F(ApiProcessorUTest, TestRunShouldReturnFalseWhenFormatDataFail)
{
    auto processor = ApiProcessor(DB_PATH, {PROF0});
    MOCKER_CPP(&FileReader::Check)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&FileReader::Check).reset();

    MOCKER_CPP(&ProcessedDataFormat::reserve)
    .stubs()
    .will(throws(std::bad_alloc()));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&ProcessedDataFormat::reserve).reset();

    MOCKER_CPP(&ProcessedDataFormat::empty)
    .stubs()
    .will(returnValue(true));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&ProcessedDataFormat::empty).reset();
}
