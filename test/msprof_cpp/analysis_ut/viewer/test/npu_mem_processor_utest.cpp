/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_mem_processor_utest.cpp
 * Description        : npu_mem_processor UT
 * Author             : msprof team
 * Creation Date      : 2024/2/19
 * *****************************************************************************
 */
#include <algorithm>
#include <vector>
#include <set>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/viewer/database/finals/npu_mem_processor.h"
#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Utils;
using namespace Analysis::Parser;
using namespace Parser::Environment;
namespace {
const int DEPTH = 0;
const std::string NPU_MEM_PATH = "./npu_mem_path";
const std::string DB_PATH = File::PathJoin({NPU_MEM_PATH, "report.db"});
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "npu_mem.db";
const std::string PROF_PATH_A = File::PathJoin({NPU_MEM_PATH,
                                                "./PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string PROF_PATH_B = File::PathJoin({NPU_MEM_PATH,
                                                "./PROF_000001_20231125090304037_012333MBJNQLKJ"});
const std::set<std::string> PROF_PATHS = {PROF_PATH_A, PROF_PATH_B};
const std::string TABLE_NAME = "NpuMem";

using OriDataFormat = std::vector<std::tuple<std::string, uint64_t, uint64_t, double, uint64_t>>;

using ProcessedDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint16_t>>;

const OriDataFormat DATA_A{{"0", 0, 47243669504, 4, 47243669504},
                           {"1", 0, 47414005760, 212, 47414005760},
                           {"abc", 0, 47414005780, 300, 47414005780}};
const OriDataFormat DATA_B{{"0", 0, 47243669504, 19986, 47243669504},
                           {"1", 0, 47415054336, 20185, 47415054336}};
}

class NpuMemProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        EXPECT_TRUE(File::CreateDir(NPU_MEM_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_B));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE})));
        CreateNpuMem(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_A);
        CreateNpuMem(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_B);
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069323851824"},
            {"endCollectionTimeEnd", "1701069338041681"},
            {"startClockMonotonicRaw", "36470610791630"},
            {"clock_monotonic_raw", "36471130547330"},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(NPU_MEM_PATH, DEPTH));
        GlobalMockObject::verify();
    }
    static void CreateNpuMem(const std::string &dbPath, OriDataFormat data)
    {
        std::shared_ptr<NpuMemDB> database;
        MAKE_SHARED0_RETURN_VOID(database, NpuMemDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

TEST_F(NpuMemProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    auto processor = NpuMemProcessor(DB_PATH, PROF_PATHS);
    EXPECT_TRUE(processor.Run());
}

TEST_F(NpuMemProcessorUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    dbPath = File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    auto processor = NpuMemProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
}

TEST_F(NpuMemProcessorUTest, TestRunShouldReturnFalseWhenCreateTableFailed)
{
    auto processor = NpuMemProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable).reset();
}

TEST_F(NpuMemProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    auto processor = NpuMemProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Utils::File::Check)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(NpuMemProcessorUTest, TestRunShouldReturnFalseWhenInsertDataFailed)
{
    auto id{TableColumn("Id", "INTEGER")};
    auto name{TableColumn("Name", "INTEGER")};
    std::vector<TableColumn> cols{id, name};
    auto processor = NpuMemProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols)
    .stubs()
    .will(returnValue(cols));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols).reset();
}

TEST_F(NpuMemProcessorUTest, TestRunShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    MOCKER_CPP(&ProcessedDataFormat::reserve)
    .stubs()
    .will(throws(std::bad_alloc()));
    auto processor = NpuMemProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&ProcessedDataFormat::reserve).reset();
}

TEST_F(NpuMemProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    std::vector<std::string> deviceList = {File::PathJoin({NPU_MEM_PATH, "test", "device_1"})};
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix)
    .stubs()
    .will(returnValue(deviceList));
    auto processor = NpuMemProcessor(DB_PATH, {File::PathJoin({NPU_MEM_PATH, "test"})});
    EXPECT_TRUE(processor.Run());
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}