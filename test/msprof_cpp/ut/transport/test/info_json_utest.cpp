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
#include "common-utils-stub.h"

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

TEST_F(InfoJsonUTest, AddOtherInfo)
{
    GlobalMockObject::verify();
    MOCKER(analysis::dvvp::common::utils::Utils::GetMac)
        .stubs()
        .will(returnValue(PROFILING_FAILED));

    InfoJson infoJson(jobInfo, devices, hostpid);

    // failed to get mac, but only warn
    EXPECT_EQ(PROFILING_SUCCESS, infoJson.AddOtherInfo());
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

    infoJson.SetPlatFormVersion();
    infoJson.SetPlatFormVersion();
    infoJson.SetPlatFormVersion();
    infoJson.SetPlatFormVersion();
}

TEST_F(InfoJsonUTest, GetDevInfo)
{
    GlobalMockObject::verify();
    InfoJson infoJson(jobInfo, devices, hostpid);
    analysis::dvvp::host::InfoDeviceInfo dev_info;

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
    EXPECT_EQ(PROFILING_FAILED, infoJson.GetDevInfo(0, dev_info));
}

TEST_F(InfoJsonUTest, GetDevInfoCheckAiCpuCoreNum)
{
    GlobalMockObject::verify();
    InfoJson infoJson(jobInfo, devices, hostpid);

    analysis::dvvp::host::InfoDeviceInfo dev_info;

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

    int invalidPid = -1;
    int validPid = 1; // system pid
    // input pid is invalid
    infoJson.SetPidInfo(invalidPid);
    EXPECT_EQ("NA", infoJson.infoMain_.pid_name);
    EXPECT_EQ("NA", infoJson.infoMain_.pid);

    // proc file size is invalid
    MOCKER_CPP(&analysis::dvvp::common::utils::Utils::GetFileSize)
        .stubs()
        .will(returnValue(MSVP_LARGE_FILE_MAX_LEN + 1))
        .then(returnValue(MSVP_LARGE_FILE_MAX_LEN));
    infoJson.SetPidInfo(validPid);
    EXPECT_EQ("NA", infoJson.infoMain_.pid_name);
    EXPECT_EQ("1", infoJson.infoMain_.pid);

    // proc file size is valid
    infoJson.SetPidInfo(validPid);
    EXPECT_NE("NA", infoJson.infoMain_.pid_name);
    EXPECT_EQ("1", infoJson.infoMain_.pid);
}

TEST_F(InfoJsonUTest, InitDeviceIdsWillReturnFailWhenStrToIntFail)
{
    GlobalMockObject::verify();
    std::vector<std::string> deviceIds{"0"};
    InfoJson infoJson(jobInfo, devices, hostpid);
    MOCKER_CPP(&Utils::Split)
        .stubs()
        .will(returnValue(deviceIds));
    MOCKER_CPP(&Utils::StrToInt)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, infoJson.InitDeviceIds());
}
 
TEST_F(InfoJsonUTest, AddSysTimeWillReturnWhenGetFileSizeReturnInvalidSize)
{
    GlobalMockObject::verify();
    InfoJson infoJson(jobInfo, devices, hostpid);
    long long size1 = -1;
    long long size2 = 512 * 1024 * 1024 + 1;
    MOCKER_CPP(&Utils::GetFileSize)
        .stubs()
        .will(returnValue(size1))
        .then(returnValue(size2));
    infoJson.infoMain_.upTime = "";
    infoJson.AddSysTime();
    EXPECT_EQ("", infoJson.infoMain_.upTime);
    infoJson.AddSysTime();
    EXPECT_EQ("", infoJson.infoMain_.upTime);
}

TEST_F(InfoJsonUTest, AddMemTotalWillReturnWhenGetFileSizeReturnInvalidSize)
{
    GlobalMockObject::verify();
    InfoJson infoJson(jobInfo, devices, hostpid);
    long long size1 = -1;
    long long size2 = 512 * 1024 * 1024 + 1;
    MOCKER_CPP(&Utils::GetFileSize)
        .stubs()
        .will(returnValue(size1))
        .then(returnValue(size2));
    infoJson.infoMain_.memoryTotal = 0;
    infoJson.AddMemTotal();
    EXPECT_EQ(0, infoJson.infoMain_.memoryTotal);
    infoJson.AddMemTotal();
    EXPECT_EQ(0, infoJson.infoMain_.memoryTotal);
}
 
TEST_F(InfoJsonUTest, AddHostInfoWillReturnFailWhenMemSetFail)
{
    GlobalMockObject::verify();
    InfoJson infoJson(jobInfo, devices, hostpid);
    MOCKER(memset_s)
        .stubs()
        .will(returnValue(EOK - 1));
    EXPECT_EQ(PROFILING_FAILED, infoJson.AddHostInfo());
}
 
TEST_F(InfoJsonUTest, GetCtrlCpuInfoWillReturnFailWhenGetCtrlCouInfoFail)
{
    GlobalMockObject::verify();
    InfoJson infoJson(jobInfo, devices, hostpid);
    MOCKER(DrvGetCtrlCpuId)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER(DrvGetCtrlCpuCoreNum)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER(DrvGetCtrlCpuEndianLittle)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    uint32_t dev = 0;
    struct analysis::dvvp::host::InfoDeviceInfo devInfo;
    EXPECT_EQ(PROFILING_FAILED, infoJson.GetCtrlCpuInfo(dev, devInfo));
    EXPECT_EQ(PROFILING_FAILED, infoJson.GetCtrlCpuInfo(dev, devInfo));
    EXPECT_EQ(PROFILING_FAILED, infoJson.GetCtrlCpuInfo(dev, devInfo));
    EXPECT_EQ(PROFILING_SUCCESS, infoJson.GetCtrlCpuInfo(dev, devInfo));
}
