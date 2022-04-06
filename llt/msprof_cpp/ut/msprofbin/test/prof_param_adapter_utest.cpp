#include "mockcpp/mockcpp.hpp"
#include "gtest/gtest.h"
#include <iostream>
#include "errno/error_code.h"
#include "prof_params_adapter.h"
#include "message/codec.h"
#include "config/config.h"
#include "config/config_manager.h"

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::Msprof;

class PROF_PARAM_ADAPTER_UTEST : public testing::Test {
protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

TEST_F(PROF_PARAM_ADAPTER_UTEST, GenerateLlcEvents) {
    GlobalMockObject::verify();
    std::shared_ptr<Analysis::Dvvp::Msprof::ProfParamsAdapter> paramsAdapter(
        new Analysis::Dvvp::Msprof::ProfParamsAdapter);
    std::shared_ptr<analysis::dvvp::message::ProfileParams> srcParams(
            new analysis::dvvp::message::ProfileParams);

    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
            .stubs()
            .will(returnValue(0));
    srcParams->llc_profiling = "";
    srcParams->hardware_mem = "on";
    paramsAdapter->GenerateLlcEvents(nullptr);
    // empty events
    paramsAdapter->GenerateLlcEvents(srcParams);
    srcParams->llc_profiling = "capacity";
    paramsAdapter->GenerateLlcEvents(srcParams);
    srcParams->llc_profiling = "bandwidth";
    paramsAdapter->GenerateLlcEvents(srcParams);
    srcParams->llc_profiling = "read";                                                      
    paramsAdapter->GenerateLlcEvents(srcParams);
    srcParams->llc_profiling = "write";
    paramsAdapter->GenerateLlcEvents(srcParams);
    GlobalMockObject::verify();
    MOCKER_CPP(&Analysis::Dvvp::Common::Config::ConfigManager::GetPlatformType)
            .stubs()
            .will(returnValue(1));
    paramsAdapter->GenerateLlcEvents(srcParams);
}


TEST_F(PROF_PARAM_ADAPTER_UTEST, UpdateParams) {
    GlobalMockObject::verify();
    std::shared_ptr<Analysis::Dvvp::Msprof::ProfParamsAdapter> paramsAdapter(
        new Analysis::Dvvp::Msprof::ProfParamsAdapter);
    std::shared_ptr<analysis::dvvp::message::ProfileParams> srcParams(
            new analysis::dvvp::message::ProfileParams);

    srcParams->io_profiling = "on";
    srcParams->interconnection_profiling = "on";
    srcParams->hardware_mem = "on";
    srcParams->cpu_profiling = "on";
    EXPECT_EQ(PROFILING_FAILED, paramsAdapter->UpdateParams(nullptr));
    EXPECT_EQ(PROFILING_SUCCESS, paramsAdapter->UpdateParams(srcParams));

}