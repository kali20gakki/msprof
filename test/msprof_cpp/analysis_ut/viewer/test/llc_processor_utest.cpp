/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : llc_processor_utest.cpp
 * Description        : llc_processor UT
 * Author             : msprof team
 * Creation Date      : 2024/2/26
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/viewer/database/finals/llc_processor.h"
#include "analysis/csrc/infrastructure/utils/thread_pool.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/services/environment/context.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Utils;
using namespace Analysis::Domain::Environment;

namespace {
    const std::string LLC_PATH = "./llc_path";
    const std::string DEVICE_DIR = "device_0";
    const std::string DB_NAME = "llc.db";
    const std::string TABLE_NAME = "LLCMetrics";
    const std::string DB_PATH = File::PathJoin({LLC_PATH, "msprof.db"});
    const std::string PROF_PATH_A = File::PathJoin({LLC_PATH,
                                                    "./PROF_000001_20231125090304037_02333394MBJNQLKJ"});
    const std::string PROF_PATH_B = File::PathJoin({LLC_PATH,
                                                    "./PROF_000001_20231125090304037_012333MBJNQLKJ"});
    const std::set<std::string> PROF_PATHS = {PROF_PATH_A, PROF_PATH_B};

    const LLCProcessor::OriDataFormat DATA_A {
        {0, 0, 8168000, 0.71, 0},
        {0, 0, 18379000, 0.72, 1000.11},
    };
    const LLCProcessor::OriDataFormat DATA_B {
        {0, 0, 8168010, 0.73, 0},
        {0, 0, 18379010, 0.74, 1200.65},
    };
}

class LLCProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(LLC_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_B));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_DIR})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_DIR, SQLITE})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_DIR})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_DIR, SQLITE})));
        CreateLLCDB(File::PathJoin({PROF_PATH_A, DEVICE_DIR, SQLITE, DB_NAME}), DATA_A);
        CreateLLCDB(File::PathJoin({PROF_PATH_B, DEVICE_DIR, SQLITE, DB_NAME}), DATA_B);
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069323851824"},
            {"endCollectionTimeEnd", "1701069338041681"},
            {"startClockMonotonicRaw", "36470610791630"},
            {"hostMonotonic", "36471130547330"},
            {"devMonotonic", "36471130547330"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
            {"llc_profiling", "read"},
            {"platform_version", "5"},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(LLC_PATH, 0));
        GlobalMockObject::verify();
    }

    static void CreateLLCDB(const std::string &dbPath, LLCProcessor::OriDataFormat data)
    {
        std::shared_ptr<LLCDB> database;
        MAKE_SHARED0_RETURN_VOID(database, LLCDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

TEST_F(LLCProcessorUTest, TestRunShouldReturnTrueWhenProcessorSuccess)
{
    auto processor = LLCProcessor(DB_PATH, PROF_PATHS);
    EXPECT_TRUE(processor.Run());
}

TEST_F(LLCProcessorUTest, TestRunShouldReturnFalseWhenCreateTableFailed)
{
    auto processor = LLCProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable).stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable).reset();
}

TEST_F(LLCProcessorUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH_A, DEVICE_DIR, SQLITE, DB_NAME});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    dbPath = File::PathJoin({PROF_PATH_B, DEVICE_DIR, SQLITE, DB_NAME});
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    auto processor = LLCProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
}

TEST_F(LLCProcessorUTest, TestRunShouldReturnFalseWhenLLCProfilingInvalid)
{
    auto processor = LLCProcessor(DB_PATH, PROF_PATHS);
    std::string llcProfiling = "readwrite";
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetLLCProfiling).stubs()
        .will(returnValue(llcProfiling));
    EXPECT_FALSE(processor.Run());
    llcProfiling = "read";
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetLLCProfiling).stubs()
        .will(returnValue(llcProfiling));
}

TEST_F(LLCProcessorUTest, TestRunShouldReturnFalseWhenGetTimeRecordFailed)
{
    auto processor = LLCProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetProfTimeRecordInfo).stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
}

TEST_F(LLCProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    auto processor = LLCProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Utils::File::Check).stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(LLCProcessorUTest, TestRunShouldReturnTrueWhenChipV1_1_0)
{
    auto processor = LLCProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Domain::Environment::Context::IsFirstChipV1).stubs()
        .will(returnValue(true));
    EXPECT_TRUE(processor.Run());
    MOCKER_CPP(&Analysis::Domain::Environment::Context::IsFirstChipV1).reset();
}

TEST_F(LLCProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    std::vector<std::string> deviceList = {File::PathJoin({LLC_PATH, "test", "device_1"})};
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).stubs()
        .will(returnValue(deviceList));
    auto processor = LLCProcessor(DB_PATH, {File::PathJoin({LLC_PATH, "test"})});
    EXPECT_TRUE(processor.Run());
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}
