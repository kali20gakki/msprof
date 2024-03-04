/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : acc_pmu_processor_utest.cpp
 * Description        : acc_pmu_processor UT
 * Author             : msprof team
 * Creation Date      : 2024/3/1
 * *****************************************************************************
 */
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/viewer/database/finals/acc_pmu_processor.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Utils;
using namespace Analysis::Parser;
namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./acc_path";
const std::string DB_PATH = File::PathJoin({BASE_PATH, "report.db"});
const std::string DEVICE = "device_0";
const std::string DB_NAME = "acc_pmu.db";
const std::string PROF_PATH_A = File::PathJoin({BASE_PATH, "./PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string PROF_PATH_B = File::PathJoin({BASE_PATH, "./PROF_000001_20231125090304037_02333394DFSFSQCD"});
const std::set<std::string> PROF_PATHS = {PROF_PATH_A, PROF_PATH_B};
const std::string TABLE_NAME = "AccPmu";

using OriDataFormat = std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, double>>;
OriDataFormat DATA_A{{1, 0, 0, 0, 0, 236368325745670},
                     {5, 0, 0, 0, 0, 236368325747550}};
OriDataFormat DATA_B{{2, 0, 0, 0, 0, 236368325745670},
                     {6, 0, 0, 0, 0, 236368325747550}};
}

class AccPmuProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE, SQLITE})));
        CreateAccPmuMetricData(File::PathJoin({PROF_PATH_A, DEVICE, SQLITE, DB_NAME}), DATA_A);
        CreateAccPmuMetricData(File::PathJoin({PROF_PATH_B, DEVICE, SQLITE, DB_NAME}), DATA_B);
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
    static void CreateAccPmuMetricData(const std::string& dbPath, OriDataFormat data)
    {
        std::shared_ptr<AccPmuDB> database;
        MAKE_SHARED0_RETURN_VOID(database, AccPmuDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

TEST_F(AccPmuProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    auto processor = AccPmuProcessor(DB_PATH, PROF_PATHS);
    EXPECT_TRUE(processor.Run());
}

TEST_F(AccPmuProcessorUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH_A, DEVICE, SQLITE, DB_NAME});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    auto processor = AccPmuProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
}

TEST_F(AccPmuProcessorUTest, TestRunShouldReturnFalseWhenCreateTableFailed)
{
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable).stubs().will(returnValue(false));
    auto processor = AccPmuProcessor(DB_PATH, PROF_PATHS);
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable).reset();
}

TEST_F(AccPmuProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    auto processor = AccPmuProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Utils::File::Check).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(AccPmuProcessorUTest, TestRunShouldReturnFalseWhenInsertDataFailed)
{
    auto id{TableColumn("Id", "INTEGER")};
    auto name{TableColumn("Name", "INTEGER")};
    std::vector<TableColumn> cols{id, name};
    auto processor = AccPmuProcessor(DB_PATH, PROF_PATHS);
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols).stubs().will(returnValue(cols));
    EXPECT_FALSE(processor.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::Database::GetTableCols).reset();
}

TEST_F(AccPmuProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    std::vector<std::string> deviceList = {File::PathJoin({BASE_PATH, "test", "device_1"})};
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).stubs().will(returnValue(deviceList));
    auto processor = AccPmuProcessor(DB_PATH, {File::PathJoin({BASE_PATH, "test"})});
    EXPECT_TRUE(processor.Run());
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}