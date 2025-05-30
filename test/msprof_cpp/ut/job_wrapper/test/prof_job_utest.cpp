#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include "utils/utils.h"
#include "errno/error_code.h"
#include "ai_drv_dev_api.h"
#include "ai_drv_prof_api.h"
#include "config/config.h"
#include "prof_timer.h"
#include "prof_job.h"
#include "uploader_mgr.h"
#include "platform/platform.h"
#include "transport/hdc/hdc_transport.h"
#include "prof_task.h"
#include "mmpa_api.h"
#include "config_manager.h"
#include "prof_drv_event.h"
#include "dcmi_plugin.h"

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::message;
using namespace Analysis::Dvvp::JobWrapper;
using namespace Analysis::Dvvp::MsprofErrMgr;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;
using namespace Analysis::Dvvp::Common::Config;

class ProfJobUtest : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

class JOB_WRAPPER_PROF_TsCPu_JOB_TEST: public testing::Test {
protected:
    virtual void SetUp() {
        collectionJobCfg_ = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCfg>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
        std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext);
        auto comParams = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams>();
        comParams->params = params;
        comParams->jobCtx = jobCtx;
        collectionJobCfg_->comParams = comParams;
        collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
    }
    virtual void TearDown() {
        collectionJobCfg_.reset();
    }
public:
    std::shared_ptr<Analysis::Dvvp::JobWrapper::CollectionJobCfg> collectionJobCfg_;
};

TEST_F(JOB_WRAPPER_PROF_TsCPu_JOB_TEST, Init) {
    GlobalMockObject::verify();

    auto profTscpuJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfTscpuJob>();
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
    collectionJobCfg_->jobParams.events = nullptr;
    EXPECT_EQ(PROFILING_FAILED, profTscpuJob->Init(collectionJobCfg_));
    collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
    collectionJobCfg_->jobParams.events->push_back("0x11");
    collectionJobCfg_->comParams->params = params;
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, profTscpuJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = false;
    EXPECT_EQ(PROFILING_SUCCESS, profTscpuJob->Init(collectionJobCfg_));
}

TEST_F(JOB_WRAPPER_PROF_TsCPu_JOB_TEST, Process) {
    GlobalMockObject::verify();

    auto proTsCpuJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfTscpuJob>();
    proTsCpuJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_FAILED, proTsCpuJob->Process());
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
    collectionJobCfg_->comParams->params = params;
    collectionJobCfg_->jobParams.events->push_back("0x11");
    collectionJobCfg_->comParams->params->cpu_sampling_interval = 20;
    proTsCpuJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, proTsCpuJob->Process());
}

TEST_F(JOB_WRAPPER_PROF_TsCPu_JOB_TEST, Uninit) {
    GlobalMockObject::verify();

    auto proTsCpuJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfTscpuJob>();
    proTsCpuJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, proTsCpuJob->Uninit());
    collectionJobCfg_->jobParams.events->push_back("0x11");
    proTsCpuJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, proTsCpuJob->Uninit());
}

class JOB_WRAPPER_PROF_TsTrack_JOB_TEST: public testing::Test {
protected:
    virtual void SetUp() {
        collectionJobCfg_ = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCfg>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
        std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext);
        auto comParams = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams>();
        comParams->params = params;
        comParams->jobCtx = jobCtx;
        collectionJobCfg_->comParams = comParams;
        collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
    }
    virtual void TearDown() {
        collectionJobCfg_.reset();
    }
public:
    std::shared_ptr<Analysis::Dvvp::JobWrapper::CollectionJobCfg> collectionJobCfg_;
};

TEST_F(JOB_WRAPPER_PROF_TsTrack_JOB_TEST, Init)
{
    GlobalMockObject::verify();

    auto profTsTrackJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfTsTrackJob>();
    EXPECT_EQ(PROFILING_FAILED, profTsTrackJob->Init(nullptr));
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, profTsTrackJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->ts_timeline = "off";
    collectionJobCfg_->comParams->params->ts_keypoint = "off";
    collectionJobCfg_->comParams->params->ts_memcpy = "off";
    EXPECT_EQ(PROFILING_FAILED, profTsTrackJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->ts_keypoint = "on";
    EXPECT_EQ(PROFILING_SUCCESS, profTsTrackJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->ts_keypoint = "off";
    collectionJobCfg_->comParams->params->ts_memcpy = "on";
    EXPECT_EQ(PROFILING_SUCCESS, profTsTrackJob->Init(collectionJobCfg_));
}

TEST_F(JOB_WRAPPER_PROF_TsTrack_JOB_TEST, Process)
{
    GlobalMockObject::verify();

    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(true));

    auto profTsTrackJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfTsTrackJob>();
    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->ts_keypoint = "on";
    profTsTrackJob->Init(collectionJobCfg_);
    profTsTrackJob->collectionJobCfg_ = nullptr;
    EXPECT_EQ(PROFILING_FAILED, profTsTrackJob->Process());
    profTsTrackJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, profTsTrackJob->Process());
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
    collectionJobCfg_->comParams->params = params;
    collectionJobCfg_->comParams->params->ts_timeline = "on";
    collectionJobCfg_->jobParams.events->push_back("0x11");
    collectionJobCfg_->comParams->params->cpu_sampling_interval = 20;
    profTsTrackJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, profTsTrackJob->Process());

    collectionJobCfg_->comParams->params->cpu_sampling_interval = 0;
    profTsTrackJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, profTsTrackJob->Process());
}

TEST_F(JOB_WRAPPER_PROF_TsTrack_JOB_TEST, ProcessWillReturnSuccWhenChannelIsNotValid)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(false));
    auto profTsTrackJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfTsTrackJob>();
    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->ts_keypoint = "on";
    profTsTrackJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, profTsTrackJob->Process());
}

TEST_F(JOB_WRAPPER_PROF_TsTrack_JOB_TEST, ProcessWillReturnValueWhenDrvTsFwStartRun)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::driver::DrvTsFwStart)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_FAILED));
    auto profTsTrackJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfTsTrackJob>();
    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->ts_keypoint = "on";
    profTsTrackJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, profTsTrackJob->Process());
    EXPECT_EQ(PROFILING_FAILED, profTsTrackJob->Process());
}

TEST_F(JOB_WRAPPER_PROF_TsTrack_JOB_TEST, Uninit)
{
    GlobalMockObject::verify();

    auto profTsTrackJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfTsTrackJob>();
    collectionJobCfg_->jobParams.events->push_back("0x11");
    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->ts_keypoint = "on";

    // collectionJobCfg_ not init
    EXPECT_EQ(PROFILING_SUCCESS, profTsTrackJob->Uninit());

    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::driver::DrvStop)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    profTsTrackJob->Init(collectionJobCfg_);
    // channel not valid
    EXPECT_EQ(PROFILING_SUCCESS, profTsTrackJob->Uninit());

    // DrvStop fail
    EXPECT_EQ(PROFILING_FAILED, profTsTrackJob->Uninit());

    // DrvStop succ
    EXPECT_EQ(PROFILING_SUCCESS, profTsTrackJob->Uninit());
}

class ProfStarsSocLogJobUtest : public testing::Test {
protected:
    virtual void SetUp() {
        collectionJobCfg_ = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCfg>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
        std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext);
        auto comParams = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams>();
        comParams->params = params;
        comParams->jobCtx = jobCtx;
        collectionJobCfg_->comParams = comParams;
        collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
    }
    virtual void TearDown() {
        collectionJobCfg_.reset();
    }
private:
    std::shared_ptr<Analysis::Dvvp::JobWrapper::CollectionJobCfg> collectionJobCfg_;
};

TEST_F(ProfStarsSocLogJobUtest, Init)
{
    GlobalMockObject::verify();

    auto profStarsSocLogJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfStarsSocLogJob>();
    EXPECT_EQ(PROFILING_FAILED, profStarsSocLogJob->Init(nullptr));
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, profStarsSocLogJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = false;
    EXPECT_EQ(PROFILING_SUCCESS, profStarsSocLogJob->Init(collectionJobCfg_));
}

TEST_F(ProfStarsSocLogJobUtest, Process)
{
    GlobalMockObject::verify();

    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::driver::DrvStarsSocLogStart)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    auto profStarsSocLogJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfStarsSocLogJob>();
    collectionJobCfg_->comParams->params->host_profiling = false;
    profStarsSocLogJob->Init(collectionJobCfg_);
    profStarsSocLogJob->collectionJobCfg_ = nullptr;
    EXPECT_EQ(PROFILING_FAILED, profStarsSocLogJob->Process());

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    collectionJobCfg_->comParams->params = params;
    profStarsSocLogJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_FAILED, profStarsSocLogJob->Process());
    EXPECT_EQ(PROFILING_SUCCESS, profStarsSocLogJob->Process());
}

TEST_F(ProfStarsSocLogJobUtest, ProcessWillReturnSuccWhenChannelIsNotValid)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(false));
    auto profStarsSocLogJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfStarsSocLogJob>();
    collectionJobCfg_->comParams->params->host_profiling = false;
    profStarsSocLogJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, profStarsSocLogJob->Process());
}

TEST_F(ProfStarsSocLogJobUtest, Uninit)
{
    GlobalMockObject::verify();

    auto profStarsSocLogJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfStarsSocLogJob>();
    collectionJobCfg_->jobParams.events->push_back("0x11");
    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->ts_keypoint = "on";

    // collectionJobCfg_ not init
    EXPECT_EQ(PROFILING_SUCCESS, profStarsSocLogJob->Uninit());

    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::driver::DrvStop)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    profStarsSocLogJob->Init(collectionJobCfg_);
    // channel not valid
    EXPECT_EQ(PROFILING_SUCCESS, profStarsSocLogJob->Uninit());

    // DrvStop fail
    EXPECT_EQ(PROFILING_FAILED, profStarsSocLogJob->Uninit());

    // DrvStop succ
    EXPECT_EQ(PROFILING_SUCCESS, profStarsSocLogJob->Uninit());
}

class ProfStarsBlockLogJobUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        collectionJobCfg_ = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCfg>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
        std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext);
        auto comParams = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams>();
        comParams->params = params;
        comParams->jobCtx = jobCtx;
        collectionJobCfg_->comParams = comParams;
        collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
    }
    virtual void TearDown()
    {
        collectionJobCfg_.reset();
    }
private:
    std::shared_ptr<Analysis::Dvvp::JobWrapper::CollectionJobCfg> collectionJobCfg_;
};

TEST_F(ProfStarsBlockLogJobUtest, Init)
{
    GlobalMockObject::verify();
    auto profStarsBlockLogJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfStarsBlockLogJob>();
    EXPECT_EQ(PROFILING_FAILED, profStarsBlockLogJob->Init(nullptr));
}

class JOB_WRAPPER_PROF_AICORE_JOB_TEST: public testing::Test {
protected:
    virtual void SetUp() {
        collectionJobCfg_ = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCfg>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
        std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext);
        auto comParams = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams>();
        comParams->params = params;
        comParams->jobCtx = jobCtx;
        collectionJobCfg_->comParams = comParams;
        collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
        collectionJobCfg_->jobParams.cores = std::make_shared<std::vector<int> >(0);
    }
    virtual void TearDown() {
        collectionJobCfg_.reset();
    }
public:
    std::shared_ptr<Analysis::Dvvp::JobWrapper::CollectionJobCfg> collectionJobCfg_;
};

TEST_F(JOB_WRAPPER_PROF_AICORE_JOB_TEST, Init) {
    GlobalMockObject::verify();

    auto profAicoreJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfAicoreJob>();
    EXPECT_EQ(PROFILING_FAILED, profAicoreJob->Init(nullptr));
    collectionJobCfg_->jobParams.events->push_back("0x11");
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
        new analysis::dvvp::message::ProfileParams);
    params->ai_core_profiling_mode = "sample-based";
    params->ai_core_profiling = "on";
    collectionJobCfg_->comParams->params = params;
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, profAicoreJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = false;
    EXPECT_EQ(PROFILING_SUCCESS, profAicoreJob->Init(collectionJobCfg_));

    auto aivSample = std::make_shared<Analysis::Dvvp::JobWrapper::ProfAivJob>();
    std::shared_ptr<analysis::dvvp::message::ProfileParams> aivParams(
        new analysis::dvvp::message::ProfileParams);
    aivParams->aiv_profiling_mode = "sample-based";
    aivParams->aiv_profiling = "on";
    collectionJobCfg_->comParams->params = aivParams;
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, aivSample->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = false;
    EXPECT_EQ(PROFILING_SUCCESS, aivSample->Init(collectionJobCfg_));
}

TEST_F(JOB_WRAPPER_PROF_AICORE_JOB_TEST, Process) {
    GlobalMockObject::verify();

    auto profAicoreJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfAicoreJob>();

    EXPECT_EQ(PROFILING_FAILED, profAicoreJob->Process());
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
        new analysis::dvvp::message::ProfileParams);
    params->ai_core_profiling_mode = "sample-based";
    params->ai_core_profiling = "on";
    collectionJobCfg_->comParams->params = params;
    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->jobParams.events->push_back("0x11");
    profAicoreJob->Init(collectionJobCfg_);
    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::driver::DrvAicoreStart)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, profAicoreJob->Process());
    EXPECT_EQ(PROFILING_FAILED, profAicoreJob->Process());
    EXPECT_EQ(PROFILING_SUCCESS, profAicoreJob->Process());
}

TEST_F(JOB_WRAPPER_PROF_AICORE_JOB_TEST, Uninit) {
    GlobalMockObject::verify();

    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    auto profAicoreJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfAicoreJob>();
    profAicoreJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_FAILED, profAicoreJob->Process());
    EXPECT_EQ(PROFILING_SUCCESS, profAicoreJob->Uninit());
    collectionJobCfg_->jobParams.events->push_back("0x11");
    profAicoreJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, profAicoreJob->Uninit());
    EXPECT_EQ(PROFILING_SUCCESS, profAicoreJob->Uninit());
}

class JOB_WRAPPER_PROF_AICORETASK_JOB_TEST: public testing::Test {
protected:
    virtual void SetUp() {
        collectionJobCfg_ = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCfg>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
        std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext);
        auto comParams = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams>();
        comParams->params = params;
        comParams->jobCtx = jobCtx;
        collectionJobCfg_->comParams = comParams;
        collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
        collectionJobCfg_->jobParams.cores = std::make_shared<std::vector<int> >(0);
    }
    virtual void TearDown() {
        collectionJobCfg_.reset();
    }
public:
    std::shared_ptr<Analysis::Dvvp::JobWrapper::CollectionJobCfg> collectionJobCfg_;
};

TEST_F(JOB_WRAPPER_PROF_AICORETASK_JOB_TEST, Init) {
    GlobalMockObject::verify();

    auto profAicoreTaskBasedJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfAicoreTaskBasedJob>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
    collectionJobCfg_->jobParams.events = nullptr;
        params->ai_core_profiling_mode = "task-based";
    params->ai_core_profiling = "on";
    collectionJobCfg_->comParams->params = params;
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, profAicoreTaskBasedJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = false;
    EXPECT_EQ(PROFILING_FAILED, profAicoreTaskBasedJob->Init(collectionJobCfg_));
    collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
    collectionJobCfg_->jobParams.events->push_back("0x11");
    EXPECT_EQ(PROFILING_SUCCESS, profAicoreTaskBasedJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, profAicoreTaskBasedJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = false;

    auto aivTaskBased = std::make_shared<Analysis::Dvvp::JobWrapper::ProfAivTaskBasedJob>();
    std::shared_ptr<analysis::dvvp::message::ProfileParams> aivParams(
        new analysis::dvvp::message::ProfileParams);
    aivParams->aiv_profiling_mode = "task-based";
    aivParams->aiv_profiling = "on";
    collectionJobCfg_->comParams->params = aivParams;
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, aivTaskBased->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = false;
    EXPECT_EQ(PROFILING_SUCCESS, aivTaskBased->Init(collectionJobCfg_));
}

TEST_F(JOB_WRAPPER_PROF_AICORETASK_JOB_TEST, Process) {
    GlobalMockObject::verify();

    auto profAicoreTaskBasedJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfAicoreTaskBasedJob>();
    EXPECT_EQ(PROFILING_FAILED, profAicoreTaskBasedJob->Process());

    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
    params->ai_core_profiling_mode = "task-based";
    params->ai_core_profiling = "on";
    collectionJobCfg_->comParams->params = params;
    collectionJobCfg_->jobParams.events->push_back("0x11");
    collectionJobCfg_->jobParams.cores->push_back(1);
    collectionJobCfg_->comParams->params->aicore_sampling_interval = 1;

    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::driver::DrvAicoreTaskBasedStart)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    profAicoreTaskBasedJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, profAicoreTaskBasedJob->Process());
    EXPECT_EQ(PROFILING_FAILED, profAicoreTaskBasedJob->Process());
    EXPECT_EQ(PROFILING_SUCCESS, profAicoreTaskBasedJob->Process());
}

TEST_F(JOB_WRAPPER_PROF_AICORETASK_JOB_TEST, Uninit) {
    GlobalMockObject::verify();

    auto profAicoreTaskBasedJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfAicoreTaskBasedJob>();
    profAicoreTaskBasedJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, profAicoreTaskBasedJob->Uninit());
    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::driver::DrvStop)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    collectionJobCfg_->jobParams.events->push_back("0x11");
    profAicoreTaskBasedJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, profAicoreTaskBasedJob->Uninit());
    EXPECT_EQ(PROFILING_SUCCESS, profAicoreTaskBasedJob->Uninit());
}

class JOB_WRAPPER_PROF_FFTS_JOB_TEST: public testing::Test {
protected:
    virtual void SetUp() {
        collectionJobCfg_ = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCfg>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
        std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext);
        auto comParams = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams>();
        comParams->params = params;
        comParams->jobCtx = jobCtx;
        collectionJobCfg_->comParams = comParams;
        collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
        collectionJobCfg_->jobParams.cores = std::make_shared<std::vector<int> >(0);
        collectionJobCfg_->jobParams.aivEvents = std::make_shared<std::vector<std::string> >(0);
        collectionJobCfg_->jobParams.aivCores = std::make_shared<std::vector<int> >(0);
    }
    virtual void TearDown() {
        collectionJobCfg_.reset();
    }
public:
    std::shared_ptr<Analysis::Dvvp::JobWrapper::CollectionJobCfg> collectionJobCfg_;
};

TEST_F(JOB_WRAPPER_PROF_FFTS_JOB_TEST, Init) {
    GlobalMockObject::verify();

    auto profFftsJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfFftsProfileJob>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
    collectionJobCfg_->jobParams.events = nullptr;
    params->ai_core_profiling_mode = "task-based";
    params->ai_core_profiling = "on";
    collectionJobCfg_->comParams->params = params;
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, profFftsJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = false;
    EXPECT_EQ(PROFILING_FAILED, profFftsJob->Init(collectionJobCfg_));
    collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
    collectionJobCfg_->jobParams.events->push_back("0x11");
    EXPECT_EQ(PROFILING_SUCCESS, profFftsJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, profFftsJob->Init(collectionJobCfg_));

    collectionJobCfg_->comParams->params->host_profiling = false;
    params->ai_core_profiling = "off";
    params->aiv_profiling = "off";
    EXPECT_EQ(PROFILING_FAILED, profFftsJob->Init(collectionJobCfg_));

    params->ai_core_profiling = "off";
    params->aiv_profiling = "on";
    EXPECT_EQ(PROFILING_SUCCESS, profFftsJob->Init(collectionJobCfg_));

    params->ai_core_profiling = "on";
    params->aiv_profiling = "on";
    EXPECT_EQ(PROFILING_SUCCESS, profFftsJob->Init(collectionJobCfg_));
}

TEST_F(JOB_WRAPPER_PROF_FFTS_JOB_TEST, Process) {
    GlobalMockObject::verify();

    auto profFftsJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfFftsProfileJob>();
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
    params->ai_core_profiling_mode = "task-based";
    params->ai_core_profiling = "on";
    params->aiv_profiling = "on";
    collectionJobCfg_->comParams->params = params;
    collectionJobCfg_->jobParams.events->push_back("0x11");
    collectionJobCfg_->jobParams.cores->push_back(1);
    collectionJobCfg_->comParams->params->aicore_sampling_interval = 20;
    profFftsJob->Init(collectionJobCfg_);

    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::driver::DrvFftsProfileStart)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, profFftsJob->Process());
    EXPECT_EQ(PROFILING_FAILED, profFftsJob->Process());
    EXPECT_EQ(PROFILING_SUCCESS, profFftsJob->Process());
}

TEST_F(JOB_WRAPPER_PROF_FFTS_JOB_TEST, Uninit) {
    GlobalMockObject::verify();

    auto profFftsJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfFftsProfileJob>();
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
    params->ai_core_profiling_mode = "sample-based";
    params->aiv_profiling_mode = "sample-based";
    params->ai_core_profiling = "on";
    params->aiv_profiling = "on";
    collectionJobCfg_->comParams->params = params;
    collectionJobCfg_->jobParams.events->push_back("0x11");
    collectionJobCfg_->jobParams.cores->push_back(1);
    collectionJobCfg_->jobParams.aivEvents->push_back("0x11");
    collectionJobCfg_->jobParams.aivCores->push_back(1);
    collectionJobCfg_->comParams->params->aicore_sampling_interval = 20;
    profFftsJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, profFftsJob->Uninit());

    profFftsJob->Init(collectionJobCfg_);
    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_SUCCESS, profFftsJob->Uninit());
}

class ProfInstrPerfJobUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        collectionJobCfg_ = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCfg>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
        std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext);
        auto comParams = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams>();
        comParams->params = params;
        comParams->jobCtx = jobCtx;
        collectionJobCfg_->comParams = comParams;
        collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
    }
    virtual void TearDown()
    {
        collectionJobCfg_.reset();
    }
private:
    std::shared_ptr<Analysis::Dvvp::JobWrapper::CollectionJobCfg> collectionJobCfg_;
};

TEST_F(ProfInstrPerfJobUtest, Init)
{
    GlobalMockObject::verify();

    auto profInstrPerfJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfInstrPerfJob>();
    EXPECT_EQ(PROFILING_FAILED, profInstrPerfJob->Init(nullptr));
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, profInstrPerfJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->instr_profiling = "off";
    EXPECT_EQ(PROFILING_FAILED, profInstrPerfJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->instr_profiling = "on";
    MOCKER(memcpy_s)
        .stubs()
        .will(returnValue(EOK - 1))
        .then(returnValue(EOK));
    EXPECT_EQ(PROFILING_FAILED, profInstrPerfJob->Init(collectionJobCfg_));
    EXPECT_EQ(PROFILING_SUCCESS, profInstrPerfJob->Init(collectionJobCfg_));
}

TEST_F(ProfInstrPerfJobUtest, ProcessWillReturnFailWhenStartFail)
{
    GlobalMockObject::verify();

    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::driver::DrvInstrProfileStart)
        .stubs()
        .will(returnValue(PROFILING_FAILED));

    auto profInstrPerfJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfInstrPerfJob>();
    profInstrPerfJob->Init(collectionJobCfg_);
    profInstrPerfJob->collectionJobCfg_ = nullptr;
    EXPECT_EQ(PROFILING_FAILED, profInstrPerfJob->Process());

    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->instr_profiling = "on";
    profInstrPerfJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_FAILED, profInstrPerfJob->Process());
}

TEST_F(ProfInstrPerfJobUtest, ProcessWillReturnSuccWhenStartSucc)
{
    GlobalMockObject::verify();

    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::driver::DrvInstrProfileStart)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    auto profInstrPerfJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfInstrPerfJob>();

    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->instr_profiling = "on";
    profInstrPerfJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, profInstrPerfJob->Process());
}

TEST_F(ProfInstrPerfJobUtest, ProcessWillReturnSuccWhenChannelIsNotValid)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::driver::DrvInstrProfileStart)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    auto profInstrPerfJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfInstrPerfJob>();
    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->instr_profiling = "on";
    profInstrPerfJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, profInstrPerfJob->Process());
}

TEST_F(ProfInstrPerfJobUtest, UninitWillReturnFailWhenStopFail)
{
    GlobalMockObject::verify();

    auto profInstrPerfJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfInstrPerfJob>();
    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->instr_profiling = "on";

    // collectionJobCfg_ not init
    EXPECT_EQ(PROFILING_FAILED, profInstrPerfJob->Uninit());

    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::driver::DrvStop)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    profInstrPerfJob->Init(collectionJobCfg_);

    EXPECT_EQ(PROFILING_FAILED, profInstrPerfJob->Uninit());
}

TEST_F(ProfInstrPerfJobUtest, UninitWillReturnSuccWhenStopSucc)
{
    GlobalMockObject::verify();

    auto profInstrPerfJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfInstrPerfJob>();
    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->instr_profiling = "on";

    // collectionJobCfg_ not init
    EXPECT_EQ(PROFILING_FAILED, profInstrPerfJob->Uninit());

    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::driver::DrvStop)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    profInstrPerfJob->Init(collectionJobCfg_);

    EXPECT_EQ(PROFILING_SUCCESS, profInstrPerfJob->Uninit());
}

TEST_F(ProfInstrPerfJobUtest, UninitWillReturnSuccWhenChannelNotValid)
{
    GlobalMockObject::verify();

    auto profInstrPerfJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfInstrPerfJob>();
    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->instr_profiling = "on";

    // collectionJobCfg_ not init
    EXPECT_EQ(PROFILING_FAILED, profInstrPerfJob->Uninit());

    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(false));
    profInstrPerfJob->Init(collectionJobCfg_);
    // channel not valid
    EXPECT_EQ(PROFILING_SUCCESS, profInstrPerfJob->Uninit());
}

class JOB_WRAPPER_PROF_CTRLCPU_JOB_TEST : public testing::Test {
protected:
    virtual void SetUp()
    {
        collectionJobCfg_ = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCfg>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
        std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext);
        auto comParams = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams>();
        comParams->params = params;
        comParams->jobCtx = jobCtx;
        collectionJobCfg_->comParams = comParams;
        collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
        collectionJobCfg_->jobParams.cores = std::make_shared<std::vector<int> >(0);
    }
    virtual void TearDown()
    {
        collectionJobCfg_.reset();
    }
private:
    std::shared_ptr<Analysis::Dvvp::JobWrapper::CollectionJobCfg> collectionJobCfg_;
};

void fake_get_files_perf(const std::string &dir, bool is_recur, std::vector<std::string>& files)
{
    files.push_back("ctrlcpu.data.1");
    files.push_back("ctrlcpu.data.2");
    files.push_back("ctrlcpu.data.3");
    files.push_back("ctrlcpu.data.4");
}

TEST_F(JOB_WRAPPER_PROF_CTRLCPU_JOB_TEST, InitFailed) {
    GlobalMockObject::verify();

    auto profCtrlCpuBasedJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfCtrlcpuJob>();
    collectionJobCfg_->jobParams.events->push_back("0x11");
    EXPECT_EQ(PROFILING_FAILED, profCtrlCpuBasedJob->Init(collectionJobCfg_));
}

TEST_F(JOB_WRAPPER_PROF_CTRLCPU_JOB_TEST, Init) {
    GlobalMockObject::verify();

    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::RunSocSide)
        .stubs()
        .will(returnValue(true));

    auto profCtrlCpuBasedJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfCtrlcpuJob>();
    collectionJobCfg_->jobParams.events->push_back("0x11");
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
    collectionJobCfg_->comParams->params = params;
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, profCtrlCpuBasedJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = false;

    EXPECT_EQ(PROFILING_FAILED, profCtrlCpuBasedJob->Init(nullptr));
}

TEST_F(JOB_WRAPPER_PROF_CTRLCPU_JOB_TEST, Process) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::RunSocSide)
        .stubs()
        .will(returnValue(true));

    auto profCtrlCpuBasedJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfCtrlcpuJob>();
    profCtrlCpuBasedJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_FAILED, profCtrlCpuBasedJob->Process());
    collectionJobCfg_->jobParams.events->push_back("0x11");
    collectionJobCfg_->jobParams.cores->push_back(1);
    collectionJobCfg_->jobParams.dataPath = "./ctrlCpu.data";
    collectionJobCfg_->comParams->params->aicore_sampling_interval = 20;
    profCtrlCpuBasedJob->Init(collectionJobCfg_);
    collectionJobCfg_->comParams->jobCtx = std::make_shared<analysis::dvvp::message::JobContext>();

    std::string filePath = "./ctrlCpu.data";
    MOCKER_CPP(&Analysis::Dvvp::JobWrapper::ProfCtrlcpuJob::PrepareDataDir)
        .stubs()
        .with(outBound(filePath))
        .will(returnValue(PROFILING_SUCCESS));

    MOCKER(analysis::dvvp::common::utils::Utils::GetFiles)
        .stubs()
        .will(invoke(fake_get_files_perf));

    MOCKER(&MmCreateTaskWithThreadAttr)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    EXPECT_EQ(PROFILING_SUCCESS, profCtrlCpuBasedJob->Process());

    std::vector<std::string> paramsV;
    MOCKER(analysis::dvvp::common::utils::Utils::Split)
        .stubs()
        .will(returnValue(paramsV));
    EXPECT_EQ(PROFILING_FAILED, profCtrlCpuBasedJob->Process());
    remove("./ctrlCpu.data");
}

TEST_F(JOB_WRAPPER_PROF_CTRLCPU_JOB_TEST, Uninit) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::RunSocSide)
        .stubs()
        .will(returnValue(true));

    auto profCtrlCpuBasedJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfCtrlcpuJob>();
    collectionJobCfg_->jobParams.events->push_back("0x11");
    collectionJobCfg_->jobParams.cores->push_back(1);
    profCtrlCpuBasedJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS,profCtrlCpuBasedJob->Uninit());
    MOCKER(analysis::dvvp::common::utils::Utils::ExecCmd)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    profCtrlCpuBasedJob->Init(collectionJobCfg_);
    profCtrlCpuBasedJob->ctrlcpuProcess_ = 123456;
    EXPECT_EQ(PROFILING_FAILED,profCtrlCpuBasedJob->Uninit());
    bool is_exited = true;
    MOCKER(analysis::dvvp::common::utils::Utils::WaitProcess)
        .stubs()
        .with(any(), outBound(is_exited), any(), any())
        .will(returnValue(PROFILING_SUCCESS));
    profCtrlCpuBasedJob->Init(collectionJobCfg_);
    profCtrlCpuBasedJob->ctrlcpuProcess_ = 123456;
    EXPECT_EQ(PROFILING_SUCCESS,profCtrlCpuBasedJob->Uninit());
}

TEST_F(JOB_WRAPPER_PROF_CTRLCPU_JOB_TEST, GetCollectCtrlCpuEventCmd) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::RunSocSide)
        .stubs()
        .will(returnValue(true));

    auto profCtrlCpuBasedJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfCtrlcpuJob>();
    collectionJobCfg_->jobParams.events->push_back("0x11");
    collectionJobCfg_->jobParams.cores->push_back(1);
    profCtrlCpuBasedJob->Init(collectionJobCfg_);

    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
        new analysis::dvvp::message::ProfileParams);
    collectionJobCfg_->comParams->params = params;
    collectionJobCfg_->comParams->params->cpu_sampling_interval = 10;
    std::string filePath = "./GetCollectCtrlCpuEventCmd";
    MOCKER_CPP(&Analysis::Dvvp::JobWrapper::ProfCtrlcpuJob::PrepareDataDir)
        .stubs()
        .with(outBound(filePath))
        .will(returnValue(PROFILING_SUCCESS));

    int dev_id = 0;
    std::vector<std::string> events;
    std::string prof_ctrl_cpu_cmd;
    EXPECT_EQ(PROFILING_FAILED, profCtrlCpuBasedJob->GetCollectCtrlCpuEventCmd(events, prof_ctrl_cpu_cmd));
    events.push_back("0x11");
    std::string fileName = "./GetCollectCtrlCpuEventCmd";
    std::ofstream ofs(fileName);
    ofs << "test" << std::endl;
    ofs.close();
    collectionJobCfg_->jobParams.dataPath = fileName;
    profCtrlCpuBasedJob->Init(collectionJobCfg_);

    EXPECT_EQ(PROFILING_SUCCESS, profCtrlCpuBasedJob->GetCollectCtrlCpuEventCmd(events, prof_ctrl_cpu_cmd));
    collectionJobCfg_->jobParams.dataPath = "./tmp_dir/test";
    profCtrlCpuBasedJob->Init(collectionJobCfg_);

    EXPECT_EQ(PROFILING_SUCCESS, profCtrlCpuBasedJob->GetCollectCtrlCpuEventCmd(events, prof_ctrl_cpu_cmd));
    remove(fileName.c_str());
}

TEST_F(JOB_WRAPPER_PROF_CTRLCPU_JOB_TEST, PrepareDataDir) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::RunSocSide)
        .stubs()
        .will(returnValue(true));

    MOCKER(analysis::dvvp::common::utils::Utils::CreateDir)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    auto profCtrlCpuBasedJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfCtrlcpuJob>();
    profCtrlCpuBasedJob->collectionJobCfg_ = collectionJobCfg_;
    std::string dir;
    EXPECT_EQ(PROFILING_FAILED, profCtrlCpuBasedJob->PrepareDataDir(dir));
}

TEST_F(JOB_WRAPPER_PROF_CTRLCPU_JOB_TEST, PerfScriptTask) {
    GlobalMockObject::verify();

    MOCKER(analysis::dvvp::common::utils::Utils::GetFiles)
        .stubs()
        .will(invoke(fake_get_files_perf));

    MOCKER(remove)
        .stubs()
        .will(returnValue(0));

    MOCKER_CPP(&analysis::dvvp::common::thread::Thread::IsQuit)
        .stubs()
        .will(returnValue(true));

    MOCKER(analysis::dvvp::common::utils::Utils::ExecCmd)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    MOCKER_CPP(&Analysis::Dvvp::JobWrapper::PerfExtraTask::StoreData)
        .stubs();
    Analysis::Dvvp::JobWrapper::PerfExtraTask perfTask(10, "./ret", collectionJobCfg_->comParams->jobCtx, collectionJobCfg_->comParams->params);
    EXPECT_EQ(PROFILING_SUCCESS, perfTask.Init());
    perfTask.PerfScriptTask();
}

TEST_F(JOB_WRAPPER_PROF_CTRLCPU_JOB_TEST, run) {
    GlobalMockObject::verify();

    MOCKER_CPP(&Analysis::Dvvp::JobWrapper::PerfExtraTask::PerfScriptTask)
        .stubs();

    MOCKER_CPP(&analysis::dvvp::common::thread::Thread::IsQuit)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));

    MOCKER_CPP(&analysis::dvvp::common::memory::Chunk::Init)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));

    Analysis::Dvvp::JobWrapper::PerfExtraTask perfTask(10, "./", collectionJobCfg_->comParams->jobCtx, collectionJobCfg_->comParams->params);
    // BufInit failed
    EXPECT_EQ(PROFILING_FAILED, perfTask.Init());
    // Init succ
    EXPECT_EQ(PROFILING_SUCCESS, perfTask.Init());
    // Init failed
    EXPECT_EQ(PROFILING_FAILED, perfTask.Init());
    auto errorContext = MsprofErrorManager::instance()->GetErrorManagerContext();
    perfTask.Run(errorContext);
    EXPECT_EQ(PROFILING_SUCCESS, perfTask.UnInit());
}

TEST_F(JOB_WRAPPER_PROF_CTRLCPU_JOB_TEST, StoreData) {
    GlobalMockObject::verify();

    MOCKER_CPP(&analysis::dvvp::transport::UploaderMgr::UploadData,
        int(analysis::dvvp::transport::UploaderMgr::*)
        (const std::string&, SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk>))
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    auto jobCtx = std::make_shared<analysis::dvvp::message::JobContext>();
    Analysis::Dvvp::JobWrapper::PerfExtraTask perfTask(10, "./", jobCtx, collectionJobCfg_->comParams->params);
    EXPECT_EQ(PROFILING_SUCCESS, perfTask.Init());
    //file open failed
    std::string fileName = "./test/test";
    perfTask.StoreData(fileName);
    //suss
    fileName = "./test";
    std::ofstream ofs(fileName);
    ofs << "test" << std::endl;
    ofs.close();

    perfTask.StoreData(fileName);
    remove("./test");
}


class JOB_WRAPPER_PROF_SYSSTAT_JOB_TEST: public testing::Test {
protected:
    virtual void SetUp() {
        collectionJobCfg_ = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCfg>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
        std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext);
        auto comParams = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams>();
        comParams->params = params;
        comParams->jobCtx = jobCtx;
        comParams->jobCtx = std::make_shared<analysis::dvvp::message::JobContext>();
        collectionJobCfg_->comParams = comParams;
        collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
        collectionJobCfg_->jobParams.cores = std::make_shared<std::vector<int> >(0);
    }
    virtual void TearDown() {
        collectionJobCfg_.reset();
    }
public:
    std::shared_ptr<Analysis::Dvvp::JobWrapper::CollectionJobCfg> collectionJobCfg_;
};

TEST_F(JOB_WRAPPER_PROF_SYSSTAT_JOB_TEST, Init) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::RunSocSide)
        .stubs()
        .will(returnValue(true));
    auto profSysStatJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfSysStatJob>();
    profSysStatJob->Init(nullptr);
    profSysStatJob->Init(collectionJobCfg_);

    collectionJobCfg_->comParams->params->sys_profiling = "off";
    EXPECT_EQ(PROFILING_FAILED, profSysStatJob->Init(collectionJobCfg_));
    HDC_SESSION session = (HDC_SESSION)0x12345678;
    auto transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
            new analysis::dvvp::transport::HDCTransport(session));
    auto uploader = std::make_shared<analysis::dvvp::transport::Uploader>(transport);
    MOCKER_CPP(&analysis::dvvp::transport::UploaderMgr::GetUploader)
        .stubs()
        .will(returnValue(uploader));
    analysis::dvvp::transport::UploaderMgr::instance()->AddUploader("0", uploader);
    collectionJobCfg_->comParams->params->sys_profiling = "on";
    profSysStatJob->Init(collectionJobCfg_);
    EXPECT_TRUE(profSysStatJob->IsGlobalJobLevel() == true);
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, profSysStatJob->Init(collectionJobCfg_));
}

TEST_F(JOB_WRAPPER_PROF_SYSSTAT_JOB_TEST, Process) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::RunSocSide)
        .stubs()
        .will(returnValue(true));
    collectionJobCfg_->comParams->params->cpu_sampling_interval = 200;
    auto profSysStatJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfSysStatJob>();
    profSysStatJob->Init(collectionJobCfg_);

    EXPECT_EQ(PROFILING_SUCCESS, profSysStatJob->Process());

    profSysStatJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, profSysStatJob->Process());
    unsigned int devId = 0;
    unsigned int bufSize = 10;
    unsigned int sampleIntervalMs = 100;
    std::string srcFileName = "srcFileName";
    std::string retFileName = "retFileName";
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams());
    std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext());
    HDC_SESSION session = (HDC_SESSION)0x12345678;
    auto transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
            new analysis::dvvp::transport::HDCTransport(session));
    auto uploader = std::make_shared<analysis::dvvp::transport::Uploader>(transport);


    Analysis::Dvvp::JobWrapper::ProcStatFileHandler memHandler(
            Analysis::Dvvp::JobWrapper::PROF_SYS_MEM, devId, bufSize,
            sampleIntervalMs, srcFileName, retFileName, params, jobCtx, uploader);

    MOCKER_CPP_VIRTUAL(
            (Analysis::Dvvp::JobWrapper::ProcTimerHandler*)&memHandler, &Analysis::Dvvp::JobWrapper::ProcStatFileHandler::Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, profSysStatJob->Process());
}

TEST_F(JOB_WRAPPER_PROF_SYSSTAT_JOB_TEST, Uninit) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::RunSocSide)
        .stubs()
        .will(returnValue(true));

    auto profSysStatJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfSysStatJob>();
    EXPECT_EQ(PROFILING_SUCCESS,profSysStatJob->Uninit());
}

class JOB_WRAPPER_PROF_ALLPIDS_JOB_TEST : public testing::Test {
protected:
    virtual void SetUp()
    {
        collectionJobCfg_ = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCfg>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
        std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext);
        auto comParams = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams>();
        comParams->params = params;
        comParams->jobCtx = jobCtx;
        comParams->jobCtx = std::make_shared<analysis::dvvp::message::JobContext>();
        collectionJobCfg_->comParams = comParams;
        collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
        collectionJobCfg_->jobParams.cores = std::make_shared<std::vector<int> >(0);
    }
    virtual void TearDown()
    {
        collectionJobCfg_.reset();
    }
private:
    std::shared_ptr<Analysis::Dvvp::JobWrapper::CollectionJobCfg> collectionJobCfg_;
};

TEST_F(JOB_WRAPPER_PROF_ALLPIDS_JOB_TEST, Init) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::RunSocSide)
        .stubs()
        .will(returnValue(true));
    auto profAllPidsJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfAllPidsJob>();
    profAllPidsJob->Init(nullptr);
    profAllPidsJob->Init(collectionJobCfg_);
    collectionJobCfg_->comParams->params->pid_profiling = "off";
    collectionJobCfg_->comParams->params->sys_profiling = "off";
    EXPECT_EQ(PROFILING_FAILED, profAllPidsJob->Init(collectionJobCfg_));
    EXPECT_TRUE(profAllPidsJob->IsGlobalJobLevel() == true);
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, profAllPidsJob->Init(collectionJobCfg_));
}

TEST_F(JOB_WRAPPER_PROF_ALLPIDS_JOB_TEST, Process) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::RunSocSide)
        .stubs()
        .will(returnValue(true));
    auto profAllPidsJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfAllPidsJob>();
    profAllPidsJob->Init(collectionJobCfg_);
    profAllPidsJob->collectionJobCfg_ = nullptr;
    EXPECT_EQ(PROFILING_FAILED, profAllPidsJob->Process());
    profAllPidsJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, profAllPidsJob->Process());
    unsigned int devId = 0;
    unsigned int sampleIntervalMs = 100;
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams());
    std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext());
    HDC_SESSION session = (HDC_SESSION)0x12345678;
    auto transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
            new analysis::dvvp::transport::HDCTransport(session));
    auto uploader = std::make_shared<analysis::dvvp::transport::Uploader>(transport);

    Analysis::Dvvp::JobWrapper::ProcAllPidsFileHandler allPidsHandler(PROF_SYS_ALL_PID, devId,
            sampleIntervalMs, params, jobCtx, uploader);

    MOCKER_CPP_VIRTUAL(&allPidsHandler, &Analysis::Dvvp::JobWrapper::ProcAllPidsFileHandler::Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, profAllPidsJob->Process());
}

TEST_F(JOB_WRAPPER_PROF_ALLPIDS_JOB_TEST, ProfTimerJobCommonInit) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::RunSocSide)
        .stubs()
        .will(returnValue(true));
    collectionJobCfg_->comParams->params->pid_profiling = "on";
    auto profAllPidsJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfAllPidsJob>();
    profAllPidsJob->Init(collectionJobCfg_);

    unsigned int devId = 0;
    unsigned int sampleIntervalMs = 100;
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams());
    std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext());
    HDC_SESSION session = (HDC_SESSION)0x12345678;
    auto transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
            new analysis::dvvp::transport::HDCTransport(session));
    auto uploader = std::make_shared<analysis::dvvp::transport::Uploader>(transport);

    Analysis::Dvvp::JobWrapper::ProcAllPidsFileHandler allPidsHandler(PROF_SYS_ALL_PID, devId,
            sampleIntervalMs, params, jobCtx, uploader);

    MOCKER_CPP(&analysis::dvvp::transport::UploaderMgr::GetUploader)
        .stubs()
        .will(returnValue(uploader));
    EXPECT_EQ(PROFILING_SUCCESS, profAllPidsJob->Process());
}

TEST_F(JOB_WRAPPER_PROF_ALLPIDS_JOB_TEST, Uninit) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::RunSocSide)
        .stubs()
        .will(returnValue(true));

    auto profAllPidsJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfAllPidsJob>();
    EXPECT_EQ(PROFILING_SUCCESS,profAllPidsJob->Uninit());
}

class JOB_WRAPPER_PROF_SYSMEM_JOB_TEST : public testing::Test {
protected:
    virtual void SetUp()
    {
        collectionJobCfg_ = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCfg>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
        std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext);
        auto comParams = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams>();
        comParams->params = params;
        comParams->jobCtx = jobCtx;
        comParams->jobCtx = std::make_shared<analysis::dvvp::message::JobContext>();
        collectionJobCfg_->comParams = comParams;
        collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
        collectionJobCfg_->jobParams.cores = std::make_shared<std::vector<int> >(0);
    }
    virtual void TearDown()
    {
        collectionJobCfg_.reset();
    }
private:
    std::shared_ptr<Analysis::Dvvp::JobWrapper::CollectionJobCfg> collectionJobCfg_;
};

TEST_F(JOB_WRAPPER_PROF_SYSMEM_JOB_TEST, Init) {
    GlobalMockObject::verify();
    auto profSysMemJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfSysMemJob>();
    EXPECT_EQ(PROFILING_FAILED, profSysMemJob->Init(nullptr));
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, profSysMemJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = false;
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::RunSocSide)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    EXPECT_EQ(PROFILING_FAILED, profSysMemJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->sys_profiling = "off";
    EXPECT_EQ(PROFILING_FAILED, profSysMemJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->sys_profiling = "on";
    collectionJobCfg_->comParams->params->msprof = "on";
    EXPECT_EQ(PROFILING_FAILED, profSysMemJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->msprof = "off";
    auto uploader = std::make_shared<analysis::dvvp::transport::Uploader>(nullptr);
    MOCKER_CPP(&analysis::dvvp::transport::UploaderMgr::GetUploader)
        .stubs()
        .with(any(), outBound(uploader));
    EXPECT_EQ(PROFILING_SUCCESS, profSysMemJob->Init(collectionJobCfg_));
    EXPECT_TRUE(profSysMemJob->IsGlobalJobLevel() == true);
}

TEST_F(JOB_WRAPPER_PROF_SYSMEM_JOB_TEST, Process) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::RunSocSide)
        .stubs()
        .will(returnValue(true));
    auto profSysMemJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfSysMemJob>();
    profSysMemJob->Init(collectionJobCfg_);
    profSysMemJob->collectionJobCfg_ = nullptr;
    EXPECT_EQ(PROFILING_FAILED, profSysMemJob->Process());
    profSysMemJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, profSysMemJob->Process());

    unsigned int devId = 0;
    unsigned int bufSize = 10;
    unsigned int sampleIntervalMs = 100;
    std::string srcFileName = "srcFileName";
    std::string retFileName = "retFileName";
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams());
    std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext());
    HDC_SESSION session = (HDC_SESSION)0x12345678;
    auto transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
            new analysis::dvvp::transport::HDCTransport(session));
    auto uploader = std::make_shared<analysis::dvvp::transport::Uploader>(transport);


    Analysis::Dvvp::JobWrapper::ProcMemFileHandler memHandler(
            Analysis::Dvvp::JobWrapper::PROF_SYS_MEM, devId, bufSize,
            sampleIntervalMs, srcFileName, retFileName, params, jobCtx, uploader);

    MOCKER_CPP_VIRTUAL((Analysis::Dvvp::JobWrapper::ProcTimerHandler*)&memHandler,
                       &Analysis::Dvvp::JobWrapper::ProcMemFileHandler::Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, profSysMemJob->Process());
}

TEST_F(JOB_WRAPPER_PROF_SYSMEM_JOB_TEST, Uninit) {
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Platform::Platform::RunSocSide)
        .stubs()
        .will(returnValue(true));

    auto profSysMemJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfSysMemJob>();
    EXPECT_EQ(PROFILING_SUCCESS,profSysMemJob->Uninit());
}

class JOB_WRAPPER_PROF_FMK_JOB_TEST : public testing::Test {
protected:
    virtual void SetUp()
    {
        collectionJobCfg_ = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCfg>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
        std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext);
        auto comParams = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams>();
        comParams->params = params;
        comParams->jobCtx = jobCtx;
        comParams->jobCtx = std::make_shared<analysis::dvvp::message::JobContext>();
        collectionJobCfg_->comParams = comParams;
        collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
        collectionJobCfg_->jobParams.cores = std::make_shared<std::vector<int> >(0);
    }
    virtual void TearDown()
    {
        collectionJobCfg_.reset();
    }
private:
    std::shared_ptr<Analysis::Dvvp::JobWrapper::CollectionJobCfg> collectionJobCfg_;
};

TEST_F(JOB_WRAPPER_PROF_FMK_JOB_TEST, Init) {
    GlobalMockObject::verify();
    auto proFmkJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfFmkJob>();
    EXPECT_EQ(PROFILING_FAILED, proFmkJob->Init(nullptr));
    EXPECT_EQ(PROFILING_SUCCESS, proFmkJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, proFmkJob->Init(collectionJobCfg_));
}

TEST_F(JOB_WRAPPER_PROF_FMK_JOB_TEST, Process) {
    GlobalMockObject::verify();
    auto proFmkJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfFmkJob>();
    proFmkJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, proFmkJob->Process());
    collectionJobCfg_->comParams->params->ts_fw_training = "on";
    proFmkJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, proFmkJob->Process());
}

TEST_F(JOB_WRAPPER_PROF_FMK_JOB_TEST, Uninit) {
    GlobalMockObject::verify();

    auto proFmkJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfFmkJob>();
    proFmkJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS,proFmkJob->Uninit());
    collectionJobCfg_->comParams->params->ts_fw_training = "on";
    proFmkJob->Init(collectionJobCfg_);
    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    EXPECT_EQ(PROFILING_SUCCESS,proFmkJob->Uninit());
    MOCKER_CPP(&analysis::dvvp::driver::DrvStop)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS,proFmkJob->Uninit());
    EXPECT_EQ(PROFILING_SUCCESS,proFmkJob->Uninit());
}

class JOB_WRAPPER_PROF_HWTS_JOB_TEST : public testing::Test {
protected:
    virtual void SetUp()
    {
        collectionJobCfg_ = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCfg>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
        std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext);
        auto comParams = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams>();
        comParams->params = params;
        comParams->jobCtx = jobCtx;
        comParams->jobCtx = std::make_shared<analysis::dvvp::message::JobContext>();
        collectionJobCfg_->comParams = comParams;
        collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
        collectionJobCfg_->jobParams.cores = std::make_shared<std::vector<int> >(0);
    }
    virtual void TearDown()
    {
        collectionJobCfg_.reset();
    }
private:
    std::shared_ptr<Analysis::Dvvp::JobWrapper::CollectionJobCfg> collectionJobCfg_;
};

TEST_F(JOB_WRAPPER_PROF_HWTS_JOB_TEST, Init) {
    GlobalMockObject::verify();
    auto profHwtsLogJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfHwtsLogJob>();
    EXPECT_EQ(PROFILING_FAILED, profHwtsLogJob->Init(nullptr));
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, profHwtsLogJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->hwts_log = "off";
    EXPECT_EQ(PROFILING_FAILED, profHwtsLogJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->hwts_log = "on";
    EXPECT_EQ(PROFILING_SUCCESS, profHwtsLogJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, profHwtsLogJob->Init(collectionJobCfg_));
}

TEST_F(JOB_WRAPPER_PROF_HWTS_JOB_TEST, Process) {
    GlobalMockObject::verify();
    auto profHwtsLogJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfHwtsLogJob>();
    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->hwts_log = "on";
    profHwtsLogJob->Init(collectionJobCfg_);
    profHwtsLogJob->collectionJobCfg_ = nullptr;
    EXPECT_EQ(PROFILING_FAILED, profHwtsLogJob->Process());
    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::driver::DrvHwtsLogStart)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    profHwtsLogJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, profHwtsLogJob->Process());

    EXPECT_EQ(PROFILING_FAILED, profHwtsLogJob->Process());

    EXPECT_EQ(PROFILING_SUCCESS, profHwtsLogJob->Process());
}

TEST_F(JOB_WRAPPER_PROF_HWTS_JOB_TEST, Uninit) {
    GlobalMockObject::verify();

    auto profHwtsLogJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfHwtsLogJob>();
    profHwtsLogJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS,profHwtsLogJob->Uninit());

    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->hwts_log = "on";
    profHwtsLogJob->Init(collectionJobCfg_);
    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::driver::DrvStop)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS,profHwtsLogJob->Uninit());
    EXPECT_EQ(PROFILING_SUCCESS,profHwtsLogJob->Uninit());
    EXPECT_EQ(PROFILING_SUCCESS,profHwtsLogJob->Uninit());
}

class JOB_WRAPPER_PROF_L2_CACHE_JOB_TEST : public testing::Test {
protected:
    virtual void SetUp()
    {
        collectionJobCfg_ = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCfg>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
        std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext);
        auto comParams = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams>();
        comParams->params = params;
        comParams->jobCtx = jobCtx;
        comParams->jobCtx = std::make_shared<analysis::dvvp::message::JobContext>();
        collectionJobCfg_->comParams = comParams;
        collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
    }
    virtual void TearDown()
    {
        collectionJobCfg_.reset();
    }
private:
    std::shared_ptr<Analysis::Dvvp::JobWrapper::CollectionJobCfg> collectionJobCfg_;
};

TEST_F(JOB_WRAPPER_PROF_L2_CACHE_JOB_TEST, Init) {
    GlobalMockObject::verify();
    auto profL2CacheJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfL2CacheTaskJob>();
    EXPECT_EQ(PROFILING_FAILED, profL2CacheJob->Init(nullptr));
    collectionJobCfg_->comParams->params->l2CacheTaskProfiling = "off";
    EXPECT_EQ(PROFILING_FAILED, profL2CacheJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->l2CacheTaskProfiling = "on";
    collectionJobCfg_->comParams->params->l2CacheTaskProfilingEvents = "0x5b, 0x59, 0xfa";
    EXPECT_EQ(PROFILING_FAILED, profL2CacheJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->l2CacheTaskProfilingEvents = "0x5b, 0x59, 0x5c";
    EXPECT_EQ(PROFILING_SUCCESS, profL2CacheJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, profL2CacheJob->Init(collectionJobCfg_));
}

TEST_F(JOB_WRAPPER_PROF_L2_CACHE_JOB_TEST, Process) {
    GlobalMockObject::verify();
    auto profL2CacheJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfL2CacheTaskJob>();
    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->l2CacheTaskProfiling = "on";
    collectionJobCfg_->comParams->params->l2CacheTaskProfilingEvents = "0x5b, 0x59, 0x5c";
    profL2CacheJob->Init(collectionJobCfg_);
    profL2CacheJob->collectionJobCfg_ = nullptr;
    EXPECT_EQ(PROFILING_FAILED, profL2CacheJob->Process());
    profL2CacheJob->Init(collectionJobCfg_);
    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::driver::DrvL2CacheTaskStart)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, profL2CacheJob->Process());
    EXPECT_EQ(PROFILING_FAILED, profL2CacheJob->Process());
    EXPECT_EQ(PROFILING_SUCCESS, profL2CacheJob->Process());
}

TEST_F(JOB_WRAPPER_PROF_L2_CACHE_JOB_TEST, Uninit) {
    GlobalMockObject::verify();
    auto profL2CacheJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfL2CacheTaskJob>();
    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->l2CacheTaskProfiling = "on";
    collectionJobCfg_->comParams->params->l2CacheTaskProfilingEvents = "0x5b, 0x59, 0x5c";
    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::driver::DrvStop)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    profL2CacheJob->Init(collectionJobCfg_);
    EXPECT_EQ(PROFILING_SUCCESS, profL2CacheJob->Uninit());
    EXPECT_EQ(PROFILING_SUCCESS, profL2CacheJob->Uninit());
    EXPECT_EQ(PROFILING_SUCCESS, profL2CacheJob->Uninit());
}

TEST_F(JOB_WRAPPER_PROF_L2_CACHE_JOB_TEST, TaskInit) {
    GlobalMockObject::verify();

    auto param_ = std::shared_ptr<analysis::dvvp::message::ProfileParams>(
            new analysis::dvvp::message::ProfileParams());
    std::vector<std::string> _devices;
     _devices.push_back("0");
    std::shared_ptr<analysis::dvvp::host::ProfTask> task(new analysis::dvvp::host::ProfTask(_devices, param_));

    MOCKER_CPP(&analysis::dvvp::transport::Uploader::Flush)
        .stubs();

    MOCKER(&MmCreateTaskWithThreadAttr)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    MOCKER(pthread_create)
        .stubs()
        .then(returnValue(-1));

    EXPECT_EQ(PROFILING_FAILED, task->Init());
    task->isInited_ = true;
}

class JOB_WRAPPER_PROF_AICPU_JOB_TEST : public testing::Test {
public:
    std::shared_ptr<Analysis::Dvvp::JobWrapper::CollectionJobCfg> collectionJobCfg_;
protected:
    virtual void SetUp()
    {
        collectionJobCfg_ = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCfg>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
        std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext);
        auto comParams = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams>();
        comParams->params = params;
        comParams->jobCtx = jobCtx;
        comParams->jobCtx = std::make_shared<analysis::dvvp::message::JobContext>();
        collectionJobCfg_->comParams = comParams;
        collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
    }
    virtual void TearDown()
    {
        collectionJobCfg_.reset();
    }
};

TEST_F(JOB_WRAPPER_PROF_AICPU_JOB_TEST, Init)
{
    GlobalMockObject::verify();
    auto profAicpuJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfAicpuJob>();
    EXPECT_EQ(PROFILING_FAILED, profAicpuJob->Init(nullptr));
    collectionJobCfg_->comParams->params->dataTypeConfig |= 0x00000000ULL;
    collectionJobCfg_->comParams->devId = 0;
    collectionJobCfg_->comParams->params->profLevel = MSVP_PROF_L1;
    EXPECT_EQ(PROFILING_FAILED, profAicpuJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->dataTypeConfig |= 0x00000008ULL;
    MOCKER(analysis::dvvp::driver::DrvGetApiVersion)
            .stubs()
            .will(returnValue(SUPPORT_OSC_FREQ_API_VERSION))
            .then(returnValue(SUPPORT_ADPROF_VERSION));
    collectionJobCfg_->comParams->params->host_profiling = false;
    EXPECT_EQ(PROFILING_FAILED, profAicpuJob->Init(collectionJobCfg_));
    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    MOCKER(&ProfDrvEvent::SubscribeEventThreadInit)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MOCKER(&analysis::dvvp::driver::DrvIsSupportAdprof)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_FAILED, profAicpuJob->Init(collectionJobCfg_));
    EXPECT_EQ(PROFILING_SUCCESS, profAicpuJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->host_profiling = true;
    EXPECT_EQ(PROFILING_FAILED, profAicpuJob->Init(collectionJobCfg_));
}

TEST_F(JOB_WRAPPER_PROF_AICPU_JOB_TEST, TsetCheckMC2SwitchShouldReturnTrueWhenMc2)
{
    GlobalMockObject::verify();
    auto profAicpuJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfAicpuJob>();
    collectionJobCfg_->comParams->params->dataTypeConfig = 0x00000000ULL;
    collectionJobCfg_->comParams->devId = 0;
    collectionJobCfg_->comParams->params->profLevel = MSVP_PROF_L1;
    MOCKER(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
        .stubs()
        .will(returnValue(Analysis::Dvvp::Common::Config::PlatformType::DC_TYPE));
    MOCKER(analysis::dvvp::driver::DrvGetApiVersion)
        .stubs()
        .will(returnValue(SUPPORT_ADPROF_VERSION));
    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(true));
    MOCKER(&analysis::dvvp::driver::DrvIsSupportAdprof)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_SUCCESS, profAicpuJob->Init(collectionJobCfg_));
    EXPECT_TRUE(profAicpuJob->CheckMC2Switch());
}

TEST_F(JOB_WRAPPER_PROF_AICPU_JOB_TEST, Init_dataTypeConfig_check)
{
    GlobalMockObject::verify();
    auto profAicpuJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfAicpuJob>();
    collectionJobCfg_->comParams->params->host_profiling = false;
    collectionJobCfg_->comParams->params->profLevel = MSVP_PROF_L1;

    MOCKER(analysis::dvvp::driver::DrvGetApiVersion)
            .stubs()
            .will(returnValue(SUPPORT_ADPROF_VERSION));
    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_FAILED, profAicpuJob->Init(collectionJobCfg_));
    collectionJobCfg_->comParams->params->dataTypeConfig |= PROF_AICPU_TRACE;
    MOCKER(&analysis::dvvp::driver::DrvIsSupportAdprof)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_SUCCESS, profAicpuJob->Init(collectionJobCfg_));
}

TEST_F(JOB_WRAPPER_PROF_AICPU_JOB_TEST, Process)
{
    GlobalMockObject::verify();
    auto profAicpuJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfAicpuJob>();
    collectionJobCfg_->comParams->params->dataTypeConfig |= PROF_AICPU_TRACE;
    collectionJobCfg_->comParams->params->profLevel = MSVP_PROF_L1;
    MOCKER(analysis::dvvp::driver::DrvGetApiVersion)
        .stubs()
        .will(returnValue(SUPPORT_ADPROF_VERSION));
    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(true));
    MOCKER(&analysis::dvvp::driver::DrvIsSupportAdprof)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_SUCCESS, profAicpuJob->Init(collectionJobCfg_));
    EXPECT_EQ(PROFILING_SUCCESS, profAicpuJob->Process());
    profAicpuJob->eventAttr_.isChannelValid = true;
    EXPECT_EQ(PROFILING_SUCCESS, profAicpuJob->Process());
    EXPECT_EQ(PROFILING_SUCCESS, profAicpuJob->Process());
}

TEST_F(JOB_WRAPPER_PROF_AICPU_JOB_TEST, Uninit)
{
    GlobalMockObject::verify();
    auto profAicpuJob = std::make_shared<Analysis::Dvvp::JobWrapper::ProfAicpuJob>();
    EXPECT_EQ(PROFILING_SUCCESS, profAicpuJob->Uninit());
    profAicpuJob->Init(collectionJobCfg_);
    profAicpuJob->eventAttr_.isThreadStart = true;
    MOCKER_CPP(&Collector::Dvvp::Mmpa::MmJoinTask)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::ChannelIsValid)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    MOCKER_CPP(&analysis::dvvp::driver::DrvChannelsMgr::GetAllChannels)
            .stubs()
            .will(returnValue(-1))
            .then(returnValue(0));
    MOCKER(&ProfDrvEvent::SubscribeEventThreadUninit).stubs();
    EXPECT_EQ(PROFILING_SUCCESS, profAicpuJob->Uninit());
    EXPECT_EQ(PROFILING_SUCCESS, profAicpuJob->Uninit());
}

class JOB_WRAPPER_PROF_PERF_EXTRA_JOB_TEST: public testing::Test {
protected:
    virtual void SetUp() {
        collectionJobCfg_ = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCfg>();
        std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
        std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext);
        auto comParams = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams>();
        comParams->params = params;
        comParams->jobCtx = jobCtx;
        collectionJobCfg_->comParams = comParams;
        collectionJobCfg_->jobParams.events = std::make_shared<std::vector<std::string> >(0);
        collectionJobCfg_->jobParams.cores = std::make_shared<std::vector<int> >(0);
    }
    virtual void TearDown() {
        collectionJobCfg_.reset();
    }
public:
    std::shared_ptr<Analysis::Dvvp::JobWrapper::CollectionJobCfg> collectionJobCfg_;
};

void fake_get_files_perf_extra(const std::string &dir, bool is_recur, std::vector<std::string>& files)
{
    files.push_back("ai_ctrl_cpu.data.1.task");
    files.push_back("ai_ctrl_cpu.data.2.task");
    files.push_back("ai_ctrl_cpu.data.3.task");
    files.push_back("ai_ctrl_cpu.data.4.task");
}

TEST_F(JOB_WRAPPER_PROF_PERF_EXTRA_JOB_TEST, PerfScriptTask) {
    GlobalMockObject::verify();

    MOCKER(analysis::dvvp::common::utils::Utils::GetFiles)
        .stubs()
        .will(invoke(fake_get_files_perf_extra));

    MOCKER_CPP(&Analysis::Dvvp::JobWrapper::PerfExtraTask::IsQuit)
        .stubs()
        .will(returnValue(true));

    MOCKER_CPP(&Analysis::Dvvp::JobWrapper::PerfExtraTask::ResolvePerfRecordData)
        .stubs();
    Analysis::Dvvp::JobWrapper::PerfExtraTask perfExtraTask(10, "./ret", collectionJobCfg_->comParams->jobCtx, collectionJobCfg_->comParams->params);
    perfExtraTask.PerfScriptTask();
}

TEST_F(JOB_WRAPPER_PROF_PERF_EXTRA_JOB_TEST, ResolvePerfRecordData) {
    GlobalMockObject::verify();

    MOCKER(analysis::dvvp::common::utils::Utils::ExecCmd)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    std::string fileName = "JOB_WRAPPER_PROF_PERF_EXTRA_JOB_TEST.ResolvePerfRecordData";
    Analysis::Dvvp::JobWrapper::PerfExtraTask perfExtraTask(10, "./ret", collectionJobCfg_->comParams->jobCtx, collectionJobCfg_->comParams->params);
    perfExtraTask.ResolvePerfRecordData(fileName);
}

class JOB_WRAPPER_PROF_NETDEV_STATS_JOB_TEST : public testing::Test {
public:
    std::shared_ptr<Analysis::Dvvp::JobWrapper::CollectionJobCfg> collectionJobCfg;
    int samplingInterval = 20;

protected:
    virtual void SetUp()
    {
        collectionJobCfg = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCfg>();
        auto params = std::make_shared<analysis::dvvp::message::ProfileParams>();
        auto jobCtx = std::make_shared<analysis::dvvp::message::JobContext>();
        auto comParams = std::make_shared<Analysis::Dvvp::JobWrapper::CollectionJobCommonParams>();
        comParams->params = params;
        comParams->jobCtx = jobCtx;
        collectionJobCfg->comParams = comParams;
        collectionJobCfg->jobParams.events = std::make_shared<std::vector<std::string>>(0);
    }

    virtual void TearDown()
    {
        collectionJobCfg.reset();
    }
};

TEST_F(JOB_WRAPPER_PROF_NETDEV_STATS_JOB_TEST, Init)
{
    GlobalMockObject::verify();

    auto netDevStatsJob = std::make_shared<Analysis::Dvvp::JobWrapper::NetDevStatsJob>();

    EXPECT_EQ(PROFILING_FAILED, netDevStatsJob->Init(nullptr));
    EXPECT_EQ(PROFILING_FAILED, netDevStatsJob->Init(collectionJobCfg));

    collectionJobCfg->comParams->params->io_profiling = analysis::dvvp::common::config::MSVP_PROF_ON;
    collectionJobCfg->comParams->params->io_sampling_interval = samplingInterval;
    EXPECT_EQ(PROFILING_SUCCESS, netDevStatsJob->Init(collectionJobCfg));
    EXPECT_EQ(PROFILING_SUCCESS, netDevStatsJob->Uninit());
}

TEST_F(JOB_WRAPPER_PROF_NETDEV_STATS_JOB_TEST, Process)
{
    GlobalMockObject::verify();

    MOCKER_CPP(&Collector::Dvvp::Plugin::DcmiPlugin::MsprofDcmiInit)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&Analysis::Dvvp::JobWrapper::NetDevStatsHandler::RegisterDevTask)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    collectionJobCfg->comParams->params->io_profiling = analysis::dvvp::common::config::MSVP_PROF_ON;
    collectionJobCfg->comParams->params->io_sampling_interval = samplingInterval;
    auto netDevStatsJob = std::make_shared<Analysis::Dvvp::JobWrapper::NetDevStatsJob>();

    uint32_t devId = DEFAULT_HOST_ID;
    collectionJobCfg->comParams->devId = devId;
    EXPECT_EQ(PROFILING_SUCCESS, netDevStatsJob->Init(collectionJobCfg));
    EXPECT_EQ(PROFILING_SUCCESS, netDevStatsJob->Process());

    devId = 0;
    collectionJobCfg->comParams->devId = devId;
    EXPECT_EQ(PROFILING_SUCCESS, netDevStatsJob->Init(collectionJobCfg));
    // DcmiInit failed
    EXPECT_EQ(PROFILING_FAILED, netDevStatsJob->Process());
    // RegisterDevTask failed
    EXPECT_EQ(PROFILING_FAILED, netDevStatsJob->Process());
    EXPECT_EQ(PROFILING_SUCCESS, netDevStatsJob->Process());
    EXPECT_EQ(PROFILING_SUCCESS, netDevStatsJob->Process());
    EXPECT_EQ(PROFILING_SUCCESS, netDevStatsJob->Uninit());
}

TEST_F(JOB_WRAPPER_PROF_NETDEV_STATS_JOB_TEST, Uninit)
{
    GlobalMockObject::verify();

    MOCKER_CPP(&Collector::Dvvp::Plugin::DcmiPlugin::MsprofDcmiInit)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&Analysis::Dvvp::JobWrapper::NetDevStatsHandler::RegisterDevTask)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&Analysis::Dvvp::JobWrapper::NetDevStatsHandler::RemoveDevTask)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    collectionJobCfg->comParams->params->io_profiling = analysis::dvvp::common::config::MSVP_PROF_ON;
    collectionJobCfg->comParams->params->io_sampling_interval = samplingInterval;
    auto netDevStatsJob = std::make_shared<Analysis::Dvvp::JobWrapper::NetDevStatsJob>();

    uint32_t devId = 0;
    collectionJobCfg->comParams->devId = devId;
    EXPECT_EQ(PROFILING_SUCCESS, netDevStatsJob->Init(collectionJobCfg));
    EXPECT_EQ(PROFILING_SUCCESS, netDevStatsJob->Uninit());

    EXPECT_EQ(PROFILING_SUCCESS, netDevStatsJob->Init(collectionJobCfg));
    EXPECT_EQ(PROFILING_SUCCESS, netDevStatsJob->Process());
    // RemoveDevTask failed
    EXPECT_EQ(PROFILING_FAILED, netDevStatsJob->Uninit());
    EXPECT_EQ(PROFILING_SUCCESS, netDevStatsJob->Uninit());
}
