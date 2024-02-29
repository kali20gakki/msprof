/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_processor_utest.cpp
 * Description        : task_processor UT
 * Author             : msprof team
 * Creation Date      : 2024/1/1
 * *****************************************************************************
 */
#include <algorithm>
#include <vector>
#include <set>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/viewer/database/finals/task_processor.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/parser/environment/context.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Association::Credential;
using namespace Analysis::Utils;
using namespace Analysis::Parser;
namespace {
Utils::ProfTimeRecord Tbo;
std::shared_ptr<DBRunner> ReportDBRunner;
const int DEPTH = 0;
const uint16_t PLATFORM_VERSION = static_cast<uint16_t>(Environment::Chip::CHIP_V4_1_0);
const uint64_t PID = 233;
const uint64_t START_TIME_NS = 1700902984041176000;
const uint64_t END_TIME_NS = 1700902986330096000;
const uint64_t BASE_TIME_NS = 8719641548578;
const uint16_t OP_NUM = 10;
const uint16_t STRING_NUM = 7;
const std::string TASK_PATH = "./task_path";
const std::string DB_PATH = File::PathJoin({TASK_PATH, "report.db"});
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "ascend_task.db";
const std::string SQLITE_SUFFIX = "sqlite";
const std::string PROF_PATH_A = File::PathJoin({TASK_PATH, "./PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string PROF_PATH_B = File::PathJoin({TASK_PATH, "./PROF_000001_20231125090304037_012333MBJNQLKJ"});
const std::set<std::string> PROF_PATHS = {PROF_PATH_A, PROF_PATH_B};
const std::string TABLE_NAME = "AscendTask";
const std::string TARGET_TABLE_NAME = "TASK";

using AscendTaskFormat = std::vector<std::tuple<uint32_t, int32_t, int32_t, uint32_t, uint32_t, uint32_t, double,
                                                  double, std::string, std::string, int64_t>>;
using ProcessedDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint32_t, int64_t, uint64_t,
                                                   uint64_t, uint32_t, uint32_t, int32_t, uint32_t, uint32_t>>;

AscendTaskFormat DATA_A{{4294967295, -1, 37, 1, 3, 0, 8719911184665.1, 680.013671875,
                           "UNKNOWN", "MIX_AIC", 14991},
                          {4294967295, -1, 37, 2, 5, 0, 8719911184665.1, 680.013671875,
                           "FFTS_PLUS", "Write Value", 14991},
                          {4294967295, -1, 37, 3, 5, 0, 8719911182265.1, 680.013671875,
                           "UNKNOWN", "11", 14991},
                          {4294967295, -1, 37, 4, 5, 0, 8719911184665.1, 680.013671875,
                           "UNKNOWN", "88", 14991},
                          {4294967295, -1, 37, 5, 7, 0, 8719911184965.11, 680.013671875,
                           "KERNEL_AICORE", "AI_CORE", 14991}};
AscendTaskFormat DATA_B{{4294967295, -1, 37, 6, 3, 0, 8719911184665.1, 680.013671875,
                           "UNKNOWN", "MEMCPY_ASYNC", 14991},
                          {4294967295, -1, 37, 7, 5, 0, 8719911184665.1, 680.013671875,
                           "FFTS_PLUS", "SDMA", 14991},
                          {4294967295, -1, 37, 8, 5, 0, 8719911182265.1, 680.013671875,
                           "UNKNOWN", "11", 14991},
                          {4294967295, -1, 37, 9, 5, 0, 8719911184665.1, 680.013671875,
                           "UNKNOWN", "88", 14991},
                          {4294967295, -1, 37, 10, 7, 0, 8719911184965.11, 680.013671875,
                           "MEMCPY_ASYNC", "PCIE_DMA_SQE", 14991}};
}

class TaskProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        IdPool::GetInstance().Clear();
        if (File::Check(TASK_PATH)) {
            File::RemoveDir(TASK_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(TASK_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_B));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX})));
        CreateAscendTask(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), DATA_A);
        CreateAscendTask(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), DATA_B);
        Tbo.startTimeNs = START_TIME_NS;
        Tbo.endTimeNs = END_TIME_NS;
        Tbo.baseTimeNs = BASE_TIME_NS;
    }
    virtual void TearDown()
    {
        IdPool::GetInstance().Clear();
        EXPECT_TRUE(File::RemoveDir(TASK_PATH, DEPTH));
    }
    static void CreateAscendTask(const std::string& dbPath, AscendTaskFormat data)
    {
        std::shared_ptr<AscendTaskDB> database;
        MAKE_SHARED0_RETURN_VOID(database, AscendTaskDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

void CheckCorrelationId(ProcessedDataFormat data)
{
    const uint16_t correlationIdIndex = 4;
    std::vector<uint64_t> correlationId;
    for (auto item : data) {
        correlationId.emplace_back(std::get<correlationIdIndex>(item));
    }
    EXPECT_EQ(correlationId.size(), OP_NUM);
    std::sort(correlationId.begin(), correlationId.end());
    for (size_t i = 0; i < correlationId.size(); ++i) {
        EXPECT_EQ(correlationId[i], i);
    }
}

void CheckStringId(ProcessedDataFormat data)
{
    const uint16_t stringIdIndex = 6;
    std::set<uint64_t> stringIdsSet;
    std::vector<uint64_t> stringIds;
    for (auto item : data) {
        stringIdsSet.insert(std::get<stringIdIndex>(item));
    }
    stringIds.assign(stringIdsSet.begin(), stringIdsSet.end());
    EXPECT_EQ(stringIds.size(), STRING_NUM);
    std::sort(stringIds.begin(), stringIds.end());
    for (size_t i = 0; i < stringIds.size(); ++i) {
        EXPECT_EQ(stringIds[i], i);
    }
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    ProcessedDataFormat result;
    MAKE_SHARED0_NO_OPERATION(ReportDBRunner, DBRunner, DB_PATH);
    std::string sql{"SELECT * FROM " + TARGET_TABLE_NAME};
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetPlatformVersion)
    .stubs()
    .will(returnValue(PLATFORM_VERSION));
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetPidFromInfoJson)
    .stubs()
    .will(returnValue(PID));
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(true));
    auto processor = TaskProcessor(DB_PATH, PROF_PATHS);
    EXPECT_TRUE(processor.Run());
    ReportDBRunner->QueryData(sql, result);
    CheckCorrelationId(result);
    CheckStringId(result);
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetPlatformVersion).reset();
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetPidFromInfoJson).reset();
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo).reset();
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    dbPath = File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX});
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    auto processor = TaskProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnFalseWhenCreateTableFailed)
{
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetPlatformVersion)
    .stubs()
    .will(returnValue(PLATFORM_VERSION));
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetPidFromInfoJson)
    .stubs()
    .will(returnValue(PID));
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(true));
    auto processor = TaskProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable).reset();
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetPlatformVersion).reset();
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetPidFromInfoJson).reset();
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo).reset();
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetPlatformVersion)
    .stubs()
    .will(returnValue(PLATFORM_VERSION));
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetPidFromInfoJson)
    .stubs()
    .will(returnValue(PID));
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(true));
    auto processor = TaskProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Utils::File::Check)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetPlatformVersion).reset();
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetPidFromInfoJson).reset();
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo).reset();
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnFalseWhenInsertDataFailed)
{
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetPlatformVersion)
    .stubs()
    .will(returnValue(PLATFORM_VERSION));
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetPidFromInfoJson)
    .stubs()
    .will(returnValue(PID));
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(true));
    auto id{TableColumn("Id", "INTEGER")};
    auto name{TableColumn("Name", "INTEGER")};
    std::vector<TableColumn> cols{id, name};
    auto processor = TaskProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols)
    .stubs()
    .will(returnValue(cols));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols).reset();
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetPlatformVersion).reset();
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetPidFromInfoJson).reset();
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo).reset();
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using TempT = std::tuple<uint64_t, uint64_t, uint32_t, int64_t, uint64_t,
                             uint64_t, uint32_t, uint32_t, int32_t, uint32_t, uint32_t>;
    MOCKER_CPP(&std::vector<TempT>::reserve)
    .stubs()
    .will(throws(std::bad_alloc()));
    auto processor = TaskProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&std::vector<TempT>::reserve).reset();
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    std::vector<std::string> deviceList = {File::PathJoin({TASK_PATH, "test", "device_1"})};
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix)
    .stubs()
    .will(returnValue(deviceList));
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(true));
    auto processor = TaskProcessor(DB_PATH, {File::PathJoin({TASK_PATH, "test"})});
    EXPECT_TRUE(processor.Run());
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo).reset();
}
