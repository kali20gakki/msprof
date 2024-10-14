/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device_context_utest.cpp
 * Description        : device context UT
 * Author             : msprof team
 * Creation Date      : 2024/6/26
 * *****************************************************************************
 */
#include "analysis/csrc/domain/services/device_context/device_context.h"

#include <dirent.h>
#include <unordered_map>
#include <typeindex>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "nlohmann/json.hpp"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/infrastructure/process/process_topo.h"
#include "analysis/csrc/infrastructure/process/include/process_control.h"


namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;
using namespace Analysis::Infra;

const auto PROF_DIR = "./PROF_0";
const auto DEVICE_DIR = "device_0";
const auto INFO_JSON = "info.json.0";
const auto SAMPLE_JSON = "sample.json";


class DeviceContextUTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_TRUE(File::CreateDir(PROF_DIR));
        const auto deviceDir = File::PathJoin({PROF_DIR, DEVICE_DIR});
        EXPECT_TRUE(File::CreateDir(deviceDir));
        CreateJson(deviceDir);
    }

    static void TearDownTestCase()
    {
        EXPECT_TRUE(File::RemoveDir(PROF_DIR, 0));
    }

    static void CreateJson(const std::string &filePath)
    {
        // info.json.0
        nlohmann::json info = {
            {"platform_version", "5"},
            {"devices", "0"},
            {"DeviceInfo", {{{"hwts_frequency", "49.000000"},
                             {"aic_frequency", "1850"},
                             {"aiv_frequency", "1850"},
                             {"ai_core_num", 25},
                             {"aiv_num", 25}}}},
            {"CPU", {{{"Frequency", "100.000000"}}}},
        };
        FileWriter infoWriter(File::PathJoin({filePath, INFO_JSON}));
        infoWriter.WriteText(info.dump());
        // sample.json
        nlohmann::json sample = {
            {"ai_core_profiling", "on"},
            {"ai_core_metrics", "PipeUtilization"},
            {"ai_core_profiling_events", "0x416,0x417,0x9,0x302,0xc,0x303,0x54,0x55"},
            {"ai_core_profiling_mode", "task-based"},
            {"aicore_sampling_interval", 10},
            {"aiv_profiling", "on"},
            {"aiv_metrics", "PipeUtilization"},
            {"aiv_profiling_events", "0x416,0x417, "}
        };
        FileWriter sampleWriter(File::PathJoin({filePath, SAMPLE_JSON}));
        sampleWriter.WriteText(sample.dump());
    }
};

TEST_F(DeviceContextUTest, TestDeviceContextEntryShouldReturn1DataInventoryWhenInfoJsonInvalid)
{
    std::unordered_map<std::type_index, RegProcessInfo> processCollection;
    ProcessControl processControl(processCollection);
    ProcessTopo processTopo(processCollection);
    EXPECT_EQ(1, DeviceContextEntry(PROF_DIR, "").size());
}

TEST_F(DeviceContextUTest, TestGetDeviceDirectoriesShouldPrintErrorLogWhenOpenDirectoryFailed)
{
    MOCKER_CPP(opendir).stubs().will(returnValue((DIR*)nullptr));
    auto subdirs = GetDeviceDirectories(PROF_DIR);
    EXPECT_TRUE(subdirs.empty());
    GlobalMockObject::verify();
}

struct dirent* Mockreaddir(DIR *dir)
{
    errno = ENOENT; // 设置errno为ENOENT
    return nullptr; // 返回nullptr
}

TEST_F(DeviceContextUTest, TestGetDeviceDirectoriesShouldPrintErrorLogWhenReadDirectoryFailed)
{
    MOCKER_CPP(readdir).stubs().will(invoke(Mockreaddir, nullptr));
    auto subdirs = GetDeviceDirectories(PROF_DIR);
    EXPECT_TRUE(subdirs.empty());
    GlobalMockObject::verify();
}
}  // Domain
}  // Analysis

