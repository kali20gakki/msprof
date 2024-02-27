/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : context_utest.cpp
 * Description        : Context UT
 * Author             : msprof team
 * Creation Date      : 2023/12/07
 * *****************************************************************************
 */
#include "analysis/csrc/parser/environment/context.h"

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/utils/file.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "collector/dvvp/common/config/config.h"

using namespace Analysis::Utils;
using namespace Analysis::Parser::Environment;
using namespace Analysis::Viewer::Database;

const auto CONTEXT_DIR = "./context";
const auto LOCAL_DIR =  "PROF1";
const auto TEST_DIR = "PROF2";
const std::string INFO_JSON = "info.json";
const std::string SAMPLE_JSON = "sample.json";
const std::string START_INFO = "start_info";
const std::string END_INFO = "end_info";
const std::string HOST_START_LOG = "host_start.log";
const std::string DEVICE_START_LOG = "dev_start.log";
const int MSVP_MMPROCESS = -1;

class ContextUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        // ut开始时，建立local（PROF1）目录, 该目录下的内容在该ut下不动
        EXPECT_TRUE(File::CreateDir(CONTEXT_DIR));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({CONTEXT_DIR, LOCAL_DIR})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({CONTEXT_DIR, LOCAL_DIR, HOST})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({CONTEXT_DIR, LOCAL_DIR, DEVICE_PREFIX + "0"})));
        CreateJsonAndLog(File::PathJoin({CONTEXT_DIR, LOCAL_DIR, HOST}), HOST_ID);
        CreateJsonAndLog(File::PathJoin({CONTEXT_DIR, LOCAL_DIR, DEVICE_PREFIX + "0"}), 0);
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(CONTEXT_DIR, 0));
    }

    static void CreateJsonAndLog(const std::string &filePath, uint16_t deviceId)
    {
        // info.json
        nlohmann::json info = {
            {"drvVersion", 467732},
            {"platform_version", "7"},
            {"pid", "2376271"},
            {"CPU", {{{"Frequency", "100.000000"}}}},
            {"DeviceInfo", {{{"hwts_frequency", "49.000000"}}}},
        };
        FileWriter infoWriter(File::PathJoin({filePath, INFO_JSON}));
        infoWriter.WriteText(info.dump());

        // sample.json
        nlohmann::json sample = {
            {"storageLimit", ""},
            {"llc_profiling", "read"},
        };
        FileWriter sampleWriter(File::PathJoin({filePath, SAMPLE_JSON}));
        sampleWriter.WriteText(sample.dump());

        // start.info
        nlohmann::json startInfo = {
            {"collectionTimeEnd", ""},
            {"clockMonotonicRaw", "8719641548578"},
            {"collectionTimeBegin", "1700902984041176"},
            {"collectionDateBegin", "2023-11-25 09:03:04.544664"},
            {"collectionDateEnd", ""},
        };
        FileWriter startInfoWriter(File::PathJoin({filePath, START_INFO}));
        startInfoWriter.WriteText(startInfo.dump());

        // end.info
        nlohmann::json endInfo = {
            {"collectionTimeEnd", "1700902986330096"},
            {"clockMonotonicRaw", "8721930460279"},
            {"collectionTimeBegin", ""},
            {"collectionDateBegin", ""},
            {"collectionDateEnd", "2023-11-25 09:03:06.833584"},
        };
        FileWriter endInfoWriter(File::PathJoin({filePath, END_INFO}));
        endInfoWriter.WriteText(endInfo.dump());

        // host_start.log
        FileWriter hostStartLogWriter(File::PathJoin({filePath, HOST_START_LOG}));
        hostStartLogWriter.WriteText("[Host]\n");
        hostStartLogWriter.WriteText("clock_monotonic_raw: 36471130547330\n");
        hostStartLogWriter.WriteText("cntvct: 3666503140109\n");
        hostStartLogWriter.WriteText("cntvct_diff: 0\n");

        // host 没有device_start_log
        if (deviceId == HOST_ID) {
            return;
        }
        // device_start.log
        FileWriter deviceStartLogWriter(File::PathJoin({filePath, DEVICE_START_LOG}));
        deviceStartLogWriter.WriteText("clock_monotonic_raw: 0\n");
        deviceStartLogWriter.WriteText("cntvct: 1833256145654\n");
    }

    virtual void SetUp()
    {
        // 每个用例生成时 生成test（PROF2)目录，保证每次该目录的内容都是新的
        EXPECT_TRUE(File::CreateDir(File::PathJoin({CONTEXT_DIR, TEST_DIR})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST})));
        EXPECT_TRUE(File::CreateDir(File::PathJoin({CONTEXT_DIR, TEST_DIR, DEVICE_PREFIX + "0"})));
        CreateJsonAndLog(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST}), HOST_ID);
        CreateJsonAndLog(File::PathJoin({CONTEXT_DIR, TEST_DIR, DEVICE_PREFIX + "0"}), 0);
    }

    virtual void TearDown()
    {
        // 每次用例结束时，删除test(PROF2)目录
        Context::GetInstance().Clear();
        EXPECT_TRUE(File::RemoveDir(File::PathJoin({CONTEXT_DIR, TEST_DIR}), 0));
    }
};

TEST_F(ContextUTest, TestLoadShouldReturnTrueWhenReadJsonSuccess)
{
    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, LOCAL_DIR})}));
}

TEST_F(ContextUTest, TestLoadShouldFalseWhenNoValue)
{
EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON})));
    nlohmann::json info = {
        {"drvVersion", 467732},
        {"pid", "2376271"},
        {"CPU", {{{"Frequency", "100.000000"}}}},
        {"DeviceInfo", {{{"hwts_frequency", "49.000000"}}}},
    };
    FileWriter infoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON}));
    infoWriter.WriteText(info.dump());
    EXPECT_FALSE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
}

TEST_F(ContextUTest, TestLoadShouldFalseWhenNoCPUFrequency)
{
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON})));
    nlohmann::json info = {
        {"drvVersion", 467732},
        {"platform_version", "7"},
        {"pid", "2376271"},
        {"CPU", {{{"abc", "100.000000"}}}},
        {"DeviceInfo", {{{"hwts_frequency", "49.000000"}}}},
    };
    FileWriter infoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON}));
    infoWriter.WriteText(info.dump());
    EXPECT_FALSE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
}

TEST_F(ContextUTest, TestLoadShouldFalseWhenNoDeviceHWTSFrequency)
{
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, DEVICE_PREFIX + "0", INFO_JSON})));
    nlohmann::json info = {
        {"drvVersion", 467732},
        {"platform_version", "7"},
        {"pid", "2376271"},
        {"CPU", {{{"Frequency", "100.000000"}}}},
        {"DeviceInfo", {{{"abc", "49.000000"}}}},
    };
    FileWriter infoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, DEVICE_PREFIX + "0", INFO_JSON}));
    infoWriter.WriteText(info.dump());
    EXPECT_FALSE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
}

TEST_F(ContextUTest, TestLoadShouldReturnFalseWhenReadMoreThanOneJson)
{
    FileWriter infoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON + ".0"}));
    infoWriter.WriteText(".");
    EXPECT_FALSE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON + ".0"})));
}

TEST_F(ContextUTest, TestLoadShouldReturnFalseWhenReadJsonException)
{
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON})));
    FileWriter infoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON}));
    infoWriter.WriteText(".");
    EXPECT_FALSE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
}

TEST_F(ContextUTest, TestLoadShouldReturnFalseWhenReadMoreThanOneLog)
{
    FileWriter textWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, HOST_START_LOG + ".0"}));
    textWriter.WriteText(".");
    EXPECT_FALSE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, HOST_START_LOG + ".0"})));
}

TEST_F(ContextUTest, TestLoadShouldReturnFalseWhenReadLogException)
{
    MOCKER_CPP(&FileReader::ReadText).stubs()
    .will(returnValue(Analysis::ANALYSIS_ERROR));
    EXPECT_FALSE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    MOCKER_CPP(&FileReader::ReadText).reset();
}

TEST_F(ContextUTest, TestLoadShouldReturnTrueWhenOnlyDevice)
{
    EXPECT_TRUE(File::RemoveDir(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST}), 0));
    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
}

TEST_F(ContextUTest, TestLoadShouldReturnTrueWhenOnlyHost)
{
    EXPECT_TRUE(File::RemoveDir(File::PathJoin({CONTEXT_DIR, TEST_DIR, DEVICE_PREFIX + "0"}), 0));
    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
}

TEST_F(ContextUTest, TestIsAllExportShouldReturnFalseWhenContextEmpty)
{
    EXPECT_FALSE(Context::GetInstance().IsAllExport());
}

TEST_F(ContextUTest, TestIsAllExportShouldReturnTrueWhenAllExportConditionIsMatch)
{
    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, LOCAL_DIR})}));
    EXPECT_TRUE(Context::GetInstance().IsAllExport());
}

TEST_F(ContextUTest, TestIsAllExportShouldReturnFalseWhenDrvVersionLessThanAllExportVersion)
{
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON})));
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, DEVICE_PREFIX + "0", INFO_JSON})));
    nlohmann::json info = {
        {"drvVersion", 1},
        {"platform_version", "7"},
        {"pid", "2376271"},
        {"CPU", {{{"Frequency", "100.000000"}}}},
        {"DeviceInfo", {{{"hwts_frequency", "49.000000"}}}},
    };
    FileWriter infoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON}));
    infoWriter.WriteText(info.dump());
    FileWriter deviceInfoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, DEVICE_PREFIX + "0", INFO_JSON}));
    deviceInfoWriter.WriteText(info.dump());
    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    EXPECT_FALSE(Context::GetInstance().IsAllExport());
}

TEST_F(ContextUTest, TestIsAllExportShouldReturnFalseWhenStrToU16Failed)
{
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON})));
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, DEVICE_PREFIX + "0", INFO_JSON})));
    nlohmann::json info = {
        {"drvVersion", 467732},
        {"platform_version", "2dd"},
        {"pid", "2376271"},
        {"CPU", {{{"Frequency", "100.000000"}}}},
        {"DeviceInfo", {{{"hwts_frequency", "49.000000"}}}},
    };
    FileWriter infoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON}));
    infoWriter.WriteText(info.dump());
    FileWriter deviceInfoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, DEVICE_PREFIX + "0", INFO_JSON}));
    deviceInfoWriter.WriteText(info.dump());
    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    EXPECT_FALSE(Context::GetInstance().IsAllExport());
}

TEST_F(ContextUTest, TestIsAllExportShouldReturnFalseWhenChipV310)
{
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON})));
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, DEVICE_PREFIX + "0", INFO_JSON})));
    nlohmann::json info = {
        {"drvVersion", 467732},
        {"platform_version", "2"},
        {"pid", "2376271"},
        {"CPU", {{{"Frequency", "100.000000"}}}},
        {"DeviceInfo", {{{"hwts_frequency", "49.000000"}}}},
    };
    FileWriter infoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON}));
    infoWriter.WriteText(info.dump());
    FileWriter deviceInfoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, DEVICE_PREFIX + "0", INFO_JSON}));
    deviceInfoWriter.WriteText(info.dump());
    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    EXPECT_FALSE(Context::GetInstance().IsAllExport());
}

TEST_F(ContextUTest, TestGetPlatformVersionShouldReturnUINT16MAXWhenNoDevice)
{
    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, LOCAL_DIR})}));
    EXPECT_EQ(Context::GetInstance().GetPlatformVersion(1, File::PathJoin({CONTEXT_DIR, LOCAL_DIR})), UINT16_MAX);
}

TEST_F(ContextUTest, TestGetPlatformVersionShouldReturnUINT16MAXWhenPlatformVersionStrToU16Failed)
{
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON})));
    nlohmann::json info = {
        {"drvVersion", 467732},
        {"platform_version", "abc"},
        {"pid", "2376271"},
        {"CPU", {{{"Frequency", "100.000000"}}}},
        {"DeviceInfo", {{{"hwts_frequency", "49.000000"}}}},
    };
    FileWriter infoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON}));
    infoWriter.WriteText(info.dump());
    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    EXPECT_EQ(Context::GetInstance().GetPlatformVersion(HOST_ID, File::PathJoin({CONTEXT_DIR, TEST_DIR})), UINT16_MAX);
}

TEST_F(ContextUTest, TestGetPlatformVersionShouldReturn7WhenSuccess)
{
    uint16_t expectVersion = 7;
    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, LOCAL_DIR})}));
    EXPECT_EQ(Context::GetInstance().GetPlatformVersion(
        HOST_ID, File::PathJoin({CONTEXT_DIR, LOCAL_DIR})), expectVersion);
}

TEST_F(ContextUTest, TestGetPidFromInfoJsonShouldReturn0WhenPidStrToU16Failed)
{
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON})));
    nlohmann::json info = {
        {"drvVersion", 467732},
        {"platform_version", "7"},
        {"pid", "abc"},
        {"CPU", {{{"Frequency", "100.000000"}}}},
        {"DeviceInfo", {{{"hwts_frequency", "49.000000"}}}},
    };
    FileWriter infoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON}));
    infoWriter.WriteText(info.dump());
    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    EXPECT_EQ(Context::GetInstance().GetPidFromInfoJson(HOST_ID, File::PathJoin({CONTEXT_DIR, TEST_DIR})), 0);
}

TEST_F(ContextUTest, TestGetPidFromInfoJsonShouldReturn2376271WhenSuccess)
{
    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, LOCAL_DIR})}));
    uint64_t expectPid = 2376271;
    EXPECT_EQ(Context::GetInstance().GetPidFromInfoJson(HOST_ID, File::PathJoin({CONTEXT_DIR, LOCAL_DIR})), expectPid);
}

TEST_F(ContextUTest, TestGetProfTimeRecordInfoShouldReturnDefaultValueWhenStrToU64Failed)
{
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, START_INFO})));
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, END_INFO})));
    // start.info
    nlohmann::json startInfo = {
        {"collectionTimeEnd", "abc"},
        {"clockMonotonicRaw", "abc"},
        {"collectionTimeBegin", "abc"},
        {"collectionDateBegin", "abc"},
        {"collectionDateEnd", "abc"},
    };
    FileWriter startInfoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, START_INFO}));
    startInfoWriter.WriteText(startInfo.dump());

    // end.info
    nlohmann::json endInfo = {
        {"collectionTimeEnd", "abc"},
        {"clockMonotonicRaw", "abc"},
        {"collectionTimeBegin", "abc"},
        {"collectionDateBegin", "abc"},
        {"collectionDateEnd", "abc"},
    };
    FileWriter endInfoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, END_INFO}));
    endInfoWriter.WriteText(endInfo.dump());
    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    ProfTimeRecord res;
    EXPECT_FALSE(Context::GetInstance().GetProfTimeRecordInfo(res, {File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    EXPECT_EQ(res.baseTimeNs, UINT64_MAX - TIME_BASE_OFFSET_NS);
    EXPECT_EQ(res.startTimeNs, UINT64_MAX);
    EXPECT_EQ(res.endTimeNs, 0);
}

TEST_F(ContextUTest, TestGetProfTimeRecordInfoShouldReturnDefaultValueWhenTimeInvalid)
{
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, START_INFO})));
    // start.info
    nlohmann::json startInfo = {
        {"collectionTimeEnd", ""},
        {"clockMonotonicRaw", "8719641548578"},
        {"collectionTimeBegin", "1"},
        {"collectionDateBegin", "2023-11-25 09:03:04.544664"},
        {"collectionDateEnd", ""},
    };
    FileWriter startInfoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, START_INFO}));
    startInfoWriter.WriteText(startInfo.dump());

    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    ProfTimeRecord res;
    EXPECT_FALSE(Context::GetInstance().GetProfTimeRecordInfo(res, {File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    EXPECT_EQ(res.baseTimeNs, UINT64_MAX - TIME_BASE_OFFSET_NS);
    EXPECT_EQ(res.startTimeNs, UINT64_MAX);
    EXPECT_EQ(res.endTimeNs, 0);
}

TEST_F(ContextUTest, TestGetProfTimeRecordInfoShouldReturnFalseWhenNoHostAndDevice)
{
    EXPECT_TRUE(File::RemoveDir(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST}), 0));
    EXPECT_TRUE(File::RemoveDir(File::PathJoin({CONTEXT_DIR, TEST_DIR, DEVICE_PREFIX + "0"}), 0));
    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    ProfTimeRecord res;
    EXPECT_FALSE(Context::GetInstance().GetProfTimeRecordInfo(res, {File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
}

TEST_F(ContextUTest, TestGetProfTimeRecordInfoShouldReturnRightValueWhenSuccess)
{
    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, LOCAL_DIR})}));
    ProfTimeRecord res;
    EXPECT_TRUE(Context::GetInstance().GetProfTimeRecordInfo(res, {File::PathJoin({CONTEXT_DIR, LOCAL_DIR})}));
    uint64_t expectBaseTime = 8719641548578;
    uint64_t expectStartTime = 1700902984041176000;
    uint64_t expectEndTime = 1700902986330096000;
    EXPECT_EQ(res.baseTimeNs, expectBaseTime);
    EXPECT_EQ(res.startTimeNs, expectStartTime);
    EXPECT_EQ(res.endTimeNs, expectEndTime);
}

TEST_F(ContextUTest, TestGetSyscntConversionParamsShouldReturnFreq1000WhenFreqIsEmpty)
{
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON})));
    // info.json
    nlohmann::json info = {
        {"drvVersion", 467732},
        {"platform_version", "7"},
        {"pid", "2376271"},
        {"CPU", {{{"Frequency", ""}}}},
        {"DeviceInfo", {{{"hwts_frequency", "50.0"}}}},
    };
    FileWriter infoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON}));
    infoWriter.WriteText(info.dump());

    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    SyscntConversionParams res;
    EXPECT_TRUE(Context::GetInstance().GetSyscntConversionParams(
        res, HOST_ID, {File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    uint64_t expectSysCnt = 3666503140109;
    uint64_t expectHostMonotonic = 36471130547330;
    EXPECT_EQ(res.freq, DEFAULT_FREQ);
    EXPECT_EQ(res.sysCnt, expectSysCnt);
    EXPECT_EQ(res.hostMonotonic, expectHostMonotonic);
}

TEST_F(ContextUTest, TestGetSyscntConversionParamsShouldReturnFreq1000WhenFreqIs0)
{
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON})));
    // info.json
    nlohmann::json info = {
        {"drvVersion", 467732},
        {"platform_version", "7"},
        {"pid", "2376271"},
        {"CPU", {{{"Frequency", "0"}}}},
        {"DeviceInfo", {{{"hwts_frequency", "50.0"}}}},
    };
    FileWriter infoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON}));
    infoWriter.WriteText(info.dump());

    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    SyscntConversionParams res;
    EXPECT_FALSE(Context::GetInstance().GetSyscntConversionParams(
        res, HOST_ID, {File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    double expectFreq = 0.0;
    uint64_t expectSysCnt = 3666503140109;
    uint64_t expectHostMonotonic = 36471130547330;
    EXPECT_EQ(res.freq, expectFreq);
    EXPECT_EQ(res.sysCnt, expectSysCnt);
    EXPECT_EQ(res.hostMonotonic, UINT64_MAX);
}

TEST_F(ContextUTest, TestGetSyscntConversionParamsShouldReturnDefaultValueWhenHostStrToU16Failed)
{
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, HOST_START_LOG})));
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON})));
    // info.json
    nlohmann::json info = {
        {"drvVersion", 467732},
        {"platform_version", "7"},
        {"pid", "2376271"},
        {"CPU", {{{"Frequency", "abc"}}}},
        {"DeviceInfo", {{{"hwts_frequency", "abc"}}}},
    };
    FileWriter infoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, INFO_JSON}));
    infoWriter.WriteText(info.dump());

    // host host_start.log
    FileWriter hostStartLogWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, HOST_START_LOG}));
    hostStartLogWriter.WriteText("[Host]\n");
    hostStartLogWriter.WriteText("clock_monotonic_raw: abc\n");
    hostStartLogWriter.WriteText("cntvct: abc\n");
    hostStartLogWriter.WriteText("cntvct_diff: 0\n");
    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    SyscntConversionParams res;
    EXPECT_FALSE(Context::GetInstance().GetSyscntConversionParams(
        res, HOST_ID, {File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    EXPECT_EQ(res.freq, DEFAULT_FREQ);
    EXPECT_EQ(res.sysCnt, UINT64_MAX);
    EXPECT_EQ(res.hostMonotonic, UINT64_MAX);
}

TEST_F(ContextUTest, TestGetSyscntConversionParamsShouldReturnDefaultValueWhenDeviceStrToU16Failed)
{
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, DEVICE_PREFIX + "0", DEVICE_START_LOG})));
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, DEVICE_PREFIX + "0", INFO_JSON})));
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, HOST_START_LOG})));
    // info.json
    nlohmann::json info = {
        {"drvVersion", 467732},
        {"platform_version", "7"},
        {"pid", "2376271"},
        {"CPU", {{{"Frequency", "abc"}}}},
        {"DeviceInfo", {{{"hwts_frequency", "abc"}}}},
    };
    FileWriter infoWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, DEVICE_PREFIX + "0", INFO_JSON}));
    infoWriter.WriteText(info.dump());

    // device dev_start.log
    FileWriter deviceStartLogWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, DEVICE_PREFIX + "0", DEVICE_START_LOG}));
    deviceStartLogWriter.WriteText("clock_monotonic_raw: abc\n");
    deviceStartLogWriter.WriteText("cntvct: abc\n");

    // host host_start.log
    FileWriter hostStartLogWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, HOST_START_LOG}));
    hostStartLogWriter.WriteText("[Host]\n");
    hostStartLogWriter.WriteText("clock_monotonic_raw: abc\n");
    hostStartLogWriter.WriteText("cntvct: abc\n");
    hostStartLogWriter.WriteText("cntvct_diff: 0\n");
    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    SyscntConversionParams res;
    EXPECT_FALSE(Context::GetInstance().GetSyscntConversionParams(
        res, 0, {File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    EXPECT_EQ(res.freq, DEFAULT_FREQ);
    EXPECT_EQ(res.sysCnt, UINT64_MAX);
    EXPECT_EQ(res.hostMonotonic, UINT64_MAX);
}

TEST_F(ContextUTest, TestInfoValueShouldReturnRightDataWhenReadMultiProfPath)
{
    EXPECT_TRUE(Context::GetInstance().Load({
        File::PathJoin({CONTEXT_DIR, LOCAL_DIR}),
        File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    uint16_t expectVersion = 7;
    uint16_t localVersion = Context::GetInstance().GetPlatformVersion(
        HOST_ID, File::PathJoin({CONTEXT_DIR, LOCAL_DIR}));
    uint16_t testVersion = Context::GetInstance().GetPlatformVersion(
        HOST_ID, File::PathJoin({CONTEXT_DIR, TEST_DIR}));
    EXPECT_EQ(localVersion, expectVersion);
    EXPECT_EQ(testVersion, expectVersion);

    uint64_t expectPid = 2376271;
    uint64_t localPid = Context::GetInstance().GetPidFromInfoJson(
        HOST_ID, File::PathJoin({CONTEXT_DIR, LOCAL_DIR}));
    uint64_t testPid = Context::GetInstance().GetPidFromInfoJson(
        HOST_ID, File::PathJoin({CONTEXT_DIR, TEST_DIR}));
    EXPECT_EQ(localPid, expectPid);
    EXPECT_EQ(testPid, expectPid);

    ProfTimeRecord expectRecord{1700902984041176000, 1700902986330096000, 8719641548578};
    ProfTimeRecord localRecord;
    EXPECT_TRUE(Context::GetInstance().GetProfTimeRecordInfo(localRecord, {File::PathJoin({CONTEXT_DIR, LOCAL_DIR})}));
    ProfTimeRecord testRecord;
    EXPECT_TRUE(Context::GetInstance().GetProfTimeRecordInfo(testRecord, {File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    EXPECT_EQ(localRecord.baseTimeNs, expectRecord.baseTimeNs);
    EXPECT_EQ(localRecord.startTimeNs, expectRecord.startTimeNs);
    EXPECT_EQ(localRecord.endTimeNs, expectRecord.endTimeNs);
    EXPECT_EQ(testRecord.baseTimeNs, expectRecord.baseTimeNs);
    EXPECT_EQ(testRecord.startTimeNs, expectRecord.startTimeNs);
    EXPECT_EQ(testRecord.endTimeNs, expectRecord.endTimeNs);

    SyscntConversionParams expectParams{100.0, 3666503140109, 36471130547330};
    SyscntConversionParams localParams;
    SyscntConversionParams testParams;
    EXPECT_TRUE(Context::GetInstance().GetSyscntConversionParams(
        localParams, HOST_ID, {File::PathJoin({CONTEXT_DIR, LOCAL_DIR})}));
    EXPECT_TRUE(Context::GetInstance().GetSyscntConversionParams(
        testParams, HOST_ID, {File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    EXPECT_EQ(localParams.freq, expectParams.freq);
    EXPECT_EQ(localParams.sysCnt, expectParams.sysCnt);
    EXPECT_EQ(localParams.hostMonotonic, expectParams.hostMonotonic);
    EXPECT_EQ(testParams.freq, expectParams.freq);
    EXPECT_EQ(testParams.sysCnt, expectParams.sysCnt);
    EXPECT_EQ(testParams.hostMonotonic, expectParams.hostMonotonic);
}

TEST_F(ContextUTest, TestGetMsprofBinPidFromInfoJsonShouldReturnMSVP_MMPROCESSWhenMsprofBinIsEmpty)
{
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, SAMPLE_JSON})));
    nlohmann::json sampleJson;
    sampleJson["msprofBinPid"] = nullptr;
    sampleJson["llc_profiling"] = "read";
    FileWriter sampleWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, SAMPLE_JSON}));
    sampleWriter.WriteText(sampleJson.dump());
    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    EXPECT_EQ(Context::GetInstance().GetMsBinPid(File::PathJoin({CONTEXT_DIR, TEST_DIR})),
              analysis::dvvp::common::config::MSVP_MMPROCESS);
}

TEST_F(ContextUTest, TestGetMsprofBinPidFromInfoJsonShouldReturnSucessWhenGetSameId)
{
    int msprofBinPid = 123456; // 123456 表示一个pid的值
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, SAMPLE_JSON})));
    EXPECT_TRUE(File::DeleteFile(File::PathJoin({CONTEXT_DIR, TEST_DIR, DEVICE_PREFIX + "0", SAMPLE_JSON})));
    // sample.json
    nlohmann::json sample = {
        {"msprofBinPid", msprofBinPid},
        {"llc_profiling", "read"},
    };
    FileWriter sampleWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, HOST, SAMPLE_JSON}));
    sampleWriter.WriteText(sample.dump());
    FileWriter deviceSampleWriter(File::PathJoin({CONTEXT_DIR, TEST_DIR, DEVICE_PREFIX + "0", SAMPLE_JSON}));
    deviceSampleWriter.WriteText(sample.dump());

    EXPECT_TRUE(Context::GetInstance().Load({File::PathJoin({CONTEXT_DIR, TEST_DIR})}));
    EXPECT_EQ(Context::GetInstance().GetMsBinPid(File::PathJoin({CONTEXT_DIR, TEST_DIR})), msprofBinPid);
}
