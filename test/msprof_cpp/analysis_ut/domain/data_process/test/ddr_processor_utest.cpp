/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ddr_processor_utest.cpp
 * Description        : ddr_processor UT
 * Author             : msprof team
 * Creation Date      : 2024/08/10
 * *****************************************************************************
 */
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/data_process/system/ddr_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/utils/thread_pool.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Domain;
using namespace Analysis::Utils;
using namespace Domain::Environment;
namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./ddr_path";
const std::string DB_PATH = File::PathJoin({BASE_PATH, "msprof.db"});
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "ddr.db";
const std::string SQLITE_SUFFIX = "sqlite";
const std::string PROF_PATH_A = File::PathJoin({BASE_PATH, "./PROF_000001_20231125090304037_02333394MBJNQLKJ"});
const std::string PROF_PATH_B = File::PathJoin({BASE_PATH, "./PROF_000001_20231125090304037_012333MBJNQLKJ"});
const std::set<std::string> PROF_PATHS = {PROF_PATH_A, PROF_PATH_B};
const std::string TABLE_NAME = "DDRMetricData";
using OriDDRDataFormat = std::vector<std::tuple<uint32_t, uint32_t, double, double, double, double, double>>;
using ProcessedFormat = std::vector<DDRData>;

OriDDRDataFormat DATA_A{{0, 0, 88698103395630, 528.971354166667, 122.0703125, 0, 0},
                     {0, 0, 88698124060630, 94.6377196497702, 974.332568087648, 0, 0}};
OriDDRDataFormat DATA_B{{0, 0, 88698144030630, 20.3216201255008, 26.076712725338, 0, 0},
                     {0, 0, 88698163950630, 21.5430815057103, 25.6335400194528, 0, 0}};
}

class DDRProcessorUTest : public testing::Test {
protected:
    virtual void SetUp()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_B));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX})));
        CreateDDRMetricData(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), DATA_A);
        CreateDDRMetricData(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), DATA_B);
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069323851824"},
            {"endCollectionTimeEnd", "1701069338041681"},
            {"startClockMonotonicRaw", "36470610791630"},
            {"hostMonotonic", "36471130547330"},
            {"devMonotonic", "36471130547330"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }
    virtual void TearDown()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
        MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
    }
    static void CreateDDRMetricData(const std::string& dbPath, OriDDRDataFormat data)
    {
        std::shared_ptr<DDRDB> database;
        MAKE_SHARED0_RETURN_VOID(database, DDRDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

TEST_F(DDRProcessorUTest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    for (auto path: PROF_PATHS) {
        auto processor = DDRProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_DDR));
    }
}

TEST_F(DDRProcessorUTest, TestRunShouldReturnFalseWhenProcessorFail)
{
    auto processor = DDRProcessor(PROF_PATH_A);
    DataInventory dataInventory;
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_DDR));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();

    MOCKER_CPP(&Context::GetClockMonotonicRaw).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_DDR));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).reset();

    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(UINT16_MAX));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_DDR));
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).reset();

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<DDRData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_DDR));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<DDRData>).reset();
}

TEST_F(DDRProcessorUTest, TestRunShouldReturnFalseWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    dbPath = File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX});
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(TABLE_NAME);
    for (auto path: PROF_PATHS) {
        auto processor = DDRProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_DDR));
    }
}

TEST_F(DDRProcessorUTest, TestRunShouldReturnFalseWhenCheckPathFailed)
{
    MOCKER_CPP(&Analysis::Utils::File::Check).stubs().will(returnValue(false));
    for (auto path: PROF_PATHS) {
        auto processor = DDRProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_DDR));
    }
    MOCKER_CPP(&Analysis::Utils::File::Check).reset();
}

TEST_F(DDRProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    std::vector<std::string> deviceList = {File::PathJoin({BASE_PATH, "test", "device_1"})};
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).stubs().will(returnValue(deviceList));
    auto processor = DDRProcessor({File::PathJoin({BASE_PATH, "test"})});
    auto dataInventory = DataInventory();
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_DDR));
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}

TEST_F(DDRProcessorUTest, TestRunShouldReturnFalseWhenReserveFailed)
{
    MOCKER_CPP(&ProcessedFormat::reserve).stubs().will(throws(std::bad_alloc()));
    for (auto path: PROF_PATHS) {
        auto processor = DDRProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_DDR));
    }
    MOCKER_CPP(&ProcessedFormat::reserve).reset();
}

TEST_F(DDRProcessorUTest, TestRunShouldReturnFalseWhenConstructDBRunnerFailed)
{
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    for (auto path: PROF_PATHS) {
        auto processor = DDRProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_DDR));
    }
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
}
