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
#include "proto/profiler.pb.h"
#include "uploader_mgr.h"
#include "utils/utils.h"
#include "dyn_prof_server.h"
#include "dyn_prof_client.h"

using namespace analysis::dvvp::common::error;
using namespace Msprof::Engine;
using namespace Collector::Dvvp::DynProf;

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

class DYN_PROF_SERVER_UTEST : public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};
 
TEST_F(DYN_PROF_SERVER_UTEST, DynProfMngSrv)
{
    int ret = DynProfMngSrv::instance()->StartDynProfSrv();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    DynProfMngSrv::instance()->StopDynProfSrv();
    ProfSetDevPara oriData = {0, 1, true};
    DynProfMngSrv::instance()->SetDeviceInfo(&oriData);
    std::vector<ProfSetDevPara> getData = DynProfMngSrv::instance()->GetDeviceInfo();
    EXPECT_EQ(1, getData[0].deviceId);
}
 
TEST_F(DYN_PROF_SERVER_UTEST, DyncProfMsgProcSrvStart)
{
    SHARED_PTR_ALIA<DyncProfMsgProcSrv> dynProfSrv;
    dynProfSrv = std::make_shared<DyncProfMsgProcSrv>();
    dynProfSrv->srvStarted_ = true;
    int ret = dynProfSrv->Start();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
 
    GlobalMockObject::verify();
    MOCKER_CPP(&DyncProfMsgProcSrv::DynProfServerCreateSock)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
 
    dynProfSrv->srvStarted_ = false;
    ret = dynProfSrv->Start();
    EXPECT_EQ(PROFILING_FAILED, ret);
 
    ret = dynProfSrv->Start();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    dynProfSrv->Stop();
    dynProfSrv->Join();
}
 
TEST_F(DYN_PROF_SERVER_UTEST, DyncProfMsgProcSrvStop)
{
    SHARED_PTR_ALIA<DyncProfMsgProcSrv> dynProfSrv;
    dynProfSrv = std::make_shared<DyncProfMsgProcSrv>();
    dynProfSrv->srvStarted_ = false;
    int ret = dynProfSrv->Stop();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    MOCKER(close)
        .stubs();
    dynProfSrv->srvStarted_ = true;
    dynProfSrv->srvSockFd_ = 0;
    dynProfSrv->cliSockFd_ = 0;
    ret = dynProfSrv->Stop();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    EXPECT_EQ(false, dynProfSrv->srvStarted_);
}
 
TEST_F(DYN_PROF_SERVER_UTEST, DynProfServerCreateSock)
{
    SHARED_PTR_ALIA<DyncProfMsgProcSrv> dynProfSrv;
    dynProfSrv = std::make_shared<DyncProfMsgProcSrv>();
    GlobalMockObject::verify();
    MOCKER(socket)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    MOCKER(memset_s)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(-1))
        .then(returnValue(0));
    MOCKER(bind)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    MOCKER(listen)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    MOCKER(setsockopt)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
 
    int ret = dynProfSrv->DynProfServerCreateSock();
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = dynProfSrv->DynProfServerCreateSock();
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = dynProfSrv->DynProfServerCreateSock();
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = dynProfSrv->DynProfServerCreateSock();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    ret = dynProfSrv->DynProfServerCreateSock();
}

TEST_F(DYN_PROF_SERVER_UTEST, IdleConnectOverTime)
{
    SHARED_PTR_ALIA<DyncProfMsgProcSrv> dynProfSrv;
    dynProfSrv = std::make_shared<DyncProfMsgProcSrv>();
    dynProfSrv->profHasStarted_ = true;
    uint32_t time = 0;
    EXPECT_EQ(false, dynProfSrv->IdleConnectOverTime(time));
    dynProfSrv->profHasStarted_ = false;
    EXPECT_EQ(false, dynProfSrv->IdleConnectOverTime(time));
    time = DYN_PROF_IDLE_LINK_HOLD_TIME;
    EXPECT_EQ(true, dynProfSrv->IdleConnectOverTime(time));
}

TEST_F(DYN_PROF_SERVER_UTEST, DynProfServerProcMsg)
{
    MOCKER(close)
        .stubs();
    SHARED_PTR_ALIA<DyncProfMsgProcSrv> dynProfSrv;
    dynProfSrv = std::make_shared<DyncProfMsgProcSrv>();
    dynProfSrv->srvStarted_ = true;
    dynProfSrv->DynProfServerProcMsg();
}
 
TEST_F(DYN_PROF_SERVER_UTEST, DynProfSrvProcStart)
{
    SHARED_PTR_ALIA<DyncProfMsgProcSrv> dynProfSrv;
    dynProfSrv = std::make_shared<DyncProfMsgProcSrv>();
    dynProfSrv->profHasStarted_ = true;
    dynProfSrv->cliSockFd_ = 0;
    dynProfSrv->DynProfSrvProcStart();
 
    GlobalMockObject::verify();
    MOCKER_CPP(&DyncProfMsgProcSrv::DynProfServerRsqMsg)
        .stubs()
        .will(returnValue(0));

    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofInitAclEnv)
        .stubs()
        .will(returnValue(1))
        .then(returnValue(0));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::Init)
        .stubs()
        .will(returnValue(1))
        .then(returnValue(0));
    MOCKER(Analysis::Dvvp::ProfilerCommon::MsprofSetDeviceCallbackImpl)
        .stubs()
        .will(returnValue(1))
        .then(returnValue(0));
    
    dynProfSrv->profHasStarted_ = false;
    dynProfSrv->DynProfSrvProcStart();
    EXPECT_EQ(false, dynProfSrv->profHasStarted_);
 
    dynProfSrv->DynProfSrvProcStart();
    EXPECT_EQ(false, dynProfSrv->profHasStarted_);
 
    dynProfSrv->DynProfSrvProcStart();
    EXPECT_EQ(false, dynProfSrv->profHasStarted_);
 
    dynProfSrv->DynProfSrvProcStart();
    EXPECT_EQ(true, dynProfSrv->profHasStarted_);
    EXPECT_EQ(1, DynProfMngSrv::instance()->startTimes_);
}
 
TEST_F(DYN_PROF_SERVER_UTEST, DynProfSrvProcStop)
{
    SHARED_PTR_ALIA<DyncProfMsgProcSrv> dynProfSrv;
    dynProfSrv = std::make_shared<DyncProfMsgProcSrv>();
    dynProfSrv->profHasStarted_ = false;
    dynProfSrv->cliSockFd_ = 1;
    dynProfSrv->DynProfSrvProcStop();
 
    GlobalMockObject::verify();
    MOCKER_CPP(&DyncProfMsgProcSrv::DynProfServerRsqMsg)
        .stubs()
        .will(returnValue(0));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofFinalizeHandle)
        .stubs()
        .will(returnValue(1))
        .then(returnValue(0));
    
    dynProfSrv->profHasStarted_ = true;
    dynProfSrv->DynProfSrvProcStop();
    EXPECT_EQ(true, dynProfSrv->profHasStarted_);
 
    dynProfSrv->DynProfSrvProcStop();
    EXPECT_EQ(false, dynProfSrv->profHasStarted_);
}
 
TEST_F(DYN_PROF_SERVER_UTEST, DynProfSrvProcQuit)
{
    SHARED_PTR_ALIA<DyncProfMsgProcSrv> dynProfSrv;
    dynProfSrv = std::make_shared<DyncProfMsgProcSrv>();
    dynProfSrv->profHasStarted_ = true;
    dynProfSrv->cliSockFd_ = 1;
    GlobalMockObject::verify();
    MOCKER_CPP(&DyncProfMsgProcSrv::DynProfServerRsqMsg)
        .stubs()
        .will(returnValue(0));
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::MsprofFinalizeHandle)
        .stubs()
        .will(returnValue(1))
        .then(returnValue(0));
    dynProfSrv->DynProfSrvProcQuit();
    EXPECT_EQ(false, dynProfSrv->profHasStarted_);
}
 
TEST_F(DYN_PROF_SERVER_UTEST, DynProfServerRsqMsg)
{
    std::string tips;
    SHARED_PTR_ALIA<DyncProfMsgProcSrv> dynProfSrv;
    dynProfSrv = std::make_shared<DyncProfMsgProcSrv>();
    dynProfSrv->cliSockFd_ = 0;
    GlobalMockObject::verify();
    MOCKER(write)
        .stubs()
        .will(returnValue(-1));
    dynProfSrv->DynProfServerRsqMsg(DynProfMsgType::START_RSP, DynProfMsgProcRes::EXE_FAIL, tips);
}

class DYN_PROF_CLIENT_UTEST : public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};
 
TEST_F(DYN_PROF_CLIENT_UTEST, DynProfMngCli)
{
    MOCKER_CPP(&DyncProfMsgProcCli::SetParams)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
 
    std::string params;
    int ret = DynProfMngCli::instance()->StartDynProfCli(params);
    EXPECT_EQ(PROFILING_FAILED, ret);
 
    std::string env = DynProfMngCli::instance()->ConstructEnv();
    EXPECT_EQ(true, env.empty());
 
    DynProfMngCli::instance()->SetAppPid(1);
    EXPECT_EQ(1, DynProfMngCli::instance()->GetAppPid());
    DynProfMngCli::instance()->EnableMode();
    EXPECT_EQ(true, DynProfMngCli::instance()->IsEnableMode());
 
    env = DynProfMngCli::instance()->ConstructEnv();
    EXPECT_EQ(false, env.empty());
}
 
TEST_F(DYN_PROF_CLIENT_UTEST, DyncProfMsgProcCliStart)
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

TEST_F(DYN_PROF_CLIENT_UTEST, DyncProfMsgProcCliRun)
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

TEST_F(DYN_PROF_CLIENT_UTEST, TryReadInputCmd)
{
    MOCKER(select)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));

    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    std::string echoTips;
    int ret = dynProfCli->TryReadInputCmd(echoTips);
    EXPECT_EQ(-1, ret);
    ret = dynProfCli->TryReadInputCmd(echoTips);
    EXPECT_EQ(1, ret);
}

TEST_F(DYN_PROF_CLIENT_UTEST, IsServerDisconnect)
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

TEST_F(DYN_PROF_CLIENT_UTEST, DyncProfMsgProcCliStop)
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
}

TEST_F(DYN_PROF_CLIENT_UTEST, SetParams)
{
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    std::string params1 = "";
    int ret = dynProfCli->SetParams(params1);
    EXPECT_EQ(PROFILING_FAILED, ret);
 
    char strBuf[DYN_PROF_REQ_MSG_MAX_LEN + 1] = {0};
    for (int i = 0; i < DYN_PROF_REQ_MSG_MAX_LEN; i++) {
        strBuf[i] = '0';
    }
    std::string params2(strBuf);
    ret = dynProfCli->SetParams(params2);
    EXPECT_EQ(PROFILING_FAILED, ret);
 
    std::string params3 = "params3";
    ret = dynProfCli->SetParams(params3);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(DYN_PROF_CLIENT_UTEST, SetSocketMsgTimeout)
{
    SHARED_PTR_ALIA<DyncProfMsgProcCli> dynProfCli;
    dynProfCli = std::make_shared<DyncProfMsgProcCli>();
    GlobalMockObject::verify();
    MOCKER(setsockopt)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    MOCKER(setsockopt)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(0));
    int fd = 0;
    int ret = dynProfCli->SetSocketMsgTimeout(fd);
    EXPECT_EQ(PROFILING_FAILED, ret);
    ret = dynProfCli->SetSocketMsgTimeout(fd);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
    ret = dynProfCli->SetSocketMsgTimeout(fd);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(DYN_PROF_CLIENT_UTEST, CreateDynProfClientSock)
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
 
TEST_F(DYN_PROF_CLIENT_UTEST, SendMsgToServer)
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
