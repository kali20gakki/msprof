#include <errno.h>
#include <strings.h>
#include <string.h>
#include <memory>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "prof_channel.h"
#include "uploader_mgr.h"

using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Common::Statistics;
using namespace Analysis::Dvvp::MsprofErrMgr;
#define MAX_BUFFER_SIZE (1024 * 1024 * 2)
#define MAX_THRESHOLD_SIZE (MAX_BUFFER_SIZE * 0.8)
class TRANSPORT_PROF_CHANNELREADER_UTEST: public testing::Test {
protected:
    virtual void SetUp() {
        std::shared_ptr<analysis::dvvp::message::JobContext> jobCtx(
            new analysis::dvvp::message::JobContext);
        _job_ctx = jobCtx;
    }
    virtual void TearDown() {
    }
public:
    std::shared_ptr<analysis::dvvp::message::JobContext> _job_ctx;
};

TEST_F(TRANSPORT_PROF_CHANNELREADER_UTEST, Execute) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::transport::ChannelReader> reader(
        new analysis::dvvp::transport::ChannelReader(
            0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU, "data/ts.12.0.0",
            _job_ctx));

    reader->Init();
    MOCKER(&analysis::dvvp::driver::DrvChannelRead)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(64));

    MOCKER_CPP(&analysis::dvvp::transport::UploaderMgr::UploadData)
        .stubs()
        .will(returnValue(PROFILING_FAILED));

    reader->dataSize_ = MAX_THRESHOLD_SIZE + 1;

    EXPECT_EQ(PROFILING_SUCCESS, reader->Execute());
    EXPECT_EQ(PROFILING_SUCCESS, reader->Execute());
    EXPECT_EQ(PROFILING_SUCCESS, reader->Execute());

    reader->SetChannelStopped();
    EXPECT_EQ(PROFILING_SUCCESS, reader->Execute());

    reader.reset();
}

TEST_F(TRANSPORT_PROF_CHANNELREADER_UTEST, HashId) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::transport::ChannelReader> reader(
        new analysis::dvvp::transport::ChannelReader(
            0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU, "data/ts.12.0.0",
            _job_ctx));
    reader->Init();

    EXPECT_NE((size_t)0, reader->HashId());

    reader.reset();
}

////////////////////////////////////////////////////////////////////
TEST_F(TRANSPORT_PROF_CHANNELREADER_UTEST, SetChannelStopped) {
    GlobalMockObject::verify();

    MOCKER_CPP(&analysis::dvvp::transport::ChannelReader::UploadData)
        .stubs();

    std::shared_ptr<analysis::dvvp::transport::ChannelReader> reader(
        new analysis::dvvp::transport::ChannelReader(
            0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU, "data/ts.12.0.0",
            _job_ctx));
    EXPECT_EQ(PROFILING_SUCCESS, reader->Init());
    reader->dataSize_ = 10;
    reader->SetChannelStopped();
    reader.reset();
}

TEST_F(TRANSPORT_PROF_CHANNELREADER_UTEST, UploadData) {
    GlobalMockObject::verify();

    MOCKER_CPP(&analysis::dvvp::transport::ChannelReader::UploadData)
        .stubs();

    std::shared_ptr<analysis::dvvp::transport::ChannelReader> reader(
        new analysis::dvvp::transport::ChannelReader(
            0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU, "data/ts.12.0.0",
            _job_ctx));
    EXPECT_EQ(PROFILING_SUCCESS, reader->Init());
    reader->dataSize_ = 0;
    reader->UploadData();

    reader->dataSize_ = 10;
    reader->UploadData();
}

TEST_F(TRANSPORT_PROF_CHANNELREADER_UTEST, FlushDrvBuff) {
    GlobalMockObject::verify();

    MOCKER_CPP(&analysis::dvvp::transport::ChannelReader::UploadData)
        .stubs();

    MOCKER(&analysis::dvvp::driver::DrvProfFlush)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    
    std::shared_ptr<analysis::dvvp::transport::ChannelReader> reader(
        new analysis::dvvp::transport::ChannelReader(
            0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU, "data/ts.12.0.0",
            _job_ctx));
    EXPECT_EQ(PROFILING_SUCCESS, reader->Init());
    reader->FlushDrvBuff();
    reader->channelId_ = analysis::dvvp::driver::PROF_CHANNEL_HWTS_LOG;
    reader->FlushDrvBuff();
    reader->FlushDrvBuff();
    reader->needWait_ = true;
    reader->CheckIfSendFlush(0);
}

class TRANSPORT_PROF_CHANNELPOLL_UTEST: public testing::Test {
protected:
    virtual void SetUp() {
        _job_ctx = std::make_shared<analysis::dvvp::message::JobContext>();
    }
    virtual void TearDown() {
    }
public:
    std::shared_ptr<analysis::dvvp::message::JobContext> _job_ctx;
};

TEST_F(TRANSPORT_PROF_CHANNELPOLL_UTEST, AddReader) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::transport::ChannelPoll> poll(
        new analysis::dvvp::transport::ChannelPoll());

    std::shared_ptr<analysis::dvvp::transport::ChannelReader> reader(
        new analysis::dvvp::transport::ChannelReader(
            0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU, "data/ts.12.0.0",
            _job_ctx));
    reader->Init();

    EXPECT_EQ(PROFILING_SUCCESS, poll->AddReader(0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU, reader));
    EXPECT_NE(nullptr, poll->GetReader(0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU).get());

    poll.reset();
}

TEST_F(TRANSPORT_PROF_CHANNELPOLL_UTEST, RemoveReader) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::transport::ChannelPoll> poll(
        new analysis::dvvp::transport::ChannelPoll());

    std::shared_ptr<analysis::dvvp::transport::ChannelReader> reader(
        new analysis::dvvp::transport::ChannelReader(
            0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU, "data/ts.12.0.0",
            _job_ctx));
    reader->Init();

    EXPECT_EQ(PROFILING_SUCCESS, poll->AddReader(0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU, reader));
    EXPECT_NE(nullptr, poll->GetReader(0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU).get());

    EXPECT_EQ(PROFILING_SUCCESS, poll->RemoveReader(0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU));
    EXPECT_EQ(nullptr, poll->GetReader(0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU).get());

    poll.reset();
}

TEST_F(TRANSPORT_PROF_CHANNELPOLL_UTEST, DispatchChannel) {
    GlobalMockObject::verify();

    //not start
    std::shared_ptr<analysis::dvvp::transport::ChannelPoll> poll(
        new analysis::dvvp::transport::ChannelPoll());

    EXPECT_EQ(PROFILING_FAILED, poll->DispatchChannel(0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU));

    MOCKER(&analysis::dvvp::driver::DrvGetDevNum)
        .stubs()
        .will(returnValue(1));
    //post start
    poll->Start();
    EXPECT_EQ(PROFILING_FAILED, poll->DispatchChannel(0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU));

    //find reader
    std::shared_ptr<analysis::dvvp::transport::ChannelReader> reader(
        new analysis::dvvp::transport::ChannelReader(
            0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU, "data/ts.12.0.0",
            _job_ctx));
    reader->Init();
    poll->AddReader(0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU, reader);

    EXPECT_EQ(PROFILING_SUCCESS, poll->DispatchChannel(0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU));

    poll.reset();
}

TEST_F(TRANSPORT_PROF_CHANNELPOLL_UTEST, FlushAllChannels) {
    GlobalMockObject::verify();

        //not start
    std::shared_ptr<analysis::dvvp::transport::ChannelPoll> poll(
        new analysis::dvvp::transport::ChannelPoll());

    MOCKER(&analysis::dvvp::driver::DrvGetDevNum)
        .stubs()
        .will(returnValue(1));
    //post start
    poll->Start();

    std::shared_ptr<analysis::dvvp::transport::ChannelReader> reader(
        new analysis::dvvp::transport::ChannelReader(
            0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU, "data/ts.12.0.0",
            _job_ctx));
    reader->Init();

    EXPECT_EQ(PROFILING_SUCCESS, poll->AddReader(0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU, reader));
    EXPECT_NE(nullptr, poll->GetReader(0, analysis::dvvp::driver::PROF_CHANNEL_TS_CPU).get());

    poll->FlushAllChannels();

    poll.reset();
}

TEST_F(TRANSPORT_PROF_CHANNELPOLL_UTEST, start) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::transport::ChannelPoll> poll(
        new analysis::dvvp::transport::ChannelPoll());

    EXPECT_EQ(PROFILING_SUCCESS, poll->Start());
    poll->Start();
    EXPECT_TRUE(poll->isStarted_);

    poll.reset();
}

TEST_F(TRANSPORT_PROF_CHANNELPOLL_UTEST, stop) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::transport::ChannelPoll> poll(
        new analysis::dvvp::transport::ChannelPoll());

    poll->Start();
    EXPECT_EQ(PROFILING_SUCCESS, poll->Stop());
    EXPECT_FALSE(poll->isStarted_);

    poll.reset();
}

TEST_F(TRANSPORT_PROF_CHANNELPOLL_UTEST, run) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::transport::ChannelPoll> poll(
        new analysis::dvvp::transport::ChannelPoll());
    EXPECT_NE(nullptr, poll);

    poll->isStarted_= true;
    MOCKER(&analysis::dvvp::driver::DrvChannelPoll)
        .stubs()
        .will(returnValue(1))
        .then(returnValue(-4))
        .then(returnValue(PROFILING_FAILED));

   MOCKER_CPP(&analysis::dvvp::transport::ChannelPoll::DispatchChannel)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    auto errorContext = MsprofErrorManager::instance()->GetErrorManagerContext();
    poll->Run(errorContext);
    poll->isStarted_=false;
    poll.reset();
}

TEST_F(TRANSPORT_PROF_CHANNELPOLL_UTEST, run_quit) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::transport::ChannelPoll> poll(
        new analysis::dvvp::transport::ChannelPoll());

    MOCKER(&analysis::dvvp::driver::DrvChannelPoll)
        .stubs()
        .will(returnValue(0));

    EXPECT_EQ(PROFILING_SUCCESS, poll->Start());
    EXPECT_EQ(PROFILING_SUCCESS, poll->Stop());

    poll.reset();
}

