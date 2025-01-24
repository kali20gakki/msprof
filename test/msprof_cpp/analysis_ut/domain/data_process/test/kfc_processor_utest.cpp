/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccl_statistic_processor_utest.cpp.cpp
 * Description        : data_process HcclStatisticProcessor UT
 * Author             : msprof team
 * Creation Date      : 2024/08/17
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "analysis/csrc/domain/data_process/ai_task/kfc_task_processor.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;
using namespace Analysis::Viewer::Database;

const std::string BASE_PATH = "./kfc_info";
const std::string DEVICE_SUFFIX = "device_0";
const std::string SQLITE_SUFFIX = "sqlite";
const std::string PROF_PATH_A = File::PathJoin({BASE_PATH, "PROF_0"});
const std::string PROF_PATH_B = File::PathJoin({BASE_PATH, "PROF_1"});
const std::string DB_SUFFIX = "kfc_info.db";
const std::string COMM_TABLE_NAME = "KfcCommTurn";
const std::string COMPUTE_TABLE_NAME = "KfcComputeTurn";
const std::set<std::string> PROF_PATHS = {PROF_PATH_A, PROF_PATH_B};

using OriKfcCommData = std::vector<std::tuple<uint16_t, uint16_t, uint32_t, uint16_t, uint16_t, uint64_t, uint64_t,
uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
// "device_id", "stream_id", "task_id", "compute_turn", "current_turn", "wait_compute_start_time",
// "compute_start_time", "compute_exe_end_time"
using OriKfcComputeData = std::vector<std::tuple<uint16_t, uint16_t, uint32_t, uint16_t, uint16_t, uint64_t, uint64_t,
uint64_t>>;


const OriKfcCommData KFC_COMM_DATA = {
    {0, 12, 22, 1, 0, 2955608765089, 2955608765983, 2955608924505, 2955608937462, 2955608924950, 2955609324419,
     2955612508700},
    {0, 12, 22, 1, 0, 2955608765089, 2955608765983, 2955608924505, 2955608937462, 2955608924950, 2955609324419,
     2955612508700}
};

const OriKfcComputeData KFC_COMPUTE_DATA = {
    {0, 12, 22, 1, 0, 2955608765089, 2955608765983, 2955608924505},
    {0, 12, 22, 1, 0, 2955608765089, 2955608765983, 2955608924505}
};

class KfcProcessorUtest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(BASE_PATH)) {
            File::RemoveDir(BASE_PATH, 0);
        }
        EXPECT_TRUE(File::CreateDir(BASE_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_B));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX})));
        CreateKfcCommData(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), KFC_COMM_DATA);
        CreateKfcCommData(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), KFC_COMM_DATA);

        CreateKfcComputeData(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), KFC_COMPUTE_DATA);
        CreateKfcComputeData(File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX}), KFC_COMPUTE_DATA);
    }

    static void TearDownTestCase()
    {
        Context::GetInstance().Clear();
        EXPECT_TRUE(File::RemoveDir(BASE_PATH, 0));
    }

    virtual void SetUp()
    {
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069324370978"},
            {"endCollectionTimeEnd", "1701069338159976"},
            {"startClockMonotonicRaw", "36471129942580"},
            {"pid", "10"},
            {"hostCntvct", "65177261204177"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
            {"DeviceInfo", {{{"hwts_frequency", "100.000000"}}}},
            {"hostMonotonic", "651599377155020"},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }

    virtual void TearDown()
    {
        MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
    }

    static void CreateKfcCommData(std::string dbPath, OriKfcCommData data)
    {
        std::shared_ptr<KfcInfo> database;
        MAKE_SHARED0_RETURN_VOID(database, KfcInfo);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(COMM_TABLE_NAME);
        dbRunner->CreateTable(COMM_TABLE_NAME, cols);
        dbRunner->InsertData(COMM_TABLE_NAME, data);
    }

    static void CreateKfcComputeData(std::string dbPath, OriKfcComputeData data)
    {
        std::shared_ptr<KfcInfo> database;
        MAKE_SHARED0_RETURN_VOID(database, KfcInfo);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(COMPUTE_TABLE_NAME);
        dbRunner->CreateTable(COMPUTE_TABLE_NAME, cols);
        dbRunner->InsertData(COMPUTE_TABLE_NAME, data);
    }
};

TEST_F(KfcProcessorUtest, TestRunShouldReturnTrueWhenProcessorRunSuccess)
{
    for (auto path: PROF_PATHS) {
        auto processor = KfcTaskProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
}

TEST_F(KfcProcessorUtest, TestRunShouldReturnTRUEWhenSourceTableNotExist)
{
    auto dbPath = File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX});
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(COMM_TABLE_NAME);
    dbRunner->DropTable(COMPUTE_TABLE_NAME);
    dbPath = File::PathJoin({PROF_PATH_B, DEVICE_SUFFIX, SQLITE_SUFFIX, DB_SUFFIX});
    MAKE_SHARED0_NO_OPERATION(dbRunner, DBRunner, dbPath);
    dbRunner->DropTable(COMM_TABLE_NAME);
    dbRunner->DropTable(COMPUTE_TABLE_NAME);
    for (auto path: PROF_PATHS) {
        auto processor = KfcTaskProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
}

TEST_F(KfcProcessorUtest, TestRunShouldReturnFalseWhenCheckPathNotExist)
{
    MOCKER_CPP(&Analysis::Utils::File::Exist).stubs().will(returnValue(false));
    for (auto path: PROF_PATHS) {
        auto processor = KfcTaskProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
    MOCKER_CPP(&Analysis::Utils::File::Exist).reset();
}

TEST_F(KfcProcessorUtest, TestRunShouldReturnTrueWhenNoDb)
{
    std::vector<std::string> deviceList = {File::PathJoin({BASE_PATH, "test", "device_1"})};
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).stubs().will(returnValue(deviceList));
    auto processor = KfcTaskProcessor({File::PathJoin({BASE_PATH, "test"})});
    auto dataInventory = DataInventory();
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    MOCKER_CPP(&Utils::File::GetFilesWithPrefix).reset();
}

TEST_F(KfcProcessorUtest, TestRunShouldReturnFalseWhenConstructDBRunnerFailed)
{
    MOCKER_CPP(&DBInfo::ConstructDBRunner).stubs().will(returnValue(false));
    for (auto path: PROF_PATHS) {
        auto processor = KfcTaskProcessor(path);
        auto dataInventory = DataInventory();
        EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_COMM_STATISTIC));
    }
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
}