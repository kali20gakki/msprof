/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : sio_processor_utest.cpp
 * Description        : SioProcessor UT
 * Author             : msprof team
 * Creation Date      : 2024/08/28
 * *****************************************************************************
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/domain/data_process/system/sio_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Domain;
using namespace Domain::Environment;
using namespace Analysis::Utils;

namespace {
const int DEPTH = 0;
const std::string SIO_DIR = "./sio";
const std::string DEVICE_SUFFIX = "device_0";
const std::string DB_SUFFIX = "sio.db";
const std::string PROF_DIR = File::PathJoin({SIO_DIR, "./PROF_0"});
const std::string TABLE_NAME = "Sio";
// die_id, req_rx, rsp_rx, snp_rx, dat_rx, req_tx, rsp_tx, snp_tx, dat_tx, timestamp
using SioDataFormat = std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
    uint32_t, uint32_t, uint32_t, uint32_t, double>>;;
const SioDataFormat SIO_DATA = {
    {0, 70781, 69062, 0, 68889, 1776, 4069, 775, 2784, 5626510749920.817},
    {1, 1355, 5017, 407, 2800, 37082, 136926, 0, 34704, 5626510749920.817},
    {0, 70622, 69165, 1, 68980, 1658, 3517, 1177, 2629, 5626551718192.833},
    {1, 1197, 5147, 436, 2570, 37010, 136822, 2, 34665, 5626551718192.833},
    {0, 70860, 69384, 0, 69140, 656, 3053, 1387, 2669, 5626592686344.825},
    {1, 1197, 3667, 252, 2125, 36176, 136729, 1, 34637, 5626592686344.825}
};
}

class SioProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        GlobalMockObject::verify();
        EXPECT_TRUE(File::CreateDir(SIO_DIR));
        EXPECT_TRUE(File::CreateDir(PROF_DIR));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_DIR, DEVICE_SUFFIX})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF_DIR, DEVICE_SUFFIX, SQLITE})));
        EXPECT_TRUE(CreateSioDB(File::PathJoin({PROF_DIR, DEVICE_SUFFIX, SQLITE, DB_SUFFIX})));
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

    static bool CreateSioDB(const std::string& dbPath)
    {
        std::shared_ptr<SioDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, SioDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, dbPath);
        EXPECT_TRUE(dbRunner->CreateTable(TABLE_NAME, database->GetTableCols(TABLE_NAME)));
        EXPECT_TRUE(dbRunner->InsertData(TABLE_NAME, SIO_DATA));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(SIO_DIR, DEPTH));
        GlobalMockObject::verify();
    }
};

TEST_F(SioProcessorUTest, TestRunShouldReturnTrueWhenRunSuccess)
{
    auto processor = SioProcessor(PROF_DIR);
    DataInventory dataInventory;
    EXPECT_TRUE(processor.Run(dataInventory, PROCESSOR_NAME_SIO));
}

TEST_F(SioProcessorUTest, TestRunShouldReturnFalseWhenProcessorFail)
{
    auto processor = SioProcessor(PROF_DIR);
    DataInventory dataInventory;
    MOCKER_CPP(&Context::GetProfTimeRecordInfo)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_SIO));
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
    // make DBRunner shared ptr failed
    MOCKER_CPP(&DBInfo::ConstructDBRunner)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_SIO));
    MOCKER_CPP(&DBInfo::ConstructDBRunner).reset();
    // db里面表不存在
    MOCKER_CPP(&DataProcessor::CheckPathAndTable)
        .stubs()
        .will(returnValue(CHECK_FAILED));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_SIO));
    MOCKER_CPP(&DataProcessor::CheckPathAndTable).reset();
}

TEST_F(SioProcessorUTest, TestFormatDataShouldReturnFalseWhenProcessDataFailed)
{
    auto processor = SioProcessor(PROF_DIR);
    DataInventory dataInventory;
    // LoadData failed
    MOCKER_CPP(&OriSioData::empty).stubs().will(returnValue(true));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_SIO));
    MOCKER_CPP(&OriSioData::empty).reset();
    // Reserve failed
    MOCKER_CPP(&std::vector<SioData>::reserve).stubs().will(throws(std::bad_alloc()));
    EXPECT_FALSE(processor.Run(dataInventory, PROCESSOR_NAME_SIO));
    MOCKER_CPP(&std::vector<SioData>::reserve).reset();
}
