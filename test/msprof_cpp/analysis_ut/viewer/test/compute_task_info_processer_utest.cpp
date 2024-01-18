/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : compute_task_info_processer_utest.cpp
 * Description        : compute_task_info_processer UT
 * Author             : msprof team
 * Creation Date      : 2024/1/4
 * *****************************************************************************
 */
#include <algorithm>
#include <vector>
#include <set>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/viewer/database/finals/compute_task_info_processer.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/utils/thread_pool.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Association::Credential;
using namespace Analysis::Utils;
namespace {
std::shared_ptr<DBRunner> ReportDBRunner;
const int DEPTH = 0;
const uint16_t OP_NUM = 4;
const uint16_t STRING_NUM = 18;
const std::string COMPUTE_TASK_PATH = "./compute_task_path";
const std::string DB_PATH = File::PathJoin({COMPUTE_TASK_PATH, "report.db"});
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

using GeInfoFormat = std::vector<std::tuple<uint32_t, std::string, int32_t, int32_t, uint32_t, uint32_t, std::string,
                                              std::string, std::string, int32_t, uint32_t, double, uint32_t, uint32_t,
                                              std::string, std::string, std::string, std::string, std::string,
                                              std::string, int32_t, uint32_t, std::string>>;

using PROCESSED_DATA_FORMAT = std::vector<std::tuple<uint64_t, uint64_t, uint32_t, uint32_t, uint64_t, uint64_t,
                                                     uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;

GeInfoFormat DATA_A{{4294967295, "aclnnMm_MatMulCommon_MatMulV2", 2, 1, 20, 40, "1", "MIX_AIC", "MatMulV2", -1,
                       3391981, 453148218443103, 0, 3, "FORMAT_ND;FORMAT_ND", "FLOAT16;FLOAT16",
                       "\"10000,10000;10000,10000\"", "FORMAT_ND", "FLOAT16", "\"10000,10000\"", 0, 0, "NO"},
                      {4294967295, "trans_TransData_0", 2, 2, 35, 0, "1", "AI_CORE", "TransData", -1,
                       250512, 569402956566, 0, 2, "FORMAT_ND", "FLOAT",
                       "\"3072,768\"", "FRACTAL_NZ", "FLOAT", "\"48,192,16,16\"", 0, 4294967295, "NO"}};
GeInfoFormat DATA_B{{4294967295, "Add", 2, 3, 20, 40, "1", "MIX_AIC", "MatMulV2", -1,
                       3391981, 453148218443103, 0, 3, "FORMAT_ND;FORMAT_ND", "FLOAT16;FLOAT16",
                       "\"10000,10000;10000,10000\"", "FORMAT_ND", "FLOAT16", "\"10000,10000\"", 0, 0, "NO"},
                      {4294967295, "trans_TransData_14", 2, 4, 35, 0, "1", "AI_CORE", "TransData", -1,
                       250512, 569402956566, 0, 2, "FORMAT_ND", "FLOAT",
                       "\"3072,768\"", "FRACTAL_NZ", "FLOAT", "\"48,192,16,16\"", 0, 4294967295, "NO"}};
}

class ComputeTaskInfoProcesserUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        File::CreateDir(COMPUTE_TASK_PATH);
        File::CreateDir(PROF_PATH_A);
        File::CreateDir(PROF_PATH_B);
        File::CreateDir(File::PathJoin({PROF_PATH_A, HOST_SUFFIX}));
        File::CreateDir(File::PathJoin({PROF_PATH_B, HOST_SUFFIX}));
        File::CreateDir(File::PathJoin({PROF_PATH_A, HOST_SUFFIX, SQLITE_SUFFIX}));
        File::CreateDir(File::PathJoin({PROF_PATH_B, HOST_SUFFIX, SQLITE_SUFFIX}));
        CreateTaskInfo(File::PathJoin({PROF_PATH_A, HOST_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), DATA_A);
        CreateTaskInfo(File::PathJoin({PROF_PATH_B, HOST_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), DATA_B);
    }
    virtual void TearDown()
    {
        IdPool::GetInstance().Clear();
        File::RemoveDir(COMPUTE_TASK_PATH, DEPTH);
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

void CheckCorrelationId(PROCESSED_DATA_FORMAT data)
{
    const uint16_t correlationIdIndex = 1;
    std::vector<uint64_t> correlationId;
    for (auto item : data) {
        correlationId.emplace_back(std::get<correlationIdIndex>(item));
    }
    std::sort(correlationId.begin(), correlationId.end());
    for (int i = 0; i < OP_NUM; ++i) {
        EXPECT_EQ(correlationId[i], i);
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
    }
    stringIds.assign(stringIdsSet.begin(), stringIdsSet.end());
    std::sort(stringIds.begin(), stringIds.end());
    for (int i = 0; i < STRING_NUM; ++i) {
        EXPECT_EQ(stringIds[i], i);
    }
}

TEST_F(ComputeTaskInfoProcesserUTest, TestRunShouldReturnTrueWhenProcesserRunSuccess)
{
    PROCESSED_DATA_FORMAT result;
    MAKE_SHARED0_NO_OPERATION(ReportDBRunner, DBRunner, DB_PATH);
    std::string sql{"SELECT * FROM " + TARGET_TABLE_NAME};
    auto processer = ComputeTaskInfoProcesser(DB_PATH, PROF_PATHS);
    EXPECT_TRUE(processer.Run());
    ReportDBRunner->QueryData(sql, result);
    CheckCorrelationId(result);
    CheckStringId(result);
}

TEST_F(ComputeTaskInfoProcesserUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH_A, HOST_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    dbPath = File::PathJoin({PROF_PATH_B, HOST_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX});
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    auto processer = ComputeTaskInfoProcesser(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processer.Run());
}

TEST_F(ComputeTaskInfoProcesserUTest, TestRunShouldReturnFalseWhenCreateTableFailed)
{
    auto processer = ComputeTaskInfoProcesser(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable).reset();
}

TEST_F(ComputeTaskInfoProcesserUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    auto processer = ComputeTaskInfoProcesser(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Utils::File::Check)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(ComputeTaskInfoProcesserUTest, TestRunShouldReturnFalseWhenInsertDataFailed)
{
    auto id{TableColumn("Id", "INTEGER")};
    auto name{TableColumn("Name", "INTEGER")};
    std::vector<TableColumn> cols{id, name};
    auto processer = ComputeTaskInfoProcesser(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols)
    .stubs()
    .will(returnValue(cols));
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols).reset();
}

TEST_F(ComputeTaskInfoProcesserUTest, TestRunShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using TempT = std::tuple<uint64_t, uint64_t, uint32_t, uint32_t, uint64_t, uint64_t,
                             uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>;
    MOCKER_CPP(&std::vector<TempT>::reserve)
    .stubs()
    .will(throws(std::bad_alloc()));
    auto processer = ComputeTaskInfoProcesser(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processer.Run());
    MOCKER_CPP(&std::vector<TempT>::reserve).reset();
}
