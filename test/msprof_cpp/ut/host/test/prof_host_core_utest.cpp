#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "hdc-log-stub.h"
#include "hdc-api-stub.h"
#include "proto/msprofiler.pb.h"
#include "errno/error_code.h"
#include "message/codec.h"
#include "prof_host_core.h"
#include "prof_manager.h"
#include "securec.h"
#include "ai_drv_dev_api.h"
#include "app/app_manager.h"
#include "transport/file_transport.h"

using namespace analysis::dvvp::common::error;

class HOST_PROF_HOST_CORE_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

TEST_F(HOST_PROF_HOST_CORE_TEST, IdeHostProfileInit) {
    GlobalMockObject::verify();

    MOCKER_CPP(&analysis::dvvp::host::ProfManager::Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCKER(IdeDeviceStateNotifierRegister)
        .stubs();

    EXPECT_NE(PROFILING_SUCCESS, IdeHostProfileInit());
    EXPECT_EQ(PROFILING_SUCCESS, IdeHostProfileInit());
}

TEST_F(HOST_PROF_HOST_CORE_TEST, IdeHostProfileProcess_ide_failed) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::transport::ITransport> trans;
    MOCKER_CPP(&analysis::dvvp::transport::TransportFactory::CreateIdeTransport)
        .stubs()
        .will(returnValue(trans));

    std::shared_ptr<analysis::dvvp::proto::CtrlChannelHandshake> message(
        new analysis::dvvp::proto::CtrlChannelHandshake);

    std::string buffer = analysis::dvvp::message::EncodeMessage(message);
    struct tlv_req * req = (struct tlv_req *)new char[sizeof(struct tlv_req) + buffer.size()];
    req->len = (int)buffer.size();
    memcpy_s(req->value, req->len, buffer.c_str(), buffer.size());

    MOCKER_CPP(&analysis::dvvp::app::AppManager::DeleteAppInfo)
        .stubs()
        .will(ignoreReturnValue());
    
    EXPECT_EQ(PROFILING_FAILED, IdeHostProfileProcess((IDE_SESSION)0x12345678, NULL, req));

    delete [] ((char*)req);
}

TEST_F(HOST_PROF_HOST_CORE_TEST, IdeHostProfileProcess) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::proto::JobStartReq> message(
        new analysis::dvvp::proto::JobStartReq);
    message->set_sampleconfig("123");
    std::string buffer = analysis::dvvp::message::EncodeMessage(message);
    struct tlv_req * req = (struct tlv_req *)new char[sizeof(struct tlv_req) + buffer.size()];
    req->len = (int)buffer.size();
    memcpy_s(req->value, req->len, buffer.c_str(), buffer.size());

    MOCKER_CPP(&analysis::dvvp::app::AppManager::DeleteAppInfo)
        .stubs()
        .will(ignoreReturnValue());
 
    //null sockfd
    EXPECT_EQ(PROFILING_FAILED, IdeHostProfileProcess(NULL, NULL, req));

    //null DrvGetPlatformInfo
    uint32_t platformInfo = 0;
    MOCKER(drvGetPlatformInfo)
        .stubs()
        .with(outBoundP(&platformInfo))
        .will(returnValue(DRV_ERROR_NO_DEVICE))
        .then(returnValue(DRV_ERROR_NONE));
    EXPECT_EQ(PROFILING_FAILED, analysis::dvvp::driver::DrvGetPlatformInfo(platformInfo));
    EXPECT_EQ((int)platformInfo, analysis::dvvp::driver::DrvGetPlatformInfo(platformInfo));

    EXPECT_EQ(PROFILING_FAILED, IdeHostProfileProcess((IDE_SESSION)0x12345678, NULL, NULL));
    
    //null req not soc
    EXPECT_EQ(PROFILING_FAILED, IdeHostProfileProcess((IDE_SESSION)0x12345678, NULL, NULL));

    //null req soc
    EXPECT_EQ(PROFILING_FAILED, IdeHostProfileProcess((IDE_SESSION)0x12345678, NULL, NULL));

    MOCKER(IdeSockDupCreate)
        .stubs()
        .will(returnValue((void *)nullptr))
        .then(returnValue((void *)0x12345));
    //dup create error
    EXPECT_EQ(PROFILING_FAILED, IdeHostProfileProcess((IDE_SESSION)0x12345678, NULL, req));
    MOCKER_CPP(&analysis::dvvp::host::ProfManager::Handle)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    
    HDC_CLIENT client = (HDC_CLIENT)0x12345678;
    EXPECT_EQ(PROFILING_FAILED, IdeHostProfileProcess((IDE_SESSION)0x12345678, client, NULL));
    EXPECT_NE(PROFILING_SUCCESS, IdeHostProfileProcess((IDE_SESSION)0x12345678, client, req));
    EXPECT_EQ(PROFILING_FAILED, IdeHostProfileProcess((IDE_SESSION)0x12345678, client, req));

    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
        new analysis::dvvp::message::ProfileParams);
    params->profiling_mode = "online";
    MOCKER_CPP(&analysis::dvvp::host::ProfManager::HandleProfilingParams)
        .stubs()
        .then(returnValue(params));

    std::shared_ptr<analysis::dvvp::transport::ITransport> fileTransport;
    fileTransport.reset();
    MOCKER_CPP(&analysis::dvvp::transport::FileTransportFactory::CreateFileTransport)
        .stubs()
        .then(returnValue(fileTransport));
    
    EXPECT_EQ(PROFILING_FAILED, IdeHostProfileProcess((IDE_SESSION)0x12345678, client, req));

    params->profiling_mode = "system-wide";
    EXPECT_EQ(PROFILING_FAILED, IdeHostProfileProcess((IDE_SESSION)0x12345678, client, req));
    params->profiling_mode="";
    params->is_cancel = true;
    EXPECT_EQ(PROFILING_FAILED, IdeHostProfileProcess((IDE_SESSION)0x12345678, client, req));

    auto appInfo = std::make_shared<analysis::dvvp::app::AppInfo>();
    params->job_id = "123";
    analysis::dvvp::app::AppManager::instance()->AddAppInfo("123", appInfo);
    EXPECT_EQ(PROFILING_SUCCESS, IdeHostProfileProcess((IDE_SESSION)0x12345678, client, req));
    params->is_cancel = false;
    EXPECT_EQ(PROFILING_FAILED, IdeHostProfileProcess((IDE_SESSION)0x12345678, client, req));
    delete[]((char *)req);
}

TEST_F(HOST_PROF_HOST_CORE_TEST, IdeHostProfileCleanup) {
    GlobalMockObject::verify();
    MOCKER_CPP(&analysis::dvvp::host::ProfManager::Uinit)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

   EXPECT_NE(PROFILING_SUCCESS, IdeHostProfileCleanup());
   EXPECT_EQ(PROFILING_SUCCESS, IdeHostProfileCleanup());
}