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
#include "params_adapter_impl.h"
#include "params_adapter.h"
#include "input_parser.h"
#include "platform/platform.h"

using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Msprof;
using namespace Collector::Dvvp::Common::PlatformAdapter;
using namespace Analysis::Dvvp::Common::Config;
using namespace Collector::Dvvp::ParamsAdapter;
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

TEST_F(MSPROF_BIN_UTEST, PlatformAdapterModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapter> PlatformAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterMgr, PlatformAdapter);
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    SHARED_PTR_ALIA<PlatformAdapterInterface> ret = PlatformAdapterMgr->Init(params, PlatformType::END_TYPE);
    EXPECT_EQ(nullptr, ret);
    
    ret = PlatformAdapterMgr->Init(params, PlatformType::MINI_TYPE);
    int val = ret->Init(params);
    EXPECT_EQ(PROFILING_SUCCESS, val);
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

TEST_F(MSPROF_BIN_UTEST, MsprofParamAdapterModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofParamAdapter> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, MsprofParamAdapter);
    int ret = MsprofParamAdapterMgr->Init();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    std::vector<std::pair<InputCfg, std::string>> cfgList;
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::MsprofCheckAppValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::MsprofCheckEnvValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::IsValidSwitch)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::MsprofParamAdapter::ParamsCheckMsprofV1)
        .stubs()
        .will(returnValue(true));
    for (auto setCfg : MsprofParamAdapterMgr->msprofConfig_) {
        MsprofParamAdapterMgr->setConfig_.insert(setCfg);
        ret = MsprofParamAdapterMgr->ParamsCheckMsprof(cfgList);
        EXPECT_EQ(PROFILING_SUCCESS, ret);
    }
}

TEST_F(MSPROF_BIN_UTEST, ParamsCheckMsprofV1)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofParamAdapter> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, MsprofParamAdapter);
    int ret;
    std::vector<InputCfg> inputCfgList = {
        INPUT_CFG_COM_AIV_MODE,
        INPUT_CFG_COM_AIC_MODE,
        INPUT_CFG_COM_BIU_FREQ,
        INPUT_CFG_COM_SYS_DEVICES,
        INPUT_CFG_COM_SYS_PERIOD,
        INPUT_CFG_HOST_SYS,
        INPUT_CFG_HOST_SYS_PID,
        INPUT_CFG_PYTHON_PATH,
        INPUT_CFG_SUMMARY_FORMAT,
        INPUT_CFG_ITERATION_ID
    };
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::MsprofCheckAiModeValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::CheckFreqValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::MsprofCheckSysDeviceValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::MsprofCheckSysPeriodValid)
            .stubs()
            .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::MsprofCheckHostSysValid)
            .stubs()
            .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckHostSysPidValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckExportIdIsValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckExportSummaryFormatIsValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckPythonPathIsValid)
        .stubs()
        .will(returnValue(true));
    for (auto inputCfg : inputCfgList) {
        ret = MsprofParamAdapterMgr->ParamsCheckMsprofV1(inputCfg, " ");
        EXPECT_EQ(true, ret);
    }
}

TEST_F(MSPROF_BIN_UTEST, SetDefaultParamsApp)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofParamAdapter> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, MsprofParamAdapter);
    MsprofParamAdapterMgr->SetDefaultParamsApp();
}

TEST_F(MSPROF_BIN_UTEST, GetMsprofMode)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofParamAdapter> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, MsprofParamAdapter);
    int ret = MsprofParamAdapterMgr->GetMsprofMode();
    EXPECT_EQ(PROFILING_FAILED, ret);
    std::vector<InputCfg> cfgList = {
        INPUT_CFG_MSPROF_APPLICATION,
        INPUT_CFG_COM_SYS_DEVICES,
        INPUT_CFG_HOST_SYS,
        INPUT_CFG_PARSE,
        INPUT_CFG_QUERY,
        INPUT_CFG_EXPORT
    };
    for (auto cfg : cfgList) {
        MsprofParamAdapterMgr->paramContainer_[cfg] = "on";
        ret = MsprofParamAdapterMgr->GetMsprofMode();
        EXPECT_EQ(PROFILING_SUCCESS, ret);
    }
}

TEST_F(MSPROF_BIN_UTEST, MsprofSetDefaultParams)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofParamAdapter> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, MsprofParamAdapter);
    MsprofParamAdapterMgr->SetDefaultParamsSystem();
    MsprofParamAdapterMgr->SetDefaultParamsParse();
    MsprofParamAdapterMgr->SetDefaultParamsQuery();
    MsprofParamAdapterMgr->SetDefaultParamsExport();
}

TEST_F(MSPROF_BIN_UTEST, SpliteAppPath)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofParamAdapter> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, MsprofParamAdapter);
    std::string appParam;
    MsprofParamAdapterMgr->SpliteAppPath(appParam);
    appParam = "./main 1";
    MsprofParamAdapterMgr->SpliteAppPath(appParam);
    appParam = "/bin/bash test.sh 1";
    MsprofParamAdapterMgr->SpliteAppPath(appParam);
}

TEST_F(MSPROF_BIN_UTEST, SetParamsSelf)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofParamAdapter> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, MsprofParamAdapter);
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    MsprofParamAdapterMgr->params_ = params;
    MsprofParamAdapterMgr->SetParamsSelf();
}

TEST_F(MSPROF_BIN_UTEST, CreateCfgMap)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofParamAdapter> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, MsprofParamAdapter);
    MsprofParamAdapterMgr->CreateCfgMap();
}

TEST_F(MSPROF_BIN_UTEST, SetModeDefaultParams)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofParamAdapter> MsprofParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(MsprofParamAdapterMgr, MsprofParamAdapter);
    std::vector<MsprofMode> modeTypeList = {
        MSPROF_MODE_APP,
        MSPROF_MODE_SYSTEM,
        MSPROF_MODE_PARSE,
        MSPROF_MODE_QUERY,
        MSPROF_MODE_EXPORT
    };
    int ret;
    for (auto modeType : modeTypeList) {
        ret = MsprofParamAdapterMgr->SetModeDefaultParams(modeType);
        EXPECT_EQ(PROFILING_SUCCESS, ret);
    }
}

TEST_F(MSPROF_BIN_UTEST, AclJsonParamAdapterModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<AclJsonParamAdapter> AclJsonParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclJsonParamAdapterMgr, AclJsonParamAdapter);
    int ret = AclJsonParamAdapterMgr->Init();
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    SHARED_PTR_ALIA<analysis::dvvp::proto::ProfAclConfig> aclCfg;
    MSVP_MAKE_SHARED0_VOID(aclCfg, analysis::dvvp::proto::ProfAclConfig);
    aclCfg->set_storage_limit("200MB");
    ret = AclJsonParamAdapterMgr->GenAclJsonContainer(aclCfg);
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::BlackSwitchCheck)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_BIN_UTEST, GenAclJsonContainer)
{
    GlobalMockObject::verify();
    std::shared_ptr<AclJsonParamAdapter> AclJsonParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclJsonParamAdapterMgr, AclJsonParamAdapter);
    SHARED_PTR_ALIA<analysis::dvvp::proto::ProfAclConfig> aclCfg;
    MSVP_MAKE_SHARED0_VOID(aclCfg, analysis::dvvp::proto::ProfAclConfig);
    aclCfg->set_storage_limit("200MB");
    aclCfg->set_l2("on");
    int ret = AclJsonParamAdapterMgr->GenAclJsonContainer(aclCfg);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_BIN_UTEST, ParamsCheckAclJson)
{
    GlobalMockObject::verify();
    std::shared_ptr<AclJsonParamAdapter> AclJsonParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclJsonParamAdapterMgr, AclJsonParamAdapter);
    AclJsonParamAdapterMgr->Init();
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::IsValidSwitch)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::CheckFreqValid)
        .stubs()
        .will(returnValue(true));
    std::vector<std::pair<InputCfg, std::string>> cfgList;
    for (auto cfg : AclJsonParamAdapterMgr->aclJsonConfig_) {
        AclJsonParamAdapterMgr->setConfig_.insert(cfg);
        int ret = AclJsonParamAdapterMgr->ParamsCheckAclJson(cfgList);
        EXPECT_EQ(PROFILING_SUCCESS, ret);
    }
}

TEST_F(MSPROF_BIN_UTEST, SetAclJsonContainerDefaultValue)
{
    GlobalMockObject::verify();
    std::shared_ptr<AclJsonParamAdapter> AclJsonParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclJsonParamAdapterMgr, AclJsonParamAdapter);
    AclJsonParamAdapterMgr->setConfig_.insert(INPUT_CFG_COM_AI_VECTOR);
    int ret = AclJsonParamAdapterMgr->SetAclJsonContainerDefaultValue();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_BIN_UTEST, AclJsonSetOutputDir)
{
    GlobalMockObject::verify();
    std::shared_ptr<AclJsonParamAdapter> AclJsonParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclJsonParamAdapterMgr, AclJsonParamAdapter);
    
    std::string output;
    std::string ret = AclJsonParamAdapterMgr->SetOutputDir(output);
    std::string result1 = analysis::dvvp::common::utils::Utils::GetSelfPath();
    size_t pos = result1.rfind(MSVP_SLASH);
        if (pos != std::string::npos) {
            result1 = result1.substr(0, pos + 1);
        }
    EXPECT_EQ(result1, ret);

    std::string result2 = analysis::dvvp::common::utils::Utils::GetSelfPath();
    result2 = analysis::dvvp::common::utils::Utils::DirName(result2) + "/";
    output = "/var";
    ret = AclJsonParamAdapterMgr->SetOutputDir(output);
    EXPECT_EQ(result2, ret);

    output = "/tmp/test";
    ret = AclJsonParamAdapterMgr->SetOutputDir(output);
    EXPECT_EQ(output, ret);
}

TEST_F(MSPROF_BIN_UTEST, GeOptParamAdapterModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<GeOptParamAdapter> GeOptParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(GeOptParamAdapterMgr, GeOptParamAdapter);
    int ret = GeOptParamAdapterMgr->Init();
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    SHARED_PTR_ALIA<analysis::dvvp::proto::ProfGeOptionsConfig> geCfg;
    MSVP_MAKE_SHARED0_VOID(geCfg, analysis::dvvp::proto::ProfGeOptionsConfig);
    geCfg->set_storage_limit("200MB");
    ret = GeOptParamAdapterMgr->GenGeOptionsContainer(geCfg);
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::BlackSwitchCheck)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_BIN_UTEST, ParamsCheckGeOpt)
{
    GlobalMockObject::verify();
    std::shared_ptr<GeOptParamAdapter> GeOptParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(GeOptParamAdapterMgr, GeOptParamAdapter);
    GeOptParamAdapterMgr->Init();
    std::vector<std::pair<InputCfg, std::string>> cfgList;
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::IsValidSwitch)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::CheckFreqValid)
        .stubs()
        .will(returnValue(true));
    for (auto cfg : GeOptParamAdapterMgr->geOptConfig_) {
        GeOptParamAdapterMgr->setConfig_.insert(cfg);
        int ret = GeOptParamAdapterMgr->ParamsCheckGeOpt(cfgList);
        EXPECT_EQ(PROFILING_SUCCESS, ret);
    }
}

TEST_F(MSPROF_BIN_UTEST, SetGeOptionsContainerDefaultValue)
{
    GlobalMockObject::verify();
    std::shared_ptr<GeOptParamAdapter> GeOptParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(GeOptParamAdapterMgr, GeOptParamAdapter);

    GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AIC_METRICS] = "ArithmeticUtilization";
    GeOptParamAdapterMgr->paramContainer_[INPUT_CFG_COM_AIV_METRICS] = "ArithmeticUtilization";
    int ret = GeOptParamAdapterMgr->SetGeOptionsContainerDefaultValue();
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::GeOptParamAdapter::SetOutputDir)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::PlatformIsHelperHostSide)
        .stubs()
        .will(returnValue(false));
    EXPECT_EQ(PROFILING_FAILED, ret);
}

TEST_F(MSPROF_BIN_UTEST, GeOptionsSetOutputDir)
{
    GlobalMockObject::verify();
    std::shared_ptr<GeOptParamAdapter> GeOptParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(GeOptParamAdapterMgr, GeOptParamAdapter);

    std::string output;
    int ret = GeOptParamAdapterMgr->SetOutputDir(output);
    EXPECT_EQ(PROFILING_FAILED, ret);
    output = "./";
    ret = GeOptParamAdapterMgr->SetOutputDir(output);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    output = "/var";
    ret = GeOptParamAdapterMgr->SetOutputDir(output);
    EXPECT_EQ(PROFILING_FAILED, ret);
}

TEST_F(MSPROF_BIN_UTEST, GeGetParamFromInputCfg)
{
    GlobalMockObject::verify();
    std::shared_ptr<GeOptParamAdapter> GeOptParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(GeOptParamAdapterMgr, GeOptParamAdapter);
    SHARED_PTR_ALIA<analysis::dvvp::proto::ProfGeOptionsConfig> geCfg = nullptr;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params = nullptr;
    int ret = GeOptParamAdapterMgr->GetParamFromInputCfg(geCfg, params);
    EXPECT_EQ(PROFILING_FAILED, ret);
    MSVP_MAKE_SHARED0_VOID(geCfg, analysis::dvvp::proto::ProfGeOptionsConfig);
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::GeOptParamAdapter::GenGeOptionsContainer)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::GeOptParamAdapter::GenGeOptionsContainer)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::GeOptParamAdapter::SetGeOptionsContainerDefaultValue)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::GeOptParamAdapter::SetGeOptionsContainerDefaultValue)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::GeOptParamAdapter::SetGeOptionsContainerDefaultValue)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_FAILED, ret);
}

TEST_F(MSPROF_BIN_UTEST, DevIdToStr)
{
    GlobalMockObject::verify();
    std::shared_ptr<AclApiParamAdapter> AclApiParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclApiParamAdapterMgr, AclApiParamAdapter);

    uint32_t devNum = 2;
    uint32_t devList[devNum] = {
        1, 64
    };
    std::string ret = AclApiParamAdapterMgr->DevIdToStr(devNum, devList);
    std::string result = "1";
    EXPECT_EQ(result, ret);
    devList[0] = 1;
    devList[1] = 2; // 2 test num
    ret = AclApiParamAdapterMgr->DevIdToStr(devNum, devList);
    result = "1,2";
    EXPECT_EQ(result, ret);
}

TEST_F(MSPROF_BIN_UTEST, AclApiParamAdapterInit)
{
    GlobalMockObject::verify();
    std::shared_ptr<AclApiParamAdapter> AclApiParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclApiParamAdapterMgr, AclApiParamAdapter);
    int ret = AclApiParamAdapterMgr->Init();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_BIN_UTEST, ParamsCheckAclApi)
{
    GlobalMockObject::verify();
    std::shared_ptr<AclApiParamAdapter> AclApiParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclApiParamAdapterMgr, AclApiParamAdapter);
    
    std::vector<std::pair<InputCfg, std::string>> cfgList;
    AclApiParamAdapterMgr->Init();
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::IsValidSwitch)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::MsprofCheckSysDeviceValid)
        .stubs()
        .will(returnValue(true));
    int ret = AclApiParamAdapterMgr->ParamsCheckAclApi(cfgList);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_BIN_UTEST, ProfTaskCfgToContainer)
{
    GlobalMockObject::verify();
    std::shared_ptr<AclApiParamAdapter> AclApiParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclApiParamAdapterMgr, AclApiParamAdapter);

    struct ProfConfig apiCfg;
    apiCfg.devNums = 1;
    apiCfg.devIdList[0] = {1};
    apiCfg.dataTypeConfig = PROF_TASK_TIME | PROF_KEYPOINT_TRACE |
                            PROF_L2CACHE | PROF_ACL_API |
                            PROF_AICPU_TRACE | PROF_RUNTIME_API |
                            PROF_HCCL_TRACE | PROF_AICORE_METRICS;
    apiCfg.aicoreMetrics = PROF_AICORE_ARITHMETIC_UTILIZATION;
    std::array<std::string, ACL_PROF_ARGS_MAX> argsArr;
    argsArr[ACL_PROF_STORAGE_LIMIT] = "200MB";
    argsArr[ACL_PROF_AIV_METRICS] = "ArithmeticUtilization";
    AclApiParamAdapterMgr->ProfTaskCfgToContainer(&apiCfg, argsArr);
}

TEST_F(MSPROF_BIN_UTEST, ProfSystemCfgToContainer)
{
    GlobalMockObject::verify();
    std::shared_ptr<AclApiParamAdapter> AclApiParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclApiParamAdapterMgr, AclApiParamAdapter);
    struct ProfConfig apiCfg;
    apiCfg.devNums = 1;
    apiCfg.devIdList[0] = {1};
    apiCfg.dataTypeConfig = PROF_TASK_TIME | PROF_KEYPOINT_TRACE |
                            PROF_L2CACHE | PROF_ACL_API |
                            PROF_AICPU_TRACE | PROF_RUNTIME_API |
                            PROF_HCCL_TRACE | PROF_AICORE_METRICS;
    apiCfg.aicoreMetrics = PROF_AICORE_ARITHMETIC_UTILIZATION;
    std::array<std::string, ACL_PROF_ARGS_MAX> argsArr;
    argsArr[ACL_PROF_SYS_CPU_FREQ] = "10";
    argsArr[ACL_PROF_SYS_IO_FREQ] = "10";
    argsArr[ACL_PROF_SYS_INTERCONNECTION_FREQ] = "10";
    argsArr[ACL_PROF_DVPP_FREQ] = "10";
    argsArr[ACL_PROF_SYS_USAGE_FREQ] = "10";
    argsArr[ACL_PROF_SYS_PID_USAGE_FREQ] = "10";
    argsArr[ACL_PROF_HOST_SYS] = "10";
    AclApiParamAdapterMgr->ProfSystemCfgToContainer(&apiCfg, argsArr);
}

TEST_F(MSPROF_BIN_UTEST, ProfSystemHardwareMemCfgToContainer)
{
    GlobalMockObject::verify();
    std::shared_ptr<AclApiParamAdapter> AclApiParamAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(AclApiParamAdapterMgr, AclApiParamAdapter);
    std::array<std::string, ACL_PROF_ARGS_MAX> argsArr;
    argsArr[ACL_PROF_SYS_HARDWARE_MEM_FREQ] = "10";
    argsArr[ACL_PROF_LLC_MODE] = "10";
    AclApiParamAdapterMgr->ProfSystemHardwareMemCfgToContainer(argsArr);
}

TEST_F(MSPROF_BIN_UTEST, CheckListInit)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapter> ParamsAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(ParamsAdapterMgr, ParamsAdapter);
    int ret = ParamsAdapterMgr->CheckListInit();
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
        .stubs()
        .will(returnValue(PlatformType::MINI_TYPE))
        .then(returnValue(PlatformType::CLOUD_TYPE))
        .then(returnValue(PlatformType::MDC_TYPE))
        .then(returnValue(PlatformType::LHISI_TYPE))
        .then(returnValue(PlatformType::DC_TYPE))
        .then(returnValue(PlatformType::CHIP_V4_1_0));
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_BIN_UTEST, TransToParam)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapter> ParamsAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(ParamsAdapterMgr, ParamsAdapter);

    std::array<std::string, INPUT_CFG_MAX> paramContainer;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    paramContainer[INPUT_CFG_COM_TASK_TRACE] = "on";
    paramContainer[INPUT_CFG_COM_TRAINING_TRACE] = "on";
    paramContainer[INPUT_CFG_COM_HCCL] = "on";
    paramContainer[INPUT_CFG_COM_SYS_USAGE] = "on";
    paramContainer[INPUT_CFG_COM_SYS_CPU] = "on";
    paramContainer[INPUT_CFG_COM_SYS_HARDWARE_MEM] = "on";
    paramContainer[INPUT_CFG_COM_SYS_IO] = "on";
    paramContainer[INPUT_CFG_COM_SYS_INTERCONNECTION] = "on";
    paramContainer[INPUT_CFG_COM_DVPP] = "on";
    paramContainer[INPUT_CFG_COM_POWER] = "on";
    paramContainer[INPUT_CFG_COM_BIU] = "on";
    paramContainer[INPUT_CFG_HOST_SYS] = "cpu,mem,disk,osrt,network";
    paramContainer[INPUT_HOST_SYS_USAGE] = "cpu,mem";
    int ret = ParamsAdapterMgr->TransToParam(paramContainer, params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}


TEST_F(MSPROF_BIN_UTEST, ComCfgCheck2)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapter> ParamsAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(ParamsAdapterMgr, ParamsAdapter);
    std::vector<std::pair<InputCfg, std::string>> cfgErrList;
    std::vector<InputCfg> cfgList = {
        INPUT_CFG_COM_SYS_USAGE_FREQ,
        INPUT_CFG_COM_LLC_MODE,
        INPUT_HOST_SYS_USAGE
    };
    std::string cfgValue = "on";
    MOCKER_CPP(&Collector::Dvvp::ParamsAdapter::ParamsAdapter::CheckFreqValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckLlcModeIsValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckHostSysUsageIsValid)
        .stubs()
        .will(returnValue(true));
    for (auto cfg : cfgList) {
        bool ret = ParamsAdapterMgr->ComCfgCheck2(cfg, cfgValue, cfgErrList);
        EXPECT_EQ(true, ret);
    }
    InputCfg  errCfg = INPUT_CFG_COM_TASK_TIME;
    bool ret = ParamsAdapterMgr->ComCfgCheck2(errCfg, cfgValue, cfgErrList);
    EXPECT_EQ(false, ret);
}

TEST_F(MSPROF_BIN_UTEST, CheckFreqValid)
{
    GlobalMockObject::verify();
    std::shared_ptr<ParamsAdapter> ParamsAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(ParamsAdapterMgr, ParamsAdapter);
    std::string freq = "20";
    InputCfg freqOpt = INPUT_CFG_COM_AIC_FREQ;
    bool ret = ParamsAdapterMgr->CheckFreqValid(freq, freqOpt);
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckFreqIsValid)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(true, ret);
}