#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <fstream>
#include <iostream>
#include <sys/inotify.h>
#include "prof_host_core.h"
#include "proto/profiler.pb.h"
#include <google/protobuf/util/json_util.h>
#include "utils/utils.h"
#include "message/codec.h"
#include "message/prof_params.h"
#include "prof_inotify.h"
#include "prof_manager.h"
#include "msprof_dlog.h"
#include "prof_params_adapter.h"
#include "config/config_manager.h"


#ifndef false
#define false                     0
#endif

#ifndef true
#define true                      1
#endif

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::host;

class PARAMS_ADAPTER_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(PARAMS_ADAPTER_TEST, Init) {
    GlobalMockObject::verify();
    std::shared_ptr<Analysis::Dvvp::Host::Adapter::ProfParamsAdapter> paramsAdapter(
        new Analysis::Dvvp::Host::Adapter::ProfParamsAdapter);
    
    const std::string profPath("./profile.cfg");
    std::string cfg ="eventsType={\"aicoreMetrics\": [{\"type\":\"aicoreArithmeticThroughput\", \"events\":\"0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f\"}, {\"type\":\"aicorePipeline\", \"events\":\"0x8,0xa,0x9,0xb,0xc,0xd,0x55,0x54\"}, {\"type\":\"aicoreSynchronization\", \"events\":\"0x57,0x58,0x59,0x5a,0x5b,0x5c\"}, {\"type\":\"aicoreMemoryBandwidth\", \"events\":\"0x15,0x16,0x31,0x32,0xf,0x10,0x12,0x13\"}, {\"type\":\"aicoreInternalMemoryBandwidth\", \"events\":\"0x3a,0x3b,0x1b,0x1c,0x21,0x22,0x27,0x28\"}, {\"type\":\"aicorePipelineStall\", \"events\":\"0x64,0x65,0x66,0x6b,0x6c,0x6d,0x6e,0x6f\"}, {\"type\":\"aicoreMetricsAll\", \"events\":\"\"}]}";
    
    std::ofstream ifs(profPath);
    ifs << cfg;
    ifs.close();
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetProfCfgPath)
        .stubs()
        .will(returnValue(profPath));
    EXPECT_EQ(PROFILING_SUCCESS, paramsAdapter->Init());
    ::remove(profPath.c_str());
}

TEST_F(PARAMS_ADAPTER_TEST, GetaicoreEvents) {
    GlobalMockObject::verify();
    std::shared_ptr<Analysis::Dvvp::Host::Adapter::ProfParamsAdapter> paramsAdapter(
        new Analysis::Dvvp::Host::Adapter::ProfParamsAdapter);

    std::string aicore = paramsAdapter->GetaicoreEvents("");

    EXPECT_TRUE(aicore.empty());
}

TEST_F(PARAMS_ADAPTER_TEST, HandleSystemTraceConf) {
    GlobalMockObject::verify();
    std::shared_ptr<Analysis::Dvvp::Host::Adapter::ProfParamsAdapter> paramsAdapter(
        new Analysis::Dvvp::Host::Adapter::ProfParamsAdapter);
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);

    EXPECT_EQ(PROFILING_FAILED, paramsAdapter->HandleSystemTraceConf("", nullptr));
    EXPECT_EQ(PROFILING_FAILED, paramsAdapter->HandleSystemTraceConf("test", params));
    std::shared_ptr<analysis::dvvp::proto::ProfilerConf> conf =
        std::make_shared<analysis::dvvp::proto::ProfilerConf>();
    conf->set_cpusamplinginterval(100);
    conf->set_syssamplinginterval(100);
    conf->set_appsamplinginterval(100);
    conf->set_hardwarememsamplinginterval(100);
    conf->set_iosamplinginterval(100);
    conf->set_interconnectionsamplinginterval(100);
    conf->set_dvppsamplinginterval(100);
    conf->set_aicoresamplinginterval(100);
    conf->set_aicoremetrics("test");
    conf->set_aivsamplinginterval(100);
    conf->set_aivmetrics("test");
    std::string buffer = analysis::dvvp::message::EncodeJson(conf);
    EXPECT_EQ(PROFILING_SUCCESS, paramsAdapter->HandleSystemTraceConf(buffer, params));
}

TEST_F(PARAMS_ADAPTER_TEST, HandleTaskTraceConf) {
    GlobalMockObject::verify();
    std::shared_ptr<Analysis::Dvvp::Host::Adapter::ProfParamsAdapter> paramsAdapter(
        new Analysis::Dvvp::Host::Adapter::ProfParamsAdapter);
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
            new analysis::dvvp::message::ProfileParams);
    params->ai_core_profiling = "on";

    EXPECT_EQ(PROFILING_FAILED, paramsAdapter->HandleTaskTraceConf("", nullptr));
    EXPECT_EQ(PROFILING_FAILED, paramsAdapter->HandleTaskTraceConf("test", params));
    paramsAdapter->aicoreEvents_["test"] = "0x00";
    std::shared_ptr<analysis::dvvp::proto::ProfilerConf> conf =
        std::make_shared<analysis::dvvp::proto::ProfilerConf>();
    conf->set_aicoremetrics("test");
    std::string buffer = analysis::dvvp::message::EncodeJson(conf);
    EXPECT_EQ(PROFILING_SUCCESS, paramsAdapter->HandleTaskTraceConf(buffer, params));
    conf->set_aicoremetrics("invalid");
    buffer = analysis::dvvp::message::EncodeJson(conf);
    EXPECT_EQ(PROFILING_SUCCESS, paramsAdapter->HandleTaskTraceConf(buffer, params));
    EXPECT_EQ("off", params->ai_core_profiling);
}

TEST_F(PARAMS_ADAPTER_TEST, UpdateHardwareMemParams) {
    GlobalMockObject::verify();
    std::shared_ptr<Analysis::Dvvp::Host::Adapter::ProfParamsAdapter> paramsAdapter(
        new Analysis::Dvvp::Host::Adapter::ProfParamsAdapter);
    std::shared_ptr<analysis::dvvp::message::ProfileParams> srcParams(
            new analysis::dvvp::message::ProfileParams);
    std::shared_ptr<analysis::dvvp::message::ProfileParams> dstParams(
            new analysis::dvvp::message::ProfileParams);

    dstParams->hardware_mem = "on";
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
            .stubs()
            .will(returnValue(1));
    paramsAdapter->UpdateHardwareMemParams(dstParams, srcParams);
    EXPECT_STREQ("on", dstParams->llc_profiling.c_str());
}

TEST_F(PARAMS_ADAPTER_TEST, SetSystemTraceParams) {
    GlobalMockObject::verify();
    std::shared_ptr<Analysis::Dvvp::Host::Adapter::ProfParamsAdapter> paramsAdapter(
        new Analysis::Dvvp::Host::Adapter::ProfParamsAdapter);
    std::shared_ptr<analysis::dvvp::message::ProfileParams> srcParams(
            new analysis::dvvp::message::ProfileParams);
    std::shared_ptr<analysis::dvvp::message::ProfileParams> dstParams(
            new analysis::dvvp::message::ProfileParams);

    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
            .stubs()
            .will(returnValue(1));
    srcParams->llc_profiling = "off";
    paramsAdapter->SetSystemTraceParams(nullptr, srcParams);
    EXPECT_STRNE("on", srcParams->llc_profiling.c_str());

    paramsAdapter->SetSystemTraceParams(dstParams, srcParams);
    EXPECT_STRNE("system_trace", srcParams->profiling_options.c_str());

    dstParams->cpu_profiling = "on";
    dstParams->io_profiling = "on";
    dstParams->interconnection_profiling = "on";
    dstParams->ai_core_profiling = "on";
    dstParams->aiv_profiling = "on";
    srcParams->profiling_options = "system_trace";
    paramsAdapter->SetSystemTraceParams(dstParams, srcParams);
    EXPECT_STREQ("on", dstParams->pcieProfiling.c_str());
}

TEST_F(PARAMS_ADAPTER_TEST, GenerateLlcEvents) {
    GlobalMockObject::verify();
    std::shared_ptr<Analysis::Dvvp::Host::Adapter::ProfParamsAdapter> paramsAdapter(
        new Analysis::Dvvp::Host::Adapter::ProfParamsAdapter);
    std::shared_ptr<analysis::dvvp::message::ProfileParams> srcParams(
            new analysis::dvvp::message::ProfileParams);

    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
            .stubs()
            .will(returnValue(0));
    srcParams->llc_profiling = "";
    paramsAdapter->GenerateLlcEvents(nullptr);
    // empty events
    paramsAdapter->GenerateLlcEvents(srcParams);
    srcParams->llc_profiling = "capacity";
    paramsAdapter->GenerateLlcEvents(srcParams);
    srcParams->llc_profiling = "bandwidth";
    paramsAdapter->GenerateLlcEvents(srcParams);
    srcParams->llc_profiling = "read";
    paramsAdapter->GenerateLlcEvents(srcParams);
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
            .stubs()
            .will(returnValue(1));
    paramsAdapter->GenerateLlcEvents(srcParams);
}


TEST_F(PARAMS_ADAPTER_TEST, UpdateOpFeature) {
    GlobalMockObject::verify();
    std::shared_ptr<Analysis::Dvvp::Host::Adapter::ProfParamsAdapter> paramsAdapter(
            new Analysis::Dvvp::Host::Adapter::ProfParamsAdapter);
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(new analysis::dvvp::message::ProfileParams);
    std::shared_ptr<analysis::dvvp::proto::MsProfStartReq> feature(new analysis::dvvp::proto::MsProfStartReq);
    std::string aicEvents = "0x00";
    MOCKER_CPP(&Analysis::Dvvp::Host::Adapter::ProfParamsAdapter::GetaicoreEvents)
        .stubs()
        .will(returnValue(aicEvents));
    // l2 events invalid
    feature->set_l2_cache_events(",,,,,,,,,,");
    paramsAdapter->UpdateOpFeature(feature, params);
    EXPECT_FALSE(params->ai_core_profiling == "on");
    EXPECT_FALSE(params->l2CacheTaskProfiling == "on");

    feature->set_l2_cache_events("0x0");
    // ok
    paramsAdapter->UpdateOpFeature(feature, params);
    EXPECT_TRUE(params->ai_core_profiling == "on");
    EXPECT_TRUE(params->l2CacheTaskProfiling == "on");
}

TEST_F(PARAMS_ADAPTER_TEST, MsprofAdapter) {
    GlobalMockObject::verify();
    std::shared_ptr<Analysis::Dvvp::Host::Adapter::ProfParamsAdapter> paramsAdapter(
            new Analysis::Dvvp::Host::Adapter::ProfParamsAdapter);
    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(new analysis::dvvp::message::ProfileParams);
    std::shared_ptr<analysis::dvvp::proto::MsProfStartReq> feature(new analysis::dvvp::proto::MsProfStartReq);
    params->msprof = "on";
    params->hbmProfiling = "on";
    params->pcieProfiling = "on";
    paramsAdapter->MsprofAdapter(params);
    paramsAdapter->MsprofAdapter(params);
}