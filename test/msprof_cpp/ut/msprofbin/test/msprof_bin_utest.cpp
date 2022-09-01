
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

using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Msprof;
using namespace Collector::Dvvp::Common::PlatformAdapter;
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

TEST_F(MSPROF_BIN_UTEST, PlatformTypeInterfaceModule) {
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterInterface> PlatformTypeInterfaceMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformTypeInterfaceMgr, PlatformAdapterInterface);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;

    int ret = PlatformTypeInterfaceMgr->Init(params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    
    PlatformTypeInterfaceMgr->SetParamsForTaskTime();
    
    PlatformTypeInterfaceMgr->SetParamsForTaskTrace();

    PlatformTypeInterfaceMgr->SetParamsForTrainingTrace();

    PlatformTypeInterfaceMgr->SetParamsForAscendCL();

    PlatformTypeInterfaceMgr->SetParamsForGE();

    PlatformTypeInterfaceMgr->SetParamsForRuntime();

    PlatformTypeInterfaceMgr->SetParamsForAICPU();

    PlatformTypeInterfaceMgr->SetParamsForHCCL();

    PlatformTypeInterfaceMgr->SetParamsForL2Cache();

    std::string metricsType;
    std::string events;
    ret = PlatformTypeInterfaceMgr->GetMetricsEvents(metricsType, events);
    EXPECT_EQ(PROFILING_FAILED, ret);
    metricsType = "ArithmeticUtilization";
    ret = PlatformTypeInterfaceMgr->GetMetricsEvents(metricsType, events);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    metricsType = "xxx";
    ret = PlatformTypeInterfaceMgr->GetMetricsEvents(metricsType, events);
    EXPECT_EQ(PROFILING_FAILED, ret);
    
    std::string aiMode = "task-based";
    std::string metrics = "ArithmeticUtilization";
    int samplingInterval = 500;
    PlatformTypeInterfaceMgr->SetParamsForAicMetrics(aiMode, metrics, samplingInterval);

    PlatformTypeInterfaceMgr->SetParamsForAivMetrics(aiMode, metrics, samplingInterval);

    PlatformTypeInterfaceMgr->SetParamsForDeviceSysCpuMemUsage(samplingInterval);

    PlatformTypeInterfaceMgr->SetParamsForDeviceAiCpuCtrlCpuTSCpuHotFuncPMU(samplingInterval);

    std::string llcMode = "read";
    PlatformTypeInterfaceMgr->SetParamsForDeviceHardwareMem(samplingInterval, llcMode);

    PlatformTypeInterfaceMgr->SetParamsForDeviceIO(samplingInterval);

    PlatformTypeInterfaceMgr->SetParamsForDeviceIntercommection(samplingInterval);

    PlatformTypeInterfaceMgr->SetParamsForDeviceDVPP(samplingInterval);

    int biuFreq = 100;
    PlatformTypeInterfaceMgr->SetParamsForDeviceBIU(biuFreq);

    PlatformTypeInterfaceMgr->SetParamsForDevicePower();
    
    PlatformTypeInterfaceMgr->SetParamsForHostPidCpu();

    PlatformTypeInterfaceMgr->SetParamsForHostPidMem();

    PlatformTypeInterfaceMgr->SetParamsForHostPidDisk();

    PlatformTypeInterfaceMgr->SetParamsForHostPidOSRT();

    PlatformTypeInterfaceMgr->SetParamsForHostNetwork();

    PlatformTypeInterfaceMgr->SetParamsForHostSysAllPidCpuUsage();

    PlatformTypeInterfaceMgr->SetParamsForHostSysAllPidMemUsage();

    ret = PlatformTypeInterfaceMgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_BIN_UTEST, PlatformAdapterCloudModule) {
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterCloud> PlatformAdapterCloudMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterCloudMgr, PlatformAdapterCloud);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapterCloudMgr->Init(params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = PlatformAdapterCloudMgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_BIN_UTEST, PlatformAdapterCloudv2Module) {
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterCloudV2> PlatformAdapterCloudv2Mgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterCloudv2Mgr, PlatformAdapterCloudV2);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapterCloudv2Mgr->Init(params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = PlatformAdapterCloudv2Mgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_BIN_UTEST, PlatformAdapterDcModule) {
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterDc> PlatformAdapterDcMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterDcMgr, PlatformAdapterDc);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapterDcMgr->Init(params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = PlatformAdapterDcMgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_BIN_UTEST, PlatformAdapterLhisiModule) {
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterLhisi> PlatformAdapterLhisiMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterLhisiMgr, PlatformAdapterLhisi);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapterLhisiMgr->Init(params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = PlatformAdapterLhisiMgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_BIN_UTEST, PlatformAdapterMiniModule) {
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterMini> PlatformAdapterMiniMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterMiniMgr, PlatformAdapterMini);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapterMiniMgr->Init(params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = PlatformAdapterMiniMgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MSPROF_BIN_UTEST, PlatformAdapterMdcModule) {
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterMdc> PlatformAdapterMdcMgr;
    MSVP_MAKE_SHARED0_BREAK(PlatformAdapterMdcMgr, PlatformAdapterMdc);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapterMdcMgr->Init(params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = PlatformAdapterMdcMgr->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

