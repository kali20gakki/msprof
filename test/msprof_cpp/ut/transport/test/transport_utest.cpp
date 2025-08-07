#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <memory>
#include <fstream>
#include "transport/file_transport.h"
#include "errno/error_code.h"
#include "utils/utils.h"
#include "param_validation.h"
#include "config/config.h"


using namespace analysis::dvvp::transport;
using namespace analysis::dvvp::common::error;
class MyTransport : public ITransport {
public:
    MyTransport() {}
    virtual ~MyTransport() {}
public:
    virtual int SendBuffer(const void * buffer, int length) {
        return length;
    }
    virtual int SendBuffer(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq)
    {
        return PROFILING_SUCCESS;
    }
    virtual void WriteDone() {
    }
    virtual int CloseSession() {
        return PROFILING_SUCCESS;
    }
};

class TRANSPORT_TRANSPORT_ITRANSPORT_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
private:
};

TEST_F(TRANSPORT_TRANSPORT_ITRANSPORT_TEST, SendFiles) {
    GlobalMockObject::verify();

    std::string root_dir = "/send_files_root";

    std::vector<std::string> files;
    files.push_back(root_dir + "/a.txt");
    files.push_back(root_dir + "/b.txt");

    MOCKER(analysis::dvvp::common::utils::Utils::GetFiles)
        .stubs()
        .with(any(), any(), outBound(files));

    std::shared_ptr<ITransport> trans(new MyTransport());

    //relative fail 
    EXPECT_EQ(PROFILING_SUCCESS, trans->SendFiles(root_dir + "/extend", ""));

    //relative success
    EXPECT_EQ(PROFILING_SUCCESS, trans->SendFiles(root_dir, ""));
}

TEST_F(TRANSPORT_TRANSPORT_ITRANSPORT_TEST, SendFile) {
    GlobalMockObject::verify();

    std::string path = "./non_exist/file";

    std::shared_ptr<ITransport> trans(new MyTransport());

    //file size fail 
    EXPECT_NE(PROFILING_SUCCESS, trans->SendFile(path, "non_exist/file", ""));

    //open fail
    MOCKER(analysis::dvvp::common::utils::Utils::GetFileSize)
        .stubs()
        .will(returnValue((long long)1024));

    EXPECT_NE(PROFILING_SUCCESS, trans->SendFile(path, "non_exist/file", ""));

    //send exist file
    GlobalMockObject::verify();

    MOCKER_CPP_VIRTUAL(trans.get() , &ITransport::SendFileChunk)
              .stubs()
              .will(returnValue(PROFILING_FAILED))
              .then(returnValue(PROFILING_SUCCESS));

    path = "./TRANSPORT_TRANSPORT_ITRANSPORT_TEST-SendFile-file_existing";
    std::ofstream out;
    out.open(path.c_str(), std::ios::app | std::ios::out);
    out << "this is send file" << std::flush << std::endl;
    out.close();

    //relative success
    EXPECT_EQ(PROFILING_FAILED, trans->SendFile(path, "TRANSPORT_TRANSPORT_ITRANSPORT_TEST-SendFile-file_existing", ""));
    EXPECT_EQ(PROFILING_SUCCESS, trans->SendFile(path, "TRANSPORT_TRANSPORT_ITRANSPORT_TEST-SendFile-file_existing", ""));
    std::move(path.c_str());
}

////////////////////////////////////////FILETransport/////////////////////////////////////////

TEST_F(TRANSPORT_TRANSPORT_ITRANSPORT_TEST, SendBuffer_without_protobuf) {
    GlobalMockObject::verify();

    std::shared_ptr<FILETransport> trans;
    trans = std::make_shared<FILETransport>("/tmp", "200MB");
    std::shared_ptr<PerfCount> perfCount;
    perfCount = std::make_shared<PerfCount>("test");
    trans->perfCount_ = perfCount;
    trans->Init();

    // test the normal procests, fileChunkReq datamodule is PROFILING_IS_FROM_DEVICE
    std::shared_ptr<analysis::dvvp::ProfileFileChunk> message(
        new analysis::dvvp::ProfileFileChunk());
    message->extraInfo = "null.64";
    message->chunkModule = analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_FROM_MSPROF_DEVICE;

    MOCKER_CPP(&FileSlice::SaveDataToLocalFiles)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    EXPECT_EQ(PROFILING_FAILED, trans->SendBuffer(nullptr));
    EXPECT_EQ(PROFILING_FAILED, trans->SendBuffer(message));
    message->extraInfo = "null.0";
    EXPECT_EQ(PROFILING_SUCCESS, trans->SendBuffer(message));

    // test fileChunkReq datamodule is PROFILING_DEFAULT_DATA_MODULE
    message->chunkModule  = analysis::dvvp::common::config::FileChunkDataModule::PROFILING_DEFAULT_DATA_MODULE;
    EXPECT_EQ(PROFILING_SUCCESS, trans->SendBuffer(message));
}

TEST_F(TRANSPORT_TRANSPORT_ITRANSPORT_TEST, SendBuffer) {
    GlobalMockObject::verify();

    std::shared_ptr<FILETransport> trans(new FILETransport("/tmp", "200MB"));
    std::shared_ptr<PerfCount> perfCount(new PerfCount("test"));
    trans->perfCount_ = perfCount;
    trans->Init();

    std::string buff = "test SendBuffer";
    // test length < 0 , retLengthError = 0;
    EXPECT_EQ(0, trans->SendBuffer(buff.c_str(), 0));

    // test the normal procests, fileChunkReq datamodule is PROFILING_IS_FROM_DEVICE
    std::shared_ptr<analysis::dvvp::ProfileFileChunk> message(
        new analysis::dvvp::ProfileFileChunk());
    analysis::dvvp::message::JobContext job_ctx;
    job_ctx.dev_id = "0";
    job_ctx.job_id = "123456789";
    message->extraInfo = Utils::PackDotInfo("123456789", "0");
    message->chunkModule = analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_FROM_DEVICE;

    MOCKER_CPP(&analysis::dvvp::transport::FileSlice::SaveDataToLocalFiles)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, trans->SendBuffer(nullptr));
    EXPECT_EQ(PROFILING_SUCCESS, trans->SendBuffer(message));

    message->chunkModule = analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_FROM_MSPROF;

    // test fileChunkReq datamodule is PROFILING_IS_FROM_DEVICE
    MOCKER_CPP(&FILETransport::UpdateFileName)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    
    MOCKER(analysis::dvvp::common::utils::Utils::CheckStringIsNonNegativeIntNum)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));

    EXPECT_EQ(0, trans->SendBuffer(message));

    // test FromString
    MOCKER_CPP(&analysis::dvvp::message::ProfileParams::FromString)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));

    EXPECT_EQ(0, trans->SendBuffer(message));
}

TEST_F(TRANSPORT_TRANSPORT_ITRANSPORT_TEST, SendBufferForMsprof) {
    GlobalMockObject::verify();

    std::shared_ptr<FILETransport> trans(new FILETransport("/tmp", "200MB"));
    std::shared_ptr<PerfCount> perfCount(new PerfCount("test"));
    trans->perfCount_ = perfCount;
    trans->SetAbility(true);
    trans->Init();

    std::shared_ptr<analysis::dvvp::ProfileFileChunk> message(
        new analysis::dvvp::ProfileFileChunk());
    analysis::dvvp::message::JobContext job_ctx;
    job_ctx.dev_id = "0";
    job_ctx.job_id = "123456789";
    message->fileName = Utils::PackDotInfo("data/test", "");
    message->extraInfo = Utils::PackDotInfo("123456789", "0");
    message->chunkModule = analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_FROM_MSPROF;
    // std::string buff = analysis::dvvp::message::EncodeMessage(message);

    // test fileChunkReq datamodule is PROFILING_IS_FROM_DEVICE
    MOCKER(analysis::dvvp::common::utils::Utils::CheckStringIsNonNegativeIntNum)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(true));
    MOCKER_CPP(&FileSlice::SaveDataToLocalFiles)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(0, trans->SendBuffer(message));
}

