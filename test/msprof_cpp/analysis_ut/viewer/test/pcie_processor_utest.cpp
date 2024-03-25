/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pcie_processor_utest.cpp
 * Description        : PCIeProcessor UT
 * Author             : msprof team
 * Creation Date      : 2024/03/02
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/pcie_processor.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Viewer::Database;
using namespace Parser::Environment;
using namespace Analysis::Utils;

// device_id, timestamp, tx_p_bandwidth_min, tx_p_bandwidth_max, tx_p_bandwidth_avg, tx_np_bandwidth_min,
// tx_np_bandwidth_max, tx_np_bandwidth_avg, tx_cpl_bandwidth_min, tx_cpl_bandwidth_max, tx_cpl_bandwidth_avg,
// tx_np_lantency_min, tx_np_lantency_max, tx_np_lantency_avg, rx_p_bandwidth_min, rx_p_bandwidth_max,
// rx_p_bandwidth_avg, rx_np_bandwidth_min, rx_np_bandwidth_max, rx_np_bandwidth_avg, rx_cpl_bandwidth_min,
// rx_cpl_bandwidth_max, rx_cpl_bandwidth_avg
using PCIeDataFormat = std::vector<std::tuple<uint32_t, uint64_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>>;
// deviceId, timestampNs, txPostMin, txPostMax, txPostAvg, txNonpostMin, txNonpostMax, txNonpostAvg,
// txCplMin, txCplMax, txCplAvg, txNonpostLatencyMin, txNonpostLatencyMax, txNonpostLatencyAvg,
// rxPostMin, rxPostMax, rxPostAvg, rxNonpostMin, rxNonpostMax, rxNonpostAvg, rxCplMin, rxCplMax, rxCplAvg
using ProcessedDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
        uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
        uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;

const std::string PCIE_DIR = "./pcie";
const std::string MSPROF = "msprof.db";
const std::string DB_PATH = File::PathJoin({PCIE_DIR, MSPROF});
const std::string PROF = File::PathJoin({PCIE_DIR, "PROF"});

// 一条异常的数据
const PCIeDataFormat PCIE_INVALID_DATA = {
    {88698127877400, 0, 1048575, 0, 0, 1048575, 0, 0, 1048575, 0, 0, 1048575, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

const PCIeDataFormat PCIE_DATA = {
    {88698147865670, 0, 0, 0, 1236, 67, 0, 104, 1, 0, 0, 0, 318, 426, 237, 0, 28, 0, 0, 0, 0, 160, 0},
    {88698167865210, 0, 0, 0, 1236, 67, 0, 104, 0, 0, 2, 0, 318, 431, 288, 0, 96, 0, 0, 4, 0, 160, 0},
    {88698187865220, 0, 0, 0, 1236, 67, 0, 104, 0, 0, 22, 0, 318, 431, 323, 0, 96, 0, 0, 12, 0, 160, 0},
    {88698207898080, 0, 0, 0, 1236, 67, 0, 104, 0, 0, 22, 0, 318, 461, 353, 0, 96, 0, 0, 12, 0, 160, 0},
    {88698227865150, 0, 0, 0, 1236, 67, 0, 104, 0, 0, 22, 0, 318, 465, 380, 0, 96, 0, 0, 12, 0, 160, 0},
};

class PCIeProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(PCIE_DIR));
        EXPECT_TRUE(File::CreateDir(PROF));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF, DEVICE_PREFIX + "0"})));
        EXPECT_TRUE(CreatePCIeDB(File::PathJoin({PROF, DEVICE_PREFIX + "0", SQLITE})));
    }

    static bool CreatePCIeDB(const std::string& sqlitePath)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));
        std::shared_ptr<PCIeDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, PCIeDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, File::PathJoin({sqlitePath, database->GetDBName()}));
        EXPECT_TRUE(dbRunner->CreateTable("PcieOriginalData", database->GetTableCols("PcieOriginalData")));
        EXPECT_TRUE(dbRunner->InsertData("PcieOriginalData", PCIE_INVALID_DATA));
        EXPECT_TRUE(dbRunner->InsertData("PcieOriginalData", PCIE_DATA));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(PCIE_DIR, 0));
    }

    virtual void SetUp()
    {
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1701069323851824"},
            {"endCollectionTimeEnd", "1701069338041681"},
            {"startClockMonotonicRaw", "36470610791630"},
            {"clock_monotonic_raw", "36471130547330"},
        };
        MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    }

    virtual void TearDown()
    {
        if (File::Exist(DB_PATH)) {
            EXPECT_TRUE(File::DeleteFile(DB_PATH));
        }
        GlobalMockObject::verify();
    }
};

TEST_F(PCIeProcessorUTest, TestRunShouldReturnTrueWhenRunSuccess)
{
    auto processor = PCIeProcessor(DB_PATH, {PROF});
    EXPECT_TRUE(processor.Run());

    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    PCIeDataFormat checkData;
    uint16_t expectNum = PCIE_DATA.size();
    std::string sqlStr = "SELECT * FROM " + TABLE_NAME_PCIE;
    ASSERT_NE(dbRunner, nullptr);
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectNum, checkData.size());
}

TEST_F(PCIeProcessorUTest, TestProcessShouldReturnFalseWhenGetRecordOrCheckPathFailed)
{
    MOCKER_CPP(&TableProcessor::CheckPath).stubs().will(returnValue(CHECK_FAILED));
    auto processor1 = PCIeProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor1.Run());
    MOCKER_CPP(&TableProcessor::CheckPath).reset();

    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    auto processor2 = PCIeProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor2.Run());
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();
}

TEST_F(PCIeProcessorUTest, TestFormatDataShouldReturnFalseWhenFormatDataFailed)
{
    // dataFormat empty
    MOCKER_CPP(&PCIeDataFormat::empty).stubs().will(returnValue(true));
    auto processor1 = PCIeProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor1.Run());
    MOCKER_CPP(&PCIeDataFormat::empty).reset();

    // Reserve failed
    MOCKER_CPP(&ProcessedDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    auto processor2 = PCIeProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor2.Run());
    MOCKER_CPP(&ProcessedDataFormat::reserve).reset();

    // ProcessedDataFormat empty
    MOCKER_CPP(&ProcessedDataFormat::empty).stubs().will(returnValue(true));
    auto processor3 = PCIeProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor3.Run());
    MOCKER_CPP(&ProcessedDataFormat::empty).reset();
}

