#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include <memory>
#include <fstream>
#include "transport/hdc/hdc_transport.h"
#include "transport/file_transport.h"
#include "errno/error_code.h"
#include "utils/utils.h"
#include "param_validation.h"
#include "config/config.h"
#include "message/codec.h"
#include "adx/inc/adx_prof_api.h"

using namespace analysis::dvvp::transport;
using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Adx;
class MyTransport : public ITransport {
public:
    MyTransport() {}
    virtual ~MyTransport() {}
public:
    virtual int SendBuffer(const void * buffer, int length) {
        return length;
    }
    virtual int RecvPacket(struct tlv_req ** packet) {
        return PROFILING_FAILED;
    }
    virtual void WriteDone() {
    }
    virtual void DestroyPacket(struct tlv_req * packet) {

    }
    virtual int CloseSession() {
        return PROFILING_SUCCESS;
    }
    virtual int DoSendBuffer(IdeBuffT out, int outLen)
    {
        return IDE_DAEMON_OK;
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
}

TEST_F(TRANSPORT_TRANSPORT_ITRANSPORT_TEST, SendFileChunk) {
    GlobalMockObject::verify();

    std::shared_ptr<ITransport> trans(new MyTransport());
    MOCKER_CPP_VIRTUAL(trans.get() , &ITransport::SendBuffer)
              .stubs()
              .will(returnValue(PROFILING_FAILED));

    FileChunk chunk;
    chunk.relativeFileName = "my_file.txt";
    chunk.dataBuf = new unsigned char[4096];
    chunk.bufLen = 4096;
    chunk.offset = 0;
    chunk.isLastChunk = 0;

    //Failed
    EXPECT_EQ(PROFILING_FAILED, trans->SendFileChunk("1231", chunk));

    GlobalMockObject::verify();
    EXPECT_EQ(PROFILING_SUCCESS, trans->SendFileChunk("12313", chunk));

    delete [] chunk.dataBuf;
}

///////////////////////////////////////////////////////////////////
class TRANSPORT_TRANSPORT_HDCTRANSPORT_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
private:
    std::string _log_file;
};

TEST_F(TRANSPORT_TRANSPORT_HDCTRANSPORT_TEST, HDCTransport) {
    GlobalMockObject::verify();

    HDC_SESSION session = (HDC_SESSION)0x12345678;

    std::shared_ptr<ITransport> trans(new HDCTransport(session));
    EXPECT_NE(nullptr, trans);
    trans.reset();
}

TEST_F(TRANSPORT_TRANSPORT_HDCTRANSPORT_TEST, SendBuffer) {
    GlobalMockObject::verify();

    HDC_SESSION session = (HDC_SESSION)0x12345678;

    std::shared_ptr<ITransport> trans(new HDCTransport(session));
    std::shared_ptr<PerfCount> perfCount(new PerfCount("test"));
    trans->perfCount_ = perfCount;

    void* out = (void*)0x12345678;
    MOCKER(IdeCreatePacket)
        .stubs()
        .with(any(), any(), any(), outBoundP(&out), any())
        .will(returnValue(IDE_DAEMON_ERROR))
        .then(returnValue(IDE_DAEMON_OK));

    MOCKER(HdcWrite)
        .stubs()
        .will(returnValue(IDE_DAEMON_ERROR))
        .then(returnValue(IDE_DAEMON_OK));

    void * buff = (void *)0x12345678;
    int length = 10;
    EXPECT_EQ(PROFILING_FAILED, trans->SendBuffer(buff, length));
    EXPECT_EQ(PROFILING_FAILED, trans->SendBuffer(buff, length));
    EXPECT_EQ(length, trans->SendBuffer(buff, length));
}

TEST_F(TRANSPORT_TRANSPORT_HDCTRANSPORT_TEST, RecvPacket) {
    GlobalMockObject::verify();

    HDC_SESSION session = (HDC_SESSION)0x12345678;
    int buf_len = sizeof(struct tlv_req);
    struct tlv_req * packet = NULL;

    std::shared_ptr<HDCTransport> trans(new HDCTransport(session));

    char containerNotSupport[] = "MESSAGE_CONTAINER_NO_SUPPORT";
    CONST_CHAR_PTR containerPtr = &containerNotSupport[0];
    char invalidMsg[sizeof(struct tlv_req)] = {0};
    CONST_CHAR_PTR invalidMsgPtr = &invalidMsg[0];

    MOCKER(HdcRead)
        .stubs()
        .with(any(), outBoundP((void **)&invalidMsgPtr), outBoundP(&buf_len))
        .will(returnValue(IDE_DAEMON_ERROR))
        .then(returnValue(IDE_DAEMON_OK));

    EXPECT_EQ(PROFILING_FAILED, trans->RecvPacket(&packet));
    EXPECT_EQ(buf_len, trans->RecvPacket(&packet));
}

TEST_F(TRANSPORT_TRANSPORT_HDCTRANSPORT_TEST, DestroyPacket) {
    GlobalMockObject::verify();

    HDC_SESSION session = (HDC_SESSION)0x12345678;

    std::shared_ptr<HDCTransport> trans(new HDCTransport(session));
    EXPECT_NE(nullptr, trans);

    struct tlv_req * packet = (struct tlv_req *)0x12345678;
    trans->DestroyPacket(NULL);
    trans->DestroyPacket(packet);
}

TEST_F(TRANSPORT_TRANSPORT_HDCTRANSPORT_TEST, CloseSession) {
    GlobalMockObject::verify();

    HDC_SESSION session = (HDC_SESSION)0x12345678;

    std::shared_ptr<ITransport> trans_s(new HDCTransport(session));
    EXPECT_EQ(PROFILING_SUCCESS, trans_s->CloseSession());
    trans_s.reset();

    session = (HDC_SESSION)0x12345678;
    std::shared_ptr<ITransport> trans_c(new HDCTransport(session, true));
    EXPECT_EQ(PROFILING_SUCCESS, trans_c->CloseSession());
    trans_c.reset();
}


///////////////////////////////////////////////////////////////////
class TRANSPORT_TRANSPORT_TRANSPORTFACTORY_TEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
private:
    std::string _log_file;
};

TEST_F(TRANSPORT_TRANSPORT_TRANSPORTFACTORY_TEST, TransportFactory) {
    GlobalMockObject::verify();

    std::shared_ptr<TransportFactory> fac(new TransportFactory());
    EXPECT_NE(nullptr, fac);
    fac.reset();
}



TEST_F(TRANSPORT_TRANSPORT_TRANSPORTFACTORY_TEST, create_hdc_transport_session) {
    GlobalMockObject::verify();

    EXPECT_TRUE(HDCTransportFactory().CreateHdcTransport(nullptr).get() == nullptr);

    HDC_SESSION session = (HDC_SESSION)0x12345678;

    auto trans = HDCTransportFactory().CreateHdcTransport(session);
    EXPECT_NE((ITransport*)NULL, trans.get());

    MOCKER_CPP(&analysis::dvvp::common::validation::ParamValidation::CheckDeviceIdIsValid)
        .stubs()
        .will(returnValue(false));
    trans = HDCTransportFactory().CreateHdcTransport(session);
    EXPECT_EQ((ITransport*)NULL, trans.get());
}

TEST_F(TRANSPORT_TRANSPORT_TRANSPORTFACTORY_TEST, create_hdc_transport_client) {
    GlobalMockObject::verify();

    HDC_CLIENT client = (HDC_CLIENT)0x12345678;
    int dev_id = 0;

    MOCKER(HdcSessionConnect)
        .stubs()
        .will(returnValue(IDE_DAEMON_ERROR))
        .then(returnValue(IDE_DAEMON_OK));

    EXPECT_EQ((ITransport*)NULL, HDCTransportFactory().CreateHdcTransport(client, dev_id).get());
    EXPECT_NE((ITransport*)NULL, HDCTransportFactory().CreateHdcTransport(client, dev_id).get());
}

TEST_F(TRANSPORT_TRANSPORT_TRANSPORTFACTORY_TEST, create_hdc_transport_getdevid) {
    GlobalMockObject::verify();

    MOCKER(IdeGetDevIdBySession)
        .stubs()
        .will(returnValue(IDE_DAEMON_ERROR));

    HDC_SESSION session = (HDC_SESSION)0x12345678;

    auto trans = HDCTransportFactory().CreateHdcTransport(session);
    EXPECT_EQ((ITransport*)NULL, trans.get());
}

////////////////////////////////////////FILETransport/////////////////////////////////////////
TEST_F(TRANSPORT_TRANSPORT_ITRANSPORT_TEST, FILETransport) {
    GlobalMockObject::verify();

    struct tlv_req packet;
    std::shared_ptr<ITransport> trans(new FILETransport("/tmp", "200MB"));

    EXPECT_EQ(PROFILING_SUCCESS, trans->CloseSession());
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
    std::shared_ptr<analysis::dvvp::proto::FileChunkReq> message(
        new analysis::dvvp::proto::FileChunkReq());
    analysis::dvvp::message::JobContext job_ctx;
    job_ctx.dev_id = "0";
    job_ctx.job_id = "123456789";
    message->mutable_hdr()->set_job_ctx(job_ctx.ToString());
    message->set_datamodule(analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_FROM_DEVICE);
    buff = analysis::dvvp::message::EncodeMessage(message);

    MOCKER_CPP(&analysis::dvvp::transport::FileSlice::SaveDataToLocalFiles)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_FAILED));

    EXPECT_EQ(0, trans->SendBuffer(nullptr, buff.size()));
    EXPECT_EQ(buff.size(), trans->SendBuffer(buff.c_str(), buff.size()));

    message->set_datamodule(analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_FROM_MSPROF);
    buff = analysis::dvvp::message::EncodeMessage(message);

    // test fileChunkReq datamodule is PROFILING_IS_FROM_DEVICE
    MOCKER(analysis::dvvp::common::utils::Utils::CheckStringIsNonNegativeIntNum)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));

    EXPECT_EQ(0, trans->SendBuffer(buff.c_str(), buff.size()));

    // test FromString
    MOCKER_CPP(&analysis::dvvp::message::ProfileParams::FromString)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    EXPECT_EQ(0, trans->SendBuffer(buff.c_str(), buff.size()));

    // test fileChunkReq is nullptr, retLengthError = 0;
    std::shared_ptr<google::protobuf::Message> fake_message = nullptr;
    MOCKER(analysis::dvvp::message::DecodeMessage)
        .stubs()
        .will(returnValue(fake_message));

    EXPECT_EQ(0, trans->SendBuffer(buff.c_str(), buff.size()));
}

TEST_F(TRANSPORT_TRANSPORT_TRANSPORTFACTORY_TEST, CreateFileTransport) {
    GlobalMockObject::verify();

    std::string storageDir = "";
    bool needSlice = true;
    std::string fileNameType = "EP";
    auto trans = FileTransportFactory().CreateFileTransport(storageDir, "200MB", needSlice);
    EXPECT_EQ((ITransport*)NULL, trans.get());
    storageDir = "/tmp";
    auto trans_ = FileTransportFactory().CreateFileTransport(storageDir, "200MB", needSlice);
    EXPECT_NE((ITransport*)NULL, trans_.get());
}

TEST_F(TRANSPORT_TRANSPORT_ITRANSPORT_TEST, SendBufferForMsprof) {
    GlobalMockObject::verify();

    std::shared_ptr<FILETransport> trans(new FILETransport("/tmp", "200MB"));
    std::shared_ptr<PerfCount> perfCount(new PerfCount("test"));
    trans->perfCount_ = perfCount;
    trans->SetAbility(true);
    trans->Init();

    std::shared_ptr<analysis::dvvp::proto::FileChunkReq> message(
        new analysis::dvvp::proto::FileChunkReq());
    analysis::dvvp::message::JobContext job_ctx;
    job_ctx.dev_id = "0";
    job_ctx.job_id = "123456789";
    message->mutable_hdr()->set_job_ctx(job_ctx.ToString());
    message->set_datamodule(analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_FROM_MSPROF);
    message->set_filename("data/test");
    std::string buff = analysis::dvvp::message::EncodeMessage(message);

    // test fileChunkReq datamodule is PROFILING_IS_FROM_DEVICE
    MOCKER(analysis::dvvp::common::utils::Utils::CheckStringIsNonNegativeIntNum)
        .stubs()
        .will(returnValue(true))
        .then(returnValue(true));

    EXPECT_EQ(0, trans->SendBuffer(buff.c_str(), buff.size()));
}

TEST_F(TRANSPORT_TRANSPORT_ITRANSPORT_TEST, AdxHdcServerAccept) {
    GlobalMockObject::verify();

    EXPECT_EQ(nullptr, AdxHdcServerAccept(nullptr));
}

TEST_F(TRANSPORT_TRANSPORT_ITRANSPORT_TEST, AdxHdcServerDestroy) {
    GlobalMockObject::verify();

    AdxHdcServerDestroy(nullptr);
}

TEST_F(TRANSPORT_TRANSPORT_ITRANSPORT_TEST, AdxHalHdcSessionConnect) {
    GlobalMockObject::verify();

    EXPECT_EQ(DRV_ERROR_NONE, AdxHalHdcSessionConnect(0, 0, 0, nullptr, nullptr));
}
