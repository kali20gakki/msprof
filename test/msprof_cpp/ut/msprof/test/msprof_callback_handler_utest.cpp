#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <string.h>
#include <google/protobuf/util/json_util.h>
#include "config/config_manager.h"
#include "errno/error_code.h"
#include "message/codec.h"
#include "msprof_callback_handler.h"
#include "msprof_callback_impl.h"
#include "prof_acl_mgr.h"
#include "proto/profiler.pb.h"
#include "uploader_mgr.h"
#include "utils/utils.h"

using namespace analysis::dvvp::common::error;
using namespace Msprof::Engine;

class MSPROF_CALLBACK_HANDLER_UTEST : public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(MSPROF_CALLBACK_HANDLER_UTEST, HandleMsprofRequestTest) {
    GlobalMockObject::verify();
    MsprofCallbackHandler handler("Framework");

    MOCKER_CPP(&MsprofCallbackHandler::ReportData)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, handler.HandleMsprofRequest(MSPROF_REPORTER_REPORT, nullptr, 0));
    int data = 0;
    EXPECT_EQ(PROFILING_SUCCESS, handler.HandleMsprofRequest(MSPROF_REPORTER_REPORT, (void *)&data, 0));

    MOCKER_CPP(&MsprofCallbackHandler::StartReporter)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, handler.HandleMsprofRequest(MSPROF_REPORTER_INIT, nullptr, 0));

    MOCKER_CPP(&MsprofCallbackHandler::StopReporter)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, handler.HandleMsprofRequest(MSPROF_REPORTER_UNINIT, nullptr, 0));

    MOCKER_CPP(&MsprofCallbackHandler::GetDataMaxLen)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, handler.HandleMsprofRequest(MSPROF_REPORTER_DATA_MAX_LEN, nullptr, 0));

    MOCKER_CPP(&MsprofCallbackHandler::GetHashId)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, handler.HandleMsprofRequest(MSPROF_REPORTER_HASH, nullptr, 0));

    uint32_t invalidType = 9999;
    EXPECT_EQ(PROFILING_FAILED, handler.HandleMsprofRequest(invalidType, nullptr, 0));
}

TEST_F(MSPROF_CALLBACK_HANDLER_UTEST, SendDataTest) {
    GlobalMockObject::verify();
    MsprofCallbackHandler handler("Framework");

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::IsInited)
        .stubs()
        .will(returnValue(true));

    MOCKER_CPP(&MsprofCallbackHandler::HandleMsprofRequest)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunk(new analysis::dvvp::proto::FileChunkReq());
    EXPECT_EQ(PROFILING_FAILED, handler.SendData(fileChunk));

    handler.StartReporter();
    EXPECT_EQ(PROFILING_SUCCESS, handler.SendData(fileChunk));
}

TEST_F(MSPROF_CALLBACK_HANDLER_UTEST, ReportDataTest) {
    GlobalMockObject::verify();
    MsprofCallbackHandler handler("Framework");

    EXPECT_EQ(PROFILING_FAILED, handler.ReportData(nullptr, 0));
}

TEST_F(MSPROF_CALLBACK_HANDLER_UTEST, GetHashIdTest) {
    GlobalMockObject::verify();
    MsprofCallbackHandler handler("Framework");
    MsprofHashData hd;

    EXPECT_EQ(PROFILING_FAILED, handler.GetHashId(nullptr, 0));
    handler.StartReporter();
    EXPECT_EQ(PROFILING_FAILED, handler.GetHashId(nullptr, 0));
    EXPECT_EQ(PROFILING_FAILED, handler.GetHashId(&hd, 0));
    EXPECT_EQ(PROFILING_FAILED, handler.GetHashId(&hd, sizeof(hd)));
}

TEST_F(MSPROF_CALLBACK_HANDLER_UTEST, FlushDataTest) {
    GlobalMockObject::verify();
    MsprofCallbackHandler handler("Framework");

    EXPECT_EQ(PROFILING_FAILED, handler.FlushData());

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::IsInited)
        .stubs()
        .will(returnValue(true));
    handler.StartReporter();
    EXPECT_EQ(PROFILING_SUCCESS, handler.FlushData());
}

TEST_F(MSPROF_CALLBACK_HANDLER_UTEST, StartReporterTest) {
    GlobalMockObject::verify();
    MsprofCallbackHandler handler("");

    EXPECT_EQ(PROFILING_FAILED, handler.StartReporter());

    handler.module_ = "Framework";
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::IsInited)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    EXPECT_EQ(PROFILING_FAILED, handler.StartReporter());
    EXPECT_EQ(PROFILING_SUCCESS, handler.StartReporter());
}

TEST_F(MSPROF_CALLBACK_HANDLER_UTEST, StopReporterTest) {
    GlobalMockObject::verify();
    MsprofCallbackHandler handler("Framework");

    EXPECT_EQ(PROFILING_SUCCESS, handler.StopReporter());

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::IsInited)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_SUCCESS, handler.StartReporter());
    EXPECT_EQ(PROFILING_SUCCESS, handler.StopReporter());
}

TEST_F(MSPROF_CALLBACK_HANDLER_UTEST, OtherTest) {
    GlobalMockObject::verify();
    FlushModule("0");
    SendAiCpuData(nullptr);
}
