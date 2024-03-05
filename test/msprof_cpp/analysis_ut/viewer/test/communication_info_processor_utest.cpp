/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : communication_task_info_processor_utest.cpp
 * Description        : communication_task_info_processor UT
 * Author             : msprof team
 * Creation Date      : 2024/1/4
 * *****************************************************************************
 */
#include <algorithm>
#include <vector>
#include <set>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/viewer/database/finals/communication_info_processor.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Association::Credential;
using namespace Analysis::Utils;
using namespace Analysis::Parser;
namespace {
const int DEPTH = 0;
const uint16_t OP_NUM = 4;
const uint16_t STRING_NUM = 13;
const uint16_t OP_NAME_NUM = 2;
const uint16_t CONNECTION_ID_NUM = 3;
const uint16_t OP_ID_NUM = 3;
const std::string COMMUNICATION_TASK_PATH = "./task_path";
const std::string DB_PATH = File::PathJoin({COMMUNICATION_TASK_PATH, "report.db"});
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "hccl_single_device.db";
const std::string PROF_PATH_A = File::PathJoin({COMMUNICATION_TASK_PATH,
                                                "./PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string PROF_PATH_B = File::PathJoin({COMMUNICATION_TASK_PATH,
                                                "./PROF_000001_20231125090304037_012333MBJNQLKJ"});
const std::set<std::string> PROF_PATHS = {PROF_PATH_A, PROF_PATH_B};
const std::string TABLE_NAME = "HCCLSingleDevice";

using HcclSingleDeviceFormat = std::vector<std::tuple<uint32_t, int32_t, std::string, uint32_t, std::string,
                                                      std::string, double, int32_t, double, double, double,
                                                      std::string, std::string, uint64_t, int32_t, uint64_t, uint32_t,
                                                      double, uint32_t, uint32_t, std::string, uint64_t, std::string,
                                                      std::string, double, uint32_t, uint64_t, uint32_t, std::string>>;

using CommunicationTaskDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint32_t, uint64_t, uint64_t,
                                                           uint64_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t,
                                                           uint64_t, uint32_t>>;

using CommunicationOpDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                                         uint64_t>>;

const HcclSingleDeviceFormat DATA_A{{4294967295, -1, "hcom_allReduce__360_0_1", 0, "Memcpy", "10652853832407468360",
                                     78180470736653, 0, 781687236999155, 2994.875, 1, "HCCL", "hcom_allReduce_", 125,
                                     1, 11, 1, 14.1825906735751, 0, 0, "SDMA", 262144, "INVALID_TYPE",
                                     "ON_CHIP", 87.530865228098, 4294967295, 4294967296, 1, "INVALID_TYPE"},
                                    {4294967295, -1, "hcom_allReduce__360_0_1", 0, "Memcpy", "10652853832407468360",
                                     78180470736653, 0, 781687236999155, 2994.875, 1, "HCCL", "hcom_allReduce_", 126,
                                     1, 11, 2, 14.1825906735751, 0, 0, "SDMA", 262144, "FP16",
                                     "HCCS", 87.530865228098, 4294967295, 8, 1, "INVALID_TYPE"}};
const HcclSingleDeviceFormat DATA_B{{4294967295, -1, "hcom_allReduce__360_0_2", 0, "Memcpy23", "10652853832407468233",
                                     78180470736653, 0, 781687236999155, 2994.875, 3, "HCCL", "hcom_allReduce_", 125,
                                     1, 11, 3, 14.1825906735751, 1, 4, "SDMA", 262144, "INVALID_TYPE",
                                     "ON_CHIP", 87.530865228098, 4294967295, 4294967296, 1, "INVALID_TYPE"},
                                    {4294967295, -1, "hcom_allReduce__360_0_1", 0, "Memcpy", "10652853832407468832",
                                     78180470736653, 0, 781687236999155, 2994.875, 1, "HCCL", "hcom_allReduce_", 126,
                                     1, 21, 4, 14.1825906735751, 4, 2, "SDMA", 262144, "FP32",
                                     "HCCS", 87.530865228098, 4294967295, 8, 2, "INVALID_TYPE"}};
}

class CommunicationInfoProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        IdPool::GetInstance().Clear();
        if (File::Exist(COMMUNICATION_TASK_PATH)) {
            File::RemoveDir(COMMUNICATION_TASK_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(COMMUNICATION_TASK_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_B));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE})));
        CreateHcclSingleDevice(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_A);
        CreateHcclSingleDevice(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_B);
        MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo)
            .stubs()
            .will(returnValue(true));
    }
    virtual void TearDown()
    {
        IdPool::GetInstance().Clear();
        EXPECT_TRUE(File::RemoveDir(COMMUNICATION_TASK_PATH, DEPTH));
        MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo).reset();
    }
    static void CreateHcclSingleDevice(const std::string& dbPath, HcclSingleDeviceFormat data)
    {
        std::shared_ptr<HCCLSingleDeviceDB> database;
        MAKE_SHARED0_RETURN_VOID(database, HCCLSingleDeviceDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

void CheckCorrelationId(CommunicationTaskDataFormat data)
{
    const uint16_t correlationIdIndex = 1;
    std::vector<uint64_t> correlationIds;
    for (auto item : data) {
        correlationIds.emplace_back(std::get<correlationIdIndex>(item));
    }
    std::sort(correlationIds.begin(), correlationIds.end());
    for (int i = 0; i < OP_NUM; ++i) {
        EXPECT_EQ(correlationIds[i], i);
    }
}

void CheckStringId(CommunicationTaskDataFormat data)
{
    const uint16_t nameIndex = 0;
    const uint16_t taskTypeIndex = 2;
    const uint16_t groupNameIndex = 4;
    const uint16_t rdmaTypeIndex = 6;
    const uint16_t srcRankIndex = 7;
    const uint16_t dstRankIndex = 8;
    const uint16_t transportTypeIndex = 9;
    const uint16_t dataTypeIndex = 11;
    const uint16_t linkTypeIndex = 12;
    std::set<uint64_t> stringIdsSet;
    std::vector<uint64_t> stringIds;
    for (auto item : data) {
        stringIdsSet.insert(std::get<nameIndex>(item));
        stringIdsSet.insert(std::get<taskTypeIndex>(item));
        stringIdsSet.insert(std::get<groupNameIndex>(item));
        stringIdsSet.insert(std::get<rdmaTypeIndex>(item));
        stringIdsSet.insert(std::get<srcRankIndex>(item));
        stringIdsSet.insert(std::get<dstRankIndex>(item));
        stringIdsSet.insert(std::get<transportTypeIndex>(item));
        stringIdsSet.insert(std::get<dataTypeIndex>(item));
        stringIdsSet.insert(std::get<linkTypeIndex>(item));
    }
    stringIds.assign(stringIdsSet.begin(), stringIdsSet.end());
    std::sort(stringIds.begin(), stringIds.end());
    for (int i = 0; i < STRING_NUM; ++i) {
        EXPECT_EQ(stringIds[i], i);
    }
}

void CheckOpInfo(CommunicationOpDataFormat data)
{
    const uint16_t opNameIndex = 0;
    const uint16_t connectionIdIndex = 3;
    const uint16_t opIdIndex = 5;
    std::set<uint64_t> opNameSet;
    std::set<uint64_t> connectionIdSet;
    std::set<uint32_t> opIdSet;
    for (auto item : data) {
        opNameSet.insert(std::get<opNameIndex>(item));
        connectionIdSet.insert(std::get<connectionIdIndex>(item));
        opIdSet.insert(std::get<opIdIndex>(item));
    }
    EXPECT_EQ(opNameSet.size(), OP_NAME_NUM);
    EXPECT_EQ(connectionIdSet.size(), CONNECTION_ID_NUM);
    EXPECT_EQ(opIdSet.size(), OP_ID_NUM);
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    std::shared_ptr<DBRunner> reportDBRunner;
    CommunicationTaskDataFormat taskResult;
    CommunicationOpDataFormat opResult;
    std::string sql{"SELECT * FROM " + TABLE_NAME_COMMUNICATION_TASK_INFO};
    MAKE_SHARED0_NO_OPERATION(reportDBRunner, DBRunner, DB_PATH);
    auto processor = CommunicationInfoProcessor(DB_PATH, PROF_PATHS);
    EXPECT_TRUE(processor.Run());
    reportDBRunner->QueryData(sql, taskResult);
    CheckCorrelationId(taskResult);
    CheckStringId(taskResult);
    sql = "SELECT * FROM " + TABLE_NAME_COMMUNICATION_OP;
    reportDBRunner->QueryData(sql, opResult);
    CheckOpInfo(opResult);
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    dbPath = File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    auto processor = CommunicationInfoProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnFalseWhenCreateTableFailed)
{
    auto processor = CommunicationInfoProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable).reset();
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    auto processor = CommunicationInfoProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Utils::File::Check)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnFalseWhenInsertDataFailed)
{
    auto id{TableColumn("Id", "INTEGER")};
    auto name{TableColumn("Name", "INTEGER")};
    std::vector<TableColumn> cols{id, name};
    auto processor = CommunicationInfoProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols)
    .stubs()
    .will(returnValue(cols));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols).reset();
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using TempT = std::tuple<uint64_t, uint64_t, uint64_t, uint32_t, uint64_t, uint64_t,
                             uint64_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t,
                             uint64_t, uint32_t>;
    MOCKER_CPP(&std::vector<TempT>::reserve)
    .stubs()
    .will(throws(std::bad_alloc()));
    auto processor = CommunicationInfoProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&std::vector<TempT>::reserve).reset();
}

TEST_F(CommunicationInfoProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    std::vector<std::string> deviceList = {File::PathJoin({COMMUNICATION_TASK_PATH, "test", "device_1"})};
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix)
    .stubs()
    .will(returnValue(deviceList));
    auto processor = CommunicationInfoProcessor(DB_PATH, {File::PathJoin({COMMUNICATION_TASK_PATH, "test"})});
    EXPECT_TRUE(processor.Run());
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}
