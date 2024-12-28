/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_processor_utest.cpp
 * Description        : task_processor UT
 * Author             : msprof team
 * Creation Date      : 2024/8/6
 * *****************************************************************************
 */
#include <algorithm>
#include <vector>
#include <set>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/data_process/ai_task/task_processor.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain;
using namespace Analysis::Utils;
using namespace Analysis::Parser;
namespace {
const int DEPTH = 0;
const std::string TASK_PATH = "./task_path";
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "ascend_task.db";
const std::string SQLITE_SUFFIX = "sqlite";
const std::string PROF_PATH_A = File::PathJoin({TASK_PATH, "./PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string TABLE_NAME = "AscendTask";
const std::string MEMCPY_INFO_TABLE_NAME = "MemcpyInfo";
// connection_id, data_size, memcpy_direction, max_size
using LoadMemcpyInfoFormat = std::vector<std::tuple<uint64_t, uint64_t, uint16_t, uint64_t>>;
// struct_type, level, thread_id, data_len, timestamp, data_size, memcpy_direction, max_size, connection_id
using MemcpyInfoFormat = std::vector<std::tuple<std::string, std::string, uint32_t, uint32_t, uint64_t,
                                       uint64_t, uint16_t, uint64_t, int64_t>>;
using DbDataType = std::vector<std::tuple<uint32_t, int32_t, int32_t, uint32_t, uint32_t, uint32_t, double, double,
        std::string, std::string, uint32_t>>;
const MemcpyInfoFormat MEMCPY_INFO_DATA = {
    {"memcpy_info", "runtime", 1534794, 24, 22101334093972, 8, 1, 67108864, 37},
    {"memcpy_info", "runtime", 1534794, 24, 22101334108563, 83886080, 1, 67108864, 181}
};
DbDataType DATA_A{{4294967295, -1, 37, 1, 3, 0, 8719911184665.1, 680.013671875, "UNKNOWN", "MIX_AIC", 1},
                  {4294967295, -1, 37, 2, 5, 0, 8719911184665.1, 680.013671875, "FFTS_PLUS", "Write Value", 2},
                  {4294967295, -1, 37, 3, 5, 0, 8719911182265.1, 680.013671875, "UNKNOWN", "11", 3},
                  {4294967295, -1, 37, 4, 5, 0, 8719911184665.1, 680.013671875, "UNKNOWN", "88", 4},
                  {4294967295, -1, 37, 5, 7, 0, 8719911184965.1, 680.013671875, "KERNEL_AICORE", "AI_CORE", 5},
                  {4294967295, -1, 37, 6, 4294967295, 0, 8719911185265.1, 680.013671875,
                   "MEMCPY_ASYNC", "PCIE_DMA_SQE", 37},
                  {4294967295, -1, 37, 7, 4294967295, 0, 8719911185565.1, 680.013671875,
                   "MEMCPY_ASYNC", "PCIE_DMA_SQE", 181},
                  {4294967295, -1, 37, 8, 4294967295, 0, 8719911186565.1, 680.013671875,
                   "MEMCPY_ASYNC", "PCIE_DMA_SQE", 181}
};
}

class TaskProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(TASK_PATH)) {
            File::RemoveDir(TASK_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(TASK_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, HOST})));
        EXPECT_TRUE(CreateRuntimeDB(File::PathJoin({PROF_PATH_A, HOST, SQLITE}), MEMCPY_INFO_DATA));
        CreateAscendTask(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), DATA_A);
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(TASK_PATH, DEPTH));
    }
    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
    static void CreateAscendTask(const std::string& dbPath, DbDataType& data)
    {
        std::shared_ptr<AscendTaskDB> database;
        MAKE_SHARED0_RETURN_VOID(database, AscendTaskDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
    static bool CreateRuntimeDB(const std::string& sqlitePath, MemcpyInfoFormat data)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));
        std::shared_ptr<RuntimeDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, RuntimeDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, File::PathJoin({sqlitePath, database->GetDBName()}));
        EXPECT_TRUE(dbRunner->CreateTable(MEMCPY_INFO_TABLE_NAME, database->GetTableCols(MEMCPY_INFO_TABLE_NAME)));
        EXPECT_TRUE(dbRunner->InsertData(MEMCPY_INFO_TABLE_NAME, data));
        return true;
    }
};

TEST_F(TaskProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    DataInventory dataInventory;
    auto processor = TaskProcessor(PROF_PATH_A);
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(true));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo).reset();
    auto memcpyInfoPtr = dataInventory.GetPtr<std::vector<MemcpyInfoData>>();
    ASSERT_NE(memcpyInfoPtr, nullptr);
    auto memcpyInfo = *memcpyInfoPtr;
    ASSERT_EQ(memcpyInfo.size(), 3);  // 存到数据中心的MemcpyInfoData的数量 3
    EXPECT_EQ(memcpyInfo[0].globalTaskId, 0);  // globalTaskId 0
    EXPECT_EQ(memcpyInfo[1].dataSize, 67108864);  // dataSize 67108864
    EXPECT_EQ(memcpyInfo[2].dataSize, 16777216);  // dataSize 16777216
    EXPECT_EQ(memcpyInfo[2].memcpyOperation, 1);  // memcpyOperation 1
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    DataInventory dataInventory;
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(true));
    MOCKER_CPP(&DBRunner::CheckTableExists).stubs().will(returnValue(false));
    auto processor = TaskProcessor(PROF_PATH_A);
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnTrueWhenCheckPathNotExists)
{
    DataInventory dataInventory;
    auto processor = TaskProcessor(PROF_PATH_A);
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(true));
    MOCKER_CPP(&Analysis::Utils::File::Exist)
    .stubs()
    .will(returnValue(false));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&Analysis::Utils::File::Exist).reset();
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo).reset();
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnFalseWhenReserveFailed)
{
    DataInventory dataInventory;
    auto processor = TaskProcessor(PROF_PATH_A);
    MOCKER_CPP(&std::vector<AscendTaskData>::reserve)
    .stubs()
    .will(throws(std::bad_alloc()));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&std::vector<AscendTaskData>::reserve).reset();
}

TEST_F(TaskProcessorUTest, TestRunShouldReturnTrueWhenMemcpyInfoNotExisted)  // 模拟不开--runtime-api
{
    MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo)
    .stubs()
    .will(returnValue(true));
    DataInventory dataInventory;
    auto processor = TaskProcessor(PROF_PATH_A);
    MOCKER_CPP(&DBRunner::CheckTableExists).stubs().will(returnValue(false)).then(returnValue(true));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
}