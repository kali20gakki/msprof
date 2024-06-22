/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : soc_processor.cpp
 * Description        : soc_processor UT
 * Author             : msprof team
 * Creation Date      : 2024/3/1
 * *****************************************************************************
 */
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/viewer/database/finals/soc_processor.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Utils;
using namespace Analysis::Parser;
namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./soc_path";
const std::string DB_PATH = File::PathJoin({BASE_PATH, "msprof.db"});
const std::string DEVICE = "device_0";
const std::string DB_NAME = "soc_profiler.db";
const std::string PROF_PATH_A = File::PathJoin({BASE_PATH, "./PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string PROF_PATH_B = File::PathJoin({BASE_PATH, "./PROF_000001_20231125090304037_02333394DFSFSQCD"});
const std::set<std::string> PROF_PATHS = {PROF_PATH_A, PROF_PATH_B};
const std::string TABLE_NAME = "InterSoc";

using OriDataFormat = std::vector<std::tuple<uint32_t, uint32_t, double>>;
OriDataFormat DATA_A{{0, 0, 236368325745670},
                     {0, 0, 236368325747550}};
OriDataFormat DATA_B{{0, 0, 236365909133170},
                     {0, 0, 236366032012930}};
}
class SocProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_B));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE, SQLITE})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE, SQLITE})));
        CreateSocMetricData(File::PathJoin({PROF_PATH_A, DEVICE, SQLITE, DB_NAME}), DATA_A);
        CreateSocMetricData(File::PathJoin({PROF_PATH_B, DEVICE, SQLITE, DB_NAME}), DATA_B);
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069323851824"},
            {"endCollectionTimeEnd", "1701069338041681"},
            {"startClockMonotonicRaw", "36470610791630"},
        };
        MOCKER_CPP(&Analysis::Parser::Environment::Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }
    virtual void TearDown()
    {
        MOCKER_CPP(&Analysis::Parser::Environment::Context::GetInfoByDeviceId).reset();
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
    }
    static void CreateSocMetricData(const std::string& dbPath, OriDataFormat data)
    {
        std::shared_ptr<SocProfilerDB> database;
        MAKE_SHARED0_RETURN_VOID(database, SocProfilerDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

TEST_F(SocProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    auto processor = SocProcessor(DB_PATH, PROF_PATHS);
    EXPECT_TRUE(processor.Run());
}

TEST_F(SocProcessorUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH_A, DEVICE, SQLITE, DB_NAME});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    auto processor = SocProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
}

TEST_F(SocProcessorUTest, TestRunShouldReturnFalseWhenCreateTableFailed)
{
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable).stubs().will(returnValue(false));
    auto processor = SocProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable).reset();
}

TEST_F(SocProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    auto processor = SocProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Utils::File::Check).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(SocProcessorUTest, TestRunShouldReturnFalseWhenInsertDataFailed)
{
    auto id{TableColumn("Id", "INTEGER")};
    auto name{TableColumn("Name", "INTEGER")};
    std::vector<TableColumn> cols{id, name};
    auto processor = SocProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols).stubs().will(returnValue(cols));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols).reset();
}

TEST_F(SocProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    std::vector<std::string> deviceList = {File::PathJoin({BASE_PATH, "test", "device_1"})};
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).stubs().will(returnValue(deviceList));
    auto processor = SocProcessor(DB_PATH, {File::PathJoin({BASE_PATH, "test"})});
    EXPECT_TRUE(processor.Run());
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}