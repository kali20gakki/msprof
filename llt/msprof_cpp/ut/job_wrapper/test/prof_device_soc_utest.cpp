#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "job_device_soc.h"
#include "config/config.h"
#include "prof_manager.h"
#include "hdc/device_transport.h"
#include "param_validation.h"
#include "prof_channel_manager.h"

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::message;
using namespace Analysis::Dvvp::JobWrapper;
using namespace analysis::dvvp::common::validation;

class PROF_DEVICE_SOC_UTEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {

    }
public:
    MockObject<Analysis::Dvvp::JobWrapper::JobDeviceSoc>mockerJobDeviceSoc;
    MockObject<analysis::dvvp::common::validation::ParamValidation>mockerParamValidation;
    MockObject<Analysis::Dvvp::JobWrapper::ProfChannelManager>mockerProfChannelManager;
    MockObject<analysis::dvvp::transport::UploaderMgr>mockerUploaderMgr;
    MockObject<analysis::dvvp::driver::DrvChannelsMgr>mockerDrvChannelsMgr;
    
};

TEST_F(PROF_DEVICE_SOC_UTEST, StartProf)
{
    GlobalMockObject::verify();
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(new analysis::dvvp::message::ProfileParams());
    params->FromString("{\"result_dir\":\"/tmp/\", \"devices\":\"1\", \"job_id\":\"1\"}");
    auto jobDeviceSoc = std::make_shared<Analysis::Dvvp::JobWrapper::JobDeviceSoc>(0);
    std::string fileName = "/tmp/test";
    MOCK_METHOD(mockerJobDeviceSoc, GenerateFileName)
        .stubs()
        .will(returnValue(fileName));
    MOCK_METHOD(mockerJobDeviceSoc, SendData)
        .stubs()
        .will(returnValue(0));
    MOCKER(analysis::dvvp::driver::DrvGetDevNum)
        .stubs()
        .will(returnValue(2))
        .then(returnValue(2));
    MOCKER(analysis::dvvp::driver::DrvGetDevIds)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_FAILED));

    MOCK_METHOD(mockerParamValidation, CheckProfilingParams)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(false));

    MOCK_METHOD(mockerProfChannelManager, Init)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_SUCCESS));

    MOCK_METHOD(mockerJobDeviceSoc, GetAndStoreStartTime)
        .stubs()
        .will(ignoreReturnValue());

    EXPECT_EQ(PROFILING_SUCCESS, jobDeviceSoc->StartProf(params));
    jobDeviceSoc->isStarted_ = true;
    EXPECT_EQ(PROFILING_FAILED, jobDeviceSoc->StartProf(params));
}

TEST_F(PROF_DEVICE_SOC_UTEST, StartProf1)
{
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
        new analysis::dvvp::message::ProfileParams);
    params->ai_ctrl_cpu_profiling_events = "0x11";
    params->ts_cpu_profiling_events = "0x11";
    params->llc_profiling_events = "read";
    params->ai_core_profiling_events = "0x12";
    params->aiv_profiling_events = "0x12";
    params->devices = "0";
    params->low_power = "on";
    params->acc_pmu_mode = "sample-based";

    MOCK_METHOD(mockerProfChannelManager, UnInit)
        .stubs()
        .will(ignoreReturnValue());
    auto jobDeviceSoc = std::make_shared<Analysis::Dvvp::JobWrapper::JobDeviceSoc>(0);
    jobDeviceSoc->isStarted_ = true;
    EXPECT_EQ(PROFILING_FAILED, jobDeviceSoc->StartProf(params));


    MOCK_METHOD(mockerJobDeviceSoc, GetAndStoreStartTime)
        .stubs()
        .will(ignoreReturnValue());

    std::shared_ptr<CollectionJobCfg> jobCfg;
    MSVP_MAKE_SHARED0_VOID(jobCfg, CollectionJobCfg);
    jobDeviceSoc->CollectionJobV_[HBM_DRV_COLLECTION_JOB].jobCfg = jobCfg;

    MSVP_MAKE_SHARED0_VOID(jobDeviceSoc->collectionjobComnCfg_, CollectionJobCommonParams);
    jobDeviceSoc->collectionjobComnCfg_->params = params;
    jobDeviceSoc->params_ = params;
    jobDeviceSoc->StartProf(params);

    jobDeviceSoc->collectionjobComnCfg_->params->nicProfiling = "on";
    jobDeviceSoc->collectionjobComnCfg_->params->dvpp_profiling = "on";
    jobDeviceSoc->collectionjobComnCfg_->params->llc_interval = 100;
    jobDeviceSoc->collectionjobComnCfg_->params->ddr_interval = 100;
    jobDeviceSoc->collectionjobComnCfg_->params->hbmProfiling = "on";
    jobDeviceSoc->collectionjobComnCfg_->params->hbm_profiling_events = "read,write";

    jobDeviceSoc->collectionjobComnCfg_->devIdOnHost = 0;

    MOCK_METHOD(mockerJobDeviceSoc, RegisterCollectionJobs)
        .stubs()
        .will(returnValue(0));
    MOCK_METHOD(mockerJobDeviceSoc, ParsePmuConfig)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    MOCKER(analysis::dvvp::driver::DrvGetDevNum)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(1));
    jobDeviceSoc->isStarted_ = false;
    EXPECT_EQ(PROFILING_FAILED, jobDeviceSoc->StartProf(params));
    EXPECT_EQ(PROFILING_SUCCESS, jobDeviceSoc->StartProf(params));
}

TEST_F(PROF_DEVICE_SOC_UTEST, StopProf)
{
    GlobalMockObject::verify();
    MOCK_METHOD(mockerProfChannelManager, UnInit)
        .stubs()
        .will(ignoreReturnValue());
    auto jobDeviceSoc = std::make_shared<Analysis::Dvvp::JobWrapper::JobDeviceSoc>(0);
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->StopProf());
    jobDeviceSoc->isStarted_ = true;

    std::shared_ptr<PMUEventsConfig> cfg = std::make_shared<PMUEventsConfig>();
    auto tsCpuEvents = std::make_shared<std::vector<std::string>>();
    tsCpuEvents->push_back("0xa,0xb");
    cfg->ctrlCPUEvents = *tsCpuEvents;
    cfg->tsCPUEvents = *tsCpuEvents;
    cfg->llcEvents = *tsCpuEvents;
    cfg->aiCoreEvents = *tsCpuEvents;
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
        new analysis::dvvp::message::ProfileParams);

    MSVP_MAKE_SHARED0_VOID(jobDeviceSoc->collectionjobComnCfg_, CollectionJobCommonParams);
    jobDeviceSoc->collectionjobComnCfg_->params = params;
    jobDeviceSoc->CreateCollectionJobArray();
    jobDeviceSoc->params_ = params;
    MOCKER(mmJoinTask)
        .stubs()
        .will(returnValue(EN_OK));

    EXPECT_EQ(PROFILING_SUCCESS,jobDeviceSoc->StopProf());
}

TEST_F(PROF_DEVICE_SOC_UTEST, SendData) {
    GlobalMockObject::verify();

    MOCK_METHOD(mockerUploaderMgr, UploadFileData)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    auto jobDeviceSoc = std::make_shared<Analysis::Dvvp::JobWrapper::JobDeviceSoc>(0);
    jobDeviceSoc->isStarted_ = true;
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
        new analysis::dvvp::message::ProfileParams);
    jobDeviceSoc->params_ = params;
    const std::string fileName = "/tmp/PROF_DEVICE_SOC_UTEST/SendData";
    std::string data;
    jobDeviceSoc->params_->host_profiling = true;
    EXPECT_EQ(PROFILING_SUCCESS,jobDeviceSoc->SendData(fileName, data));
    jobDeviceSoc->params_->host_profiling = false;
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->SendData(fileName, data));
    data = "tmpTestSenData";
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->SendData(fileName, data));
    EXPECT_EQ(PROFILING_SUCCESS,jobDeviceSoc->SendData(fileName, data));
}

TEST_F(PROF_DEVICE_SOC_UTEST, GenerateFileName) {
    GlobalMockObject::verify();

    const std::string fileName = "/tmp/PROF_DEVICE_SOC_UTEST/GenerateFileName";
    auto jobDeviceSoc = std::make_shared<Analysis::Dvvp::JobWrapper::JobDeviceSoc>(0);
    MSVP_MAKE_SHARED0_VOID(jobDeviceSoc->collectionjobComnCfg_, CollectionJobCommonParams);
    jobDeviceSoc->GenerateFileName(fileName);
}

TEST_F(PROF_DEVICE_SOC_UTEST, ParseAiCoreConfig) {
    GlobalMockObject::verify();

    MOCK_METHOD(mockerParamValidation, CheckAiCoreEventsIsValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));

    MOCK_METHOD(mockerParamValidation, CheckAiCoreEventCoresIsValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));

    std::shared_ptr<PMUEventsConfig> cfg = std::make_shared<PMUEventsConfig>();
    auto jobDeviceSoc = std::make_shared<Analysis::Dvvp::JobWrapper::JobDeviceSoc>(0);
    auto tsCpuEvents = std::make_shared<std::vector<std::string>>();
    auto tmpAiCoreEventsCoreIds = std::make_shared<std::vector<int>>();
    tsCpuEvents->push_back("0xa,0xb");
    tmpAiCoreEventsCoreIds->push_back(1);
    cfg->aiCoreEvents = *tsCpuEvents;
    cfg->aiCoreEventsCoreIds = *tmpAiCoreEventsCoreIds;
    std::shared_ptr<CollectionJobCfg> jobCfg1;
    MSVP_MAKE_SHARED0_VOID(jobCfg1, CollectionJobCfg);
    jobDeviceSoc->CollectionJobV_[AI_CORE_SAMPLE_DRV_COLLECTION_JOB].jobCfg = jobCfg1;
    jobDeviceSoc->CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].jobCfg = jobCfg1;
    std::shared_ptr<CollectionJobCfg> jobCfg2;
    MSVP_MAKE_SHARED0_VOID(jobCfg2, CollectionJobCfg);
    jobDeviceSoc->CollectionJobV_[AI_CORE_TASK_DRV_COLLECTION_JOB].jobCfg = jobCfg2;
    jobDeviceSoc->CollectionJobV_[FFTS_PROFILE_COLLECTION_JOB].jobCfg = jobCfg2;
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->ParseAiCoreConfig(cfg));
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->ParseAiCoreConfig(cfg));
    EXPECT_EQ(PROFILING_SUCCESS,jobDeviceSoc->ParseAiCoreConfig(cfg));
}
TEST_F(PROF_DEVICE_SOC_UTEST, ParsePmuConfig) {
    GlobalMockObject::verify();

    MOCK_METHOD(mockerJobDeviceSoc, ParseTsCpuConfig)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));

    MOCK_METHOD(mockerJobDeviceSoc, ParseAiCoreConfig)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));

    MOCK_METHOD(mockerJobDeviceSoc, ParseControlCpuConfig)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));

    MOCK_METHOD(mockerJobDeviceSoc, ParseLlcConfig)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));

    MOCK_METHOD(mockerJobDeviceSoc, ParseDdrCpuConfig)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));

    MOCK_METHOD(mockerJobDeviceSoc, ParseAivConfig)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));

    MOCK_METHOD(mockerJobDeviceSoc, ParseHbmConfig)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));

    auto jobDeviceSoc = std::make_shared<Analysis::Dvvp::JobWrapper::JobDeviceSoc>(0);
    std::shared_ptr<PMUEventsConfig> cfg = std::make_shared<PMUEventsConfig>();
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->ParsePmuConfig(cfg));
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->ParsePmuConfig(cfg));
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->ParsePmuConfig(cfg));
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->ParsePmuConfig(cfg));
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->ParsePmuConfig(cfg));
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->ParsePmuConfig(cfg));
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->ParsePmuConfig(cfg));
}

TEST_F(PROF_DEVICE_SOC_UTEST, GetAndStoreStartTime) {
    GlobalMockObject::verify();

    auto jobDeviceSoc = std::make_shared<Analysis::Dvvp::JobWrapper::JobDeviceSoc>(0);
    MSVP_MAKE_SHARED0_VOID(jobDeviceSoc->collectionjobComnCfg_, CollectionJobCommonParams);
    jobDeviceSoc->GetAndStoreStartTime(true);

    MOCK_METHOD(mockerJobDeviceSoc, StoreTime)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_FAILED));
    jobDeviceSoc->GetAndStoreStartTime(false);
    jobDeviceSoc->GetAndStoreStartTime(false);
    jobDeviceSoc->GetAndStoreStartTime(false);
}

TEST_F(PROF_DEVICE_SOC_UTEST, StoreTime) {
    GlobalMockObject::verify();

    auto jobDeviceSoc = std::make_shared<Analysis::Dvvp::JobWrapper::JobDeviceSoc>(0);
    MSVP_MAKE_SHARED0_VOID(jobDeviceSoc->collectionjobComnCfg_, CollectionJobCommonParams);

    MOCK_METHOD(mockerUploaderMgr, UploadFileData)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
        new analysis::dvvp::message::ProfileParams);
    jobDeviceSoc->params_ = params;

    std::string fileName = "tmp/PROF_DEVICE_SOC_UTEST/StoreTime";
    std::string startTime = "123456";
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->StoreTime(fileName, startTime));
}

TEST_F(PROF_DEVICE_SOC_UTEST, StartProfFailed)
{
    GlobalMockObject::verify();

    auto jobDeviceSoc = std::make_shared<Analysis::Dvvp::JobWrapper::JobDeviceSoc>(0);
    MSVP_MAKE_SHARED0_VOID(jobDeviceSoc->collectionjobComnCfg_, CollectionJobCommonParams);
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(new analysis::dvvp::message::ProfileParams());
    params->FromString("{\"result_dir\":\"/tmp/\", \"devices\":\"1\", \"job_id\":\"1\"}");

    jobDeviceSoc->params_ = params;
    jobDeviceSoc->isStarted_ = false;
    MOCK_METHOD(mockerParamValidation, CheckProfilingParams)
        .stubs()
        .will(returnValue(true));
    MOCK_METHOD(mockerJobDeviceSoc, StartProfHandle)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCK_METHOD(mockerProfChannelManager, Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCK_METHOD(mockerDrvChannelsMgr, GetAllChannels)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCK_METHOD(mockerJobDeviceSoc, RegisterCollectionJobs)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->StartProf(params));
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->StartProf(params));
    jobDeviceSoc->params_->host_profiling = false;
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->StartProf(params));
    jobDeviceSoc->params_->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->StartProf(params));
}

TEST_F(PROF_DEVICE_SOC_UTEST, ParseTsCpuConfig)
{
    GlobalMockObject::verify();

    MOCK_METHOD(mockerParamValidation, CheckTsCpuEventIsValid)
        .stubs()
        .will(returnValue(false));
    auto jobDeviceSoc = std::make_shared<Analysis::Dvvp::JobWrapper::JobDeviceSoc>(0);
    std::shared_ptr<PMUEventsConfig> cfg = std::make_shared<PMUEventsConfig>();
    auto tsCpuEvents = std::make_shared<std::vector<std::string>>();
    tsCpuEvents->push_back("0xa,0xb");
    cfg->tsCPUEvents = *tsCpuEvents;
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->ParseTsCpuConfig(cfg));
}

TEST_F(PROF_DEVICE_SOC_UTEST, ParseHbmConfig)
{
    GlobalMockObject::verify();

    MOCK_METHOD(mockerParamValidation, CheckHbmEventsIsValid)
        .stubs()
        .will(returnValue(false));
    auto jobDeviceSoc = std::make_shared<Analysis::Dvvp::JobWrapper::JobDeviceSoc>(0);
    std::shared_ptr<PMUEventsConfig> cfg = std::make_shared<PMUEventsConfig>();
    auto tsCpuEvents = std::make_shared<std::vector<std::string>>();
    tsCpuEvents->push_back("0xa,0xb");
    cfg->hbmEvents = *tsCpuEvents;
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->ParseHbmConfig(cfg));
}

TEST_F(PROF_DEVICE_SOC_UTEST, ParseAivConfig)
{
    GlobalMockObject::verify();

    MOCK_METHOD(mockerParamValidation, CheckAivEventsIsValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    MOCK_METHOD(mockerParamValidation, CheckAivEventCoresIsValid)
        .stubs()
        .will(returnValue(false));
    auto jobDeviceSoc = std::make_shared<Analysis::Dvvp::JobWrapper::JobDeviceSoc>(0);
    std::shared_ptr<PMUEventsConfig> cfg = std::make_shared<PMUEventsConfig>();
    auto tsCpuEvents = std::make_shared<std::vector<std::string>>();
    tsCpuEvents->push_back("0xa,0xb");
    cfg->aivEvents = *tsCpuEvents;
    std::vector<int> aivEventsCoreIds(3, 1);
    cfg->aivEventsCoreIds = aivEventsCoreIds;
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->ParseAivConfig(cfg));
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->ParseAivConfig(cfg));
}

TEST_F(PROF_DEVICE_SOC_UTEST, ParseControlCpuConfig)
{
    GlobalMockObject::verify();

    MOCK_METHOD(mockerParamValidation, CheckCtrlCpuEventIsValid)
        .stubs()
        .will(returnValue(false));
    auto jobDeviceSoc = std::make_shared<Analysis::Dvvp::JobWrapper::JobDeviceSoc>(0);
    std::shared_ptr<PMUEventsConfig> cfg = std::make_shared<PMUEventsConfig>();
    auto tsCpuEvents = std::make_shared<std::vector<std::string>>();
    tsCpuEvents->push_back("0xa,0xb");
    cfg->ctrlCPUEvents = *tsCpuEvents;
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->ParseControlCpuConfig(cfg));
}

TEST_F(PROF_DEVICE_SOC_UTEST, ParseDdrCpuConfig)
{
    GlobalMockObject::verify();

    MOCK_METHOD(mockerParamValidation, CheckDdrEventsIsValid)
        .stubs()
        .will(returnValue(false));
    auto jobDeviceSoc = std::make_shared<Analysis::Dvvp::JobWrapper::JobDeviceSoc>(0);
    std::shared_ptr<PMUEventsConfig> cfg = std::make_shared<PMUEventsConfig>();
    auto tsCpuEvents = std::make_shared<std::vector<std::string>>();
    tsCpuEvents->push_back("0xa,0xb");
    cfg->ddrEvents = *tsCpuEvents;
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->ParseDdrCpuConfig(cfg));
}

TEST_F(PROF_DEVICE_SOC_UTEST, ParseLlcConfig)
{
    GlobalMockObject::verify();

    MOCK_METHOD(mockerParamValidation, CheckLlcEventsIsValid)
        .stubs()
        .will(returnValue(false));
    auto jobDeviceSoc = std::make_shared<Analysis::Dvvp::JobWrapper::JobDeviceSoc>(0);
    std::shared_ptr<PMUEventsConfig> cfg = std::make_shared<PMUEventsConfig>();
    auto tsCpuEvents = std::make_shared<std::vector<std::string>>();
    tsCpuEvents->push_back("0xa,0xb");
    cfg->llcEvents = *tsCpuEvents;
    EXPECT_EQ(PROFILING_FAILED,jobDeviceSoc->ParseLlcConfig(cfg));
}

TEST_F(PROF_DEVICE_SOC_UTEST, CreateCollectionJobArray)
{
    GlobalMockObject::verify();

    auto jobDeviceSoc = std::make_shared<Analysis::Dvvp::JobWrapper::JobDeviceSoc>(0);
    MSVP_MAKE_SHARED0_VOID(jobDeviceSoc->collectionjobComnCfg_, CollectionJobCommonParams);
    MOCKER(analysis::dvvp::common::utils::Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
        new analysis::dvvp::message::ProfileParams);
    jobDeviceSoc->collectionjobComnCfg_->params = params;
    jobDeviceSoc->CreateCollectionJobArray();
}
