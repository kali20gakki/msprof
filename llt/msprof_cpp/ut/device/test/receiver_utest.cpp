#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <memory>
#include "securec.h"
#include "thread/thread.h"
#include "prof_msg_handler.h"
#include "message/codec.h"
#include "message/dispatcher.h"
#include "transport/hdc/hdc_transport.h"
#include "errno/error_code.h"
#include "prof_task.h"
#include "receiver.h"

using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::MsprofErrMgr;

class RECEIVER_TEST: public testing::Test {
protected:
    virtual void SetUp() {
        HDC_SESSION session = (HDC_SESSION)0x12345678;
        _transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
            new analysis::dvvp::transport::HDCTransport(session));
    }
    virtual void TearDown() {
        _transport.reset();
    }
public:
    std::shared_ptr<analysis::dvvp::transport::HDCTransport> _transport;
    MockObject<analysis::dvvp::transport::HDCTransport>mockerHDCTransport;
    MockObject<analysis::dvvp::message::MsgDispatcher>mockerMsgDispatcher;
};

TEST_F(RECEIVER_TEST, Receiver_destructor) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::device::Receiver> recv(
        new analysis::dvvp::device::Receiver(_transport));
    EXPECT_EQ(PROFILING_SUCCESS, recv->Uinit());
    recv.reset();
}

TEST_F(RECEIVER_TEST, Init) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::device::Receiver> recv(
        new analysis::dvvp::device::Receiver(_transport));

    EXPECT_EQ(PROFILING_SUCCESS, recv->Init(0));
}

TEST_F(RECEIVER_TEST, Uinit) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::device::Receiver> recv(
        new analysis::dvvp::device::Receiver(_transport));

    EXPECT_EQ(PROFILING_SUCCESS, recv->Uinit());
}

TEST_F(RECEIVER_TEST, GetTransport) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::device::Receiver> recv(
        new analysis::dvvp::device::Receiver(_transport));

    EXPECT_EQ(_transport.get(), recv->GetTransport().get());
}

TEST_F(RECEIVER_TEST, run) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::device::Receiver> recv(
        new analysis::dvvp::device::Receiver(_transport));

     //before init
    auto errorContext = MsprofErrorManager::instance()->GetErrorManagerContext();
    recv->Run(errorContext);
    EXPECT_EQ(PROFILING_SUCCESS, recv->Init(0));

    std::shared_ptr<analysis::dvvp::proto::JobStartReq> message(new analysis::dvvp::proto::JobStartReq);
    std::string buffer = analysis::dvvp::message::EncodeMessage(message);

    int length = (int)(sizeof(struct tlv_req) + buffer.size());

    std::shared_ptr<char> req_buffer(new char[length], 
                                std::default_delete<char[]>());

    struct tlv_req * req = (struct tlv_req *)req_buffer.get();
    req->len = (int)buffer.size();
    memcpy_s(req->value, req->len, buffer.c_str(), buffer.size());

    MOCK_METHOD(mockerHDCTransport, RecvPacket)
        .stubs()
        .with(outBoundP(&req))
        .will(returnValue(-1))
        .then(returnValue(length));

    MOCK_METHOD(mockerHDCTransport, DestroyPacket)
        .stubs();

    MOCK_METHOD(mockerMsgDispatcher, OnNewMessage)
        .stubs();
    recv->Run(errorContext);
    EXPECT_EQ(PROFILING_SUCCESS, recv->Stop());
    recv->Run(errorContext);
}