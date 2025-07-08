/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : acc_pmu_processor_utest.cpp
 * Description        : acc_pmu_processor UT
 * Author             : msprof team
 * Creation Date      : 2024/8/12
 * *****************************************************************************
 */
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/data_process/system/acc_pmu_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain::Environment;
namespace {
const int DEPTH = 0;
const std::string BASE_PATH = "./acc_path";
const std::string DEVICE = "device_0";
const std::string DB_NAME = "acc_pmu.db";
const std::string PROF_PATH_A = File::PathJoin({BASE_PATH, "./PROF_0"});
const std::string TABLE_NAME = "AccPmu";
OriAccPmuData ACC_PMU_DATA{{1, 0, 0, 0, 0, 236368325745670}, {5, 0, 0, 0, 0, 236368325747550}};
}

class AccPmuProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE, SQLITE})));
        CreateAccPmuMetricData(File::PathJoin({PROF_PATH_A, DEVICE, SQLITE, DB_NAME}), ACC_PMU_DATA);
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, DEPTH));
    }
    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
    static void CreateAccPmuMetricData(const std::string& dbPath, OriAccPmuData data)
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

TEST_F(AccPmuProcessorUTest, ShouldReturnTrueWhenProcessRunSuccess)
{
    DataInventory dataInventory;
    auto processor = AccPmuProcessor(PROF_PATH_A);
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069324370978"},
        {"endCollectionTimeEnd", "1701069338159976"},
        {"startClockMonotonicRaw", "36471129942580"},
        {"pid", "10"},
        {"hostCntvct", "65177261204177"},
        {"CPU", {{{"Frequency", "100.000000"}}}},
        {"hostMonotonic", "651599377155020"},
    };
    MOCKER_CPP(&Analysis::Domain::Environment::Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_ACC_PMU));
    auto res = dataInventory.GetPtr<std::vector<AccPmuData>>();
    EXPECT_EQ(2ul, res->size());
}

TEST_F(AccPmuProcessorUTest, ShouldReturnFalseWhenCheckFailed)
{
    DataInventory dataInventory;
    auto processor = AccPmuProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_ACC_PMU));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(INVALID_DEVICE_ID));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_ACC_PMU));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Utils::FileReader::Check).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_ACC_PMU));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    MOCKER_CPP(&Utils::FileReader::Check).reset();
}

TEST_F(AccPmuProcessorUTest, TestRunShouldReturnFalseWhenProcessFailed)
{
    DataInventory dataInventory;
    auto processor = AccPmuProcessor(PROF_PATH_A);
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<AccPmuData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_ACC_PMU));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<AccPmuData>).reset();
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_ACC_PMU));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
    MOCKER_CPP(&AccPmuProcessor::LoadData).stubs().will(returnValue({}));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_ACC_PMU));
    MOCKER_CPP(&AccPmuProcessor::LoadData).reset();
}

TEST_F(AccPmuProcessorUTest, TestLoadDataShouldReturnOriDataWhenDbRunnerIsNull)
{
    auto processor = AccPmuProcessor(PROF_PATH_A);
    DBInfo accPmuDB("acc_pmu.db", "AccPmu");
    accPmuDB.dbRunner = nullptr;
    EXPECT_EQ(processor.LoadData(accPmuDB, "").size(), 0ul);
}

TEST_F(AccPmuProcessorUTest, ShouldReturnFalseWhenReserveException)
{
    DataInventory dataInventory;
    auto processor = AccPmuProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&std::vector<AccPmuData>::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_ACC_PMU));
}