#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <google/protobuf/util/json_util.h>
#include "config/config_manager.h"
#include "errno/error_code.h"
#include "message/codec.h"
#include "msprof_callback_handler.h"
#include "msprof_callback_impl.h"
#include "prof_acl_mgr.h"
#include "proto/msprofiler.pb.h"
#include "uploader_mgr.h"
#include "utils/utils.h"
#include "dyn_prof_server.h"
#include "dyn_prof_client.h"
#include "dyn_prof_mgr.h"

using namespace analysis::dvvp::common::error;
using namespace Msprof::Engine;
using namespace Collector::Dvvp::DynProf;

class MsprofCallbackHandlerUtest : public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(MsprofCallbackHandlerUtest, HandleMsprofRequestTest) {
    GlobalMockObject::verify();
    MsprofCallbackHandler handler("Framework");

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

TEST_F(MsprofCallbackHandlerUtest, SendDataTest) {
    GlobalMockObject::verify();
    MsprofCallbackHandler handler("Framework");

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::IsInited)
        .stubs()
        .will(returnValue(true));

    MOCKER_CPP(&MsprofCallbackHandler::HandleMsprofRequest)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunk(new analysis::dvvp::ProfileFileChunk());
    EXPECT_EQ(PROFILING_FAILED, handler.SendData(fileChunk));

    handler.StartReporter();
    EXPECT_EQ(PROFILING_SUCCESS, handler.SendData(fileChunk));
}

TEST_F(MsprofCallbackHandlerUtest, ReportDataTest) {
    GlobalMockObject::verify();
    MsprofCallbackHandler handler("Framework");

    EXPECT_EQ(PROFILING_FAILED, handler.ReportData(nullptr, 0));
}

TEST_F(MsprofCallbackHandlerUtest, ReportApiDataTest)
{
    GlobalMockObject::verify();
    MsprofCallbackHandler handler("unaging.api_event");
 
    MsprofApi data;
    EXPECT_EQ(PROFILING_FAILED, handler.ReportData(data));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::IsInited)
        .stubs()
        .will(returnValue(true));
    handler.StartReporter();
    EXPECT_EQ(PROFILING_SUCCESS, handler.ReportData(data));
    handler.StopReporter();
}
 
TEST_F(MsprofCallbackHandlerUtest, ReportCompactDataTest)
{
    GlobalMockObject::verify();
    MsprofCallbackHandler handler("unaging.compact");
 
    MsprofCompactInfo data;
    EXPECT_EQ(PROFILING_FAILED, handler.ReportData(data));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::IsInited)
        .stubs()
        .will(returnValue(true));
    handler.StartReporter();
    EXPECT_EQ(PROFILING_SUCCESS, handler.ReportData(data));
    handler.StopReporter();
}
 
TEST_F(MsprofCallbackHandlerUtest, ReportAdditonalDataTest)
{
    GlobalMockObject::verify();
    MsprofCallbackHandler handler("unaging.additional");
 
    MsprofAdditionalInfo data;
    EXPECT_EQ(PROFILING_FAILED, handler.ReportData(data));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::IsInited)
        .stubs()
        .will(returnValue(true));
    handler.StartReporter();
    EXPECT_EQ(PROFILING_SUCCESS, handler.ReportData(data));
    handler.StopReporter();
}

TEST_F(MsprofCallbackHandlerUtest, GetHashIdTest) {
    GlobalMockObject::verify();
    MsprofCallbackHandler handler("Framework");
    MsprofHashData hd;

    EXPECT_EQ(PROFILING_FAILED, handler.GetHashId(nullptr, 0));
    handler.StartReporter();
    EXPECT_EQ(PROFILING_FAILED, handler.GetHashId(nullptr, 0));
    EXPECT_EQ(PROFILING_FAILED, handler.GetHashId(&hd, 0));
    EXPECT_EQ(PROFILING_FAILED, handler.GetHashId(&hd, sizeof(hd)));
}

TEST_F(MsprofCallbackHandlerUtest, FlushDataTest) {
    GlobalMockObject::verify();
    MsprofCallbackHandler handler("Framework");

    EXPECT_EQ(PROFILING_FAILED, handler.FlushData());

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::IsInited)
        .stubs()
        .will(returnValue(true));
    handler.StartReporter();
    EXPECT_EQ(PROFILING_SUCCESS, handler.FlushData());
}

TEST_F(MsprofCallbackHandlerUtest, StartReporterTest) {
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

TEST_F(MsprofCallbackHandlerUtest, StopReporterTest) {
    GlobalMockObject::verify();
    MsprofCallbackHandler handler("Framework");

    EXPECT_EQ(PROFILING_SUCCESS, handler.StopReporter());

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::IsInited)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_SUCCESS, handler.StartReporter());
    EXPECT_EQ(PROFILING_SUCCESS, handler.StopReporter());
}

TEST_F(MsprofCallbackHandlerUtest, OtherTest) {
    GlobalMockObject::verify();
    FlushModule("0");
    SendAiCpuData(nullptr);
}

class DynProfClientUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

size_t RecvReturnNotDissconMsgStub(int fd, VOID_PTR msg, size_t msgSize, int flags)
{
    auto tmpMsg = reinterpret_cast<DynProfRspMsg *>(msg);
    tmpMsg->msgType = DynProfMsgType::START_REQ;
    return sizeof(DynProfRspMsg);
}

size_t RecvReturnInvalidMsgDataLenStub(int fd, VOID_PTR msg, size_t msgSize, int flags)
{
    auto tmpMsg = reinterpret_cast<DynProfRspMsg *>(msg);
    tmpMsg->msgType = DynProfMsgType::DISCONN_RSP;
    tmpMsg->msgDataLen = DYN_PROF_RSP_MSG_MAX_LEN;
    return sizeof(DynProfRspMsg);
}

size_t RecvReturnMsgLenNotEqualToMsgDataStub(int fd, VOID_PTR msg, size_t msgSize, int flags)
{
    auto tmpMsg = reinterpret_cast<DynProfRspMsg *>(msg);
    tmpMsg->msgType = DynProfMsgType::DISCONN_RSP;
    tmpMsg->msgDataLen = 1;
    (void)memset_s(tmpMsg->msgData, DYN_PROF_RSP_MSG_MAX_LEN, 0, DYN_PROF_RSP_MSG_MAX_LEN);
    return sizeof(DynProfRspMsg);
}

size_t RecvReturnMsgTypeForSendMsgToServerStub(int fd, VOID_PTR msg, size_t msgSize, int flags)
{
    auto tmpMsg = reinterpret_cast<DynProfRspMsg *>(msg);
    tmpMsg->msgType = DynProfMsgType::DISCONN_RSP;
    tmpMsg->msgDataLen = 0;
    (void)memset_s(tmpMsg->msgData, DYN_PROF_RSP_MSG_MAX_LEN, 0, DYN_PROF_RSP_MSG_MAX_LEN);
    return sizeof(DynProfRspMsg);
}

size_t RecvReturnMsgStatusCodeForSendMsgToServerStub(int fd, VOID_PTR msg, size_t msgSize, int flags)
{
    auto tmpMsg = reinterpret_cast<DynProfRspMsg *>(msg);
    tmpMsg->msgType = DynProfMsgType::START_RSP;
    tmpMsg->statusCode = static_cast<DynProfMsgProcRes>(2); // 2 invalid status code
    tmpMsg->msgDataLen = 0;
    (void)memset_s(tmpMsg->msgData, DYN_PROF_RSP_MSG_MAX_LEN, 0, DYN_PROF_RSP_MSG_MAX_LEN);
    return sizeof(DynProfRspMsg);
}

size_t RecvReturnMsgDataLenForSendMsgToServerStub(int fd, VOID_PTR msg, size_t msgSize, int flags)
{
    auto tmpMsg = reinterpret_cast<DynProfRspMsg *>(msg);
    tmpMsg->msgType = DynProfMsgType::START_RSP;
    tmpMsg->msgDataLen = DYN_PROF_RSP_MSG_MAX_LEN;
    (void)memset_s(tmpMsg->msgData, DYN_PROF_RSP_MSG_MAX_LEN, 0, DYN_PROF_RSP_MSG_MAX_LEN);
    return sizeof(DynProfRspMsg);
}

size_t RecvReturnMsgSatusCodeNotExeSuccForSendMsgToServerStub(int fd, VOID_PTR msg, size_t msgSize, int flags)
{
    auto tmpMsg = reinterpret_cast<DynProfRspMsg *>(msg);
    tmpMsg->msgType = DynProfMsgType::START_RSP;
    tmpMsg->statusCode = DynProfMsgProcRes::EXE_FAIL;
    tmpMsg->msgDataLen = 0;
    (void)memset_s(tmpMsg->msgData, DYN_PROF_RSP_MSG_MAX_LEN, 0, DYN_PROF_RSP_MSG_MAX_LEN);
    return sizeof(DynProfRspMsg);
}

size_t RecvReturnMsgLenNotEqualToMsgDataForSendMsgToServerStub(int fd, VOID_PTR msg, size_t msgSize, int flags)
{
    auto tmpMsg = reinterpret_cast<DynProfRspMsg *>(msg);
    tmpMsg->msgType = DynProfMsgType::START_RSP;
    tmpMsg->msgDataLen = 1;
    (void)memset_s(tmpMsg->msgData, DYN_PROF_RSP_MSG_MAX_LEN, 0, DYN_PROF_RSP_MSG_MAX_LEN);
    return sizeof(DynProfRspMsg);
}

size_t RecvReturnValidMsgStubForSendMsgToServerStub(int fd, VOID_PTR msg, size_t msgSize, int flags)
{
    auto tmpMsg = reinterpret_cast<DynProfRspMsg *>(msg);
    tmpMsg->msgType = DynProfMsgType::START_RSP;
    tmpMsg->statusCode = DynProfMsgProcRes::EXE_SUCC;
    tmpMsg->msgDataLen = 0;
    (void)memset_s(tmpMsg->msgData, DYN_PROF_RSP_MSG_MAX_LEN, 0, DYN_PROF_RSP_MSG_MAX_LEN);
    return sizeof(DynProfRspMsg);
}

size_t RecvReturnValidMsgStub(int fd, VOID_PTR msg, size_t msgSize, int flags)
{
    auto tmpMsg = reinterpret_cast<DynProfRspMsg *>(msg);
    tmpMsg->msgType = DynProfMsgType::DISCONN_RSP;
    tmpMsg->msgDataLen = 0;
    (void)memset_s(tmpMsg->msgData, DYN_PROF_RSP_MSG_MAX_LEN, 0, DYN_PROF_RSP_MSG_MAX_LEN);
    return sizeof(DynProfRspMsg);
}

TEST_F(DynProfClientUtest, DyncProfMsgProcCliStart)
{
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    dynProfCli->cliStarted_ = true;
    int ret = dynProfCli->Start();
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    GlobalMockObject::verify();
    MOCKER_CPP(&DyncProfMsgProcCli::CreateDynProfClientSock)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    dynProfCli->cliStarted_ = false;
    ret = dynProfCli->Start();
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = dynProfCli->Start();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    dynProfCli->Stop();
    dynProfCli->Join();
}

TEST_F(DynProfClientUtest, DyncProfMsgProcCliRun)
{
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();

    GlobalMockObject::verify();
    MOCKER_CPP(&DyncProfMsgProcCli::IsServerDisconnect)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(false));
    std::string cmd;
    MOCKER_CPP(&DyncProfMsgProcCli::TryReadInputCmd)
        .stubs()
        // .with(outBoundP(&cmd))
        .will(returnValue(-1))
        .then(returnValue(1));

    error_message::Context errorContext;
    dynProfCli->cliStarted_ = true;
    dynProfCli->Run(errorContext);
}

TEST_F(DynProfClientUtest, TryReadInputCmdWillReturnInvalidSizeWhenSelectReturnZero)
{
    MOCKER(select)
        .stubs()
        .will(returnValue(0));

    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    std::string echoTips;
    EXPECT_EQ(-1, dynProfCli->TryReadInputCmd(echoTips));
}

TEST_F(DynProfClientUtest, IsServerDisconnect)
{
    MOCKER(recv)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(sizeof(DynProfRspMsg)));

    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    std::string echoTips;
    int ret = dynProfCli->IsServerDisconnect(echoTips);
    EXPECT_EQ(false, ret);
    ret = dynProfCli->IsServerDisconnect(echoTips);
    EXPECT_EQ(false, ret);

    dynProfCli->DynProfCliProcHelp();
}

TEST_F(DynProfClientUtest, IsServerDisconnectWilReturnFalseWhenMemsetFail)
{
    MOCKER(memset_s)
        .stubs()
        .will(returnValue(EOK - 1));
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    std::string echoTips;
    EXPECT_EQ(false, dynProfCli->IsServerDisconnect(echoTips));
}

TEST_F(DynProfClientUtest, IsServerDisconnectWillReturnFalseWhenRecvReturnInvalidLen)
{
    MOCKER(recv)
        .stubs()
        .will(returnValue(-1));
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    std::string echoTips;
    EXPECT_EQ(false, dynProfCli->IsServerDisconnect(echoTips));
}

TEST_F(DynProfClientUtest, IsServerDisconnectWillReturnFalseWhenRecvReturnNotDisconnMsg)
{
    MOCKER(recv)
        .stubs()
        .will(invoke(RecvReturnNotDissconMsgStub));
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    std::string echoTips;
    EXPECT_EQ(false, dynProfCli->IsServerDisconnect(echoTips));
}

TEST_F(DynProfClientUtest, IsServerDisconnectWillReturnFalseWhenRecvReturnInvalidMsgDataLen)
{
    MOCKER(recv)
        .stubs()
        .will(invoke(RecvReturnInvalidMsgDataLenStub));
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    std::string echoTips;
    EXPECT_EQ(false, dynProfCli->IsServerDisconnect(echoTips));
}

TEST_F(DynProfClientUtest, IsServerDisconnectWillReturnFalseWhenRecvReturnInvalidMsg)
{
    MOCKER(recv)
        .stubs()
        .will(invoke(RecvReturnMsgLenNotEqualToMsgDataStub));
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    std::string echoTips;
    EXPECT_EQ(false, dynProfCli->IsServerDisconnect(echoTips));
}

TEST_F(DynProfClientUtest, DyncProfMsgProcCliStop)
{
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    dynProfCli->cliStarted_ = false;
    int ret = dynProfCli->Stop();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
 
    dynProfCli->cliStarted_ = true;
    ret = dynProfCli->Stop();
    EXPECT_EQ(false, dynProfCli->cliStarted_);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    dynProfCli->cliStarted_ = true;
    dynProfCli->cliSockFd_ = 1;
    MOCKER(close)
        .stubs()
        .will(returnValue(0));
    EXPECT_EQ(PROFILING_SUCCESS, dynProfCli->Stop());
    EXPECT_EQ(false, dynProfCli->cliStarted_);
}

TEST_F(DynProfClientUtest, SetParams)
{
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    std::string params1 = "";
    int ret = dynProfCli->SetParams(params1);
    EXPECT_EQ(PROFILING_FAILED, ret);
 
    char strBuf[DYN_PROF_REQ_MSG_MAX_LEN + 1] = {0};
    for (uint32_t i = 0; i < DYN_PROF_REQ_MSG_MAX_LEN; i++) {
        strBuf[i] = '0';
    }
    std::string params2(strBuf);
    ret = dynProfCli->SetParams(params2);
    EXPECT_EQ(PROFILING_FAILED, ret);
 
    std::string params3 = "params3";
    ret = dynProfCli->SetParams(params3);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(DynProfClientUtest, SetSocketMsgTimeoutWillReturnFailWhenSetSendsockoptReturnInvalid)
{
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    GlobalMockObject::verify();
    MOCKER(setsockopt)
        .stubs()
        .will(returnValue(-1));
    int fd = 0;
    EXPECT_EQ(PROFILING_FAILED, dynProfCli->SetSocketMsgTimeout(fd));
    MOCKER(setsockopt).reset();
}

TEST_F(DynProfClientUtest, SetSocketMsgTimeoutWillReturnFailWhenSetRecvsockoptReturnInvalid)
{
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    GlobalMockObject::verify();
    MOCKER(setsockopt)
        .stubs()
        .will(returnValue(0))
        .then(returnValue(-1));
    int fd = 0;
    EXPECT_EQ(PROFILING_FAILED, dynProfCli->SetSocketMsgTimeout(fd));
    MOCKER(setsockopt).reset();
}

TEST_F(DynProfClientUtest, SetSocketMsgTimeoutWillReturnSuccWhenSetSockoptReturnValid)
{
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    GlobalMockObject::verify();
    MOCKER(setsockopt)
        .stubs()
        .will(returnValue(0));
    int fd = 0;
    EXPECT_EQ(PROFILING_SUCCESS, dynProfCli->SetSocketMsgTimeout(fd));
    MOCKER(setsockopt).reset();
}

TEST_F(DynProfClientUtest, CreateDynProfClientSock)
{
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
 
    GlobalMockObject::verify();
    MOCKER(socket)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    MOCKER(&DyncProfMsgProcCli::ConnectSocket)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    MOCKER(setsockopt)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    MOCKER_CPP(&DyncProfMsgProcCli::SendMsgToServer)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    MOCKER(&DyncProfMsgProcCli::SetSocketMsgTimeout)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));

    int ret = dynProfCli->CreateDynProfClientSock();
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = dynProfCli->CreateDynProfClientSock();
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = dynProfCli->CreateDynProfClientSock();
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = dynProfCli->CreateDynProfClientSock();
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = dynProfCli->CreateDynProfClientSock();
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = dynProfCli->CreateDynProfClientSock();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(DynProfClientUtest, SendMsgToServer)
{
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
 
    GlobalMockObject::verify();
    std::string invalidParams(DYN_PROF_REQ_MSG_MAX_LEN + 1, 'x');
    std::string tips;
    int ret = dynProfCli->SendMsgToServer(DynProfMsgType::START_REQ, DynProfMsgType::START_RSP, invalidParams, tips);
    EXPECT_EQ(PROFILING_FAILED, ret);
    
    MOCKER(send)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(sizeof(DynProfReqMsg)));
    MOCKER(recv)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(sizeof(DynProfRspMsg)));

    std::string params;
    ret = dynProfCli->SendMsgToServer(DynProfMsgType::START_REQ, DynProfMsgType::START_RSP, params, tips);
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = dynProfCli->SendMsgToServer(DynProfMsgType::START_REQ, DynProfMsgType::START_RSP, params, tips);
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = dynProfCli->SendMsgToServer(DynProfMsgType::START_REQ, DynProfMsgType::START_RSP, params, tips);
    EXPECT_EQ(PROFILING_FAILED, ret);
}

TEST_F(DynProfClientUtest, SendMsgToServerWillReturnFailWhenMemSetFail)
{
    MOCKER(memset_s)
        .stubs()
        .will(returnValue(EOK - 1));
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    GlobalMockObject::verify();
    std::string invalidParams;
    std::string tips;
    EXPECT_EQ(PROFILING_FAILED, dynProfCli->SendMsgToServer(DynProfMsgType::START_REQ,
        DynProfMsgType::START_RSP, invalidParams, tips));

    MOCKER(memset_s)
        .stubs()
        .will(returnValue(EOK))
        .then(returnValue(EOK - 1));
    MOCKER(send)
        .stubs()
        .will(returnValue(sizeof(DynProfReqMsg)));
    EXPECT_EQ(PROFILING_FAILED, dynProfCli->SendMsgToServer(DynProfMsgType::START_REQ,
        DynProfMsgType::START_RSP, invalidParams, tips));
}

TEST_F(DynProfClientUtest, SendMsgToServerWillReturnFailWhenRecvInvalidRsqMsgType)
{
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    GlobalMockObject::verify();
    std::string invalidParams;
    std::string tips;
    MOCKER(send)
        .stubs()
        .will(returnValue(sizeof(DynProfReqMsg)));
    MOCKER(recv)
        .stubs()
        .will(invoke(RecvReturnMsgTypeForSendMsgToServerStub));
    EXPECT_EQ(PROFILING_FAILED, dynProfCli->SendMsgToServer(DynProfMsgType::START_REQ,
        DynProfMsgType::START_RSP, invalidParams, tips));
}

TEST_F(DynProfClientUtest, SendMsgToServerWillReturnFailWhenRecvInvalidRsqMsgStatusCode)
{
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    GlobalMockObject::verify();
    std::string invalidParams;
    std::string tips;
    MOCKER(send)
        .stubs()
        .will(returnValue(sizeof(DynProfReqMsg)));
    MOCKER(recv)
        .stubs()
        .will(invoke(RecvReturnMsgStatusCodeForSendMsgToServerStub));
    EXPECT_EQ(PROFILING_FAILED, dynProfCli->SendMsgToServer(DynProfMsgType::START_REQ,
        DynProfMsgType::START_RSP, invalidParams, tips));
}

TEST_F(DynProfClientUtest, SendMsgToServerWillReturnFailWhenRecvInvalidRsqMsgDataLen)
{
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    GlobalMockObject::verify();
    std::string invalidParams;
    std::string tips;
    MOCKER(send)
        .stubs()
        .will(returnValue(sizeof(DynProfReqMsg)));
    MOCKER(recv)
        .stubs()
        .will(invoke(RecvReturnMsgDataLenForSendMsgToServerStub));
    EXPECT_EQ(PROFILING_FAILED, dynProfCli->SendMsgToServer(DynProfMsgType::START_REQ,
        DynProfMsgType::START_RSP, invalidParams, tips));
}

TEST_F(DynProfClientUtest, SendMsgToServerWillReturnFailWhenRecvMsgDataLenNotEqualToMsgData)
{
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    GlobalMockObject::verify();
    std::string invalidParams;
    std::string tips;
    MOCKER(send)
        .stubs()
        .will(returnValue(sizeof(DynProfReqMsg)));
    MOCKER(recv)
        .stubs()
        .will(invoke(RecvReturnMsgLenNotEqualToMsgDataForSendMsgToServerStub));
    EXPECT_EQ(PROFILING_FAILED, dynProfCli->SendMsgToServer(DynProfMsgType::START_REQ,
        DynProfMsgType::START_RSP, invalidParams, tips));
}

TEST_F(DynProfClientUtest, SendMsgToServerWillReturnFailWhenRecvRsqMsgStatusCodeNotExeSucc)
{
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    GlobalMockObject::verify();
    std::string invalidParams;
    std::string tips;
    MOCKER(send)
        .stubs()
        .will(returnValue(sizeof(DynProfReqMsg)));
    MOCKER(recv)
        .stubs()
        .will(invoke(RecvReturnMsgSatusCodeNotExeSuccForSendMsgToServerStub));
    EXPECT_EQ(PROFILING_FAILED, dynProfCli->SendMsgToServer(DynProfMsgType::START_REQ,
        DynProfMsgType::START_RSP, invalidParams, tips));
}

TEST_F(DynProfClientUtest, SendMsgToServerWillReturnSuccWhenRecvValidMsg)
{
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    GlobalMockObject::verify();
    std::string invalidParams;
    std::string tips;
    MOCKER(send)
        .stubs()
        .will(returnValue(sizeof(DynProfReqMsg)));
    MOCKER(recv)
        .stubs()
        .will(invoke(RecvReturnValidMsgStubForSendMsgToServerStub));
    EXPECT_EQ(PROFILING_SUCCESS, dynProfCli->SendMsgToServer(DynProfMsgType::START_REQ,
        DynProfMsgType::START_RSP, invalidParams, tips));
}

class DynProfMngCliUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(DynProfMngCliUtest, StartDynProfCliWillReturnFailWhenSetParamsFail)
{
    MOCKER_CPP(&DyncProfMsgProcCli::SetParams)
        .stubs()
        .will(returnValue(PROFILING_FAILED));

    std::string params;
    EXPECT_EQ(PROFILING_FAILED, DynProfMngCli::instance()->StartDynProfCli(params));
}

TEST_F(DynProfMngCliUtest, StartDynProfCliWillReturnFailWhenStartClientFail)
{
    MOCKER_CPP(&DyncProfMsgProcCli::SetParams)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&DyncProfMsgProcCli::CreateDynProfClientSock)
        .stubs()
        .will(returnValue(PROFILING_FAILED));

    std::string params;
    EXPECT_EQ(PROFILING_FAILED, DynProfMngCli::instance()->StartDynProfCli(params));
}
