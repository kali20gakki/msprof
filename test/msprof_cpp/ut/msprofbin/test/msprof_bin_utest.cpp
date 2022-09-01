
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

TEST_F(MSPROF_BIN_UTEST, PlatformTypeInterfaceInit) {
    GlobalMockObject::verify();
    std::shared_ptr<PlatformAdapterInterface> PlatformTypeInterface;
    MSVP_MAKE_SHARED0_BREAK(PlatformTypeInterface, PlatformAdapterInterface);

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;

    int ret = PlatformTypeInterface->Init(params);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    
    PlatformTypeInterface->SetParamsForTaskTime();
    
    PlatformTypeInterface->SetParamsForTaskTrace();

    PlatformTypeInterface->SetParamsForTrainingTrace();

    PlatformTypeInterface->SetParamsForAscendCL();

    PlatformTypeInterface->SetParamsForGE();

    PlatformTypeInterface->SetParamsForRuntime();

    PlatformTypeInterface->SetParamsForAICPU();

    PlatformTypeInterface->SetParamsForHCCL();

    PlatformTypeInterface->SetParamsForL2Cache();

    std::string metricsType;
    std::string events;
    ret = PlatformTypeInterface->GetMetricsEvents(metricsType, events);
    EXPECT_EQ(PROFILING_FAILED, ret);
    metricsType = "ArithmeticUtilization";
    ret = PlatformTypeInterface->GetMetricsEvents(metricsType, events);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    metricsType = "xxx";
    ret = PlatformTypeInterface->GetMetricsEvents(metricsType, events);
    EXPECT_EQ(PROFILING_FAILED, ret);
    
    std::string aiMode = "task-based";
    std::string metrics = "ArithmeticUtilization";
    int samplingInterval = 500;
    PlatformTypeInterface->SetParamsForAicMetrics(aiMode, metrics, samplingInterval);

    PlatformTypeInterface->SetParamsForAivMetrics(aiMode, metrics, samplingInterval);

    PlatformTypeInterface->SetParamsForDeviceSysCpuMemUsage(samplingInterval);

    PlatformTypeInterface->SetParamsForDeviceAiCpuCtrlCpuTSCpuHotFuncPMU(samplingInterval);

    std::string llcMode = "read";
    PlatformTypeInterface->SetParamsForDeviceHardwareMem(samplingInterval, llcMode);

    PlatformTypeInterface->SetParamsForDeviceIO(samplingInterval);

    PlatformTypeInterface->SetParamsForDeviceIntercommection(samplingInterval);

    PlatformTypeInterface->SetParamsForDeviceDVPP(samplingInterval);

    int biuFreq = 100;
    PlatformTypeInterface->SetParamsForDeviceBIU(biuFreq);

    PlatformTypeInterface->SetParamsForDevicePower();
    
    PlatformTypeInterface->SetParamsForHostPidCpu();

    PlatformTypeInterface->SetParamsForHostPidMem();

    PlatformTypeInterface->SetParamsForHostPidDisk();

    PlatformTypeInterface->SetParamsForHostPidOSRT();

    PlatformTypeInterface->SetParamsForHostNetwork();

    PlatformTypeInterface->SetParamsForHostSysAllPidCpuUsage();

    PlatformTypeInterface->SetParamsForHostSysAllPidMemUsage();

    ret = PlatformTypeInterface->Uninit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

