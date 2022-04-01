#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "hdc-common/log/hdc_log.h"
#include "uploader.h"
#include "transport/hdc/hdc_transport.h"
#include "errno/error_code.h"

using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Common::Statistics;
using namespace Analysis::Dvvp::MsprofErrMgr;

class UPLOADER_TEST: public testing::Test {
protected:
    virtual void SetUp() {
        HDC_SESSION session = (HDC_SESSION)0x12345678;
        _transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
            new analysis::dvvp::transport::HDCTransport(session));
        _transport->perfCount_ = std::shared_ptr<PerfCount> (new PerfCount("test"));
    }
    virtual void TearDown() {
        _transport.reset();
    }
public:
    std::shared_ptr<analysis::dvvp::transport::HDCTransport> _transport;
};

TEST_F(UPLOADER_TEST, Uploader_destructor) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::transport::Uploader> uploader(
        new analysis::dvvp::transport::Uploader(_transport));
    uploader->isInited_ = false;
    EXPECT_EQ(PROFILING_SUCCESS, uploader->Uinit());
    uploader.reset();
}

TEST_F(UPLOADER_TEST, Init) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::transport::Uploader> uploader(
        new analysis::dvvp::transport::Uploader(_transport));

    EXPECT_EQ(PROFILING_SUCCESS, uploader->Init());
    EXPECT_TRUE(uploader->isInited_);
}

TEST_F(UPLOADER_TEST, Uinit) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::transport::Uploader> uploader(
        new analysis::dvvp::transport::Uploader(_transport));

    EXPECT_EQ(PROFILING_SUCCESS, uploader->Uinit());
    EXPECT_FALSE(uploader->isInited_);
}

TEST_F(UPLOADER_TEST, UploadData) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::transport::Uploader> uploader(
        new analysis::dvvp::transport::Uploader(_transport));

    std::string buffer("123456");

    EXPECT_EQ(PROFILING_FAILED, uploader->UploadData(buffer.c_str(), buffer.size()));

    EXPECT_EQ(PROFILING_SUCCESS, uploader->Init());
    EXPECT_EQ(PROFILING_SUCCESS, uploader->UploadData(buffer.c_str(), buffer.size()));
}

TEST_F(UPLOADER_TEST, run_uploader_not_init) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::transport::Uploader> uploader(
        new analysis::dvvp::transport::Uploader(_transport));
    EXPECT_NE(nullptr, uploader);
    auto errorContext = MsprofErrorManager::instance()->GetErrorManagerContext();
    uploader->Run(errorContext);
}

TEST_F(UPLOADER_TEST, run_no_data) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::transport::Uploader> uploader(
        new analysis::dvvp::transport::Uploader(_transport));

    MOCKER_CPP_VIRTUAL(_transport.get(), &analysis::dvvp::transport::HDCTransport::SendBuffer)
        .stubs()
        .will(returnValue(PROFILING_FAILED));

    EXPECT_EQ(PROFILING_SUCCESS, uploader->Init());

    uploader->forceQuit_ = true;
    uploader->queue_->Quit();
    auto errorContext = MsprofErrorManager::instance()->GetErrorManagerContext();
    uploader->Run(errorContext);
    uploader.reset();
}

TEST_F(UPLOADER_TEST, run) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::transport::Uploader> uploader(
        new analysis::dvvp::transport::Uploader(_transport));

    MOCKER_CPP_VIRTUAL(_transport.get(), &analysis::dvvp::transport::HDCTransport::SendBuffer)
        .stubs()
        .will(returnValue(PROFILING_FAILED));

    std::shared_ptr<std::string> buffer_p(new std::string("123456"));

    EXPECT_EQ(PROFILING_SUCCESS, uploader->Init());
    uploader->queue_->Push(buffer_p);
    uploader->queue_->Push(buffer_p);

    EXPECT_EQ(PROFILING_SUCCESS, uploader->Start());

    uploader->Flush();
    EXPECT_EQ(PROFILING_SUCCESS, uploader->Stop(false));
    uploader.reset();
}

TEST_F(UPLOADER_TEST, Flush) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::transport::Uploader> uploader(
        new analysis::dvvp::transport::Uploader(_transport));

    MOCKER_CPP(&analysis::dvvp::transport::UploaderQueue::size)
        .stubs()
        .will(returnValue((unsigned long)1))
        .then(returnValue((unsigned long)0));
    uploader->Init();
    uploader->Flush();
    uploader->Flush();
}
