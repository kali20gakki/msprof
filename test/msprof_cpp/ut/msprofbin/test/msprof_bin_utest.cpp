/**
* @file msprof_bin_utest.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "mockcpp/mockcpp.hpp"
#include "gtest/gtest.h"
#include <iostream>
#include <fstream>
#include "errno/error_code.h"
#include "msprof_bin.h"
#include "running_mode.h"
#include "msprof_manager.h"
#include "message/prof_params.h"
#include "prof_callback.h"
#include "platform/platform_adapter.h"
#include "platform/platform_adapter_interface.h"
#include "platform/platform_adapter_cloud.h"
#include "platform/platform_adapter_cloudv2.h"
#include "platform/platform_adapter_dc.h"
#include "platform/platform_adapter_lhisi.h"
#include "platform/platform_adapter_mdc.h"
#include "platform/platform_adapter_mini.h"
#include "config/config_manager.h"

using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Msprof;
using namespace Collector::Dvvp::Common::PlatformAdapter;
using namespace Analysis::Dvvp::Common::Config;
class MSPROF_BIN_UTEST : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

extern int LltMain(int argc, const char **argv, const char **envp);
extern void SetEnvList(const char **envp, std::vector<std::string> &envpList);

TEST_F(MSPROF_BIN_UTEST, LltMain) {
    GlobalMockObject::verify();
    char* argv[10];
    argv[0] = "--help";
    argv[1] = "--test";
    char* envp[2];
    envp[0] = "test=a";
    envp[1] = "a=b";
    
    MOCKER(&SetEnvList)
        .stubs();
    EXPECT_EQ(PROFILING_FAILED, LltMain(1, (const char**)argv, (const char**)envp));
    EXPECT_EQ(PROFILING_FAILED, LltMain(2, (const char**)argv, (const char**)envp));

    std::ofstream test_file("prof_bin_test");
    test_file << "echo test" << std::endl;
    test_file.close();
    chmod("./prof_bin_test", 0700);
    argv[2] = "--app=./prof_bin_test";
    argv[3] = "--task-time=on";
    EXPECT_EQ(PROFILING_FAILED, LltMain(4, (const char**)argv, (const char**)envp));
    std::remove("prof_bin_test");
    argv[4] = "--sys-devices=0";
    argv[5] = "--output=./msprof_bin_utest";
    argv[6] = "--sys-period=1";
    argv[7] = "--sys-pid-profiling=on";
    EXPECT_EQ(PROFILING_FAILED, LltMain(8, (const char**)argv, (const char**)envp));
}

TEST_F(MSPROF_BIN_UTEST, SetEnvList) {
    GlobalMockObject::verify();
    char* envp[4097];
    char str[] = "a=a";
    for (int i = 0; i < 4097; i++) {
        envp[i] = str;
    }
    envp[4096] = nullptr;
    std::vector<std::string> envpList;
    SetEnvList((const char**)envp, envpList);
}

TEST_F(MSPROF_BIN_UTEST, PlatformAdapterInterfaceModule1)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterInterface> PlatformAdapterInterfaceMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterInterfaceMgr, PlatformAdapterInterface);
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    int ret = PlatformAdapterInterfaceMgr->Init(params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    PlatformAdapterInterfaceMgr->supportSwitch_ = {
        PLATFORM_TASK_ASCENDCL, PLATFORM_TASK_GRAPH_ENGINE, PLATFORM_TASK_RUNTIME, PLATFORM_TASK_AICPU,
        PLATFORM_TASK_HCCL, PLATFORM_TASK_TS_KEYPOINT, PLATFORM_TASK_TS_KEYPOINT_TRAINING, PLATFORM_TASK_TS_MEMCPY,
        PLATFORM_TASK_AIC_METRICS, PLATFORM_TASK_AIV_METRICS, PLATFORM_TASK_STARS_ACSQ,
        PLATFORM_SYS_DEVICE_SYS_CPU_MEM_USAGE, PLATFORM_SYS_DEVICE_ALL_PID_CPU_MEM_USAGE,
        PLATFORM_SYS_DEVICE_TS_CPU_HOT_FUNC_PMU, PLATFORM_SYS_DEVICE_AI_CTRL_CPU_HOT_FUNC_PMU, PLATFORM_SYS_DEVICE_LLC,
        PLATFORM_SYS_DEVICE_DDR, PLATFORM_SYS_DEVICE_HBM, PLATFORM_SYS_DEVICE_NIC, PLATFORM_SYS_DEVICE_ROCE,
        PLATFORM_SYS_DEVICE_HCCS, PLATFORM_SYS_DEVICE_PCIE, PLATFORM_SYS_DEVICE_DVPP, PLATFORM_SYS_DEVICE_BIU,
        PLATFORM_SYS_DEVICE_POWER, PLATFORM_SYS_HOST_ONE_PID_CPU, PLATFORM_SYS_HOST_ONE_PID_MEM,
        PLATFORM_SYS_HOST_ONE_PID_DISK, PLATFORM_SYS_HOST_ONE_PID_OSRT, PLATFORM_SYS_HOST_NETWORK,
        PLATFORM_SYS_HOST_SYS_CPU_MEM_USAGE, PLATFORM_SYS_HOST_ALL_PID_CPU_MEM_USAGE, PLATFORM_TASK_TS_TIMELINE,
        PLATFORM_TASK_AIC_HWTS, PLATFORM_TASK_AIV_HWTS, PLATFORM_TASK_L2_CACHE
    };
    struct CommonParams comParams;
    PlatformAdapterInterfaceMgr->SetParamsForGlobal(comParams);
    PlatformAdapterInterfaceMgr->SetParamsForTaskTime();
    PlatformAdapterInterfaceMgr->SetParamsForTaskTrace();
    PlatformAdapterInterfaceMgr->SetParamsForTrainingTrace();
    PlatformAdapterInterfaceMgr->SetParamsForAscendCL();
    PlatformAdapterInterfaceMgr->SetParamsForGE();
    PlatformAdapterInterfaceMgr->SetParamsForRuntime();
    PlatformAdapterInterfaceMgr->SetParamsForAICPU();
    PlatformAdapterInterfaceMgr->SetParamsForHCCL();
    PlatformAdapterInterfaceMgr->SetParamsForL2Cache();
    std::string metricsType;
    std::string events;
    ret = PlatformAdapterInterfaceMgr->GetMetricsEvents(metricsType, events);
    EXPECT_EQ(PROFILING_FAILED, ret);
    metricsType = "xxx";
    ret = PlatformAdapterInterfaceMgr->GetMetricsEvents(metricsType, events);
    EXPECT_EQ(PROFILING_FAILED, ret);
    metricsType = "ArithmeticUtilization";
    ret = PlatformAdapterInterfaceMgr->GetMetricsEvents(metricsType, events);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    ret = PlatformAdapterInterfaceMgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    params = nullptr;
}

TEST_F(MSPROF_BIN_UTEST, PlatformAdapterInterfaceModule2)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterInterface> PlatformAdapterInterfaceMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterInterfaceMgr, PlatformAdapterInterface);
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    int ret = PlatformAdapterInterfaceMgr->Init(params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    PlatformAdapterInterfaceMgr->supportSwitch_ = {
        PLATFORM_TASK_ASCENDCL, PLATFORM_TASK_GRAPH_ENGINE, PLATFORM_TASK_RUNTIME, PLATFORM_TASK_AICPU,
        PLATFORM_TASK_HCCL, PLATFORM_TASK_TS_KEYPOINT, PLATFORM_TASK_TS_KEYPOINT_TRAINING, PLATFORM_TASK_TS_MEMCPY,
        PLATFORM_TASK_AIC_METRICS, PLATFORM_TASK_AIV_METRICS, PLATFORM_TASK_STARS_ACSQ,
        PLATFORM_SYS_DEVICE_SYS_CPU_MEM_USAGE, PLATFORM_SYS_DEVICE_ALL_PID_CPU_MEM_USAGE,
        PLATFORM_SYS_DEVICE_TS_CPU_HOT_FUNC_PMU, PLATFORM_SYS_DEVICE_AI_CTRL_CPU_HOT_FUNC_PMU, PLATFORM_SYS_DEVICE_LLC,
        PLATFORM_SYS_DEVICE_DDR, PLATFORM_SYS_DEVICE_HBM, PLATFORM_SYS_DEVICE_NIC, PLATFORM_SYS_DEVICE_ROCE,
        PLATFORM_SYS_DEVICE_HCCS, PLATFORM_SYS_DEVICE_PCIE, PLATFORM_SYS_DEVICE_DVPP, PLATFORM_SYS_DEVICE_BIU,
        PLATFORM_SYS_DEVICE_POWER, PLATFORM_SYS_HOST_ONE_PID_CPU, PLATFORM_SYS_HOST_ONE_PID_MEM,
        PLATFORM_SYS_HOST_ONE_PID_DISK, PLATFORM_SYS_HOST_ONE_PID_OSRT, PLATFORM_SYS_HOST_NETWORK,
        PLATFORM_SYS_HOST_SYS_CPU_MEM_USAGE, PLATFORM_SYS_HOST_ALL_PID_CPU_MEM_USAGE, PLATFORM_TASK_TS_TIMELINE,
        PLATFORM_TASK_AIC_HWTS, PLATFORM_TASK_AIV_HWTS, PLATFORM_TASK_L2_CACHE
    };
    std::string metricsType = "ArithmeticUtilization";
    std::string aiMode = "task-based";
    int samplingInterval = 500;
    MOCKER(&PlatformAdapterInterface::GetMetricsEvents)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    PlatformAdapterInterfaceMgr->SetParamsForAicMetrics(aiMode, metricsType, samplingInterval);
    PlatformAdapterInterfaceMgr->SetParamsForAivMetrics(aiMode, metricsType, samplingInterval);
    aiMode = "sample-based";
    PlatformAdapterInterfaceMgr->SetParamsForAicMetrics(aiMode, metricsType, samplingInterval);
    PlatformAdapterInterfaceMgr->SetParamsForAivMetrics(aiMode, metricsType, samplingInterval);
    params = nullptr;
}

TEST_F(MSPROF_BIN_UTEST, PlatformAdapterInterfaceModule3)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterInterface> PlatformAdapterInterfaceMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterInterfaceMgr, PlatformAdapterInterface);
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    int ret = PlatformAdapterInterfaceMgr->Init(params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    PlatformAdapterInterfaceMgr->supportSwitch_ = {
        PLATFORM_TASK_ASCENDCL, PLATFORM_TASK_GRAPH_ENGINE, PLATFORM_TASK_RUNTIME, PLATFORM_TASK_AICPU,
        PLATFORM_TASK_HCCL, PLATFORM_TASK_TS_KEYPOINT, PLATFORM_TASK_TS_KEYPOINT_TRAINING, PLATFORM_TASK_TS_MEMCPY,
        PLATFORM_TASK_AIC_METRICS, PLATFORM_TASK_AIV_METRICS, PLATFORM_TASK_STARS_ACSQ,
        PLATFORM_SYS_DEVICE_SYS_CPU_MEM_USAGE, PLATFORM_SYS_DEVICE_ALL_PID_CPU_MEM_USAGE,
        PLATFORM_SYS_DEVICE_TS_CPU_HOT_FUNC_PMU, PLATFORM_SYS_DEVICE_AI_CTRL_CPU_HOT_FUNC_PMU, PLATFORM_SYS_DEVICE_LLC,
        PLATFORM_SYS_DEVICE_DDR, PLATFORM_SYS_DEVICE_HBM, PLATFORM_SYS_DEVICE_NIC, PLATFORM_SYS_DEVICE_ROCE,
        PLATFORM_SYS_DEVICE_HCCS, PLATFORM_SYS_DEVICE_PCIE, PLATFORM_SYS_DEVICE_DVPP, PLATFORM_SYS_DEVICE_BIU,
        PLATFORM_SYS_DEVICE_POWER, PLATFORM_SYS_HOST_ONE_PID_CPU, PLATFORM_SYS_HOST_ONE_PID_MEM,
        PLATFORM_SYS_HOST_ONE_PID_DISK, PLATFORM_SYS_HOST_ONE_PID_OSRT, PLATFORM_SYS_HOST_NETWORK,
        PLATFORM_SYS_HOST_SYS_CPU_MEM_USAGE, PLATFORM_SYS_HOST_ALL_PID_CPU_MEM_USAGE, PLATFORM_TASK_TS_TIMELINE,
        PLATFORM_TASK_AIC_HWTS, PLATFORM_TASK_AIV_HWTS, PLATFORM_TASK_L2_CACHE
    };
    int samplingInterval = 500;
    PlatformAdapterInterfaceMgr->SetParamsForDeviceSysCpuMemUsage(samplingInterval);
    PlatformAdapterInterfaceMgr->SetParamsForDeviceAllPidCpuMemUsage(samplingInterval);
    PlatformAdapterInterfaceMgr->SetParamsForDeviceAiCpuCtrlCpuTSCpuHotFuncPMU(samplingInterval);
    std::string llcMode = "read";
    PlatformAdapterInterfaceMgr->SetParamsForDeviceHardwareMem(samplingInterval, llcMode);
    PlatformAdapterInterfaceMgr->SetParamsForDeviceIO(samplingInterval);
    PlatformAdapterInterfaceMgr->SetParamsForDeviceIntercommection(samplingInterval);
    PlatformAdapterInterfaceMgr->SetParamsForDeviceDVPP(samplingInterval);
    int biuFreq = 100;
    PlatformAdapterInterfaceMgr->SetParamsForDeviceBIU(biuFreq);
    PlatformAdapterInterfaceMgr->SetParamsForDevicePower();
    PlatformAdapterInterfaceMgr->SetParamsForHostPidCpu();
    PlatformAdapterInterfaceMgr->SetParamsForHostPidMem();
    PlatformAdapterInterfaceMgr->SetParamsForHostPidDisk();
    PlatformAdapterInterfaceMgr->SetParamsForHostPidOSRT();
    PlatformAdapterInterfaceMgr->SetParamsForHostNetwork();
    PlatformAdapterInterfaceMgr->SetParamsForHostSysAllPidCpuUsage();
    PlatformAdapterInterfaceMgr->SetParamsForHostSysAllPidMemUsage();
    params = nullptr;
}

TEST_F(MSPROF_BIN_UTEST, PlatformAdapterCloudModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterCloud> PlatformAdapterCloudMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterCloudMgr, PlatformAdapterCloud);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapterCloudMgr->Init(params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    params = nullptr;

    ret = PlatformAdapterCloudMgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_BIN_UTEST, PlatformAdapterCloudv2Module)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterCloudV2> PlatformAdapterCloudv2Mgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterCloudv2Mgr, PlatformAdapterCloudV2);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapterCloudv2Mgr->Init(params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    params = nullptr;

    ret = PlatformAdapterCloudv2Mgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_BIN_UTEST, PlatformAdapterDcModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterDc> PlatformAdapterDcMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterDcMgr, PlatformAdapterDc);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapterDcMgr->Init(params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    params = nullptr;

    ret = PlatformAdapterDcMgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_BIN_UTEST, PlatformAdapterLhisiModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterLhisi> PlatformAdapterLhisiMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterLhisiMgr, PlatformAdapterLhisi);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapterLhisiMgr->Init(params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    params = nullptr;

    ret = PlatformAdapterLhisiMgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_BIN_UTEST, PlatformAdapterMiniModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterMini> PlatformAdapterMiniMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterMiniMgr, PlatformAdapterMini);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapterMiniMgr->Init(params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    params = nullptr;

    ret = PlatformAdapterMiniMgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_BIN_UTEST, PlatformAdapterMdcModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterMdc> PlatformAdapterMdcMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterMdcMgr, PlatformAdapterMdc);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapterMdcMgr->Init(params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    params = nullptr;

    ret = PlatformAdapterMdcMgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

