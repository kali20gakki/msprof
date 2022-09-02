#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <string>
#include <memory>
#include "message/prof_params.h"
#include "proto/profiler.pb.h"
#include "message/codec.h"
#include "param_validation.h"
#include "platform/platform.h"
#include "devdrv_runtime_api_stub.h"
#include "errno/error_code.h"
#include "ai_drv_dev_api.h"
#include "utils/utils.h"
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;

class COMMON_VALIDATION_PARAM_VALIDATION_TEST: public testing::Test {
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

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckProfilingParams) {
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

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckProfilingIntervalIsValid) {
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

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckSystemTraceSwitchProfiling) {
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

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckTsSwitchProfiling) {
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(new analysis::dvvp::message::ProfileParams());
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    params->ts_task_track = "asd";
    EXPECT_EQ(false, entry->CheckTsSwitchProfiling(params));
    params->ts_cpu_usage = "asd";
    params->ts_task_track = "on";
    EXPECT_EQ(false, entry->CheckTsSwitchProfiling(params));
    params->ai_core_status = "asd";
    params->ts_cpu_usage = "on";
    EXPECT_EQ(false, entry->CheckTsSwitchProfiling(params));
    params->ai_core_status = "on";
    params->ts_timeline = "asd";
    EXPECT_EQ(false, entry->CheckTsSwitchProfiling(params));
    params->ts_timeline = "on";
    params->ts_fw_training = "asd";
    EXPECT_EQ(false, entry->CheckTsSwitchProfiling(params));
    params->ts_fw_training = "on";
    params->hwts_log = "asd";
    EXPECT_EQ(false, entry->CheckTsSwitchProfiling(params));
    params->hwts_log = "on";
    params->hwts_log1 = "asd";
    EXPECT_EQ(false, entry->CheckTsSwitchProfiling(params));
    params->hwts_log1 = "on";
    params->ai_vector_status = "ada";
    EXPECT_EQ(false, entry->CheckTsSwitchProfiling(params));
    params->ai_vector_status = "on";
    EXPECT_EQ(true, entry->CheckTsSwitchProfiling(params));
}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckPmuSwitchProfiling) {
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

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckAivEventCoresIsValid) {
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    const std::vector<int> coreId = {0,1,2,3,4,5,6,7,8};
    EXPECT_EQ(true, entry->CheckAivEventCoresIsValid(coreId));
    const std::vector<int> coreId1 = {0,1,2,3,-4};
    EXPECT_EQ(false, entry->CheckAivEventCoresIsValid(coreId1));
    const std::vector<int> coreId2 = {0,1,2,3,4};
    EXPECT_EQ(true, entry->CheckAivEventCoresIsValid(coreId2));
}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckTsCpuEventIsValid) {
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    std::vector<std::string> events = {"read","write","read","write","read", "asd","write","read","write"};
    EXPECT_EQ(false, entry->CheckTsCpuEventIsValid(events));
    std::vector<std::string> events1 = {"read","write","read","write"};
    EXPECT_EQ(false, entry->CheckTsCpuEventIsValid(events1));
    std::vector<std::string> events2 = {"0xa"};
    EXPECT_EQ(true, entry->CheckTsCpuEventIsValid(events2));
}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckDdrEventsIsValid) {
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

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckHbmEventsIsValid) {
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    std::vector<std::string> events = {"read","write","read","write","read","write","read","write","read","write"};
    EXPECT_EQ(false, entry->CheckHbmEventsIsValid(events));
    events = {"read","write1"};
    EXPECT_EQ(false, entry->CheckHbmEventsIsValid(events));
    events = {"read","write"};
    EXPECT_EQ(true, entry->CheckHbmEventsIsValid(events));
}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckAivEventsIsValid) {
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    const std::vector<std::string> aiv;
    EXPECT_EQ(true, entry->CheckAivEventsIsValid(aiv));
}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckNameContainsDangerCharacter) {
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(true, entry->CheckNameContainsDangerCharacter(""));
    const std::string cmd = "rm -rf *";
    EXPECT_EQ(false, entry->CheckNameContainsDangerCharacter(cmd));
}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckAppNameIsValid) {
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(false, entry->CheckAppNameIsValid(""));

    const std::string appName = "asd0";
    EXPECT_EQ(true, entry->CheckAppNameIsValid(appName));

    const std::string invalidAppName = "$SD";
    EXPECT_EQ(false, entry->CheckAppNameIsValid(invalidAppName));
}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckProfilingParams1) {
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(new analysis::dvvp::message::ProfileParams());
    std::shared_ptr<analysis::dvvp::proto::JobStartReq> start(new analysis::dvvp::proto::JobStartReq);
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    //profiling_mode is illegal
    std::string sampleConfig = "{\"result_dir\":\"/tmp/\", \"job_id\":\"aaaZZZZ000-\", \"profiling_mode\":\"system-wide\", \"devices\":\"1\"}";
    params->FromString(sampleConfig);
    EXPECT_EQ(true, entry->CheckProfilingParams(params));

}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, Init) {
    GlobalMockObject::verify();
    EXPECT_EQ(0, analysis::dvvp::common::validation::ParamValidation::instance()->Init());
}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, UnInit) {
    GlobalMockObject::verify();
    EXPECT_EQ(0, analysis::dvvp::common::validation::ParamValidation::instance()->Uninit());
}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckLlcEventsIsValid) {
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

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckEventsSize)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(PROFILING_SUCCESS, entry->CheckEventsSize(""));
    EXPECT_EQ(PROFILING_FAILED, entry->CheckEventsSize("0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9"));
}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckProfilingAicoreMetricsIsValid) {
    GlobalMockObject::verify();

    std::string aicoreMetrics;
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(1, entry->CheckProfilingAicoreMetricsIsValid(aicoreMetrics));
    aicoreMetrics = "/$$}";
    EXPECT_EQ(0, entry->CheckProfilingAicoreMetricsIsValid(aicoreMetrics));
    aicoreMetrics = "PipeUtilization";
    EXPECT_EQ(1, entry->CheckProfilingAicoreMetricsIsValid(aicoreMetrics));
    aicoreMetrics = "aicoreMetricsAll";
    EXPECT_EQ(0, entry->CheckProfilingAicoreMetricsIsValid(aicoreMetrics));
    aicoreMetrics = "trace";
    EXPECT_EQ(0, entry->CheckProfilingAicoreMetricsIsValid(aicoreMetrics));
}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckFreqIsValid)
{
    GlobalMockObject::verify();
    
    std::string freq;
    int rangeMin = 1;
    int rangeMax = 10;
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    freq = "";
    EXPECT_EQ(true, entry->CheckFreqIsValid(freq, rangeMin, rangeMax));
    freq = "ww11";
    EXPECT_EQ(false, entry->CheckFreqIsValid(freq, rangeMin, rangeMax));
    freq = "0";
    EXPECT_EQ(false, entry->CheckFreqIsValid(freq, rangeMin, rangeMax));
    freq = "11";
    EXPECT_EQ(false, entry->CheckFreqIsValid(freq, rangeMin, rangeMax));
    freq = "5";
    EXPECT_EQ(true, entry->CheckFreqIsValid(freq, rangeMin, rangeMax));
}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckHostSysPidValid)
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

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, MsprofCheckEnvValid)
{
    GlobalMockObject::verify();
    
    std::string env;
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(false, entry->MsprofCheckEnvValid(env));
    env = "ww11";
    EXPECT_EQ(true, entry->MsprofCheckEnvValid(env));
}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, MsprofCheckAiModeValid)
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

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, MsprofCheckSysDeviceValid)
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

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckExportIdIsValid)
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

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckExportSummaryFormatIsValid)
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

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, MsprofCheckAppValid)
{
    GlobalMockObject::verify();
    
    std::string app;
    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(false, entry->MsprofCheckAppValid(app));
    app = "bash";
    EXPECT_EQ(false, entry->MsprofCheckAppValid(app));
    app = "bash 1";
    EXPECT_EQ(false, entry->MsprofCheckAppValid(app));
    app = "main";
    EXPECT_EQ(false, entry->MsprofCheckAppValid(app));
}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckL2CacheEventsValid) {
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

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, IsValidSleepPeriod) {
    GlobalMockObject::verify();

    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(0, entry->IsValidSleepPeriod(-1));
    EXPECT_EQ(1, entry->IsValidSleepPeriod(1));
}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckHostSysOptionsIsValid) {
    GlobalMockObject::verify();

    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    std::string hostSysOptions;
    EXPECT_EQ(false, entry->CheckHostSysOptionsIsValid(hostSysOptions));
    hostSysOptions = "COMMON_VALIDATION_PARAM_VALIDATION_TEST/CheckHostSysOptionsIsValid";
    EXPECT_EQ(false, entry->CheckHostSysOptionsIsValid(hostSysOptions));
    hostSysOptions = "cpu";
    EXPECT_EQ(true, entry->CheckHostSysOptionsIsValid(hostSysOptions));
}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckHostSysPidIsValid) {
    GlobalMockObject::verify();

    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    int hostSysPid = -1;
    EXPECT_EQ(0, entry->CheckHostSysPidIsValid(hostSysPid));
    hostSysPid = 100000000;
    EXPECT_EQ(0, entry->CheckHostSysPidIsValid(hostSysPid));
    hostSysPid = 1;
    EXPECT_EQ(1, entry->CheckHostSysPidIsValid(hostSysPid));
}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, ProfStarsAcsqParamIsValid) {
    GlobalMockObject::verify();

    auto entry = analysis::dvvp::common::validation::ParamValidation::instance();
    EXPECT_EQ(0, entry->ProfStarsAcsqParamIsValid("ddd"));
    EXPECT_EQ(1, entry->ProfStarsAcsqParamIsValid("dsa,vdec"));
}

TEST_F(COMMON_VALIDATION_PARAM_VALIDATION_TEST, CheckStorageLimit) {
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
