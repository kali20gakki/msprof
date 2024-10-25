/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : info_json_utest.cpp
 * Description        : InfoJson UT
 * Author             : msprof team
 * Creation Date      : 2024/10/24
 * *****************************************************************************
 */
#include "gtest/gtest.h"
#include "errno/error_code.h"
#include "utils/utils.h"
#include "config/config_manager.h"
#include "config/config.h"
#include "mockcpp/mockcpp.hpp"
#include "info_json.h"
#include "mmpa_api.h"
#include "platform/platform.h"
#include "proto/msprofiler.pb.h"
#include "common-utils-stub.h"

using namespace analysis::dvvp::proto;
using namespace analysis::dvvp::host;
using namespace analysis::dvvp::driver;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::config;

const int TEST_HOST_PID = 1;

class InfoJsonUTest : public testing::Test {
public:
    std::string jobInfo;
    std::string devices;
    int hostpid;
protected:
    virtual void SetUp()
    {
        GlobalMockObject::verify();
        jobInfo = "";
        devices = "0";
        hostpid = TEST_HOST_PID;
    }
    virtual void TearDown()
    {
    }
};

TEST_F(InfoJsonUTest, InfoXml_init)
{
    GlobalMockObject::verify();

    MOCKER_CPP(&InfoJson::InitDeviceIds)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCKER_CPP(&InfoJson::AddHostInfo)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCKER_CPP(&InfoJson::AddDeviceInfo)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCKER_CPP(&InfoJson::AddOtherInfo)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    InfoJson infoJson(jobInfo, devices, hostpid);
    std::string cont;

    // init device ids failed
    EXPECT_EQ(PROFILING_FAILED, infoJson.Generate(cont));

    // add host info failed
    EXPECT_EQ(PROFILING_FAILED, infoJson.Generate(cont));

    // add device info failed
    EXPECT_EQ(PROFILING_FAILED, infoJson.Generate(cont));

    // add other info failed
    EXPECT_EQ(PROFILING_FAILED, infoJson.Generate(cont));

    // save file failed
    EXPECT_EQ(PROFILING_SUCCESS, infoJson.Generate(cont));

    // ok
    EXPECT_EQ(PROFILING_SUCCESS, infoJson.Generate(cont));
}

TEST_F(InfoJsonUTest, AddDeviceInfo)
{
    GlobalMockObject::verify();

    MOCKER_CPP(&InfoJson::GetDevInfo)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    InfoJson infoJson(jobInfo, devices, hostpid);
    infoJson.InitDeviceIds();
    std::shared_ptr<InfoMain> infoMain = std::make_shared<InfoMain>();

    // failed to get dev info
    EXPECT_EQ(PROFILING_FAILED, infoJson.AddDeviceInfo(infoMain));

    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
        .stubs()
        .will(returnValue(Analysis::Dvvp::Common::Config::PlatformType::MINI_TYPE))
        .then(returnValue(Analysis::Dvvp::Common::Config::PlatformType::CHIP_V4_1_0));

    // ok
    EXPECT_EQ(PROFILING_SUCCESS, infoJson.AddDeviceInfo(infoMain));
    EXPECT_EQ(PROFILING_SUCCESS, infoJson.AddDeviceInfo(infoMain));
}

TEST_F(InfoJsonUTest, AddOtherInfo)
{
    GlobalMockObject::verify();
    MOCKER(analysis::dvvp::common::utils::Utils::GetMac)
        .stubs()
        .will(returnValue(PROFILING_FAILED));

    InfoJson infoJson(jobInfo, devices, hostpid);
    std::shared_ptr<InfoMain> infoMain = std::make_shared<InfoMain>();

    // failed to get mac, but only warn
    EXPECT_EQ(PROFILING_SUCCESS, infoJson.AddOtherInfo(infoMain));
}

TEST_F(InfoJsonUTest, SetPlatFormVersion)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
        .stubs()
        .will(returnValue(Analysis::Dvvp::Common::Config::PlatformType::MINI_TYPE))
        .then(returnValue(Analysis::Dvvp::Common::Config::PlatformType::CLOUD_TYPE))
        .then(returnValue(Analysis::Dvvp::Common::Config::PlatformType::CHIP_V4_1_0))
        .then(returnValue(Analysis::Dvvp::Common::Config::PlatformType::LHISI_TYPE));

    InfoJson infoJson(jobInfo, devices, hostpid);
    std::shared_ptr<InfoMain> infoMain = std::make_shared<InfoMain>();

    infoJson.SetPlatFormVersion(infoMain);
    infoJson.SetPlatFormVersion(infoMain);
    infoJson.SetPlatFormVersion(infoMain);
    infoJson.SetPlatFormVersion(infoMain);
}

TEST_F(InfoJsonUTest, GetDevInfo)
{
    GlobalMockObject::verify();
    InfoJson infoJson(jobInfo, devices, hostpid);

    analysis::dvvp::host::DeviceInfo dev_info;

    MOCKER_CPP(&analysis::dvvp::driver::DrvGetEnvType)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&InfoJson::GetCtrlCpuInfo)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::driver::DrvGetAiCpuCoreNum)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::driver::DrvGetAivNum)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::driver::DrvGetAiCpuCoreId)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::driver::DrvGetAiCpuOccupyBitmap)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::driver::DrvGetTsCpuCoreNum)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::driver::DrvGetAiCoreId)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::driver::DrvGetAiCoreNum)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, infoJson.GetDevInfo(0, dev_info));
    EXPECT_EQ(PROFILING_FAILED, infoJson.GetDevInfo(0, dev_info));
    EXPECT_EQ(PROFILING_FAILED, infoJson.GetDevInfo(0, dev_info));
    EXPECT_EQ(PROFILING_FAILED, infoJson.GetDevInfo(0, dev_info));
    EXPECT_EQ(PROFILING_FAILED, infoJson.GetDevInfo(0, dev_info));
    EXPECT_EQ(PROFILING_FAILED, infoJson.GetDevInfo(0, dev_info));
    EXPECT_EQ(PROFILING_FAILED, infoJson.GetDevInfo(0, dev_info));
    EXPECT_EQ(PROFILING_FAILED, infoJson.GetDevInfo(0, dev_info));
    EXPECT_EQ(PROFILING_SUCCESS, infoJson.GetDevInfo(0, dev_info));
}

TEST_F(InfoJsonUTest, GetDevInfoCheckAiCpuCoreNum)
{
    GlobalMockObject::verify();
    InfoJson infoJson(jobInfo, devices, hostpid);

    analysis::dvvp::host::DeviceInfo dev_info;

    MOCKER_CPP(&analysis::dvvp::driver::DrvGetEnvType)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&InfoJson::GetCtrlCpuInfo)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::driver::DrvGetAiCpuCoreNum)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::driver::DrvGetAivNum)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::driver::DrvGetAiCpuCoreId)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MOCKER_CPP(&analysis::dvvp::driver::DrvGetAiCpuOccupyBitmap)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::driver::DrvGetTsCpuCoreNum)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::driver::DrvGetAiCoreId)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::driver::DrvGetAiCoreNum)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    dev_info.ai_cpu_core_num = 1;
    EXPECT_EQ(PROFILING_FAILED, infoJson.GetDevInfo(0, dev_info));
    dev_info.ai_cpu_core_num = 0;
    EXPECT_EQ(PROFILING_SUCCESS, infoJson.GetDevInfo(0, dev_info));
}

TEST_F(InfoJsonUTest, SetPidInfo)
{
    GlobalMockObject::verify();
    InfoJson infoJson(jobInfo, devices, hostpid);
    std::shared_ptr<InfoMain> infoMain = std::make_shared<InfoMain>();

    int invalidPid = -1;
    int validPid = 1; // system pid
    // input pid is invalid
    infoJson.SetPidInfo(infoMain, invalidPid);
    EXPECT_EQ("NA", infoMain->pid_name());
    EXPECT_EQ("NA", infoMain->pid());

    // proc file size is invalid
    MOCKER_CPP(&analysis::dvvp::common::utils::Utils::GetFileSize)
        .stubs()
        .will(returnValue(MSVP_LARGE_FILE_MAX_LEN + 1))
        .then(returnValue(MSVP_LARGE_FILE_MAX_LEN));
    infoJson.SetPidInfo(infoMain, validPid);
    EXPECT_EQ("NA", infoMain->pid_name());
    EXPECT_EQ("1", infoMain->pid());

    // proc file size is valid
    infoJson.SetPidInfo(infoMain, validPid);
    EXPECT_NE("NA", infoMain->pid_name());
    EXPECT_EQ("1", infoMain->pid());
}