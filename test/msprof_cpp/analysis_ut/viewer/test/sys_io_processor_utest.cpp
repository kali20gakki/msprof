/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : sys_io_processor_utest.cpp
 * Description        : SysIOProcessor UT
 * Author             : msprof team
 * Creation Date      : 2024/02/21
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/sys_io_processor.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"


using namespace Analysis::Viewer::Database;
using namespace Parser::Environment;
using namespace Analysis::Utils;
// device_id, replayid, timestamp, bandwidth, rxpacket, rxbyte, rxpackets, rxbytes, rxerrors, rxdropped,
// txpacket, txbyte, txpackets, txbytes, txerrors, txdropped, funcid
using SysIODataFormat = std::vector<std::tuple<uint16_t, uint16_t, double, uint32_t, double, double, uint32_t,
    uint32_t, uint32_t, uint32_t, double, double, uint32_t, uint32_t, uint32_t, uint32_t, uint16_t>>;
// deviceId, timestamp, bandwidth, rxPacketRate, rxByteRate, rxPackets, rxBytes, rxErrors, rxDropped
// txPacketRate, txByteRate, txPackets, txBytes, txErrors, txDropped, funcId
using ProcessedDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, double, double, uint32_t,
    uint32_t, uint32_t, uint32_t, double, double, uint32_t, uint32_t, uint32_t, uint32_t, uint16_t>>;
// device_id, timestamp, bandwidth, rxpacket, rxbyte, rxpackets, rxbytes, rxerrors, rxdropped,
// txpacket, txbyte, txpackets, txbytes, txerrors, txdropped, funcid
using QueryataFormat = std::vector<std::tuple<uint32_t, double, uint32_t, double, double, uint32_t,
    uint32_t, uint32_t, uint32_t, double, double, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>>;

const std::string SYS_IO_DIR = "./sys_io";
const std::string REPORT = "report.db";
const std::string DB_PATH = File::PathJoin({SYS_IO_DIR, REPORT});
const std::string PROF = File::PathJoin({SYS_IO_DIR, "PROF"});

const SysIODataFormat NIC_DATA = {
    {0, 0, 236328380142660, 200000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 236328388200700, 200000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 236328400152980, 200000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 236328408182900, 200000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 236328420031060, 200000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

const SysIODataFormat RoCE_DATA = {
    {0, 0, 236328388350260, 200000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 236328396248260, 200000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 236328408033420, 200000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 236328416076480, 200000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 236328428032780, 200000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

class SysIOProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(SYS_IO_DIR));
        EXPECT_TRUE(File::CreateDir(PROF));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF, DEVICE_PREFIX + "0"})));
        EXPECT_TRUE(CreateSysIODB(File::PathJoin({PROF, DEVICE_PREFIX + "0", SQLITE})));
    }

    static bool CreateSysIODB(const std::string& sqlitePath)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));
        std::shared_ptr<NicDB> nicDb;
        MAKE_SHARED0_RETURN_VALUE(nicDb, NicDB, false);
        std::shared_ptr<DBRunner> nicDbRunner;
        MAKE_SHARED_RETURN_VALUE(nicDbRunner, DBRunner, false, File::PathJoin({sqlitePath, nicDb->GetDBName()}));
        EXPECT_TRUE(nicDbRunner->CreateTable("NicOriginalData", nicDb->GetTableCols("NicOriginalData")));
        EXPECT_TRUE(nicDbRunner->InsertData("NicOriginalData", NIC_DATA));

        std::shared_ptr<RoceDB> roceDb;
        MAKE_SHARED0_RETURN_VALUE(roceDb, RoceDB, false);
        std::shared_ptr<DBRunner> roceDbRunner;
        MAKE_SHARED_RETURN_VALUE(roceDbRunner, DBRunner, false, File::PathJoin({sqlitePath, roceDb->GetDBName()}));
        EXPECT_TRUE(roceDbRunner->CreateTable("RoceOriginalData", roceDb->GetTableCols("RoceOriginalData")));
        EXPECT_TRUE(roceDbRunner->InsertData("RoceOriginalData", RoCE_DATA));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(SYS_IO_DIR, 0));
    }

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        if (File::Exist(DB_PATH)) {
            EXPECT_TRUE(File::DeleteFile(DB_PATH));
        }
        GlobalMockObject::verify();
    }
};

TEST_F(SysIOProcessorUTest, TestNicRunShouldReturnTrueWhenProcessorRunSuccess)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId)
    .stubs()
    .will(returnValue(record));
    auto processor = NicProcessor(DB_PATH, {PROF});
    EXPECT_TRUE(processor.Run());
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    QueryataFormat checkData;
    uint16_t expectNum = NIC_DATA.size();
    std::string sqlStr = "SELECT * FROM " + TABLE_NAME_NIC;
    ASSERT_NE(dbRunner, nullptr);
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectNum, checkData.size());
}

TEST_F(SysIOProcessorUTest, TestRoCERunShouldReturnTrueWhenProcessorRunSuccess)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
    };
    MOCKER_CPP(&Context::GetInfoByDeviceId)
    .stubs()
    .will(returnValue(record));
    auto processor = RoCEProcessor(DB_PATH, {PROF});
    EXPECT_TRUE(processor.Run());
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    QueryataFormat checkData;
    uint16_t expectNum = RoCE_DATA.size();
    std::string sqlStr = "SELECT * FROM " + TABLE_NAME_ROCE;
    ASSERT_NE(dbRunner, nullptr);
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectNum, checkData.size());
}

TEST_F(SysIOProcessorUTest, TestNicRunShouldReturnFalseWhenProcessorRunFailed)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
    };
    // CheckPath failed
    MOCKER_CPP(&TableProcessor::CheckPath)
    .stubs()
    .will(returnValue(CHECK_FAILED));
    auto processor1 = NicProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor1.Run());
    MOCKER_CPP(&TableProcessor::CheckPath).reset();

    // GetProfTimeRecordInfo failed
    auto processor2 = NicProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor2.Run());

    // save failed
    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    MOCKER_CPP(&DBRunner::CreateTable).stubs().will(returnValue(false));
    auto processor3 = NicProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor3.Run());
    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
    MOCKER_CPP(&DBRunner::CreateTable).reset();
}

TEST_F(SysIOProcessorUTest, TestNicRunShouldReturnFalseWhenFormatDataFailed)
{
    nlohmann::json record = {
        {"startCollectionTimeBegin", "1701069323851824"},
        {"endCollectionTimeEnd", "1701069338041681"},
        {"startClockMonotonicRaw", "36470610791630"},
    };

    MOCKER_CPP(&Context::GetInfoByDeviceId).stubs().will(returnValue(record));
    // sysIOData empty
    MOCKER_CPP(&QueryataFormat::empty).stubs().will(returnValue(true));
    auto processor1 = NicProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor1.Run());
    MOCKER_CPP(&QueryataFormat::empty).reset();

    // Reserve failed
    using SysIOFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, double, double, uint32_t,
        uint32_t, uint32_t, uint32_t, double, double, uint32_t, uint32_t, uint32_t, uint32_t, uint16_t>>;
    MOCKER_CPP(&SysIOFormat::reserve).stubs().will(throws(std::bad_alloc()));
    auto processor2 = NicProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor2.Run());
    MOCKER_CPP(&SysIOFormat::reserve).reset();

    // ProcessedDataFormat empty
    MOCKER_CPP(&SysIOFormat::empty).stubs().will(returnValue(true));
    auto processor3 = NicProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor3.Run());
    MOCKER_CPP(&SysIOFormat::empty).reset();

    MOCKER_CPP(&Context::GetInfoByDeviceId).reset();
}
