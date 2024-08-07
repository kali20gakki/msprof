/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : aicore_freq_processor_utest.cpp
 * Description        : AicoreFreqProcessor UT
 * Author             : msprof team
 * Creation Date      : 2024/05/17
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/aicore_freq_processor.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

using namespace Analysis::Viewer::Database;
using namespace Parser::Environment;
using namespace Analysis::Utils;

// syscnt, freq
using FreqDataFormat = std::vector<std::tuple<uint64_t, double>>;
// deviceId, timestampNs, freq
using ProcessedDataFormat = std::vector<std::tuple<uint16_t, uint64_t, double>>;
// deviceId, timestampNs, freq
using QueryDataFormat = std::vector<std::tuple<uint32_t, uint64_t, double>>;

const std::string DATA_DIR = "./freq";
const std::string MSPROF = "msprof.db";
const std::string DB_PATH = File::PathJoin({DATA_DIR, MSPROF});
const std::string PROF = File::PathJoin({DATA_DIR, "PROF"});

const FreqDataFormat FREQ_DATA = {
    {484500000000000, 200},
    {484576969200418, 1650},
    {484576973402096, 1650},
    {484576973456197, 800},
    {484576973512465, 1650},
    {484577067389576, 1650},
};

class AicoreFreqProcessorUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(DATA_DIR));
        EXPECT_TRUE(File::CreateDir(PROF));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({PROF, DEVICE_PREFIX + "0"})));
        EXPECT_TRUE(CreateFreqDB(File::PathJoin({PROF, DEVICE_PREFIX + "0", SQLITE})));
    }

    static bool CreateFreqDB(const std::string& sqlitePath)
    {
        EXPECT_TRUE(File::CreateDir(sqlitePath));
        std::shared_ptr<FreqDB> database;
        MAKE_SHARED0_RETURN_VALUE(database, FreqDB, false);
        std::shared_ptr<DBRunner> dbRunner;
        MAKE_SHARED_RETURN_VALUE(dbRunner, DBRunner, false, File::PathJoin({sqlitePath, database->GetDBName()}));
        EXPECT_TRUE(dbRunner->CreateTable("FreqParse", database->GetTableCols("FreqParse")));
        EXPECT_TRUE(dbRunner->InsertData("FreqParse", FREQ_DATA));
        return true;
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(DATA_DIR, 0));
    }

    virtual void SetUp()
    {
        nlohmann::json record = {
            {"startCollectionTimeBegin", "1715760307197379"},
            {"endCollectionTimeEnd", "1715760313397397"},
            {"startClockMonotonicRaw", "9691377159398230"},
            {"hostMonotonic", "9691377161797070"},
            {"platform_version", "5"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
            {"DeviceInfo", {{{"hwts_frequency", "50"}, {"aic_frequency", "1650"}}}},
            {"devCntvct", "484576969200418"},
            {"hostCntvctDiff", "0"},
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

TEST_F(AicoreFreqProcessorUTest, TestRunShouldReturnTrueWhenRunSuccess)
{
    auto processor = AicoreFreqProcessor(DB_PATH, {PROF});
    EXPECT_TRUE(processor.Run());

    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    QueryDataFormat checkData;
    // 有一条数据在采集范围外，将数据过滤后，再新增两条开始和结束记录
    uint16_t expectNum = FREQ_DATA.size() - 1 + 2;
    std::string sqlStr = "SELECT deviceId, timestampNs, freq FROM " + TABLE_NAME_AICORE_FREQ;
    ASSERT_NE(dbRunner, nullptr);
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectNum, checkData.size());
}

TEST_F(AicoreFreqProcessorUTest, TestRunShouldReturnTrueWhenNoDb)
{
    if (File::Exist(File::PathJoin({PROF, DEVICE_PREFIX + "0", SQLITE, "freq.db"}))) {
        EXPECT_TRUE(File::DeleteFile(File::PathJoin({PROF, DEVICE_PREFIX + "0", SQLITE, "freq.db"})));
    }
    auto processor = AicoreFreqProcessor(DB_PATH, {PROF});
    EXPECT_TRUE(processor.Run());

    std::shared_ptr<DBRunner> dbRunner;
    MAKE_SHARED_NO_OPERATION(dbRunner, DBRunner, DB_PATH);
    QueryDataFormat checkData;
    uint16_t expectNum = 2;
    std::string sqlStr = "SELECT deviceId, timestampNs, freq FROM " + TABLE_NAME_AICORE_FREQ;
    ASSERT_NE(dbRunner, nullptr);
    EXPECT_TRUE(dbRunner->QueryData(sqlStr, checkData));
    EXPECT_EQ(expectNum, checkData.size());
}

TEST_F(AicoreFreqProcessorUTest, TestProcessShouldReturnTrueWhenChipNotStars)
{
    MOCKER_CPP(&Context::GetPlatformVersion).stubs().will(returnValue(1));
    auto processor = AicoreFreqProcessor(DB_PATH, {PROF});
    EXPECT_TRUE(processor.Run());
}

TEST_F(AicoreFreqProcessorUTest, TestProcessShouldReturnFalseWhenProcessFailed)
{
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).stubs().will(returnValue(false));
    auto processor1 = AicoreFreqProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor1.Run());
    MOCKER_CPP(&Context::GetProfTimeRecordInfo).reset();

    MOCKER_CPP(&TableProcessor::CheckPath).stubs().will(returnValue(CHECK_FAILED));
    auto processor2 = AicoreFreqProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor2.Run());
    MOCKER_CPP(&TableProcessor::CheckPath).reset();

    MOCKER_CPP(&Context::GetSyscntConversionParams).stubs().will(returnValue(false));
    auto processor3 = AicoreFreqProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor3.Run());
    MOCKER_CPP(&Context::GetSyscntConversionParams).reset();

    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable).stubs().will(returnValue(false));
    auto processor4 = AicoreFreqProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor4.Run());
    MOCKER_CPP(&Analysis::Viewer::Database::DBRunner::CreateTable).reset();
}

TEST_F(AicoreFreqProcessorUTest, TestFormatDataShouldReturnFalseWhenFormatDataFailed)
{
    // GetPmuFreq failed
    MOCKER_CPP(&Context::GetPmuFreq).stubs().will(returnValue(false));
    auto processor1 = AicoreFreqProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor1.Run());
    MOCKER_CPP(&Context::GetPmuFreq).reset();

    // Reserve failed
    MOCKER_CPP(&ProcessedDataFormat::reserve).stubs().will(throws(std::bad_alloc()));
    auto processor2 = AicoreFreqProcessor(DB_PATH, {PROF});
    EXPECT_FALSE(processor2.Run());
    MOCKER_CPP(&ProcessedDataFormat::reserve).reset();
}

