#include "securec.h"
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "proto/profiler.pb.h"
#include "errno/error_code.h"
#include "message/codec.h"
#include "collection_entry.h"
#include "prof_device_core.h"
#include "hdc-common/hdc_api.h"
#include "transport/hdc/hdc_transport.h"
#include "task_manager.h"

using namespace analysis::dvvp::common::error;

class DEVICE_PROF_DEVICE_CORE_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
public:
    MockObject<analysis::dvvp::device::CollectionEntry>mockerCollectionEntry;
    MockObject<analysis::dvvp::device::TaskManager>mockerTaskManager;
    MockObject<analysis::dvvp::transport::HDCTransportFactory>mockerHDCTransportFactory;
};

TEST_F(DEVICE_PROF_DEVICE_CORE_TEST, IdeDeviceProfileInit) {
    GlobalMockObject::verify();

    MOCK_METHOD(mockerCollectionEntry, Init)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    EXPECT_NE(PROFILING_SUCCESS, IdeDeviceProfileInit());
    EXPECT_EQ(PROFILING_SUCCESS, IdeDeviceProfileInit());
}

TEST_F(DEVICE_PROF_DEVICE_CORE_TEST, IdeDeviceProfileCleanup) {
    GlobalMockObject::verify();

    MOCK_METHOD(mockerCollectionEntry, Uinit)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    MOCK_METHOD(mockerTaskManager, Uninit)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    EXPECT_EQ(PROFILING_FAILED, IdeDeviceProfileCleanup());
    EXPECT_EQ(PROFILING_SUCCESS, IdeDeviceProfileCleanup());
}

TEST_F(DEVICE_PROF_DEVICE_CORE_TEST, IdeDeviceProfileProcess_hdc_failed) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::transport::AdxTransport> trans;
    trans.reset();
    MOCK_OVERLOAD_METHOD(mockerHDCTransportFactory, std::shared_ptr<analysis::dvvp::transport::AdxTransport>(analysis::dvvp::transport::HDCTransportFactory::*)(HDC_SESSION ), CreateHdcTransport)
        .stubs()
        .will(returnValue(trans));

    std::shared_ptr<analysis::dvvp::proto::CtrlChannelHandshake> message(
        new analysis::dvvp::proto::CtrlChannelHandshake);

    std::string buffer = analysis::dvvp::message::EncodeMessage(message);
    struct tlv_req * req = (struct tlv_req *)new char[sizeof(struct tlv_req) + buffer.size()];
    req->len = (int)buffer.size();
    memcpy_s(req->value, req->len, buffer.c_str(), buffer.size());
    HDC_SESSION session = (HDC_SESSION)0x12345678;
    EXPECT_EQ(PROFILING_FAILED, IdeDeviceProfileProcess(session, req));

    delete [] ((char*)req);
}

TEST_F(DEVICE_PROF_DEVICE_CORE_TEST, IdeDeviceProfileProcess) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::proto::CtrlChannelHandshake> message(
        new analysis::dvvp::proto::CtrlChannelHandshake);

    std::string buffer = analysis::dvvp::message::EncodeMessage(message);
    struct tlv_req * req = (struct tlv_req *)new char[sizeof(struct tlv_req) + buffer.size()];
    req->len = (int)buffer.size();
    memcpy_s(req->value, req->len, buffer.c_str(), buffer.size());

    //null params
    EXPECT_EQ(PROFILING_FAILED, IdeDeviceProfileProcess(NULL, req));

    //null req
    HDC_SESSION session = (HDC_SESSION)0x12345678;
    EXPECT_EQ(PROFILING_FAILED, IdeDeviceProfileProcess(session, NULL));

    MOCKER(analysis::dvvp::common::utils::Utils::CheckStringIsNonNegativeIntNum)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true))
        .then(returnValue(true));

    MOCK_METHOD(mockerCollectionEntry, Handle)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    HDC_CLIENT client = (HDC_CLIENT)0x12345678;
    std::shared_ptr<analysis::dvvp::transport::AdxTransport> data_tran = std::make_shared<analysis::dvvp::transport::HDCTransport>(client);

    std::shared_ptr<analysis::dvvp::transport::AdxTransport> trans;
    trans.reset();

    MOCK_OVERLOAD_METHOD(mockerHDCTransportFactory, std::shared_ptr<analysis::dvvp::transport::AdxTransport>(analysis::dvvp::transport::HDCTransportFactory::*)(HDC_SESSION ), CreateHdcTransport)
        .stubs()
        .will(returnValue(trans))
        .then(returnValue(data_tran));

    EXPECT_EQ(PROFILING_FAILED, IdeDeviceProfileProcess(session, req));
    EXPECT_EQ(PROFILING_FAILED, IdeDeviceProfileProcess(session, req));
    EXPECT_EQ(PROFILING_SUCCESS, IdeDeviceProfileProcess(session, req));
    EXPECT_EQ(PROFILING_SUCCESS, IdeDeviceProfileProcess(session, req));

    delete [] ((char*)req);
}
