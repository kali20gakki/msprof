/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_module_mem_processor_utest.cpp
 * Description        : npu_module_mem_processor UT
 * Author             : msprof team
 * Creation Date      : 2024/1/4
 * *****************************************************************************
 */
#include <algorithm>
#include <vector>
#include <set>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/viewer/database/finals/npu_module_mem_processor.h"
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
const std::string NPU_MODULE_MEM_PATH = "./npu_module_mem_path";
const std::string DB_PATH = File::PathJoin({NPU_MODULE_MEM_PATH, "report.db"});
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "npu_module_mem.db";
const std::string PROF_PATH_A = File::PathJoin({NPU_MODULE_MEM_PATH,
                                                "./PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string PROF_PATH_B = File::PathJoin({NPU_MODULE_MEM_PATH,
                                                "./PROF_000001_20231125090304037_012333MBJNQLKJ"});
const std::set<std::string> PROF_PATHS = {PROF_PATH_A, PROF_PATH_B};
const std::string TABLE_NAME = "NpuModuleMem";

using OriDataFormat = std::vector<std::tuple<uint32_t, double, uint64_t, std::string>>;

using ProcessedDataFormat = std::vector<std::tuple<uint32_t, std::string, double, uint16_t>>;

const OriDataFormat DATA_A{{0, 48673240, 0, "NPU:1"}, {7, 48673240, 20025344, "NPU:1"}};
const OriDataFormat DATA_B{{30, 623107876, 0, "NPU:1"}, {33, 623107876, 58720256, "NPU:1"}};
}

class NpuModuleMemProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        EXPECT_TRUE(File::CreateDir(NPU_MODULE_MEM_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_B));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE})));
        CreateNpuModuleMem(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_A);
        CreateNpuModuleMem(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_B);
        MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo)
            .stubs()
            .will(returnValue(true));
        MOCKER_CPP(&Analysis::Parser::Environment::Context::GetSyscntConversionParams)
            .stubs()
            .will(returnValue(true));
    }
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(NPU_MODULE_MEM_PATH, DEPTH));
        MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo).reset();
        MOCKER_CPP(&Analysis::Parser::Environment::Context::GetSyscntConversionParams).reset();
    }
    static void CreateNpuModuleMem(const std::string &dbPath, OriDataFormat data)
    {
        std::shared_ptr<NpuModuleMemDB> database;
        MAKE_SHARED0_RETURN_VOID(database, NpuModuleMemDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

TEST_F(NpuModuleMemProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    auto processor = NpuModuleMemProcessor(DB_PATH, PROF_PATHS);
    EXPECT_TRUE(processor.Run());
}

TEST_F(NpuModuleMemProcessorUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    dbPath = File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE, DB_SUFFIX});
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    auto processor = NpuModuleMemProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
}

TEST_F(NpuModuleMemProcessorUTest, TestRunShouldReturnFalseWhenCreateTableFailed)
{
    auto processor = NpuModuleMemProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable).reset();
}

TEST_F(NpuModuleMemProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    auto processor = NpuModuleMemProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Utils::File::Check)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(NpuModuleMemProcessorUTest, TestRunShouldReturnFalseWhenInsertDataFailed)
{
    auto id{TableColumn("Id", "INTEGER")};
    auto name{TableColumn("Name", "INTEGER")};
    std::vector<TableColumn> cols{id, name};
    auto processor = NpuModuleMemProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols)
    .stubs()
    .will(returnValue(cols));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols).reset();
}

TEST_F(NpuModuleMemProcessorUTest, TestRunShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using TempT = std::tuple<uint32_t, uint64_t, double, uint16_t>;
    MOCKER_CPP(&std::vector<TempT>::reserve)
    .stubs()
    .will(throws(std::bad_alloc()));
    auto processor = NpuModuleMemProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&std::vector<TempT>::reserve).reset();
}

TEST_F(NpuModuleMemProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    std::vector<std::string> deviceList = {File::PathJoin({NPU_MODULE_MEM_PATH, "test", "device_1"})};
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix)
    .stubs()
    .will(returnValue(deviceList));
    auto processor = NpuModuleMemProcessor(DB_PATH, {File::PathJoin({NPU_MODULE_MEM_PATH, "test"})});
    EXPECT_TRUE(processor.Run());
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}
