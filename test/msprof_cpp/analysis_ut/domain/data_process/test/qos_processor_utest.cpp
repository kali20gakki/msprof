/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : qos_processor_utest.cpp
 * Description        : QosProcessor UT
 * Author             : msprof team
 * Creation Date      : 2024/08/28
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/data_process/system/qos_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;

namespace {
const int DEPTH = 0;
const std::string QOS_DIR = "./qos";
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "qos.db";
const std::string PROF_DIR = File::PathJoin({QOS_DIR, "./PROF_0"});
const std::string TABLE_NAME = "QosBwData";
const OriQosData QOS_DATA = {
    {10332804861006.51, 217, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {10332804960128.49, 220, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {10332805060230.49, 220, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {10332805160232.488, 227, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {10332805260134.484, 238, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};
}

class QosProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        GlobalMockObject::verify();
        EXPECT_TRUE(File::CreateDir(QOS_DIR));
        EXPECT_TRUE(File::CreateDir(PROF_DIR));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_DIR, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_DIR, DEVICE_SUFFIX, SQLITE})));
        EXPECT_TRUE(CreateQosDB(File::PathJoin({PROF_DIR, DEVICE_SUFFIX, SQLITE, DB_SUFFIX})));
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

    static bool CreateQosDB(const std::string& dbPath)
    {
        std::shared_ptr<QosDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, QosDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        EXPECT_TRUE(dbRunner->CreateTable(TABLE_NAME, database->GetTableCols(TABLE_NAME)));
        EXPECT_TRUE(dbRunner->InsertData(TABLE_NAME, QOS_DATA));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(QOS_DIR, DEPTH));
        GlobalMockObject::verify();
    }
};

TEST_F(QosProcessorUTest, TestRunShouldReturnTrueWhenRunSuccess)
{
    auto processor = QosProcessor(PROF_DIR);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_QOS));
}

TEST_F(QosProcessorUTest, TestRunShouldReturnFalseWhenProcessorFail)
{
    auto processor = QosProcessor(PROF_DIR);
    DataInventory dataInventory;
    MOCKER_CPP(&Context::GetProfTimeRecordInfo)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_QOS));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    // make DBRunner shared ptr failed
    MOCKER_CPP(&DBInfo::ConstructDBRunner)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_QOS));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
    // db里面表不存在
    MOCKER_CPP(&DataProcessor::CheckPathAndTable)
        .stubs()
        .will(returnValue(CHECK_FAILED));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_QOS));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();

    MOCKER_CPP(&DataProcessor::SaveToDataInventory<QosData>).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_QOS));
    MOCKER_CPP(&DataProcessor::SaveToDataInventory<QosData>).reset();
}

TEST_F(QosProcessorUTest, TestRunShouldReturnFalseWhenProcessDataFailed)
{
    auto processor = QosProcessor(PROF_DIR);
    DataInventory dataInventory;
    // LoadData failed
    MOCKER_CPP(&OriQosData::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_QOS));
    MOCKER_CPP(&OriQosData::empty).reset();
    // Reserve failed
    MOCKER_CPP(&std::vector<QosData>::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_QOS));
    MOCKER_CPP(&std::vector<QosData>::reserve).reset();
}

TEST_F(QosProcessorUTest, TestRunShouldReturnFalseWhenProcessSingleDeviceFailed)
{
    auto processor = QosProcessor(PROF_DIR);
    DataInventory dataInventory;

    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).stubs().will(returnValue(INVALID_DEVICE_ID));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_QOS));
    MOCKER_CPP(&Utils::GetDeviceIdByDevicePath).reset();
}