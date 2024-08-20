/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccs_processor_utest.cpp
 * Description        : HCCSProcessor UT
 * Author             : msprof team
 * Creation Date      : 2024/03/02
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/data_process/system/hccs_processor.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Domain;
using namespace Parser::Environment;
using namespace Analysis::Utils;

namespace {
const int DEPTH = 0;
const std::string HCCS_DIR = "./hccs";
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "hccs.db";
const std::string PROF_DIR = File::PathJoin({HCCS_DIR, "./PROF_0"});
const std::string TABLE_NAME = "HCCSEventsData";
// device_id, timestamp, txthroughput, rxthroughput
using HccsDataFormat = std::vector<std::tuple<uint32_t, double, double, double>>;
using SummaryData = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
const HccsDataFormat HCCS_DATA = {
    {1, 331124176903000, 280146.575, 0},
    {1, 331124196902780, 280146.903, 0},
    {1, 331124216902500, 280146.937, 0},
    {1, 331124236902500, 280146.26, 0},
};
}
class HCCSProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(HCCS_DIR));
        EXPECT_TRUE(File::CreateDir(PROF_DIR));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_DIR, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_DIR, DEVICE_SUFFIX, SQLITE})));
        EXPECT_TRUE(CreateHCCSDB(File::PathJoin({PROF_DIR, DEVICE_SUFFIX, SQLITE, DB_SUFFIX})));
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

    static bool CreateHCCSDB(const std::string& dbPath)
    {
        std::shared_ptr<HCCSDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, HCCSDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        EXPECT_TRUE(dbRunner->CreateTable("HCCSEventsData", database->GetTableCols(TABLE_NAME)));
        EXPECT_TRUE(dbRunner->InsertData("HCCSEventsData", HCCS_DATA));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(HCCS_DIR, 0));
    }
};

TEST_F(HCCSProcessorUTest, TestRunShouldReturnTrueWhenRunSuccess)
{
    auto processor = HCCSProcessor(PROF_DIR);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_HCCS));
}

TEST_F(HCCSProcessorUTest, TestRunShouldReturnFalseWhenProcessorFail)
{
    auto processor = HCCSProcessor(PROF_DIR);
    DataInventory dataInventory;
    MOCKER_CPP(&Context::GetProfTimeRecordInfo)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HCCS));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    // make DBRunner shared ptr failed
    MOCKER_CPP(&DBInfo::ConstructDBRunner)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HCCS));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
    // db里面表不存在
    MOCKER_CPP(&DataProcessor::CheckPathAndTable)
        .stubs()
        .will(returnValue(CHECK_FAILED));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HCCS));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();
}

TEST_F(HCCSProcessorUTest, TestFormatDataShouldReturnFalseWhenProcessDataFailed)
{
    auto processor = HCCSProcessor(PROF_DIR);
    DataInventory dataInventory;
    // 时间相关信息不存在
    MOCKER_CPP(&Context::GetClockMonotonicRaw).stubs().will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HCCS));
    MOCKER_CPP(&Context::GetClockMonotonicRaw).reset();
    // LoadData failed
    MOCKER_CPP(&OriHccsData::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HCCS));
    MOCKER_CPP(&OriHccsData::empty).reset();
    // Reserve failed
    MOCKER_CPP(&std::vector<HccsData>::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_NPU_MEM));
    MOCKER_CPP(&std::vector<HccsData>::reserve).reset();
}

TEST_F(HCCSProcessorUTest, TestFormatDataShouldReturnFalseWhenProcessSummaryDataFailed)
{
    auto processor = HCCSProcessor(PROF_DIR);
    DataInventory dataInventory;
    // 取到的数据为空
    MOCKER_CPP(&SummaryData::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_HCCS));
    MOCKER_CPP(&SummaryData::empty).reset();
}
