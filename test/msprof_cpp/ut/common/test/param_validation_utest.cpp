#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <string>
#include <memory>
#include "message/prof_params.h"

#include "param_validation.h"
#include "platform/platform.h"
#include "devdrv_runtime_api_stub.h"
#include "errno/error_code.h"
#include "ai_drv_dev_api.h"
#include "utils/utils.h"
#include "config/config_manager.h"
#include "mmpa_api.h"
#include "platform/platform_adapter.h"
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Mmpa;
using namespace Analysis::Dvvp::Common::Config;
using namespace Collector::Dvvp::Common::PlatformAdapter;
using namespace analysis::dvvp::common::validation;

class ParamValidationUtest: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }

};

static int _drv_get_dev_ids(int num_devices, std::vector<int> & dev_ids) {
    static int phase = 0;
    if (phase == 0) {
        phase++;
        return -1;
    }

    if (phase >= 1) {
        dev_ids.push_back(0);
        return 0;
    }
}

TEST_F(ParamValidationUtest, CheckProfilingParams) {
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(new analysis::dvvp::message::ProfileParams());
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    params->devices = "all";
    //jobStartReq == nullptr
    EXPECT_EQ(false, entry->CheckProfilingParams(nullptr));
    //job_id is empty
    EXPECT_EQ(true, entry->CheckProfilingParams(params));
    params->host_sys = "cpu";
    EXPECT_EQ(true, entry->CheckProfilingParams(params));
    //job_id is illegal
    params->job_id = "0aA-$";
    EXPECT_EQ(true, entry->CheckProfilingParams(params));
    params->job_id = "0aA-";
    EXPECT_EQ(true, entry->CheckProfilingParams(params));
    //profiling_mode is empty
    EXPECT_EQ(true, entry->CheckProfilingParams(params));
    params->profiling_mode = "def_mode";
    params->devices = "0,1";
    EXPECT_EQ(true, entry->CheckProfilingParams(params));
    params->devices = "0,1,a";
    EXPECT_EQ(false, entry->CheckProfilingParams(params));
    params->devices = "0,88";
    EXPECT_EQ(false, entry->CheckProfilingParams(params));
    params->devices = "7";
    EXPECT_EQ(true, entry->CheckProfilingParams(params));
    params->devices = "0,1";
    params->l2CacheTaskProfilingEvents = "0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b,0x5b";
    EXPECT_EQ(false, entry->CheckProfilingParams(params));

    params->l2CacheTaskProfilingEvents = "0x5b,0x5b,0x5b,0x5b";

    params->llc_profiling_events = "asdff_$";
    EXPECT_EQ(false, entry->CheckProfilingParams(params));
    params->llc_profiling_events = "asdff_01";
    EXPECT_EQ(true, entry->CheckProfilingParams(params));
}

TEST_F(ParamValidationUtest, CheckProfilingIntervalIsValid) {
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(new analysis::dvvp::message::ProfileParams());
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(false, entry->CheckProfilingIntervalIsValid(nullptr));
    params->cpu_sampling_interval = 0;
    EXPECT_EQ(false, entry->CheckProfilingIntervalIsValid(params));
    params->cpu_sampling_interval = 10;
    params->sys_sampling_interval = 0;
    EXPECT_EQ(false, entry->CheckProfilingIntervalIsValid(params));
    params->sys_sampling_interval = 10;
    params->aicore_sampling_interval = 0;
    EXPECT_EQ(false, entry->CheckProfilingIntervalIsValid(params));
    params->aiv_sampling_interval = 0;
    params->aicore_sampling_interval = 10;
    EXPECT_EQ(false, entry->CheckProfilingIntervalIsValid(params));
    params->aiv_sampling_interval = 10;
    params->hccsInterval  = 0;
    EXPECT_EQ(false, entry->CheckProfilingIntervalIsValid(params));
    params->hccsInterval = 10;
    params->pcieInterval  = 0;
    EXPECT_EQ(false, entry->CheckProfilingIntervalIsValid(params));
    params->pcieInterval = 10;
    params->roceInterval  = 0;
    EXPECT_EQ(false, entry->CheckProfilingIntervalIsValid(params));
    params->roceInterval = 10;
    params->llc_interval  = 0;
    EXPECT_EQ(false, entry->CheckProfilingIntervalIsValid(params));
    params->llc_interval = 10;
    params->ddr_interval  = 0;
    EXPECT_EQ(false, entry->CheckProfilingIntervalIsValid(params));
    params->ddr_interval = 10;
    params->hbmInterval  = 0;
    EXPECT_EQ(false, entry->CheckProfilingIntervalIsValid(params));
    params->hbmInterval = 10;
    params->hardware_mem_sampling_interval   = 0;
    EXPECT_EQ(false, entry->CheckProfilingIntervalIsValid(params));
    params->hardware_mem_sampling_interval = 10;
    params->profiling_period    = 0;
    EXPECT_EQ(true, entry->CheckProfilingIntervalIsValid(params));
}

TEST_F(ParamValidationUtest, CheckSystemTraceSwitchProfiling) {
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(new analysis::dvvp::message::ProfileParams());
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(false, entry->CheckSystemTraceSwitchProfiling(nullptr));
    params->cpu_profiling = "asd";
    EXPECT_EQ(false, entry->CheckSystemTraceSwitchProfiling(params));
    params->cpu_profiling = "on";
    params->tsCpuProfiling = "asd";
    EXPECT_EQ(false, entry->CheckSystemTraceSwitchProfiling(params));
    params->tsCpuProfiling = "on";
    params->aiCtrlCpuProfiling = "asd";
    EXPECT_EQ(false, entry->CheckSystemTraceSwitchProfiling(params));
    params->aiCtrlCpuProfiling = "on";
    params->sys_profiling = "asd";
    EXPECT_EQ(false, entry->CheckSystemTraceSwitchProfiling(params));
    params->sys_profiling = "on";
    params->pid_profiling = "asd";
    EXPECT_EQ(false, entry->CheckSystemTraceSwitchProfiling(params));
    params->pid_profiling = "on";
    params->hardware_mem = "asd";
    EXPECT_EQ(false, entry->CheckSystemTraceSwitchProfiling(params));
    params->hardware_mem = "on";
    params->io_profiling = "asd";
    EXPECT_EQ(false, entry->CheckSystemTraceSwitchProfiling(params));
    params->io_profiling = "on";
    params->interconnection_profiling = "asd";
    EXPECT_EQ(false, entry->CheckSystemTraceSwitchProfiling(params));
    params->interconnection_profiling = "on";
    params->dvpp_profiling = "asd";
    EXPECT_EQ(false, entry->CheckSystemTraceSwitchProfiling(params));
    params->dvpp_profiling = "on";
    params->nicProfiling = "asd";
    EXPECT_EQ(false, entry->CheckSystemTraceSwitchProfiling(params));
    params->nicProfiling = "on";
    params->roceProfiling = "asd";
    EXPECT_EQ(false, entry->CheckSystemTraceSwitchProfiling(params));
    params->roceProfiling = "on";
    EXPECT_EQ(true, entry->CheckSystemTraceSwitchProfiling(params));
}

TEST_F(ParamValidationUtest, CheckPmuSwitchProfiling) {
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(new analysis::dvvp::message::ProfileParams());
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    params->ai_core_profiling = "asd";
    EXPECT_EQ(false, entry->CheckPmuSwitchProfiling(params));
    params->ai_core_profiling = "on";
    params->aiv_profiling = "asd";
    EXPECT_EQ(false, entry->CheckPmuSwitchProfiling(params));
    params->aiv_profiling = "on";
    params->llc_profiling = "on";
    params->ddr_profiling="asd";
    EXPECT_EQ(false, entry->CheckPmuSwitchProfiling(params));
    params->ddr_profiling="on";
    params->hccsProfiling = "asd";
    EXPECT_EQ(false, entry->CheckPmuSwitchProfiling(params));
    params->pcieProfiling = "asd";
    params->hccsProfiling = "on";
    EXPECT_EQ(false, entry->CheckPmuSwitchProfiling(params));
    params->hbmProfiling = "asd";
    params->pcieProfiling = "on";
    EXPECT_EQ(false, entry->CheckPmuSwitchProfiling(params));
    params->hbmProfiling = "on";
    EXPECT_EQ(true, entry->CheckPmuSwitchProfiling(params));
}

TEST_F(ParamValidationUtest, CheckAivEventCoresIsValid) {
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    const std::vector<int> coreId = {0,1,2,3,4,5,6,7,8};
    EXPECT_EQ(true, entry->CheckAivEventCoresIsValid(coreId));
    const std::vector<int> coreId1 = {0,1,2,3,-4};
    EXPECT_EQ(false, entry->CheckAivEventCoresIsValid(coreId1));
    const std::vector<int> coreId2 = {0,1,2,3,4};
    EXPECT_EQ(true, entry->CheckAivEventCoresIsValid(coreId2));
}

TEST_F(ParamValidationUtest, CheckTsCpuEventIsValid) {
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    std::vector<std::string> events = {"read","write","read","write","read", "asd","write","read","write"};
    EXPECT_EQ(false, entry->CheckTsCpuEventIsValid(events));
    std::vector<std::string> events1 = {"read","write","read","write"};
    EXPECT_EQ(false, entry->CheckTsCpuEventIsValid(events1));
    std::vector<std::string> events2 = {"0xa"};
    EXPECT_EQ(true, entry->CheckTsCpuEventIsValid(events2));
}

TEST_F(ParamValidationUtest, CheckDdrEventsIsValid) {
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    std::vector<std::string> events = {"read","write","read","write","read","write","read","write","read","write"};
    EXPECT_EQ(false, entry->CheckDdrEventsIsValid(events));
    events = {"read","write1"};
    EXPECT_EQ(false, entry->CheckDdrEventsIsValid(events));
    events = {"read","write"};
    EXPECT_EQ(true, entry->CheckDdrEventsIsValid(events));
    events = {"master_id"};
    EXPECT_EQ(true, entry->CheckDdrEventsIsValid(events));
}

TEST_F(ParamValidationUtest, CheckHbmEventsIsValid) {
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    std::vector<std::string> events = {"read","write","read","write","read","write","read","write","read","write"};
    EXPECT_EQ(false, entry->CheckHbmEventsIsValid(events));
    events = {"read","write1"};
    EXPECT_EQ(false, entry->CheckHbmEventsIsValid(events));
    events = {"read","write"};
    EXPECT_EQ(true, entry->CheckHbmEventsIsValid(events));
}

TEST_F(ParamValidationUtest, CheckAivEventsIsValid) {
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    const std::vector<std::string> aiv;
    EXPECT_EQ(true, entry->CheckAivEventsIsValid(aiv));
}

TEST_F(ParamValidationUtest, CheckAppNameIsValid) {
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(false, entry->CheckAppNameIsValid(""));

    const std::string appName = "asd0";
    EXPECT_EQ(true, entry->CheckAppNameIsValid(appName));

    const std::string appName2 = ".;";
    EXPECT_EQ(false, entry->CheckAppNameIsValid(appName2));

    const std::string appName3 = "|";
    EXPECT_EQ(false, entry->CheckAppNameIsValid(appName3));

    const std::string invalidAppName = "$SD";
    EXPECT_EQ(false, entry->CheckAppNameIsValid(invalidAppName));
}

TEST_F(ParamValidationUtest, Init) {
    GlobalMockObject::verify();
    EXPECT_EQ(0, analysis::dvvp::common::validation::ParamValidation::instance()->Init());
}

TEST_F(ParamValidationUtest, UnInit) {
    GlobalMockObject::verify();
    EXPECT_EQ(0, analysis::dvvp::common::validation::ParamValidation::instance()->Uninit());
}

TEST_F(ParamValidationUtest, CheckLlcEventsIsValid) {
    GlobalMockObject::verify();

    std::string llcEvents;
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(true, entry->CheckLlcEventsIsValid(llcEvents));
    llcEvents = "/$$}";
    EXPECT_EQ(false, entry->CheckLlcEventsIsValid(llcEvents));
    llcEvents = "read";
    EXPECT_EQ(true, entry->CheckLlcEventsIsValid(llcEvents));
    llcEvents = "/llc/0/abc";
    EXPECT_EQ(true, entry->CheckLlcEventsIsValid(llcEvents));
}

TEST_F(ParamValidationUtest, CheckEventsSize)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(PROFILING_SUCCESS, entry->CheckEventsSize(""));
    EXPECT_EQ(PROFILING_FAILED, entry->CheckEventsSize("0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9"));
}

TEST_F(ParamValidationUtest, CovertMetricToHex)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    std::string event;
    const int HEX_MODE = 16;
    const int DEC_MODE = 10;
    EXPECT_EQ(false, entry->CovertMetricToHex(event, HEX_MODE));
    event = "$";
    EXPECT_EQ(false, entry->CovertMetricToHex(event, HEX_MODE));
    event = "/$";
    EXPECT_EQ(false, entry->CovertMetricToHex(event, HEX_MODE));
    event = "0x/$";
    EXPECT_EQ(false, entry->CovertMetricToHex(event, HEX_MODE));
    event = "0x2f";
    EXPECT_EQ(true, entry->CovertMetricToHex(event, HEX_MODE));
    event = "/$";
    EXPECT_EQ(false, entry->CovertMetricToHex(event, DEC_MODE));
    event = "2147483648";
    EXPECT_EQ(false, entry->CovertMetricToHex(event, DEC_MODE));
    event = "2147483647";
    EXPECT_EQ(true, entry->CovertMetricToHex(event, DEC_MODE));
    event = "20";
    EXPECT_EQ(true, entry->CovertMetricToHex(event, DEC_MODE));
    EXPECT_EQ("0x14", event);
}

TEST_F(ParamValidationUtest, CovertMetricToHexWillReturnFalseWhenInputInvalidEvents)
{
    GlobalMockObject::verify();
    auto entry = ParamValidation::instance();
    std::string events = "xxx";
    size_t mode = 1;
    MOCKER_CPP(&Utils::Trim)
        .stubs()
        .will(returnValue(events));
    EXPECT_EQ(false, entry->CovertMetricToHex(events, mode));
}

TEST_F(ParamValidationUtest, CustomHexCharConfig)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    std::string aicoreEvent;
    EXPECT_EQ(PROFILING_FAILED, entry->CustomHexCharConfig(aicoreEvent, ","));
    aicoreEvent = "0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9";
    EXPECT_EQ(PROFILING_FAILED, entry->CustomHexCharConfig(aicoreEvent, ","));
    aicoreEvent = "0xw,0x2,0x3,0x4,0x5,0x6,0x7,0x8";
    EXPECT_EQ(PROFILING_FAILED, entry->CustomHexCharConfig(aicoreEvent, ","));
    aicoreEvent = "0x1,0x2,0x3,0x4,0x5,0x6,0x7,0xFF";
    EXPECT_EQ(PROFILING_FAILED, entry->CustomHexCharConfig(aicoreEvent, ","));
    aicoreEvent = "0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x7";
    EXPECT_EQ(PROFILING_FAILED, entry->CustomHexCharConfig(aicoreEvent, ","));
    aicoreEvent = "0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8";
    EXPECT_EQ(PROFILING_SUCCESS, entry->CustomHexCharConfig(aicoreEvent, ","));
}

TEST_F(ParamValidationUtest, CheckProfilingMetricsIsValid) {
    GlobalMockObject::verify();

    std::string aicoreMetrics;
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    int ret = PlatformAdapter::instance()->Init(params, PlatformType::CHIP_V4_1_0);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    EXPECT_EQ(1, entry->CheckProfilingMetricsIsValid("aic-metrics", aicoreMetrics));
    aicoreMetrics = "/$$}";
    EXPECT_EQ(0, entry->CheckProfilingMetricsIsValid("aic-metrics", aicoreMetrics));
    aicoreMetrics = "/$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$";
    EXPECT_EQ(0, entry->CheckProfilingMetricsIsValid("aic-metrics", aicoreMetrics));
    aicoreMetrics = "Custom:0x2,0x3,0x4,0xF";
    EXPECT_EQ(1, entry->CheckProfilingMetricsIsValid("aic-metrics", aicoreMetrics));
    aicoreMetrics = "Custom:";
    EXPECT_EQ(0, entry->CheckProfilingMetricsIsValid("aic-metrics", aicoreMetrics));
    aicoreMetrics = "PipeUtilization";
    EXPECT_EQ(1, entry->CheckProfilingMetricsIsValid("aic-metrics", aicoreMetrics));
    aicoreMetrics = "aicoreMetricsAll";
    EXPECT_EQ(0, entry->CheckProfilingMetricsIsValid("aic-metrics", aicoreMetrics));
    aicoreMetrics = "trace";
    EXPECT_EQ(0, entry->CheckProfilingMetricsIsValid("aic-metrics", aicoreMetrics));
}

TEST_F(ParamValidationUtest, CheckProfilingMetricsIsValidWillReturnFalseWhenGetPlatformDatapterFail)
{
    GlobalMockObject::verify();
    std::string metricsName;
    std::string metricsVal = "x";
    MOCKER_CPP(&PlatformAdapter::GetAdapter)
        .stubs()
        .will(returnValue((PlatformAdapterInterface *)nullptr));
    EXPECT_EQ(false, ParamValidation::instance()->CheckProfilingMetricsIsValid(metricsName, metricsVal));
}

TEST_F(ParamValidationUtest, CheckFreqIsValid)
{
    GlobalMockObject::verify();
    
    std::string freq;
    int rangeMin = 1;
    int rangeMax = 10;
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    freq = "";
    EXPECT_EQ(true, entry->CheckFreqIsValid("freqName", freq, rangeMin, rangeMax));
    freq = "ww11";
    EXPECT_EQ(false, entry->CheckFreqIsValid("freqName", freq, rangeMin, rangeMax));
    freq = "0";
    EXPECT_EQ(false, entry->CheckFreqIsValid("freqName", freq, rangeMin, rangeMax));
    freq = "11";
    EXPECT_EQ(false, entry->CheckFreqIsValid("freqName", freq, rangeMin, rangeMax));
    freq = "5";
    EXPECT_EQ(true, entry->CheckFreqIsValid("freqName", freq, rangeMin, rangeMax));
}

TEST_F(ParamValidationUtest, CheckFreqIsValidWillReturnFalseWhenStrToIntFail)
{
    GlobalMockObject::verify();
    std::string freq;
    int rangeMin = 1;
    int rangeMax = 10;
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    freq = "5";
    MOCKER_CPP(&Utils::StrToInt)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(false, entry->CheckFreqIsValid("freqName", freq, rangeMin, rangeMax));
}

TEST_F(ParamValidationUtest, CheckHostSysPidValid)
{
    GlobalMockObject::verify();
    
    std::string hostSysPid;
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(false, entry->CheckHostSysPidValid(hostSysPid));
    hostSysPid = "ww11";
    EXPECT_EQ(false, entry->CheckHostSysPidValid(hostSysPid));
    hostSysPid = "1";
    EXPECT_EQ(true, entry->CheckHostSysPidValid(hostSysPid));
    hostSysPid = "-1";
    EXPECT_EQ(false, entry->CheckHostSysPidValid(hostSysPid));
    hostSysPid = "100000000";
    EXPECT_EQ(false, entry->CheckHostSysPidValid(hostSysPid));
}

TEST_F(ParamValidationUtest, MsprofCheckEnvValid)
{
    GlobalMockObject::verify();
    
    std::string env;
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(false, entry->MsprofCheckEnvValid(env));
    env = "ww11";
    EXPECT_EQ(true, entry->MsprofCheckEnvValid(env));
}

TEST_F(ParamValidationUtest, MsprofCheckAiModeValid)
{
    GlobalMockObject::verify();
    
    std::string aiMode;
    std::string aiModeType = "";
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(false, entry->MsprofCheckAiModeValid(aiMode, aiModeType));
    aiMode = "ww11";
    EXPECT_EQ(false, entry->MsprofCheckAiModeValid(aiMode, aiModeType));
    aiMode = "task-based";
    EXPECT_EQ(true, entry->MsprofCheckAiModeValid(aiMode, aiModeType));
    aiMode = "sample-based";
    EXPECT_EQ(true, entry->MsprofCheckAiModeValid(aiMode, aiModeType));
}

TEST_F(ParamValidationUtest, MsprofCheckSysDeviceValid)
{
    GlobalMockObject::verify();
    
    std::string sysDevId;
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(false, entry->MsprofCheckSysDeviceValid(sysDevId));
    sysDevId = "all";
    EXPECT_EQ(true, entry->MsprofCheckSysDeviceValid(sysDevId));
    sysDevId = "2ss,2,3";
    EXPECT_EQ(false, entry->MsprofCheckSysDeviceValid(sysDevId));
    sysDevId = "1,2,3";
    EXPECT_EQ(true, entry->MsprofCheckSysDeviceValid(sysDevId));
    sysDevId = "1,2,65";
    EXPECT_EQ(false, entry->MsprofCheckSysDeviceValid(sysDevId));
}

TEST_F(ParamValidationUtest, CheckExportIdIsValid)
{
    GlobalMockObject::verify();
    
    std::string exportId;
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(false, entry->CheckExportIdIsValid(exportId, ""));
    exportId = "www";
    EXPECT_EQ(false, entry->CheckExportIdIsValid(exportId, ""));
    exportId = "-1";
    EXPECT_EQ(false, entry->CheckExportIdIsValid(exportId, ""));
    exportId = "1";
    EXPECT_EQ(true, entry->CheckExportIdIsValid(exportId, ""));
}

TEST_F(ParamValidationUtest, CheckExportSummaryFormatIsValid)
{
    GlobalMockObject::verify();
    
    std::string summaryFormat;
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(false, entry->CheckExportSummaryFormatIsValid(summaryFormat));
    summaryFormat = "www";
    EXPECT_EQ(false, entry->CheckExportSummaryFormatIsValid(summaryFormat));
    summaryFormat = "json";
    EXPECT_EQ(true, entry->CheckExportSummaryFormatIsValid(summaryFormat));
    summaryFormat = "csv";
    EXPECT_EQ(true, entry->CheckExportSummaryFormatIsValid(summaryFormat));
}

TEST_F(ParamValidationUtest, MsprofCheckAppValid)
{
    GlobalMockObject::verify();

    std::string app;
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(false, entry->MsprofCheckAppValid(app));
    app = "bash";
    MOCKER_CPP(&ParamValidation::MsprofCheckNotAppValid)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(false, entry->MsprofCheckAppValid(app));
    EXPECT_EQ(true, entry->MsprofCheckAppValid(app));
    app = "main 1 2";
    std::string empty;
    std::string notEmpty = "x";
    MOCKER_CPP(&ParamValidation::MsprofCheckAppParamValid)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&Utils::CanonicalizePath)
        .stubs()
        .will(returnValue(empty))
        .then(returnValue(notEmpty));
    EXPECT_EQ(false, entry->MsprofCheckAppValid(app));
    EXPECT_EQ(true, entry->MsprofCheckAppValid(app));
    EXPECT_EQ(true, entry->MsprofCheckAppValid(app));
}

TEST_F(ParamValidationUtest, MsprofCheckNotAppValidWillReturnFailWhenCanonicalizePathReturnEmpty)
{
    GlobalMockObject::verify();
    std::string empty;
    std::vector<std::string> appParamsList = {"/", "x"};
    std::string resultAppParam;
    MOCKER_CPP(&Utils::CanonicalizePath)
        .stubs()
        .will(returnValue(empty));
    EXPECT_EQ(PROFILING_FAILED, ParamValidation::instance()->MsprofCheckNotAppValid(appParamsList, resultAppParam));
}

TEST_F(ParamValidationUtest, MsprofCheckNotAppValidWillReturnFailWhenMmAccess2Fail)
{
    GlobalMockObject::verify();
    std::string path = "/path";
    std::vector<std::string> appParamsList = {"/", "x"};
    std::string resultAppParam;
    MOCKER_CPP(&Utils::CanonicalizePath)
        .stubs()
        .will(returnValue(path));
    MOCKER_CPP(&MmAccess2)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, ParamValidation::instance()->MsprofCheckNotAppValid(appParamsList, resultAppParam));
}

TEST_F(ParamValidationUtest, MsprofCheckNotAppValidWillReturnSuccWhenMmAccess2Succ1)
{
    GlobalMockObject::verify();
    std::string path = "/path";
    std::vector<std::string> appParamsList = {"/", "x"};
    std::string resultAppParam;
    MOCKER_CPP(&Utils::CanonicalizePath)
        .stubs()
        .will(returnValue(path));
    MOCKER_CPP(&MmAccess2)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, ParamValidation::instance()->MsprofCheckNotAppValid(appParamsList, resultAppParam));
}

TEST_F(ParamValidationUtest, MsprofCheckNotAppValidWillReturnSuccWhenMmAccess2Succ2)
{
    GlobalMockObject::verify();
    std::string path = "/path";
    std::vector<std::string> appParamsList = {"x", "x"};
    std::string resultAppParam;
    EXPECT_EQ(PROFILING_SUCCESS, ParamValidation::instance()->MsprofCheckNotAppValid(appParamsList, resultAppParam));
}

TEST_F(ParamValidationUtest, MsprofCheckAppParamValidWillReturnFailWhenCanonicalizePathFail)
{
    std::string appParam;
    EXPECT_EQ(PROFILING_FAILED, ParamValidation::instance()->MsprofCheckAppParamValid(appParam));
}

TEST_F(ParamValidationUtest, MsprofCheckAppParamValidWillReturnFailWhenPathIsSoftLink)
{
    GlobalMockObject::verify();
    std::string path = "x";
    std::string appParam;
    MOCKER_CPP(&Utils::CanonicalizePath)
        .stubs()
        .will(returnValue(path));
    MOCKER_CPP(&Utils::IsSoftLink)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_FAILED, ParamValidation::instance()->MsprofCheckAppParamValid(appParam));
}

TEST_F(ParamValidationUtest, MsprofCheckAppParamValidWillReturnFailWhenMmAccess2Fail)
{
    GlobalMockObject::verify();
    std::string path = "x";
    std::string appParam;
    MOCKER_CPP(&Utils::CanonicalizePath)
        .stubs()
        .will(returnValue(path));
    MOCKER_CPP(&Utils::IsSoftLink)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&Utils::IsDir)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&MmAccess2)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, ParamValidation::instance()->MsprofCheckAppParamValid(appParam));
}

TEST_F(ParamValidationUtest, MsprofCheckAppParamValidWillReturnFailWhenCheckParamPermissionFail)
{
    GlobalMockObject::verify();
    std::string path = "x";
    std::string appParam;
    MOCKER_CPP(&Utils::CanonicalizePath)
        .stubs()
        .will(returnValue(path));
    MOCKER_CPP(&Utils::IsSoftLink)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&Utils::IsDir)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&MmAccess2)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&ParamValidation::CheckParamPermission)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, ParamValidation::instance()->MsprofCheckAppParamValid(appParam));
}

TEST_F(ParamValidationUtest, MsprofCheckAppParamValidWillReturnFailWhenSplitPathFail)
{
    GlobalMockObject::verify();
    std::string path = "x";
    std::string appParam;
    MOCKER_CPP(&Utils::CanonicalizePath)
        .stubs()
        .will(returnValue(path));
    MOCKER_CPP(&Utils::IsSoftLink)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&Utils::IsDir)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&MmAccess2)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&ParamValidation::CheckParamPermission)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&Utils::SplitPath)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, ParamValidation::instance()->MsprofCheckAppParamValid(appParam));
}

TEST_F(ParamValidationUtest, MsprofCheckAppParamValidWillReturnFailWhenCheckAppNameIsValidFail)
{
    GlobalMockObject::verify();
    std::string path = "x";
    std::string appParam;
    MOCKER_CPP(&Utils::CanonicalizePath)
        .stubs()
        .will(returnValue(path));
    MOCKER_CPP(&Utils::IsSoftLink)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&Utils::IsDir)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&MmAccess2)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&ParamValidation::CheckParamPermission)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&Utils::SplitPath)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&ParamValidation::CheckAppNameIsValid)
        .stubs()
        .will(returnValue(false));
    EXPECT_EQ(PROFILING_FAILED, ParamValidation::instance()->MsprofCheckAppParamValid(appParam));
}

TEST_F(ParamValidationUtest, CheckL2CacheEventsValid) {
    GlobalMockObject::verify();

    std::vector<std::string> events = {"0x00","0x01","0x02","0x03","0x04","0x05","0x06","0x07","0x08"};
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(0, entry->CheckL2CacheEventsValid(events));
    events = {"0x00","0x01","0x02","0x03","0x04","0x05","0x06","0x07"};
    EXPECT_EQ(0, entry->CheckL2CacheEventsValid(events));
    events = {" 0x00","0x01 "," 0x02 ","  0x03 "," 0x04   ","0x05    ","     0x06"};
    EXPECT_EQ(0, entry->CheckL2CacheEventsValid(events));
    events = {"0x 00","0x01","0x02","0x03","0x04","0x05","0x06"};
    EXPECT_EQ(0, entry->CheckL2CacheEventsValid(events));
    events = {"0x78", "0x79", "0x77", "0x71", "0x6a", "0x6c", "0x74", "0x62"};
    EXPECT_EQ(1, entry->CheckL2CacheEventsValid(events));
}

TEST_F(ParamValidationUtest, CheckParamsModeRegexMatch)
{
    GlobalMockObject::verify();
    std::string paramsMode;
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(true, entry->CheckParamsModeRegexMatch(paramsMode));
    paramsMode = "xxx";
    EXPECT_EQ(false, entry->CheckParamsModeRegexMatch(paramsMode));
    paramsMode = "def_mode";
    EXPECT_EQ(true, entry->CheckParamsModeRegexMatch(paramsMode));
}

TEST_F(ParamValidationUtest, IsValidSleepPeriod) {
    GlobalMockObject::verify();

    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(0, entry->IsValidSleepPeriod(-1));
    EXPECT_EQ(1, entry->IsValidSleepPeriod(1));
}

TEST_F(ParamValidationUtest, CheckHostSysOptionsIsValid) {
    GlobalMockObject::verify();

    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    std::string hostSysOptions;
    EXPECT_EQ(false, entry->CheckHostSysOptionsIsValid(hostSysOptions));
    hostSysOptions = "ParamValidationUtest/CheckHostSysOptionsIsValid";
    EXPECT_EQ(false, entry->CheckHostSysOptionsIsValid(hostSysOptions));
    hostSysOptions = "cpu";
    EXPECT_EQ(true, entry->CheckHostSysOptionsIsValid(hostSysOptions));
}

TEST_F(ParamValidationUtest, CheckHostSysPidIsValid) {
    GlobalMockObject::verify();

    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    int hostSysPid = -1;
    EXPECT_EQ(0, entry->CheckHostSysPidIsValid(hostSysPid));
    hostSysPid = 100000000;
    EXPECT_EQ(0, entry->CheckHostSysPidIsValid(hostSysPid));
    hostSysPid = 1;
    EXPECT_EQ(1, entry->CheckHostSysPidIsValid(hostSysPid));
}

TEST_F(ParamValidationUtest, ProfStarsAcsqParamIsValid) {
    GlobalMockObject::verify();

    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(0, entry->ProfStarsAcsqParamIsValid("ddd"));
    EXPECT_EQ(1, entry->ProfStarsAcsqParamIsValid("dsa,vdec"));
}

TEST_F(ParamValidationUtest, CheckAnalysisOutputIsPathValid)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    std::string output;
    EXPECT_EQ(false, entry->CheckAnalysisOutputIsPathValid(output));
    output = std::string(1025, 'x'); // 1025 = MAX_PATH_LENGTH + 1;
    EXPECT_EQ(false, entry->CheckAnalysisOutputIsPathValid(output));
    output.clear();
    output = "./";
    EXPECT_EQ(true, entry->CheckAnalysisOutputIsPathValid(output));
}

TEST_F(ParamValidationUtest, CheckLlcModeIsValid1)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    std::string llcMode;
    EXPECT_EQ(false, entry->CheckLlcModeIsValid(llcMode));
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
        .stubs()
        .will(returnValue(0));
    llcMode = "xxx";
    EXPECT_EQ(false, entry->CheckLlcModeIsValid(llcMode));
    llcMode = "capacity";
    EXPECT_EQ(true, entry->CheckLlcModeIsValid(llcMode));
    llcMode = "bandwidth";
    EXPECT_EQ(true, entry->CheckLlcModeIsValid(llcMode));
}

TEST_F(ParamValidationUtest, CheckLlcModeIsValid2)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    std::string llcMode;
    EXPECT_EQ(false, entry->CheckLlcModeIsValid(llcMode));
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
        .stubs()
        .will(returnValue(1));
    llcMode = "xxx";
    EXPECT_EQ(false, entry->CheckLlcModeIsValid(llcMode));
    llcMode = "read";
    EXPECT_EQ(true, entry->CheckLlcModeIsValid(llcMode));
    llcMode = "write";
    EXPECT_EQ(true, entry->CheckLlcModeIsValid(llcMode));
}

TEST_F(ParamValidationUtest, CheckHostSysUsageValid)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(false, entry->CheckHostSysUsageValid("10"));
    EXPECT_EQ(false, entry->CheckHostSysUsageValid("disk"));
    EXPECT_EQ(true, entry->CheckHostSysUsageValid("cpu"));
    EXPECT_EQ(true, entry->CheckHostSysUsageValid("mem"));
}

TEST_F(ParamValidationUtest, CheckPythonPathIsValid)
{
    GlobalMockObject::verify();
    std::string pythonPath;
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(false, entry->CheckPythonPathIsValid(pythonPath));

    pythonPath = std::string(1025, 'c'); // 1025 = MAX_PATH_LENGTH + 1;
    EXPECT_EQ(false, entry->CheckPythonPathIsValid(pythonPath));

    pythonPath = "@SUM(cmd|'/c calc'!A0)";
    EXPECT_EQ(false, entry->CheckPythonPathIsValid(pythonPath));
    
    Utils::CreateDir("TestPython");
    MOCKER(&MmAccess2).stubs().will(returnValue(-1)).then(returnValue(0));
    pythonPath = "TestPython";
    EXPECT_EQ(false, entry->CheckPythonPathIsValid(pythonPath));
    Utils::RemoveDir("TestPython");

    pythonPath = "./TestPython/TestPython";
    EXPECT_EQ(false, entry->CheckPythonPathIsValid(pythonPath));
    
    pythonPath = "testpython";
    std::ofstream ftest("testpython");
    ftest << "test";
    ftest.close();
    EXPECT_EQ(true, entry->CheckPythonPathIsValid(pythonPath));
    remove("testpython");

    system("ln -s ./testpython");
    pythonPath = "/testpython";
    EXPECT_EQ(false, entry->CheckPythonPathIsValid(pythonPath));
    system("rm ./testpython");
}

TEST_F(ParamValidationUtest, CheckPythonPathIsValidWillReturnFalseWhenPathIsSoftlink)
{
    GlobalMockObject::verify();
    std::string pythonPath = "/testpython";
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    MOCKER_CPP(&Utils::IsSoftLink)
        .stubs()
        .will(returnValue(false));
    EXPECT_EQ(false, entry->CheckPythonPathIsValid(pythonPath));
}

TEST_F(ParamValidationUtest, CheckParamLengthIsValid)
{
    GlobalMockObject::verify();
    std::string paramPath;
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();

    paramPath = "a";
    EXPECT_EQ(true, entry->CheckParamLengthIsValid(paramPath));

    paramPath = std::string(1025, 'c'); // 1025 = MAX_PATH_LENGTH + 1;
    EXPECT_EQ(false, entry->CheckParamLengthIsValid(paramPath));
}

TEST_F(ParamValidationUtest, CheckParamPermission)
{
    GlobalMockObject::verify();
    std::string paramPath;
    struct stat fileStat;
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();

    EXPECT_EQ(PROFILING_FAILED, entry->CheckParamPermission(paramPath));

    paramPath = "./non/existent/paramPath";
    EXPECT_EQ(PROFILING_FAILED, entry->CheckParamPermission(paramPath));

    paramPath = "./writable/paramPath";
    std::ofstream ftest(paramPath);
    ftest << "test" << std::endl;
    ftest.close();
    chmod(paramPath.c_str(), S_IWOTH);
    EXPECT_EQ(PROFILING_FAILED, entry->CheckParamPermission(paramPath));
    remove(paramPath.c_str());

    paramPath = "./inconsistent/paramPath";
    std::ofstream ftest2(paramPath);
    ftest2 << "test" << std::endl;
    ftest2.close();
    chmod(paramPath.c_str(), S_IRUSR);
    EXPECT_EQ(PROFILING_FAILED, entry->CheckParamPermission(paramPath));
    remove(paramPath.c_str());

    paramPath = "./valid/paramPath/current/user/owner";
    std::ofstream ftest3(paramPath);
    ftest3 << "test" << std::endl;
    ftest3.close();
    fileStat.st_uid = MmGetUid() + 1;
    stat(paramPath.c_str(), &fileStat);
    EXPECT_EQ(PROFILING_FAILED, entry->CheckParamPermission(paramPath));
    remove(paramPath.c_str());
}

TEST_F(ParamValidationUtest, CheckParamsJobIdRegexMatch)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    std::string JobId;
    EXPECT_EQ(false, entry->CheckParamsJobIdRegexMatch(JobId));
    JobId = std::string(513, 'c'); // 513 = MAX_JOBID_LENGTH + 1;
    EXPECT_EQ(false, entry->CheckParamsJobIdRegexMatch(JobId));
    JobId.clear();
    JobId = "12";
    EXPECT_EQ(true, entry->CheckParamsJobIdRegexMatch(JobId));
    JobId = "ad";
    EXPECT_EQ(true, entry->CheckParamsJobIdRegexMatch(JobId));
    JobId = "AF";
    EXPECT_EQ(true, entry->CheckParamsJobIdRegexMatch(JobId));
    JobId = "--";
    EXPECT_EQ(true, entry->CheckParamsJobIdRegexMatch(JobId));
    JobId = "@@";
    EXPECT_EQ(false, entry->CheckParamsJobIdRegexMatch(JobId));
}

TEST_F(ParamValidationUtest, MsprofCheckSysPeriodValid)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    std::string SysPerid;
    EXPECT_EQ(false, entry->MsprofCheckSysPeriodValid(SysPerid));
    SysPerid = "xq2";
    EXPECT_EQ(false, entry->MsprofCheckSysPeriodValid(SysPerid));
    SysPerid = "0";
    EXPECT_EQ(false, entry->MsprofCheckSysPeriodValid(SysPerid));
    SysPerid = "1296000001";
    EXPECT_EQ(false, entry->MsprofCheckSysPeriodValid(SysPerid));
    SysPerid = "10";
    EXPECT_EQ(true, entry->MsprofCheckSysPeriodValid(SysPerid));
}

TEST_F(ParamValidationUtest, MsprofCheckHostSysValid)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::RunSocSide)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(false));
    std::string hostSys;
    EXPECT_EQ(false, entry->MsprofCheckHostSysValid(hostSys));
    EXPECT_EQ(false, entry->MsprofCheckHostSysValid(hostSys));
    hostSys = "xxx";
    EXPECT_EQ(false, entry->MsprofCheckHostSysValid(hostSys));
    hostSys = "cpu";
    EXPECT_EQ(true, entry->MsprofCheckHostSysValid(hostSys));
    hostSys = "cpu,mem";
    EXPECT_EQ(true, entry->MsprofCheckHostSysValid(hostSys));
}

TEST_F(ParamValidationUtest, CheckHostSysToolsExit)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    std::string hostSys;
    std::string resultDir;
    std::string appDir;
    EXPECT_EQ(false, entry->CheckHostSysToolsExit(hostSys, resultDir, appDir));
    hostSys = "cpu";
    EXPECT_EQ(true, entry->CheckHostSysToolsExit(hostSys, resultDir, appDir));
    hostSys = "osrt,disk";
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckHostSysToolsIsExist)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(true, entry->CheckHostSysToolsExit(hostSys, resultDir, appDir));
}

TEST_F(ParamValidationUtest, CheckHostSysToolsIsExist)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    std::string tool;
    std::string resultDir;
    std::string appDir;
    EXPECT_EQ(PROFILING_FAILED, entry->CheckHostSysToolsIsExist(tool, resultDir, appDir));
    tool = "iotop";
    EXPECT_EQ(PROFILING_FAILED, entry->CheckHostSysToolsIsExist(tool, resultDir, appDir));
    resultDir = "./";
    MOCKER(Utils::IsSoftLink)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(false));
    EXPECT_EQ(PROFILING_FAILED, entry->CheckHostSysToolsIsExist(tool, resultDir, appDir));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckParamPermission)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, entry->CheckHostSysToolsIsExist(tool, resultDir, appDir));
    MOCKER_CPP(&analysis::dvvp::common::utils::Utils::ExecCmd)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckHostSysCmdOutIsExist)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, entry->CheckHostSysToolsIsExist(tool, resultDir, appDir));
}

TEST_F(ParamValidationUtest, CheckHostSysCmdOutIsExist)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    MOCKER(analysis::dvvp::common::utils::Utils::ExecCmd)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    std::string tempFile = "./CheckHostSysCmdOutIsExist";
    std::ofstream file(tempFile);
    file << "command not found" << std::endl;
    file.close();
    std::string toolName = "iotop";
    MmProcess tmpProcess = 1;
    // invalid options
    EXPECT_EQ(PROFILING_FAILED, entry->CheckHostSysCmdOutIsExist(tempFile, toolName, tmpProcess));
}

TEST_F(ParamValidationUtest, CheckHostOutString)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    std::string tmpStr = "";
    std::string toolName = "iotop";
    EXPECT_EQ(PROFILING_FAILED, entry->CheckHostOutString(tmpStr, toolName));
    tmpStr = "sudo";
    EXPECT_EQ(PROFILING_FAILED, entry->CheckHostOutString(tmpStr, toolName));
    tmpStr = "iotop";
    EXPECT_EQ(PROFILING_SUCCESS, entry->CheckHostOutString(tmpStr, toolName));
}

TEST_F(ParamValidationUtest, UninitCheckHostSysCmd)
{
    GlobalMockObject::verify();

    MOCKER(analysis::dvvp::common::utils::Utils::ExecCmd)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCKER(analysis::dvvp::common::utils::Utils::ProcessIsRuning)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));

    MOCKER(analysis::dvvp::common::utils::Utils::WaitProcess)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    MmProcess checkProcess = 1;

    EXPECT_EQ(PROFILING_SUCCESS, entry->UninitCheckHostSysCmd(checkProcess));
    EXPECT_EQ(PROFILING_FAILED, entry->UninitCheckHostSysCmd(checkProcess));
    EXPECT_EQ(PROFILING_SUCCESS, entry->UninitCheckHostSysCmd(checkProcess));
}

TEST_F(ParamValidationUtest, CheckStorageLimit) {
    using namespace analysis::dvvp::common::validation;
    bool ret = ParamValidation::instance()->CheckStorageLimit("MB");
    EXPECT_EQ(false, ret);

    ret = ParamValidation::instance()->CheckStorageLimit("10MB0");
    EXPECT_EQ(false, ret);

    ret = ParamValidation::instance()->CheckStorageLimit("A12345MB");
    EXPECT_EQ(false, ret);

    ret = ParamValidation::instance()->CheckStorageLimit("123456789100MB");
    EXPECT_EQ(false, ret);

    ret = ParamValidation::instance()->CheckStorageLimit("100MB");
    EXPECT_EQ(false, ret);

    ret = ParamValidation::instance()->CheckStorageLimit("8294967296MB");
    EXPECT_EQ(false, ret);

    ret = ParamValidation::instance()->CheckStorageLimit("4294967295MB");
    EXPECT_EQ(true, ret);

    ret = ParamValidation::instance()->CheckStorageLimit("1000MB");
    EXPECT_EQ(true, ret);

    ret = ParamValidation::instance()->CheckStorageLimit("");
    EXPECT_EQ(true, ret);
}

TEST_F(ParamValidationUtest, IsValidTaskTimeSwitch)
{
    using namespace analysis::dvvp::common::validation;
    int ret = ParamValidation::instance()->IsValidTaskTimeSwitch("");
    EXPECT_EQ(true, ret);

    ret = ParamValidation::instance()->IsValidTaskTimeSwitch("l0");
    EXPECT_EQ(true, ret);

    ret = ParamValidation::instance()->IsValidTaskTimeSwitch("l1");
    EXPECT_EQ(true, ret);

    ret = ParamValidation::instance()->IsValidTaskTimeSwitch("on");
    EXPECT_EQ(true, ret);

    ret = ParamValidation::instance()->IsValidTaskTimeSwitch("off");
    EXPECT_EQ(true, ret);

    ret = ParamValidation::instance()->IsValidTaskTimeSwitch("xxx");
    EXPECT_EQ(false, ret);
}

TEST_F(ParamValidationUtest, IsValidAnalyzeRuleSwitch)
{
    using namespace analysis::dvvp::common::validation;
    int ret = ParamValidation::instance()->IsValidAnalyzeRuleSwitch("rule", "");
    EXPECT_EQ(true, ret);

    ret = ParamValidation::instance()->IsValidAnalyzeRuleSwitch("rule", "communication");
    EXPECT_EQ(true, ret);

    ret = ParamValidation::instance()->IsValidAnalyzeRuleSwitch("rule", "communication_matrix");
    EXPECT_EQ(true, ret);

    ret = ParamValidation::instance()->IsValidAnalyzeRuleSwitch("rule", "communication,communication_matrix");
    EXPECT_EQ(true, ret);

    ret = ParamValidation::instance()->IsValidAnalyzeRuleSwitch("rule", "xxxx");
    EXPECT_EQ(false, ret);
    ret = ParamValidation::instance()->IsValidAnalyzeRuleSwitch("rule", "communication,xxxx");
    EXPECT_EQ(false, ret);
}

TEST_F(ParamValidationUtest,
    TestCheckCollectOutputIsValidShouldReturnFalseWhenPathHasEscapeChar)
{
    using namespace analysis::dvvp::common::validation;
    const std::string path = "/test/profiling\n/a";
    EXPECT_FALSE(ParamValidation::instance()->CheckCollectOutputIsValid(path));
}

TEST_F(ParamValidationUtest,
    TestCheckAnalysisOutputIsPathValidShouldReturnFalseWhenPathHasEscapeChar)
{
    using namespace analysis::dvvp::common::validation;
    const std::string path = "/test/profiling\n/a";
    EXPECT_FALSE(ParamValidation::instance()->CheckAnalysisOutputIsPathValid(path));
}

TEST_F(ParamValidationUtest, CheckAnalysisOutputIsPathValidWillReturnFalseWhenCheckDirValidFail)
{
    GlobalMockObject::verify();
    auto entry = ParamValidation::instance();
    std::string invalidLenPath(1025, 'x'); // 1025 : larger than max len
    MOCKER_CPP(&Utils::IsSoftLink)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&Utils::IsFileExist)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(false, entry->CheckAnalysisOutputIsPathValid(invalidLenPath));
}

TEST_F(ParamValidationUtest, CheckReportsJsonIsPathValidShouldReturnFalseWhenCheckFailed)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    // Json path not exist
    std::string reportsJson;
    EXPECT_EQ(false, entry->CheckReportsJsonIsPathValid(reportsJson));

    MOCKER(&Utils::IsFileExist).stubs().will(returnValue(true));
    // Path length > max length
    reportsJson = std::string(1025, 'x'); // 1025 = MAX_PATH_LENGTH + 1;
    EXPECT_EQ(false, entry->CheckReportsJsonIsPathValid(reportsJson));
    // Soft link
    MOCKER(Utils::IsSoftLink).stubs().will(returnValue(true)).then(returnValue(false));
    EXPECT_EQ(false, entry->CheckReportsJsonIsPathValid(reportsJson));
    // permission denied
    reportsJson = "./reports.json";
    MOCKER(&MmAccess2).stubs().will(returnValue(1)).then(returnValue(0));
    EXPECT_EQ(false, entry->CheckReportsJsonIsPathValid(reportsJson));
    // canonicalized absolute pathname failed
    std::string empty = "";
    MOCKER(&Utils::CanonicalizePath).stubs().will(returnValue(empty));
    EXPECT_EQ(false, entry->CheckReportsJsonIsPathValid(reportsJson));
    // File bigger than 64MB
    auto len = 65 * 1024 * 1024;  // 65 * 1024 *1024 means 65mb
    MOCKER(&Utils::GetFileSize).stubs().will(returnValue(static_cast<long long>(len)));
    EXPECT_EQ(false, entry->CheckReportsJsonIsPathValid(reportsJson));
    // Path Char Valid
    reportsJson = "./reports%.json";
    EXPECT_EQ(false, entry->CheckReportsJsonIsPathValid(reportsJson));
}

TEST_F(ParamValidationUtest, CheckReportsJsonIsPathValidShouldReturnTrueWhenCheckSuccess)
{
    std::string reportsJson = "./reports.json";
    std::ofstream ftest(reportsJson);
    ftest << "test" << std::endl;
    ftest.close();

    GlobalMockObject::verify();
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(true, entry->CheckReportsJsonIsPathValid(reportsJson));
    remove(reportsJson.c_str());
}

TEST_F(ParamValidationUtest, CheckDirValidWillReturnFalseWhenInputPathSizeLargerThanMaxSize)
{
    GlobalMockObject::verify();
    // use CheckCollectOutputIsValid to run CheckDirValid
    auto entry = ParamValidation::instance();
    int invalidLen = 1025;
    std::string path(invalidLen, 'x');
    MOCKER_CPP(&Utils::IsFileExist)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(false, entry->CheckCollectOutputIsValid(path));
}

TEST_F(ParamValidationUtest, CheckDirValidWillReturnFalseWhenInputPathIsNotDir)
{
    GlobalMockObject::verify();
    // use CheckCollectOutputIsValid to run CheckDirValid
    auto entry = ParamValidation::instance();
    std::string path = "/proc/stat"; // set /proc/stat as path
    EXPECT_EQ(false, entry->CheckCollectOutputIsValid(path));
}

TEST_F(ParamValidationUtest, CheckDirValidWillReturnFalseWhenInputPathIsWriteable)
{
    GlobalMockObject::verify();
    // use CheckCollectOutputIsValid to run CheckDirValid
    auto entry = ParamValidation::instance();
    std::string path = "/sys"; // set /sys as path
    EXPECT_EQ(false, entry->CheckCollectOutputIsValid(path));
}

TEST_F(ParamValidationUtest, CheckDirValidWillReturnFalseWhenCanonicalizePathReturnEmptyPath)
{
    GlobalMockObject::verify();
    // use CheckCollectOutputIsValid to run CheckDirValid
    auto entry = ParamValidation::instance();
    std::string path = "xx";
    std::string emptyPath = "";
    MOCKER_CPP(&Utils::IsFileExist)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&Utils::CanonicalizePath)
        .stubs()
        .will(returnValue(emptyPath));
    EXPECT_EQ(false, entry->CheckCollectOutputIsValid(path));
}

TEST_F(ParamValidationUtest, CheckDirValidWillReturnTrueWhenInputValidPath)
{
    GlobalMockObject::verify();
    // use CheckCollectOutputIsValid to run CheckDirValid
    auto entry = ParamValidation::instance();
    std::string path = "./";
    EXPECT_EQ(true, entry->CheckCollectOutputIsValid(path));
}

TEST_F(ParamValidationUtest, CheckCollectOutputIsValidWillReturnTrueWhenInputEmptyPath)
{
    GlobalMockObject::verify();
    auto entry = ParamValidation::instance();
    EXPECT_EQ(true, entry->CheckCollectOutputIsValid(""));
}

TEST_F(ParamValidationUtest, CheckCollectOutputIsValidWillReturnFalseWhenPathSizeLargerThanMaxLen)
{
    GlobalMockObject::verify();
    auto entry = ParamValidation::instance();
    std::string path = "x";
    std::string invalidLenPath(1025, 'x'); // 1025 : larger than max len
    MOCKER_CPP(&Utils::IsFileExist)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&Utils::RelativePathToAbsolutePath)
        .stubs()
        .will(returnValue(invalidLenPath));
    EXPECT_EQ(false, entry->CheckCollectOutputIsValid(path));
}

TEST_F(ParamValidationUtest, CheckCollectOutputIsValidWillReturnFalseWhenCreateDirFail)
{
    GlobalMockObject::verify();
    auto entry = ParamValidation::instance();
    std::string path = "x";
    std::string validLenPath(1, 'x');
    MOCKER_CPP(&Utils::IsFileExist)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&Utils::RelativePathToAbsolutePath)
        .stubs()
        .will(returnValue(validLenPath));
    MOCKER_CPP(&Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(false, entry->CheckCollectOutputIsValid(path));
}

TEST_F(ParamValidationUtest, CheckCollectOutputIsValidWillReturnTrueWhenCreateDirSucc)
{
    GlobalMockObject::verify();
    auto entry = ParamValidation::instance();
    std::string path = "x";
    std::string validLenPath(1, 'x');
    MOCKER_CPP(&Utils::IsFileExist)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&Utils::RelativePathToAbsolutePath)
        .stubs()
        .will(returnValue(validLenPath));
    MOCKER_CPP(&Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(true, entry->CheckCollectOutputIsValid(path));
}

TEST_F(ParamValidationUtest, CheckProfilingMetricsLengthWillReturnFalseWhenInputLenLargerThenMaxLen)
{
    std::string metricsVal(71, 'x'); // 71 larger than max len
    EXPECT_EQ(false, ParamValidation::instance()->CheckProfilingMetricsLength(metricsVal));
}

TEST_F(ParamValidationUtest, CheckExportTypeIsValidWillReturnFalseWhenInputEmptyType)
{
    EXPECT_EQ(false, ParamValidation::instance()->CheckExportTypeIsValid(""));
}

TEST_F(ParamValidationUtest, CheckExportTypeIsValidWillReturnFalseWhenInputInvalidType)
{
    EXPECT_EQ(false, ParamValidation::instance()->CheckExportTypeIsValid("xx"));
}

TEST_F(ParamValidationUtest, CheckExportTypeIsValidWillReturnTrueWhenInputValidType)
{
    EXPECT_EQ(true, ParamValidation::instance()->CheckExportTypeIsValid("db"));
}

TEST_F(ParamValidationUtest, CheckProfilingIntervalIsValidTWOWillReturnFalseWhenInputNullPatams)
{
    EXPECT_EQ(false, ParamValidation::instance()->CheckProfilingIntervalIsValidTWO(nullptr));
}

TEST_F(ParamValidationUtest, CheckProfilingIntervalIsValidTWOWillReturnFalseWhenInputInvalidPidSamplingInterVal)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    params->sys_sampling_interval = 1;
    params->pid_sampling_interval = 0;
    EXPECT_EQ(false, ParamValidation::instance()->CheckProfilingIntervalIsValidTWO(params));
}

TEST_F(ParamValidationUtest, CheckProfilingIntervalIsValidTWOWillReturnFalseWhenInputInvalidIoSamplingInterVal)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    params->sys_sampling_interval = 1;
    params->pid_sampling_interval = 1;
    params->hardware_mem_sampling_interval = 1;
    params->io_sampling_interval = 0;
    EXPECT_EQ(false, ParamValidation::instance()->CheckProfilingIntervalIsValidTWO(params));
}

TEST_F(ParamValidationUtest, CheckProfilingIntervalIsValidTWOWillReturnFalseWhenInputInvalidInterSamplingInterVal)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    params->sys_sampling_interval = 1;
    params->pid_sampling_interval = 1;
    params->hardware_mem_sampling_interval = 1;
    params->io_sampling_interval = 1;
    params->interconnection_sampling_interval = 0;
    EXPECT_EQ(false, ParamValidation::instance()->CheckProfilingIntervalIsValidTWO(params));
}

TEST_F(ParamValidationUtest, CheckProfilingIntervalIsValidTWOWillReturnFalseWhenInputInvalidDvppSamplingInterVal)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    params->sys_sampling_interval = 1;
    params->pid_sampling_interval = 1;
    params->hardware_mem_sampling_interval = 1;
    params->io_sampling_interval = 1;
    params->interconnection_sampling_interval = 1;
    params->dvpp_sampling_interval = 0;
    EXPECT_EQ(false, ParamValidation::instance()->CheckProfilingIntervalIsValidTWO(params));
}

TEST_F(ParamValidationUtest, CheckProfilingIntervalIsValidTWOWillReturnFalseWhenInputInvalidNicSamplingInterVal)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    params->sys_sampling_interval = 1;
    params->pid_sampling_interval = 1;
    params->hardware_mem_sampling_interval = 1;
    params->io_sampling_interval = 1;
    params->interconnection_sampling_interval = 1;
    params->dvpp_sampling_interval = 1;
    params->nicInterval = 0;
    EXPECT_EQ(false, ParamValidation::instance()->CheckProfilingIntervalIsValidTWO(params));
}

TEST_F(ParamValidationUtest, CheckProfilingIntervalIsValidTWOWillReturnFalseWhenInputInvalidRoceSamplingInterVal)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    params->sys_sampling_interval = 1;
    params->pid_sampling_interval = 1;
    params->hardware_mem_sampling_interval = 1;
    params->io_sampling_interval = 1;
    params->interconnection_sampling_interval = 1;
    params->dvpp_sampling_interval = 1;
    params->nicInterval = 1;
    params->roceInterval = 0;
    EXPECT_EQ(false, ParamValidation::instance()->CheckProfilingIntervalIsValidTWO(params));
}

TEST_F(ParamValidationUtest, CheckProfilingSwitchIsValidWillReturnFalseWhenInputNullParams)
{
    EXPECT_EQ(false, ParamValidation::instance()->CheckProfilingSwitchIsValid(nullptr));
}

TEST_F(ParamValidationUtest, CheckProfilingSwitchIsValidWillReturnFalseWhenCheckTsSwitchFail)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    params->ts_timeline = "x";
    EXPECT_EQ(false, ParamValidation::instance()->CheckProfilingSwitchIsValid(params));
    params->ts_timeline = "";
    params->ts_keypoint = "x";
    EXPECT_EQ(false, ParamValidation::instance()->CheckProfilingSwitchIsValid(params));
    params->ts_keypoint = "";
    params->ts_fw_training = "x";
    EXPECT_EQ(false, ParamValidation::instance()->CheckProfilingSwitchIsValid(params));
    params->ts_fw_training = "";
    params->hwts_log = "x";
    EXPECT_EQ(false, ParamValidation::instance()->CheckProfilingSwitchIsValid(params));
    params->hwts_log = "";
    params->hwts_log1 = "x";
    EXPECT_EQ(false, ParamValidation::instance()->CheckProfilingSwitchIsValid(params));
    params->hwts_log1 = "";
    params->ts_memcpy = "x";
    EXPECT_EQ(false, ParamValidation::instance()->CheckProfilingSwitchIsValid(params));
}

TEST_F(ParamValidationUtest, CheckProfilingSwitchIsValidWillReturnFalseWhenCheckPmuSwitchFail)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    MOCKER_CPP(&ParamValidation::CheckPmuSwitchProfiling)
        .stubs()
        .will(returnValue(false));
    EXPECT_EQ(false, ParamValidation::instance()->CheckProfilingSwitchIsValid(params));
}

TEST_F(ParamValidationUtest, CheckProfilingSwitchIsValidWillReturnFalseWhenCheckSysTraceSwitchFail)
{
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    MOCKER_CPP(&ParamValidation::CheckSystemTraceSwitchProfiling)
        .stubs()
        .will(returnValue(false));
    EXPECT_EQ(false, ParamValidation::instance()->CheckProfilingSwitchIsValid(params));
}

TEST_F(ParamValidationUtest, CheckDeviceIdIsValidWillReturnFalseWhenStrToIntFail)
{
    MOCKER_CPP(&Utils::StrToInt)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(false, ParamValidation::instance()->CheckDeviceIdIsValid("0"));
}

TEST_F(ParamValidationUtest, CheckAiCoreEventsIsValidWillReturnFalseWhenInputInvalidSizePmuEvents)
{
    size_t size = 9;
    std::vector<std::string> events;
    for (size_t i = 0; i < size; i++) {
        events.push_back("x");
    }
    EXPECT_EQ(false, ParamValidation::instance()->CheckAiCoreEventsIsValid(events));
}

TEST_F(ParamValidationUtest, CheckAiCoreEventsIsValidWillReturnFalseWhenInputInvalidPmuEvents)
{
    size_t size = 8;
    std::vector<std::string> events;
    for (size_t i = 0; i < size; i++) {
        events.push_back("0");
    }
    EXPECT_EQ(false, ParamValidation::instance()->CheckAiCoreEventsIsValid(events));
    for (size_t i = 0; i < size; i++) {
        events[i] = "2000";
    }
    EXPECT_EQ(false, ParamValidation::instance()->CheckAiCoreEventsIsValid(events));
}

TEST_F(ParamValidationUtest, CheckProfilingParamsWillReturnTrueWhenDevicesAndAppEmptyWithHostSysNotEmpty)
{
    EXPECT_EQ(true, ParamValidation::instance()->CheckParamsDevices("", "", "x"));
}

TEST_F(ParamValidationUtest, CheckProfilingParamsWillReturnFalseWhenDevicesAndAppEmptyWithHostSysEmpty)
{
    EXPECT_EQ(false, ParamValidation::instance()->CheckParamsDevices("", "", ""));
}

TEST_F(ParamValidationUtest, CheckMsopprofBinValidWillReturnFailWhenCanonicalizaPathFail)
{
    GlobalMockObject::verify();
    std::string emptyPath;
    MOCKER_CPP(&Utils::CanonicalizePath)
        .stubs()
        .will(returnValue(emptyPath));
    std::string binpath = "/path/to/bin";
    EXPECT_EQ(PROFILING_FAILED, ParamValidation::instance()->CheckMsopprofBinValid(binpath));
}

TEST_F(ParamValidationUtest, CheckMsopprofBinValidWillReturnFailWhenPathIsSoftLink)
{
    GlobalMockObject::verify();
    std::string binpath = "/path/to/bin";
    MOCKER_CPP(&Utils::CanonicalizePath)
        .stubs()
        .will(returnValue(binpath));
    MOCKER_CPP(&Utils::IsSoftLink)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_FAILED, ParamValidation::instance()->CheckMsopprofBinValid(binpath));
}

TEST_F(ParamValidationUtest, CheckMsopprofBinValidWillReturnFailWhenPathIsDir)
{
    GlobalMockObject::verify();
    std::string binpath = "/path/to/bin";
    MOCKER_CPP(&Utils::CanonicalizePath)
        .stubs()
        .will(returnValue(binpath));
    MOCKER_CPP(&Utils::IsSoftLink)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&Utils::IsDir)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_FAILED, ParamValidation::instance()->CheckMsopprofBinValid(binpath));
}

TEST_F(ParamValidationUtest, CheckMsopprofBinValidWillReturnFailWhenPathIsNotExecutable)
{
    GlobalMockObject::verify();
    std::string binpath = "/path/to/bin";
    MOCKER_CPP(&Utils::CanonicalizePath)
        .stubs()
        .will(returnValue(binpath));
    MOCKER_CPP(&Utils::IsSoftLink)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&Utils::IsDir)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&MmAccess2)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, ParamValidation::instance()->CheckMsopprofBinValid(binpath));
}

TEST_F(ParamValidationUtest, CheckMsopprofBinValidWillReturnFailWhenCheckParamPermissionFail)
{
    GlobalMockObject::verify();
    std::string binpath = "/path/to/bin";
    MOCKER_CPP(&Utils::CanonicalizePath)
        .stubs()
        .will(returnValue(binpath));
    MOCKER_CPP(&Utils::IsSoftLink)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&Utils::IsDir)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&MmAccess2)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&ParamValidation::CheckParamPermission)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, ParamValidation::instance()->CheckMsopprofBinValid(binpath));
}

TEST_F(ParamValidationUtest, CheckMsopprofBinValidWillReturnSuccWhenCheckParamPermissionSucc)
{
    GlobalMockObject::verify();
    std::string binpath = "/path/to/bin";
    MOCKER_CPP(&Utils::CanonicalizePath)
        .stubs()
        .will(returnValue(binpath));
    MOCKER_CPP(&Utils::IsSoftLink)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&Utils::IsDir)
        .stubs()
        .will(returnValue(false));
    MOCKER_CPP(&MmAccess2)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&ParamValidation::CheckParamPermission)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, ParamValidation::instance()->CheckMsopprofBinValid(binpath));
}

TEST_F(ParamValidationUtest, CheckDelayAndDurationValidWillReturnFalseWhenInputInvalidNum)
{
    EXPECT_EQ(false, ParamValidation::instance()->CheckDelayAndDurationValid("x", ""));
}

TEST_F(ParamValidationUtest, CheckDelayAndDurationValidWillReturnFalseWhenInputZeroNum)
{
    EXPECT_EQ(false, ParamValidation::instance()->CheckDelayAndDurationValid("0", ""));
}

TEST_F(ParamValidationUtest, CheckDelayAndDurationValidWillReturnTrueWhenInputValidNum)
{
    EXPECT_EQ(true, ParamValidation::instance()->CheckDelayAndDurationValid("1", ""));
}

TEST_F(ParamValidationUtest, CheckMstxDomainSwitchValidWillReturnFalseWhenIncludeAndExcludeBothSetWithTxEnabled)
{
    EXPECT_EQ(false, ParamValidation::instance()->CheckMstxDomainSwitchValid("on", "xx", "yy"));
}

TEST_F(ParamValidationUtest, CheckMstxDomainSwitchValidWillReturnTrueWhenIncludeAndExcludeNotBothSetWithTxEnabled)
{
    EXPECT_EQ(true, ParamValidation::instance()->CheckMstxDomainSwitchValid("on", "xx", ""));
    EXPECT_EQ(true, ParamValidation::instance()->CheckMstxDomainSwitchValid("on", "", "yy"));
}

TEST_F(ParamValidationUtest, CheckMstxDomainSwitchValidWillReturnFalseWhenIncludeOrExcludeSetWithTxNotEnabled)
{
    EXPECT_EQ(false, ParamValidation::instance()->CheckMstxDomainSwitchValid("", "xx", "yy"));
    EXPECT_EQ(false, ParamValidation::instance()->CheckMstxDomainSwitchValid("", "", "yy"));
    EXPECT_EQ(false, ParamValidation::instance()->CheckMstxDomainSwitchValid("", "xx", ""));
}

TEST_F(ParamValidationUtest, CheckMstxDomainSwitchValidWillReturnTrueWhenIncludeAndExcludeNotSetWithTxNotEnabled)
{
    EXPECT_EQ(true, ParamValidation::instance()->CheckMstxDomainSwitchValid("", "", ""));
}
