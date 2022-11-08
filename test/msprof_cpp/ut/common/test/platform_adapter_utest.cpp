/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: UT for platform adapter
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-10-31
 */
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"

#include <iostream>
#include <fstream>
#include <memory>

#include "platform/platform_adapter.h"
#include "platform/platform_adapter_interface.h"
#include "platform/platform_adapter_cloud.h"
#include "platform/platform_adapter_cloudv2.h"
#include "platform/platform_adapter_dc.h"
#include "platform/platform_adapter_lhisi.h"
#include "platform/platform_adapter_mdc.h"
#include "platform/platform_adapter_mini.h"
#include "errno/error_code.h"

using namespace Collector::Dvvp::Common::PlatformAdapter;
using namespace analysis::dvvp::common::error;

class PlatformAdapterUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
 
    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
};

TEST_F(PlatformAdapterUtest, PlatformAdapterModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapter> PlatformAdapterMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterMgr, PlatformAdapter);
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    SHARED_PTR_ALIA<PlatformAdapterInterface> ret = PlatformAdapterMgr->Init(params, PlatformType::END_TYPE);
    EXPECT_EQ(nullptr, ret);
    
    ret = PlatformAdapterMgr->Init(params, PlatformType::MINI_TYPE);
    int val = ret->Init(params, PlatformType::MINI_TYPE);
    EXPECT_EQ(PROFILING_SUCCESS, val);
}

TEST_F(PlatformAdapterUtest, PlatformAdapterInterfaceModule1)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterInterface> PlatformAdapterInterfaceMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterInterfaceMgr, PlatformAdapterInterface);
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    int ret = PlatformAdapterInterfaceMgr->Init(params, PlatformType::MINI_TYPE);
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
        PLATFORM_TASK_TS_TIMELINE, PLATFORM_SYS_HOST_ALL_PID_CPU, PLATFORM_SYS_HOST_ALL_PID_MEM,
        PLATFORM_TASK_AIC_HWTS, PLATFORM_TASK_AIV_HWTS, PLATFORM_TASK_L2_CACHE, PLATFORM_SYS_HOST_SYS_CPU
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

TEST_F(PlatformAdapterUtest, PlatformAdapterInterfaceModule2)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterInterface> PlatformAdapterInterfaceMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterInterfaceMgr, PlatformAdapterInterface);
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    int ret = PlatformAdapterInterfaceMgr->Init(params, PlatformType::MINI_TYPE);
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
        PLATFORM_TASK_TS_TIMELINE,
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

TEST_F(PlatformAdapterUtest, PlatformAdapterInterfaceModule3)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterInterface> PlatformAdapterInterfaceMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterInterfaceMgr, PlatformAdapterInterface);
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    int ret = PlatformAdapterInterfaceMgr->Init(params, PlatformType::MINI_TYPE);
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
        PLATFORM_TASK_TS_TIMELINE,
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
    PlatformAdapterInterfaceMgr->SetParamsForHostOnePidCpu(samplingInterval);
    PlatformAdapterInterfaceMgr->SetParamsForHostOnePidMem(samplingInterval);
    PlatformAdapterInterfaceMgr->SetParamsForHostPidDisk();
    PlatformAdapterInterfaceMgr->SetParamsForHostPidOSRT();
    PlatformAdapterInterfaceMgr->SetParamsForHostNetwork();
    PlatformAdapterInterfaceMgr->SetParamsForHostAllPidCpu(samplingInterval);
    PlatformAdapterInterfaceMgr->SetParamsForHostAllPidMem(samplingInterval);
    params = nullptr;
}

TEST_F(PlatformAdapterUtest, PlatformAdapterCloudModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterCloud> PlatformAdapterCloudMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterCloudMgr, PlatformAdapterCloud);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapterCloudMgr->Init(params, PlatformType::CLOUD_TYPE);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    params = nullptr;

    ret = PlatformAdapterCloudMgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(PlatformAdapterUtest, PlatformAdapterCloudv2Module)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterCloudV2> PlatformAdapterCloudv2Mgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterCloudv2Mgr, PlatformAdapterCloudV2);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapterCloudv2Mgr->Init(params, PlatformType::CHIP_V4_1_0);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    params = nullptr;

    ret = PlatformAdapterCloudv2Mgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(PlatformAdapterUtest, PlatformAdapterDcModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterDc> PlatformAdapterDcMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterDcMgr, PlatformAdapterDc);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapterDcMgr->Init(params, PlatformType::DC_TYPE);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    params = nullptr;

    ret = PlatformAdapterDcMgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(PlatformAdapterUtest, PlatformAdapterLhisiModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterLhisi> PlatformAdapterLhisiMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterLhisiMgr, PlatformAdapterLhisi);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapterLhisiMgr->Init(params, PlatformType::LHISI_TYPE);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    params = nullptr;

    ret = PlatformAdapterLhisiMgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(PlatformAdapterUtest, PlatformAdapterMiniModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterMini> PlatformAdapterMiniMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterMiniMgr, PlatformAdapterMini);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapterMiniMgr->Init(params, PlatformType::MINI_TYPE);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    params = nullptr;

    ret = PlatformAdapterMiniMgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(PlatformAdapterUtest, PlatformAdapterMdcModule)
{
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterMdc> PlatformAdapterMdcMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterMdcMgr, PlatformAdapterMdc);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapterMdcMgr->Init(params, PlatformType::MDC_TYPE);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    params = nullptr;

    ret = PlatformAdapterMdcMgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}
