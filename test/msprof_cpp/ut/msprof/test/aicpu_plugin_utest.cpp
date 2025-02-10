#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <iostream>
#include "errno/error_code.h"
#include "config/config.h"
#include "aicpu_plugin.h"
#include "message/codec.h"
#include "message/prof_params.h"
#include "proto/msprofiler.pb.h"
#include "msprof_callback_handler.h"
#include "param_validation.h"
#include "adx_prof_api.h"

using namespace Msprof::Engine;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;

class AicpuPluginUtest : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

TEST_F(AicpuPluginUtest, InitWillReturnFailWhenInputInvalidDeviceId)
{
    GlobalMockObject::verify();
    auto aicpu = std::make_shared<AicpuPlugin>();
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckDeviceIdIsValid)
        .stubs()
        .will(returnValue(false));
    EXPECT_EQ(PROFILING_FAILED, aicpu->Init(0));
}

TEST_F(AicpuPluginUtest, InitWillReturnFailWhenCreateServerFail)
{
    GlobalMockObject::verify();
    auto aicpu = std::make_shared<AicpuPlugin>();
    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckDeviceIdIsValid)
        .stubs()
        .will(returnValue(true));
    int server = 1;
    MOCKER_CPP(&Analysis::Dvvp::Adx::AdxHdcServerCreate)
        .stubs()
        .will(returnValue(static_cast<HDC_SERVER>(nullptr)))
        .then(returnValue(static_cast<HDC_SERVER>(&server)));
    MOCKER_CPP(&Analysis::Dvvp::Adx::AdxHdcServerDestroy)
        .stubs();
    EXPECT_EQ(PROFILING_SUCCESS, aicpu->Init(0));
    EXPECT_EQ(PROFILING_SUCCESS, aicpu->Init(0));
    aicpu->UnInit();
}

TEST_F(AicpuPluginUtest, ReceiveStreamData) {
    GlobalMockObject::verify();
    auto aicpu = std::make_shared<AicpuPlugin>();
 
    EXPECT_EQ(PROFILING_FAILED, aicpu->ReceiveStreamData(nullptr, 0));
 
    std::string message = "test";
    EXPECT_EQ(PROFILING_FAILED, aicpu->ReceiveStreamData(message.c_str(), 0));
    EXPECT_EQ(PROFILING_FAILED, aicpu->ReceiveStreamData(message.c_str(), PROFILING_PACKET_MAX_LEN + 1));
    EXPECT_EQ(PROFILING_FAILED, aicpu->ReceiveStreamData(message.c_str(), message.size()));
 
    std::shared_ptr<analysis::dvvp::proto::FileChunkReq> req(
        new analysis::dvvp::proto::FileChunkReq());
    std::string encode = analysis::dvvp::message::EncodeMessage(req); 
    EXPECT_EQ(PROFILING_FAILED, aicpu->ReceiveStreamData(encode.c_str(), encode.size()));
    req->set_islastchunk(true);
    encode = analysis::dvvp::message::EncodeMessage(req);
    EXPECT_EQ(PROFILING_SUCCESS, aicpu->ReceiveStreamData(encode.c_str(), encode.size()));
    
    req->set_islastchunk(false);
    analysis::dvvp::message::JobContext jobCtx;
    SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq;
    encode = analysis::dvvp::message::EncodeMessage(req);
    MOCKER_CPP(&analysis::dvvp::message::BaseInfo::FromString).stubs().will(returnValue(true));
    MOCKER_CPP(&Msprof::Engine::SendAiCpuData).stubs().will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, aicpu->ReceiveStreamData(encode.c_str(), encode.size()));
}