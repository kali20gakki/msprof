/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : compute_task_info_processor_utest.cpp
 * Description        : compute_task_info_processor UT
 * Author             : msprof team
 * Creation Date      : 2024/1/4
 * *****************************************************************************
 */
#include <algorithm>
#include <vector>
#include <set>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/viewer/database/finals/compute_task_info_processor.h"
#include "analysis/csrc/application/credential/id_pool.h"
#include "analysis/csrc/infrastructure/utils/thread_pool.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Application::Credential;
using namespace Analysis::Utils;
namespace {
std::shared_ptr<DBRunner> MsprofDBRunner;
const int DEPTH = 0;
const uint16_t OP_NUM = 4;
const uint16_t STRING_NUM = 18;
const std::string COMPUTE_TASK_PATH = "./compute_task_path";
const std::string DB_PATH = File::PathJoin({COMPUTE_TASK_PATH, "msprof.db"});
const std::string HOST_SUFFIX = "host";
const std::string DB_SUFFIX = "ge_info.db";
const std::string SQLITE_SUFFIX = "sqlite";
const std::string PROF_PATH_A = File::PathJoin({COMPUTE_TASK_PATH,
                                                "PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string PROF_PATH_B = File::PathJoin({COMPUTE_TASK_PATH,
                                                "PROF_000001_20231125090304037_012333MBJNQLKJ"});
const std::set<std::string> PROF_PATHS = {PROF_PATH_A, PROF_PATH_B};
const std::string TABLE_NAME = "TaskInfo";
const std::string TARGET_TABLE_NAME = "COMPUTE_TASK_INFO";
const std::string TABLE_COMMUNICATION_SCHEDULE_TASK_INFO = "COMMUNICATION_SCHEDULE_TASK_INFO";

using GeInfoFormat = std::vector<std::tuple<uint32_t, std::string, int32_t, int32_t, uint32_t, uint32_t, std::string,
                                              std::string, std::string, int32_t, uint32_t, double, uint32_t, uint32_t,
                                              std::string, std::string, std::string, std::string, std::string,
                                              std::string, int32_t, uint32_t, std::string, std::string>>;

using PROCESSED_DATA_FORMAT = std::vector<std::tuple<uint64_t, uint64_t, uint32_t, uint32_t,
                                                     uint64_t, uint64_t, uint64_t, uint64_t,
                                                     uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
using CommScheduleDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t>>;

GeInfoFormat DATA_A{{4294967295, "aclnnMm_MatMulCommon_MatMulV2", 2, 1, 20, 40, "1", "MIX_AIC", "MatMulV2", -1,
                       3391981, 453148218443103, 0, 3, "FORMAT_ND;FORMAT_ND", "FLOAT16;FLOAT16",
                       "\"10000,10000;10000,10000\"", "FORMAT_ND", "FLOAT16", "\"10000,10000\"", 0, 0, "NO", "N/A"},
                      {4294967295, "trans_TransData_0", 2, 2, 35, 0, "1", "AI_CORE", "TransData", -1,
                       250512, 569402956566, 0, 2, "FORMAT_ND", "FLOAT",
                       "\"3072,768\"", "FRACTAL_NZ", "FLOAT", "\"48,192,16,16\"", 0, 4294967295, "NO", "N/A"},
                      {4294967295, "allreduceAicpuKernel", 2, 3, 0, 0, "1", "AI_CPU", "allreduceAicpuKernel", -1,
                       250512, 569402956569, 0, 0, "N/A", "N/A",
                       "N/A", "N/A", "N/A", "N/A", 0, 4294967295, "NO", "N/A"}};
GeInfoFormat DATA_B{{4294967295, "Add", 2, 3, 20, 40, "1", "MIX_AIC", "MatMulV2", -1,
                       3391981, 453148218443103, 0, 3, "FORMAT_ND;FORMAT_ND", "FLOAT16;FLOAT16",
                       "\"10000,10000;10000,10000\"", "FORMAT_ND", "FLOAT16", "\"10000,10000\"", 0, 0, "NO", "N/A"},
                      {4294967295, "trans_TransData_14", 2, 4, 35, 0, "1", "AI_CORE", "TransData", -1,
                       250512, 569402956566, 0, 2, "FORMAT_ND", "FLOAT",
                       "\"3072,768\"", "FRACTAL_NZ", "FLOAT", "\"48,192,16,16\"", 0, 4294967295, "NO", "N/A"}};
}

class ComputeTaskInfoProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        IdPool::GetInstance().Clear();
        if (File::Check(COMPUTE_TASK_PATH)) {
            File::RemoveDir(COMPUTE_TASK_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(COMPUTE_TASK_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_B));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, HOST_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, HOST_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, HOST_SUFFIX, SQLITE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, HOST_SUFFIX, SQLITE_SUFFIX})));
        CreateTaskInfo(File::PathJoin({PROF_PATH_A, HOST_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), DATA_A);
        CreateTaskInfo(File::PathJoin({PROF_PATH_B, HOST_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), DATA_B);
    }
    virtual void TearDown()
    {
        IdPool::GetInstance().Clear();
        EXPECT_TRUE(File::RemoveDir(COMPUTE_TASK_PATH, DEPTH));
    }
    static void CreateTaskInfo(const std::string& dbPath, GeInfoFormat data)
    {
        std::shared_ptr<GEInfoDB> database;
        MAKE_SHARED0_RETURN_VOID(database, GEInfoDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

void CheckGlobalTaskId(PROCESSED_DATA_FORMAT data)
{
    const uint16_t globalTaskIdIndex = 1;
    std::vector<uint64_t> globalTaskIds;
    for (auto item : data) {
        globalTaskIds.emplace_back(std::get<globalTaskIdIndex>(item));
    }
    std::sort(globalTaskIds.begin(), globalTaskIds.end());
    for (int i = 0; i < OP_NUM; ++i) {
        EXPECT_EQ(globalTaskIds[i], i);
    }
}

void CheckStringId(PROCESSED_DATA_FORMAT data)
{
    const uint16_t nameIndex = 0;
    const uint16_t taskTypeIndex = 4;
    const uint16_t opTypeIndex = 5;
    const uint16_t inputFormatsIndex = 6;
    const uint16_t inputDataTypesIndex = 7;
    const uint16_t inputShapesIndex = 8;
    const uint16_t outputFormatsIndex = 9;
    const uint16_t outputDataTypesIndex = 10;
    const uint16_t outputShapesIndex = 11;
    const uint16_t hashidIndex = 12;
    std::set<uint64_t> stringIdsSet;
    std::vector<uint64_t> stringIds;
    for (auto item : data) {
        stringIdsSet.insert(std::get<nameIndex>(item));
        stringIdsSet.insert(std::get<taskTypeIndex>(item));
        stringIdsSet.insert(std::get<opTypeIndex>(item));
        stringIdsSet.insert(std::get<inputFormatsIndex>(item));
        stringIdsSet.insert(std::get<inputDataTypesIndex>(item));
        stringIdsSet.insert(std::get<inputShapesIndex>(item));
        stringIdsSet.insert(std::get<outputFormatsIndex>(item));
        stringIdsSet.insert(std::get<outputDataTypesIndex>(item));
        stringIdsSet.insert(std::get<outputShapesIndex>(item));
        stringIdsSet.insert(std::get<hashidIndex>(item));
    }
    stringIds.assign(stringIdsSet.begin(), stringIdsSet.end());
    std::sort(stringIds.begin(), stringIds.end());
    for (int i = 0; i < STRING_NUM; ++i) {
        EXPECT_EQ(stringIds[i], i);
    }
}

TEST_F(ComputeTaskInfoProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    PROCESSED_DATA_FORMAT result;
    MAKE_SHARED0_NO_OPERATION(MsprofDBRunner, DBRunner, DB_PATH);
    std::string sql{"SELECT * FROM " + TARGET_TABLE_NAME};
    auto processor = ComputeTaskInfoProcessor(DB_PATH, PROF_PATHS);
    EXPECT_TRUE(processor.Run());
    MsprofDBRunner->QueryData(sql, result);
    CheckGlobalTaskId(result);
    CheckStringId(result);
    CommScheduleDataFormat scheduleRes;
    std::string sql2{"SELECT * FROM " + TABLE_COMMUNICATION_SCHEDULE_TASK_INFO};
    MsprofDBRunner->QueryData(sql2, scheduleRes);
    ASSERT_EQ(1, scheduleRes.size());
    const uint16_t scheduleTaskGlobalTaskId = 4;
    EXPECT_EQ(scheduleTaskGlobalTaskId, std::get<1>(scheduleRes[0]));
}

TEST_F(ComputeTaskInfoProcessorUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH_A, HOST_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    dbPath = File::PathJoin({PROF_PATH_B, HOST_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX});
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    auto processor = ComputeTaskInfoProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
}

TEST_F(ComputeTaskInfoProcessorUTest, TestRunShouldReturnFalseWhenCreateTableFailed)
{
    auto processor = ComputeTaskInfoProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable).reset();
}

TEST_F(ComputeTaskInfoProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    auto processor = ComputeTaskInfoProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Utils::File::Check)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(ComputeTaskInfoProcessorUTest, TestRunShouldReturnFalseWhenInsertDataFailed)
{
    auto id{TableColumn("Id", "INTEGER")};
    auto name{TableColumn("Name", "INTEGER")};
    std::vector<TableColumn> cols{id, name};
    auto processor = ComputeTaskInfoProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols)
    .stubs()
    .will(returnValue(cols));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols).reset();
}

TEST_F(ComputeTaskInfoProcessorUTest, TestRunShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using TempT = std::tuple<uint64_t, uint64_t, uint32_t, uint32_t,
                             uint64_t, uint64_t, uint64_t, uint64_t,
                             uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>;
    MOCKER_CPP(&std::vector<TempT>::reserve)
    .stubs()
    .will(throws(std::bad_alloc()));
    auto processor = ComputeTaskInfoProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&std::vector<TempT>::reserve).reset();
}

TEST_F(ComputeTaskInfoProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    auto processor = ComputeTaskInfoProcessor(DB_PATH, {File::PathJoin({COMPUTE_TASK_PATH, "test"})});
    EXPECT_TRUE(processor.Run());
}
