/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : msproftx_device_processor_utest.cpp
 * Description        : msproftx_device_processor UT
 * Author             : msprof team
 * Creation Date      : 2024/8/6
 * *****************************************************************************
 */
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/data_process/ai_task/msproftx_device_processor.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Viewer::Database;
using namespace Analysis::Domain;
using namespace Analysis::Utils;
using namespace Analysis::Parser;
using namespace Analysis::Parser::Environment;
namespace {
const int DEPTH = 0;
const std::string DEVICE_TX_PATH = "./msprof_tx_device";
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "step_trace.db";
const std::string PROF_PATH_A = File::PathJoin({DEVICE_TX_PATH, "./PROF_0"});
const std::string TABLE_NAME = "StepTrace";

using DbDataType = std::vector<std::tuple<uint32_t, uint32_t, double, uint32_t, uint32_t, uint32_t>>;

DbDataType DATA_A{{0, 4294967295, 26248923229230, 2, 10, 11},
                  {0, 4294967295, 26248923229240, 2, 11, 11}};
}

class MsprofTxDeviceProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        if (File::Check(DEVICE_TX_PATH)) {
            File::RemoveDir(DEVICE_TX_PATH, DEPTH);
        }
        EXPECT_TRUE(File::CreateDir(DEVICE_TX_PATH));
        EXPECT_TRUE(File::CreateDir(PROF_PATH_A));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE})));
        CreateStepTrace(File::PathJoin({PROF_PATH_A, DEVICE_SUFFIX, SQLITE, DB_SUFFIX}), DATA_A);
    }
    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(DEVICE_TX_PATH, DEPTH));
    }
    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
    static void CreateStepTrace(const std::string& dbPath, DbDataType& data)
    {
        std::shared_ptr<StepTraceDB> database;
        MAKE_SHARED0_RETURN_VOID(database, StepTraceDB);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VOID(dbRunner, DBRunner, dbPath);
        auto cols = database->GetTableCols(TABLE_NAME);
        dbRunner->CreateTable(TABLE_NAME, cols);
        dbRunner->InsertData(TABLE_NAME, data);
    }
};

TEST_F(MsprofTxDeviceProcessorUTest, ShouldReturnTrueWhenProcessorRunSuccess)
{
    DataInventory dataInventory;
    auto processor = MsprofTxDeviceProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    auto res = dataInventory.GetPtr<std::vector<MsprofTxDeviceData>>();
    EXPECT_EQ(2ul, res->size());
}

TEST_F(MsprofTxDeviceProcessorUTest, ShouldReturnFalseWhenCheckFailed)
{
    DataInventory dataInventory;
    auto processor = MsprofTxDeviceProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(HOST_ID));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Utils::FileReader::Check).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    MOCKER_CPP(&Utils::FileReader::Check).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();
}

TEST_F(MsprofTxDeviceProcessorUTest, ShouldReturnFalseWhenReserveException)
{
    DataInventory dataInventory;
    auto processor = MsprofTxDeviceProcessor(PROF_PATH_A);
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(true));
    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(true));
    MOCKER_CPP(&std::vector<MsprofTxDeviceData>::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_TASK));
}
