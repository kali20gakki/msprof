/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : unified_db_manager_utest.cpp
 * Description        : unifiedDbManager.cpp UT
 * Author             : msprof team
 * Creation Date      : 2024/01/27
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/unified_db_manager.h"
#include "analysis/csrc/parser/environment/context.h"

#include <cstdlib>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/utils/file.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "collector/dvvp/common/errno/error_code.h"

using namespace Analysis::Utils;
using namespace Analysis::Parser::Environment;
using namespace Analysis::Viewer::Database;
using namespace analysis::dvvp::common::error;

const auto UNIFIED_DB_DIR = "./unifiedDBManagerUTest";
const auto LOCAL_DIR =  "PROF1";
const auto TEST_DIR = "PROF2";
const std::string INFO_JSON = "info.json";
const std::string SAMPLE_JSON = "sample.json";
const std::string START_INFO = "start_info";
const std::string END_INFO = "end_info";
const std::string HOST_START_LOG = "host_start.log";
const std::string DEVICE_START_LOG = "dev_start.log";
const int MSVP_MMPROCESS = -1;

namespace {
    void creatSampleJson(const std::string &filePath)
    {
        // sample.json
        nlohmann::json sample = {
            {"test11111", 123456}
        };
        FileWriter sampleWriter(File::PathJoin({filePath, SAMPLE_JSON}));
        sampleWriter.WriteText(sample.dump());
    }

    void CreateJsonAndLog(const std::string &filePath, uint16_t deviceId, bool isSampleJsonCreat)
    {
        if (isSampleJsonCreat) {
            creatSampleJson(filePath);
        }
    
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

    void CreatProfFolderDir(std::vector<std::string> pathList, bool isSampleJsonCreat)
    {
        EXPECT_TRUE(File::CreateDir(UNIFIED_DB_DIR));
        for (auto &path : pathList) {
            EXPECT_TRUE(File::CreateDir(File::PathJoin({UNIFIED_DB_DIR, path})));
            EXPECT_TRUE(File::CreateDir(File::PathJoin({UNIFIED_DB_DIR, path, HOST})));
            EXPECT_TRUE(File::CreateDir(File::PathJoin({UNIFIED_DB_DIR, path, DEVICE_PREFIX + "0"})));
            CreateJsonAndLog(File::PathJoin({UNIFIED_DB_DIR, path, HOST}), HOST_ID, isSampleJsonCreat);
            CreateJsonAndLog(File::PathJoin({UNIFIED_DB_DIR, path, DEVICE_PREFIX + "0"}), 0, isSampleJsonCreat);
        }
    }
}

class UnifiedDBManagerUTest : public testing::Test {
protected:
    static void SetUpTestCase() {}

    static void TearDownTestCase() {}

    virtual void SetUp() {}

    virtual void TearDown() { GlobalMockObject::verify(); }
};

TEST_F(UnifiedDBManagerUTest, TestCheckProfDirsValidReturnFalseWhenprofFolderPathsIsEmpty)
{
    std::string errInfo;
    const std::set<std::string> profFolderPaths;
    EXPECT_FALSE(UnifiedDBManager::CheckProfDirsValid("./test", profFolderPaths, errInfo));
}

TEST_F(UnifiedDBManagerUTest, TestCheckProfDirsValidReturnFalseWhenContextLoadError)
{
    std::string errInfo;
    const std::set<std::string> profFolderPaths = {"./test11"};
    MOCKER_CPP(&Context::Load)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(UnifiedDBManager::CheckProfDirsValid("./test", profFolderPaths, errInfo));
    GlobalMockObject::verify();
}

TEST_F(UnifiedDBManagerUTest, TestCheckProfDirsValidReturnFalseWhenPathIsExceedingFixedLength)
{
    std::string errInfo;
    const std::set<std::string> profFolderPaths = {"./test11"};
    MOCKER_CPP(&Context::Load)
        .stubs()
        .will(returnValue(false));
    EXPECT_FALSE(UnifiedDBManager::CheckProfDirsValid("./test", profFolderPaths, errInfo));
    GlobalMockObject::verify();
}

TEST_F(UnifiedDBManagerUTest, TestCheckProfDirsValidReturnFalseWhenMsprofBinPidExistButIsDifferent)
{
    std::string errInfo;
    const std::set<std::string> profFolderPaths = {"./test11", "./test222"};
    MOCKER_CPP(&Context::Load)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Context::GetMsBinPid)
        .stubs()
        .will(returnValue(123456)).then(returnValue(34567)); // 123456 和 34567表示两个msprofbinpid的值
    EXPECT_FALSE(UnifiedDBManager::CheckProfDirsValid("./test", profFolderPaths, errInfo));
    GlobalMockObject::verify();
}

TEST_F(UnifiedDBManagerUTest, TestCheckProfDirsValidReturnFalseWhenMsprofBinPidExistAndEqualToMSVP_MMPROCESS)
{
    std::string errInfo;
    const std::set<std::string> profFolderPaths = {"./test11", "./test222"};
    MOCKER_CPP(&Context::Load)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Context::GetMsBinPid)
        .stubs()
        .will(returnValue(MSVP_MMPROCESS)).then(returnValue(34567)); // 34567 表示一个msprofBinpid的有效值
    EXPECT_FALSE(UnifiedDBManager::CheckProfDirsValid("./test", profFolderPaths, errInfo));
    GlobalMockObject::verify();
}

TEST_F(UnifiedDBManagerUTest, TestCheckProfDirsValidReturnTrueWhenMsprofBinPidExistAndEqual)
{
    std::string errInfo;
    std::vector<uint16_t> deviceIds ={64};
    const std::set<std::string> profFolderPaths = {"./test11", "./test222"};
    MOCKER_CPP(&Context::Load)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Context::GetDeviceId)
        .stubs()
        .will(returnValue(deviceIds));
    MOCKER_CPP(&Context::GetMsBinPid)
        .stubs()
        .will(returnValue(34567)).then(returnValue(34567)); // 34567 表示一个msprofBinPid的示例
    EXPECT_TRUE(UnifiedDBManager::CheckProfDirsValid("./test", profFolderPaths, errInfo));
    GlobalMockObject::verify();
}

TEST_F(UnifiedDBManagerUTest, TestUnifiedDBManagerInitReturnFailedWhenSampleJsonIsExist)
{
    CreatProfFolderDir({"PROF1"}, true);
    // 环境构造, samplejson存在
    const std::set<std::string> profFolderPaths = {"./unifiedDBManagerUTest/PROF1"};
    Analysis::Viewer::Database::UnifiedDBManager unifiedDbManager =
        Analysis::Viewer::Database::UnifiedDBManager("./unifiedDBManagerUTest", profFolderPaths);
    nlohmann::json sampleJson;
    sampleJson["msprofBinPid"] = nullptr;
    FileWriter sampleWriter(File::PathJoin({"./unifiedDBManagerUTest/PROF1", HOST, SAMPLE_JSON}));
    sampleWriter.WriteText(sampleJson.dump());
    EXPECT_EQ(unifiedDbManager.Init(), PROFILING_SUCCESS);
    GlobalMockObject::verify();
    EXPECT_TRUE(File::RemoveDir(UNIFIED_DB_DIR, 0));
}

TEST_F(UnifiedDBManagerUTest, TestUnifiedDBManagerInitReturnFailedWhenSampleJsonIsNotExist)
{
    CreatProfFolderDir({"PROF1"}, false);
    // 环境构造, samplejson不存在
    const std::set<std::string> profFolderPaths = {"./unifiedDBManagerUTest/PROF1"};
    Analysis::Viewer::Database::UnifiedDBManager unifiedDbManager =
        Analysis::Viewer::Database::UnifiedDBManager("./unifiedDBManagerUTest", profFolderPaths);
    nlohmann::json sampleJson;
    sampleJson["msprofBinPid"] = nullptr;
    FileWriter sampleWriter(File::PathJoin({"./unifiedDBManagerUTest/PROF1", HOST, SAMPLE_JSON}));
    sampleWriter.WriteText(sampleJson.dump());
    EXPECT_EQ(unifiedDbManager.Init(), PROFILING_FAILED);
    GlobalMockObject::verify();
    EXPECT_TRUE(File::RemoveDir(UNIFIED_DB_DIR, 0));
}
