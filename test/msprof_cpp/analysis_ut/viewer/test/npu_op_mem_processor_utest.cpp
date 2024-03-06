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
#include "analysis/csrc/viewer/database/finals/npu_op_mem_processor.h"
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
const uint16_t STRING_NUM = 4;
const std::string NPU_OP_MEM_PATH = "./npu_op_mem_path";
const std::string DB_PATH = File::PathJoin({NPU_OP_MEM_PATH, "report.db"});
const std::string HOST_SUFFIX = "host";
const std::string DB_SUFFIX = "task_memory.db";
const std::string DB_HASH_SUFFIX = "ge_hash.db";
const std::string PROF_PATH_A = File::PathJoin({NPU_OP_MEM_PATH,
                                                "./PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string PROF_PATH_B = File::PathJoin({NPU_OP_MEM_PATH,
                                                "./PROF_000001_20231125090304037_012333MBJNQLKJ"});
const std::set<std::string> PROF_PATHS = {PROF_PATH_A, PROF_PATH_B};
const std::string TABLE_NAME = "NpuOpMemRaw";
const std::string HASH_TABLE_NAME = "GeHashInfo";

using OriDataFormat = std::vector<std::tuple<std::string, std::string, int64_t, double, uint32_t, uint64_t,
                                             uint64_t, uint32_t, uint32_t, std::string>>;

using ProcessedDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint32_t, uint64_t, uint64_t, uint64_t,
                                                   uint64_t, uint64_t, uint64_t, uint32_t>>;

using HashDataFormat = std::vector<std::tuple<std::string, std::string>>;

const OriDataFormat DATA_A{{"7891295173964629722", "20074680643584", 196608, 7686603804672, 2973237, 262144, 623706112,
                            10000, 6, "NPU:1"},
                           {"7891295173964629722", "20074680643584", -196608, 7686603809555, 2973237, 65536, 623706112,
                            10000, 6, "NPU:1"}};
const OriDataFormat DATA_B{{"4195562513624859856", "20074679136256", 65536, 7686620130320, 2968935, 196608, 623706112,
                            10000, 6, "NPU:1"},
                           {"6478531244988989337", "20074680287232", -65536, 7686620130452, 2968935, 131072, 623706112,
                            10000, 6, "NPU:1"}};
const HashDataFormat HASH_DATA{{"7891295173964629722", "npu"}, {"4195562513624859856", "Memcpy"},
                               {"6478531244988989337", "Notify_Record"}};
}

class NpuOpMemProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        EXPECT_TRUE(File::CreateDir(NPU_OP_MEM_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_B));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, HOST_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, HOST_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, HOST_SUFFIX, SQLITE})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, HOST_SUFFIX, SQLITE})));
        CreateNpuOpMem(File::PathJoin({PROF_PATH_A, HOST_SUFFIX, SQLITE, DB_SUFFIX}), DATA_A);
        CreateNpuOpMem(File::PathJoin({PROF_PATH_B, HOST_SUFFIX, SQLITE, DB_SUFFIX}), DATA_B);
        CreateHash(File::PathJoin({PROF_PATH_A, HOST_SUFFIX, SQLITE, DB_HASH_SUFFIX}), HASH_DATA);
        CreateHash(File::PathJoin({PROF_PATH_B, HOST_SUFFIX, SQLITE, DB_HASH_SUFFIX}), HASH_DATA);
        MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo)
            .stubs()
            .will(returnValue(true));
        MOCKER_CPP(&Analysis::Parser::Environment::Context::GetSyscntConversionParams)
            .stubs()
            .will(returnValue(true));
    }
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(NPU_OP_MEM_PATH, DEPTH));
        MOCKER_CPP(&Analysis::Parser::Environment::Context::GetProfTimeRecordInfo).reset();
        MOCKER_CPP(&Analysis::Parser::Environment::Context::GetSyscntConversionParams).reset();
    }
    static void CreateNpuOpMem(const std::string &dbPath, OriDataFormat data)
    {
        std::shared_ptr<TaskMemoryDB> database;
        MAKE_SHARED0_RETURN_VOID(database, TaskMemoryDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
    static void CreateHash(const std::string &dbPath, HashDataFormat data)
    {
        std::shared_ptr<HashDB> database;
        MAKE_SHARED0_RETURN_VOID(database, HashDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(HASH_TABLE_NAME);
        dbRunner->CreateTable(HASH_TABLE_NAME, cols);
        dbRunner->InsertData(HASH_TABLE_NAME, data);
    }
};

void CheckStringId(ProcessedDataFormat data)
{
    const uint16_t operatorNameIndex = 0;
    const uint16_t componentIndex = 8;
    std::set<uint64_t> stringIdsSet;
    std::vector<uint64_t> stringIds;
    for (auto item : data) {
        stringIdsSet.insert(std::get<operatorNameIndex>(item));
        stringIdsSet.insert(std::get<componentIndex>(item));
    }
    stringIds.assign(stringIdsSet.begin(), stringIdsSet.end());
    std::sort(stringIds.begin(), stringIds.end());
    for (int i = 0; i < STRING_NUM; ++i) {
        EXPECT_EQ(stringIds[i], i);
    }
}

TEST_F(NpuOpMemProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    std::shared_ptr<DBRunner> reportDBRunner;
    ProcessedDataFormat memResult;
    std::string sql{"SELECT * FROM " + TABLE_NAME_NPU_OP_MEM};
    MAKE_SHARED0_NO_OPERATION(reportDBRunner, DBRunner, DB_PATH);
    auto processor = NpuOpMemProcessor(DB_PATH, PROF_PATHS);
    EXPECT_TRUE(processor.Run());
    reportDBRunner->QueryData(sql, memResult);
    CheckStringId(memResult);
}

TEST_F(NpuOpMemProcessorUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH_A, HOST_SUFFIX, SQLITE, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    dbPath = File::PathJoin({PROF_PATH_B, HOST_SUFFIX, SQLITE, DB_SUFFIX});
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    auto processor = NpuOpMemProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
}

TEST_F(NpuOpMemProcessorUTest, TestRunShouldReturnFalseWhenCreateTableFailed)
{
    auto processor = NpuOpMemProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable).reset();
}

TEST_F(NpuOpMemProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    auto processor = NpuOpMemProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Utils::File::Check)
    .stubs()
    .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(NpuOpMemProcessorUTest, TestRunShouldReturnFalseWhenInsertDataFailed)
{
    auto id{TableColumn("Id", "INTEGER")};
    auto name{TableColumn("Name", "INTEGER")};
    std::vector<TableColumn> cols{id, name};
    auto processor = NpuOpMemProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols)
    .stubs()
    .will(returnValue(cols));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols).reset();
}

TEST_F(NpuOpMemProcessorUTest, TestRunShouldReturnFalseWhenReserveFailedThenDataIsEmpty)
{
    using TempT = std::tuple<uint64_t, uint64_t, uint32_t, uint64_t, uint64_t, uint64_t,
                             uint64_t, uint64_t, uint64_t, uint16_t>;
    MOCKER_CPP(&std::vector<TempT>::reserve)
    .stubs()
    .will(throws(std::bad_alloc()));
    auto processor = NpuOpMemProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&std::vector<TempT>::reserve).reset();
}

TEST_F(NpuOpMemProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    std::vector<std::string> deviceList = {File::PathJoin({NPU_OP_MEM_PATH, "test", "device_1"})};
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix)
    .stubs()
    .will(returnValue(deviceList));
    auto processor = NpuOpMemProcessor(DB_PATH, {File::PathJoin({NPU_OP_MEM_PATH, "test"})});
    EXPECT_TRUE(processor.Run());
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}