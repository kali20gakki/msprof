#include <iostream>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "proto/profiler.pb.h"
#include "message/dispatcher.h"

class MESSAGE_DISPATCHER_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(MESSAGE_DISPATCHER_TEST, IMsgHandler_OnNewMessage) {
    GlobalMockObject::verify();

    MockObject<analysis::dvvp::message::IMsgHandler> handler;
    MOCK_METHOD(handler, OnNewMessage)
        .stubs();

    std::shared_ptr<analysis::dvvp::proto::JobStartReq> message(
        new analysis::dvvp::proto::JobStartReq);
    EXPECT_NE(nullptr, message);

    handler->OnNewMessage(message);
}

class FakeJobStartHandler : public analysis::dvvp::message::IMsgHandler {
public:
    FakeJobStartHandler() {}
    virtual ~FakeJobStartHandler() {}

public:
    virtual void OnNewMessage(std::shared_ptr<google::protobuf::Message> message) {
        std::cout << "handle fake job start" << std::endl;
    }
};

TEST_F(MESSAGE_DISPATCHER_TEST, RegisterMessageHandler) {
    GlobalMockObject::verify();
    std::shared_ptr<analysis::dvvp::message::MsgDispatcher> disp(
        new analysis::dvvp::message::MsgDispatcher());
    EXPECT_NE(nullptr, disp);

    //find handler
    std::shared_ptr<analysis::dvvp::message::IMsgHandler> handler(
        new FakeJobStartHandler());
    disp->RegisterMessageHandler<analysis::dvvp::proto::JobStartReq>(
        handler);
}

TEST_F(MESSAGE_DISPATCHER_TEST, OnNewMessage) {
    GlobalMockObject::verify();
    std::shared_ptr<analysis::dvvp::message::MsgDispatcher> disp(
        new analysis::dvvp::message::MsgDispatcher());
    EXPECT_NE(nullptr, disp);

    //null param
    disp->OnNewMessage(nullptr);

    //not register handler
    std::shared_ptr<analysis::dvvp::proto::JobStartReq> message(
        new analysis::dvvp::proto::JobStartReq);
    disp->OnNewMessage(message);

    //register handler
    std::shared_ptr<analysis::dvvp::message::IMsgHandler> handler(
        new FakeJobStartHandler());
    disp->RegisterMessageHandler<analysis::dvvp::proto::JobStartReq>(
        handler);
    disp->OnNewMessage(message);
}