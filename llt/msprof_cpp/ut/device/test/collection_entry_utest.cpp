#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include "ai_drv_dev_api.h"
#include "ai_drv_prof_api.h"
#include "utils/utils.h"
#include "config/config.h"
#include "errno/error_code.h"
#include "transport/transport.h"
#include "collection_entry.h"
#include "prof_timer.h"
#include "uploader_mgr.h"
#include "prof_channel_manager.h"
#include "config/config_manager.h"

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::JobWrapper;
using namespace Analysis::Dvvp::Common::Statistics;

using namespace analysis::dvvp::device;
class DEVICE_COLLECTION_ENTRY_TEST: public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown() {}
public:
    MockObject<ProfChannelManager>mockerProfChannelManager;
    MockObject<analysis::dvvp::device::CollectionEntry>mockerCollectionEntry;
    MockObject<analysis::dvvp::transport::HDCTransport>mockerHDCTransport;
    MockObject<analysis::dvvp::device::Receiver>mockerReceiver;
    MockObject<analysis::dvvp::common::thread::Thread>mockerThread;
    MockObject<analysis::dvvp::transport::Uploader>mockerUploader;
    
};
TEST_F(DEVICE_COLLECTION_ENTRY_TEST, Init) {
    GlobalMockObject::verify();
    
    MOCK_METHOD(mockerProfChannelManager, Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER(analysis::dvvp::common::utils::Utils::IsFileExist)
        .stubs()
        .will(returnValue(true));
    auto entry = analysis::dvvp::device::CollectionEntry::instance();
    EXPECT_EQ(PROFILING_SUCCESS, entry->Init());
    GlobalMockObject::verify();
    
    EXPECT_EQ(PROFILING_SUCCESS, entry->Init());

}

TEST_F(DEVICE_COLLECTION_ENTRY_TEST, Uinit) {
    GlobalMockObject::verify();


    auto entry = analysis::dvvp::device::CollectionEntry::instance();
    entry->isInited_ = true;

    MOCKER(mmCreateTaskWithThreadAttr)
        .stubs()
        .will(returnValue(EN_OK));
	MOCKER(mmJoinTask)
        .stubs()
        .will(returnValue(EN_OK));
    entry->Init();

    EXPECT_EQ(PROFILING_SUCCESS, entry->Uinit());
}

TEST_F(DEVICE_COLLECTION_ENTRY_TEST, GetChannelPoller) {

    GlobalMockObject::verify();
    auto profChannelManager = ProfChannelManager::instance();
    EXPECT_EQ(nullptr, profChannelManager->GetChannelPoller());
}

TEST_F(DEVICE_COLLECTION_ENTRY_TEST, Handle) {
    GlobalMockObject::verify();

    HDC_SESSION session = (HDC_SESSION)0x12345678;
    auto transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
            new analysis::dvvp::transport::HDCTransport(session));
    std::shared_ptr<PerfCount> perfCount(new PerfCount("test"));
    transport->perfCount_ = perfCount;

    std::shared_ptr<analysis::dvvp::proto::JobStartReq> start(
        new analysis::dvvp::proto::JobStartReq);
    std::string start_buffer = analysis::dvvp::message::EncodeMessage(start);

    std::shared_ptr<analysis::dvvp::proto::CtrlChannelHandshake> ctrl(
        new analysis::dvvp::proto::CtrlChannelHandshake);
    std::string ctrl_buffer = analysis::dvvp::message::EncodeMessage(ctrl);

    std::shared_ptr<analysis::dvvp::proto::DataChannelHandshake> data(
        new analysis::dvvp::proto::DataChannelHandshake);
    std::string data_buffer = analysis::dvvp::message::EncodeMessage(data);

    std::string non_message("123456");

    MOCK_METHOD(mockerCollectionEntry, HandleCtrlSession)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCK_METHOD(mockerCollectionEntry, HandleDataSession)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCK_METHOD(mockerHDCTransport, SendBuffer)
        .stubs()
        .will(returnValue(PROFILING_FAILED));

    auto entry = analysis::dvvp::device::CollectionEntry::instance();

    EXPECT_EQ(PROFILING_FAILED, entry->Handle(transport, non_message, 0));

    entry->isInited_ = true;

    EXPECT_EQ(PROFILING_FAILED, entry->Handle(transport, non_message, 0));
    EXPECT_EQ(PROFILING_FAILED, entry->Handle(transport, start_buffer, 0));
    EXPECT_EQ(PROFILING_FAILED, entry->Handle(transport, ctrl_buffer, 0));
    EXPECT_EQ(PROFILING_SUCCESS, entry->Handle(transport, ctrl_buffer, 0));
    EXPECT_EQ(PROFILING_FAILED, entry->Handle(transport, data_buffer, 0));
    EXPECT_EQ(PROFILING_SUCCESS, entry->Handle(transport, data_buffer, 0));
}

TEST_F(DEVICE_COLLECTION_ENTRY_TEST, HandleCtrlSession) {
    GlobalMockObject::verify();

    HDC_SESSION session = (HDC_SESSION)0x12345678;
    auto transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
            new analysis::dvvp::transport::HDCTransport(session));
    std::shared_ptr<analysis::dvvp::device::Receiver> receiver(
        new analysis::dvvp::device::Receiver(transport));

    std::shared_ptr<analysis::dvvp::proto::CtrlChannelHandshake> ctrl(
        new analysis::dvvp::proto::CtrlChannelHandshake);

    MOCK_METHOD(mockerReceiver, Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    //MOCK_METHOD(&analysis::dvvp::common::thread::Thread::Start)
    //    .stubs()
    //    .will(returnValue(PROFILING_SUCCESS));
    MOCKER(mmCreateTaskWithThreadAttr)
        .stubs()
        .will(returnValue(EN_OK));
	

    MOCK_METHOD(mockerThread, Join)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    MOCK_METHOD(mockerHDCTransport, SendBuffer)
        .stubs()
        .will(returnValue(PROFILING_FAILED));

    auto entry = analysis::dvvp::device::CollectionEntry::instance();

     MOCKER(analysis::dvvp::common::utils::Utils::CheckStringIsNonNegativeIntNum)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true))
        .then(returnValue(true));

    analysis::dvvp::message::StatusInfo status_info;
    EXPECT_EQ(PROFILING_FAILED, entry->HandleCtrlSession(receiver, ctrl, status_info, 0));
    EXPECT_EQ(PROFILING_FAILED, entry->HandleCtrlSession(receiver, ctrl, status_info, 0));
    EXPECT_EQ(PROFILING_SUCCESS, entry->HandleCtrlSession(receiver, ctrl, status_info, 0));
}

TEST_F(DEVICE_COLLECTION_ENTRY_TEST, HandleDataSession) {
    GlobalMockObject::verify();

    HDC_SESSION session = (HDC_SESSION)0x12345678;
    auto transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
            new analysis::dvvp::transport::HDCTransport(session));
    std::shared_ptr<analysis::dvvp::transport::Uploader> uploader(
        new analysis::dvvp::transport::Uploader(transport));

    std::shared_ptr<analysis::dvvp::proto::DataChannelHandshake> data(
        new analysis::dvvp::proto::DataChannelHandshake);

    MOCK_METHOD(mockerUploader, Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    //MOCK_METHOD(&analysis::dvvp::common::thread::Thread::Start)
    //    .stubs()
    //    .will(returnValue(PROFILING_SUCCESS));
    MOCKER(mmCreateTaskWithThreadAttr)
        .stubs()
        .will(returnValue(EN_OK));

    MOCK_METHOD(mockerHDCTransport, SendBuffer)
        .stubs()
        .will(returnValue(PROFILING_FAILED));

    auto entry = analysis::dvvp::device::CollectionEntry::instance();

    EXPECT_EQ(PROFILING_FAILED, entry->HandleDataSession(uploader, data, 0));
    EXPECT_EQ(PROFILING_FAILED, entry->HandleDataSession(uploader, data, 0));
}

TEST_F(DEVICE_COLLECTION_ENTRY_TEST, FinishCollection) {
    GlobalMockObject::verify();
    MOCK_METHOD(mockerUploader, Flush)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    auto entry = analysis::dvvp::device::CollectionEntry::instance();

    HDC_SESSION session = (HDC_SESSION)0x12345678;
    auto transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
            new analysis::dvvp::transport::HDCTransport(session));
    auto uploader = std::shared_ptr<analysis::dvvp::transport::Uploader>(
            new analysis::dvvp::transport::Uploader(transport));
    analysis::dvvp::transport::UploaderMgr::instance()->AddUploader("0", uploader);
    analysis::dvvp::message::JobContext job_ctx;
    job_ctx.dev_id = "0";

    EXPECT_EQ(PROFILING_SUCCESS, entry->FinishCollection(0, job_ctx.ToString()));
    uploader->Init();

    EXPECT_EQ(PROFILING_SUCCESS, entry->FinishCollection(0, job_ctx.ToString()));
    EXPECT_EQ(PROFILING_SUCCESS, entry->FinishCollection(0, job_ctx.ToString()));
}

TEST_F(DEVICE_COLLECTION_ENTRY_TEST, destrcutor) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::device::CollectionEntry> entry(
        new analysis::dvvp::device::CollectionEntry());
    entry->isInited_ = false;
    EXPECT_EQ(PROFILING_SUCCESS, entry->Uinit());
    entry.reset();
}

TEST_F(DEVICE_COLLECTION_ENTRY_TEST, SendMsgByDevId) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::device::CollectionEntry> entry(
        new analysis::dvvp::device::CollectionEntry());

    EXPECT_EQ(PROFILING_FAILED, entry->SendMsgByDevId("1231", 0, nullptr));
}
