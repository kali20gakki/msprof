#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <iostream>
#include "errno/error_code.h"
#include "aicpu_plugin.h"
#include "message/codec.h"
#include "message/prof_params.h"
#include "proto/msprofiler.pb.h"
#include "msprof_callback_handler.h"

using namespace Msprof::Engine;
using namespace analysis::dvvp::common::error;

class AICPU_PLUGIN_UTEST : public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(AICPU_PLUGIN_UTEST, ReceiveStreamData) {
    GlobalMockObject::verify();
    auto aicpu = std::make_shared<AicpuPlugin>();
    
    EXPECT_EQ(PROFILING_FAILED, aicpu->ReceiveStreamData(nullptr, 0));

    std::string message = "test";
    EXPECT_EQ(PROFILING_FAILED, aicpu->ReceiveStreamData(message.c_str(), message.size()));

    std::shared_ptr<analysis::dvvp::proto::FileChunkReq> req(
        new analysis::dvvp::proto::FileChunkReq());

    std::string encode = analysis::dvvp::message::EncodeMessage(req); 
    EXPECT_EQ(PROFILING_FAILED, aicpu->ReceiveStreamData(encode.c_str(), encode.size()));
    req->set_islastchunk(true);
    encode = analysis::dvvp::message::EncodeMessage(req);
    EXPECT_EQ(PROFILING_SUCCESS, aicpu->ReceiveStreamData(encode.c_str(), encode.size()));
    analysis::dvvp::message::JobContext jobCtx;
    req->set_islastchunk(false);
    req->mutable_hdr()->set_job_ctx(jobCtx.ToString());
    encode = analysis::dvvp::message::EncodeMessage(req);
}