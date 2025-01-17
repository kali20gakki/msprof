#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "securec.h"
#include "message/codec.h"
#include "errno/error_code.h"
#include "prof_manager.h"
#include "hdc/device_transport.h"
#include "data_handle.h"
#include "mmpa_api.h"

using namespace analysis::dvvp::host;
using namespace analysis::dvvp::transport;
using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::MsprofErrMgr;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;

class HOST_PROF_DEVICE_TRANSPORT_UTEST: public testing::Test {
protected:

    virtual void SetUp() {
    }
    virtual void TearDown() {

    }

public:
    HDC_CLIENT client = (HDC_CLIENT)0x12345678;
    std::string dev_id = "0";
    std::shared_ptr<DeviceTransport> dev_tran;
    std::shared_ptr<analysis::dvvp::transport::AdxTransport> data_tran;
    std::shared_ptr<analysis::dvvp::transport::AdxTransport> ctrl_tran;
};

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, CreateCoparamsnn){
    GlobalMockObject::verify();

    dev_tran = std::make_shared<DeviceTransport>(client, "-1", "123", "def_mode");

    std::shared_ptr<analysis::dvvp::proto::DataChannelHandshake> data_message(
            new analysis::dvvp::proto::DataChannelHandshake());

    data_tran = std::make_shared<HDCTransport>(client);
    std::shared_ptr<AdxTransport> fake_trans;

    HDC_SESSION session = (HDC_SESSION)0x12345678;
    auto transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
        new analysis::dvvp::transport::HDCTransport(session));

    MOCKER_CPP(&analysis::dvvp::transport::HDCTransportFactory::CreateHdcTransport,
                        std::shared_ptr<AdxTransport>(HDCTransportFactory::*)(HDC_CLIENT client, int dev_id))
        .stubs()
        .with(any(), any())
        .will(returnValue(fake_trans))
        .then(returnValue(data_tran));

    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");
    //tran empty
    EXPECT_EQ(fake_trans, dev_tran->CreateConn());
    EXPECT_EQ(data_tran, dev_tran->CreateConn());
    dev_tran->dataTran_ = data_tran;
    MOCKER(analysis::dvvp::transport::SendBufferWithFixedLength)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(-1))
        .then(returnValue(-1))
        .then(returnValue(-1))
        .then(returnValue(-1))
        .then(returnValue(0));  // retry 5 times
}


static int _drv_get_dev_ids_suc(int num_devices, std::vector<int> & dev_ids) {
    dev_ids.push_back(0);
    return PROFILING_SUCCESS;
}

static int _drv_get_dev_ids_fail(int num_devices, std::vector<int> & dev_ids) {
    return PROFILING_FAILED;
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, init_ctrl_tran) {
    
    GlobalMockObject::verify();

    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");

    std::shared_ptr<AdxTransport> fake_tran;
    ctrl_tran = std::make_shared<HDCTransport>(client);

    MOCKER_CPP(&analysis::dvvp::transport::DeviceTransport::HandleShake)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS))
        .then(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, dev_tran->Init());
    EXPECT_EQ(PROFILING_FAILED, dev_tran->Init());
    MOCKER(&analysis::dvvp::driver::DrvGetDevNum)
        .stubs()
        .will(returnValue(0))
        .then(returnValue(1));

    MOCKER(analysis::dvvp::driver::DrvGetDevIds)
        .stubs()
        .will(invoke(_drv_get_dev_ids_fail))
        .then(invoke(_drv_get_dev_ids_suc));
    //tran empty

    auto entry = analysis::dvvp::transport::DevTransMgr::instance();
    EXPECT_EQ(PROFILING_FAILED, entry->Init("123", 0, "def_mode"));
    EXPECT_EQ(PROFILING_FAILED, entry->Init("123", 0, "def_mode"));
    EXPECT_EQ(PROFILING_FAILED, entry->Init("123", 0x12345678, "def_mode"));
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, DoInit) {
    MOCKER_CPP(&analysis::dvvp::transport::DeviceTransport::Init)
        .stubs()
        .will(returnValue(-1))
        .then(returnValue(-1))
        .then(returnValue(0));

    MOCKER(&MmCreateTaskWithThreadAttr)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    std::vector<int> devIds;
    devIds.push_back(0);

    auto entry = analysis::dvvp::transport::DevTransMgr::instance();
    EXPECT_EQ(PROFILING_FAILED, entry->Init("123", 0, "def_mode"));
    EXPECT_EQ(PROFILING_FAILED, entry->Init("123", 0, "def_mode"));
    // EXPECT_EQ(PROFILING_SUCCESS, entry->DoInit(devIds));

}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, init_data_tran) {
    GlobalMockObject::verify();

    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");

    ctrl_tran = std::make_shared<HDCTransport>(client);
    data_tran = std::make_shared<HDCTransport>(client);

    MOCKER_CPP(&analysis::dvvp::transport::DeviceTransport::CreateConn)
        .stubs()
        .will(returnValue(ctrl_tran))
        .then(returnValue(data_tran));
    MOCKER_CPP(&analysis::dvvp::transport::DeviceTransport::HandleShake)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    //success
    EXPECT_EQ(PROFILING_SUCCESS, dev_tran->Init());
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, UninitWillCloseDataTranWhenDataInitialized)
{
    GlobalMockObject::verify();
    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");

    HDC_SESSION session = (HDC_SESSION)0x12345678;
    dev_tran->dataInitialized_ = true;
    dev_tran->Uinit();
    EXPECT_EQ(true, dev_tran->dataInitialized_);
    EXPECT_EQ(nullptr, dev_tran->dataTran_);

    dev_tran->dataInitialized_ = true;
    dev_tran->dataTran_ = std::make_shared<HDCTransport>(session);
    dev_tran->Uinit();
    EXPECT_EQ(false, dev_tran->dataInitialized_);
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, UninitWillCloseCtrlTranWhenCtrlInitialized)
{
    GlobalMockObject::verify();
    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");

    HDC_SESSION session = (HDC_SESSION)0x12345678;
    dev_tran->ctrlInitialized_ = true;
    dev_tran->Uinit();
    EXPECT_EQ(true, dev_tran->ctrlInitialized_);
    EXPECT_EQ(nullptr, dev_tran->ctrlTran_);

    dev_tran->ctrlInitialized_ = true;
    dev_tran->ctrlTran_ = std::make_shared<HDCTransport>(session);
    dev_tran->Uinit();
    EXPECT_EQ(false, dev_tran->ctrlInitialized_);
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, run) {
    GlobalMockObject::verify();

    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");

    data_tran = std::make_shared<HDCTransport>(client);
    //dataInitialized_ false
    auto errorContext = MsprofErrorManager::instance()->GetErrorManagerContext();
    dev_tran->Run(errorContext);    
    EXPECT_FALSE(dev_tran->dataInitialized_);

    HDC_SESSION session = (HDC_SESSION)0x12345678;
    auto transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
        new analysis::dvvp::transport::HDCTransport(session));

    struct tlv_req* packet = (struct tlv_req *)new char[sizeof(struct tlv_req)];
    MOCKER_CPP_VIRTUAL(transport.get(), &analysis::dvvp::transport::HDCTransport::RecvPacket)
        .stubs()
        .with(outBoundP(&packet))
        .will(returnValue(-1))
        .then(returnValue(0));

    MOCKER_CPP_VIRTUAL(transport.get(), &analysis::dvvp::transport::HDCTransport::DestroyPacket)
        .stubs();

    MOCKER_CPP(&analysis::dvvp::common::thread::Thread::IsQuit)
        .stubs()
        .will(returnValue(true));
    //RecvPacket failed
    dev_tran->dataInitialized_ = true;
    dev_tran->dataTran_ = data_tran;
    dev_tran->Run(errorContext);
    EXPECT_FALSE(dev_tran->dataInitialized_);
     
    //ReceiveStreamData faield
    dev_tran->dataInitialized_ = true;
    dev_tran->dataTran_ = data_tran;
    dev_tran->Run(errorContext);
    EXPECT_FALSE(dev_tran->dataInitialized_);
     
    //success
    dev_tran->dataInitialized_ = true;
    dev_tran->dataTran_ = data_tran;
    dev_tran->Run(errorContext);
    EXPECT_FALSE(dev_tran->dataInitialized_);
    
    delete [] ((char*)packet);
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, GetDevTransportWillReturnNullptrWhenInputInvalidKey)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::transport::DevTransMgr::instance();
    entry->devTransMap_.clear();
    std::string jobId = "0";
    int devId = 0;
    // invalid jobId
    EXPECT_EQ(nullptr, entry->GetDevTransport(jobId, devId));

    // invalid devId
    std::map<int, SHARED_PTR_ALIA<DeviceTransport>> devTransports;
    entry->devTransMap_.insert(std::make_pair(jobId, devTransports));
    EXPECT_EQ(nullptr, entry->GetDevTransport(jobId, devId));

    // not initialized
    SHARED_PTR_ALIA<DeviceTransport> devTran;
    MSVP_MAKE_SHARED4_VOID(devTran, DeviceTransport, client, std::to_string(devId), jobId, "mode");
    entry->devTransMap_[jobId].insert(std::make_pair(devId, devTran));
    EXPECT_EQ(nullptr, entry->GetDevTransport(jobId, devId));
    entry->devTransMap_.clear();
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, GetDevTransportWillReturnValidDeviceTransportWhenInputValidKey)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::transport::DevTransMgr::instance();
    entry->devTransMap_.clear();
    std::string jobId = "0";
    int devId = 0;

    SHARED_PTR_ALIA<DeviceTransport> devTran;
    MSVP_MAKE_SHARED4_VOID(devTran, DeviceTransport, client, std::to_string(devId), jobId, "mode");
    devTran->dataInitialized_ = true;
    devTran->ctrlInitialized_ = true;
    entry->devTransMap_[jobId].insert(std::make_pair(devId, devTran));
    EXPECT_EQ(devTran, entry->GetDevTransport(jobId, devId));
    entry->devTransMap_.clear();
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, CloseDevTransportWillReturnSuccWhenInputInvalidKey)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::transport::DevTransMgr::instance();
    entry->devTransMap_.clear();
    std::string jobId = "0";
    int devId = 0;

    EXPECT_EQ(PROFILING_SUCCESS, entry->CloseDevTransport(jobId, devId));
    entry->devTransMap_.clear();
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, CloseDevTransportWillCloseTransportsByJobIdWhenInputInvalidDevId)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::transport::DevTransMgr::instance();
    entry->devTransMap_.clear();
    std::string jobId = "0";
    int devId = 0;

    std::map<int, SHARED_PTR_ALIA<DeviceTransport>> devTransports;
    entry->devTransMap_.insert(std::make_pair(jobId, devTransports));

    SHARED_PTR_ALIA<DeviceTransport> devTran;
    MSVP_MAKE_SHARED4_VOID(devTran, DeviceTransport, client, std::to_string(devId), jobId, "mode");
    entry->devTransMap_[jobId].insert(std::make_pair(devId, devTran));

    EXPECT_EQ(PROFILING_SUCCESS, entry->CloseDevTransport(jobId, -1));
    EXPECT_EQ(nullptr, entry->GetDevTransport(jobId, devId));
    entry->devTransMap_.clear();
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, CloseDevTransportWillCloseTransportsByDevIdWhenInputValidDevId)
{
    GlobalMockObject::verify();
    auto entry = analysis::dvvp::transport::DevTransMgr::instance();
    entry->devTransMap_.clear();
    std::string jobId = "0";
    int devId1 = 0;
    int devId2 = 1;

    std::map<int, SHARED_PTR_ALIA<DeviceTransport>> devTransports;
    SHARED_PTR_ALIA<DeviceTransport> devTran1;
    MSVP_MAKE_SHARED4_VOID(devTran1, DeviceTransport, client, std::to_string(devId1), jobId, "mode");
    devTran1->dataInitialized_ = true;
    devTran1->ctrlInitialized_ = true;
    SHARED_PTR_ALIA<DeviceTransport> devTran2;
    MSVP_MAKE_SHARED4_VOID(devTran2, DeviceTransport, client, std::to_string(devId2), jobId, "mode");
    devTran2->dataInitialized_ = true;
    devTran2->ctrlInitialized_ = true;
    devTransports.insert(std::make_pair(devId1, devTran1));
    devTransports.insert(std::make_pair(devId2, devTran2));
    entry->devTransMap_.insert(std::make_pair(jobId, devTransports));

    EXPECT_EQ(PROFILING_SUCCESS, entry->CloseDevTransport(jobId, devId1));
    EXPECT_EQ(nullptr, entry->GetDevTransport(jobId, devId1));
    EXPECT_EQ(devTran2, entry->GetDevTransport(jobId, devId2));

    EXPECT_EQ(PROFILING_SUCCESS, entry->CloseDevTransport(jobId, devId2));
    entry->devTransMap_.clear();
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, SendMsgAndRecvResponse)
{
    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");
    HDC_SESSION session = (HDC_SESSION)0x12345678;
    auto transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
        new analysis::dvvp::transport::HDCTransport(session));

    std::string msg = "profiling msg";
    
    struct tlv_req **packetFake = nullptr;
    struct tlv_req* packet = nullptr;

    // invalid parameter
    EXPECT_EQ(PROFILING_FAILED, dev_tran->SendMsgAndRecvResponse(msg, packetFake));

    // closed
    dev_tran->isClosed_ = true;
    EXPECT_EQ(PROFILING_FAILED, dev_tran->SendMsgAndRecvResponse(msg, &packet));

    dev_tran->isClosed_ = false;
    // null ctrlTran_
    EXPECT_EQ(PROFILING_FAILED, dev_tran->SendMsgAndRecvResponse(msg, &packet));

    ctrl_tran = std::make_shared<HDCTransport>(session);
    dev_tran->ctrlTran_ = ctrl_tran;

    MOCKER_CPP_VIRTUAL(*transport.get(), &HDCTransport::SendBuffer, int(HDCTransport::*)(const void *, int))
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    // send data failed
    EXPECT_EQ(PROFILING_FAILED, dev_tran->SendMsgAndRecvResponse(msg, &packet));

    MOCKER_CPP_VIRTUAL(*transport.get(), &HDCTransport::RecvPacket)
        .stubs()
        .with(outBoundP(&packet))
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));

    EXPECT_EQ(PROFILING_FAILED, dev_tran->SendMsgAndRecvResponse(msg, &packet));
    EXPECT_EQ(PROFILING_SUCCESS, dev_tran->SendMsgAndRecvResponse(msg, &packet));
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, CloseConn)
{
    GlobalMockObject::verify();

    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");
    EXPECT_NE(nullptr, dev_tran);
    // ctrl_tran is null
    dev_tran->CloseConn();
    
    ctrl_tran = std::make_shared<HDCTransport>(client);
    EXPECT_NE(nullptr, ctrl_tran);
    dev_tran->ctrlTran_ = ctrl_tran;

    dev_tran->CloseConn();
    EXPECT_EQ(true, dev_tran->isClosed_);
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, IsInitialized)
{
    GlobalMockObject::verify();

    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");
    EXPECT_NE(nullptr, dev_tran);
    dev_tran->dataInitialized_ = false;
    dev_tran->ctrlInitialized_ = false;
    EXPECT_EQ(false, dev_tran->IsInitialized());
    dev_tran->dataInitialized_ = true;
    dev_tran->ctrlInitialized_ = false;
    EXPECT_EQ(false, dev_tran->IsInitialized());
    dev_tran->dataInitialized_ = false;
    dev_tran->ctrlInitialized_ = true;
    EXPECT_EQ(false, dev_tran->IsInitialized());
    dev_tran->dataInitialized_ = true;
    dev_tran->ctrlInitialized_ = true;
    EXPECT_EQ(true, dev_tran->IsInitialized());
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, HandlePacketWillreturnFailWhenInputNullPacket)
{
    GlobalMockObject::verify();

    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");
    EXPECT_NE(nullptr, dev_tran);
    ctrl_tran = std::make_shared<HDCTransport>(client);
    dev_tran->ctrlTran_ = ctrl_tran;
    analysis::dvvp::message::StatusInfo status;
    EXPECT_EQ(PROFILING_FAILED, dev_tran->HandlePacket(nullptr, status));
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, HandlePacketWillreturnFailWhenInputInvalidMsgPacket)
{
    GlobalMockObject::verify();

    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");
    EXPECT_NE(nullptr, dev_tran);
    ctrl_tran = std::make_shared<HDCTransport>(client);
    dev_tran->ctrlTran_ = ctrl_tran;
    std::string invalidValue = "invalid";
    size_t totalSize = invalidValue.size() + sizeof(struct tlv_req);
    TLV_REQ_PTR req = (TLV_REQ_PTR)malloc(totalSize);
    if (req == nullptr) {
        std::cout << "Failed to malloc";
        return;
    }
    req->type = static_cast<cmd_class>(0);
    req->dev_id = 0;
    req->len = invalidValue.size();
    (void)memcpy_s(req->value, req->len, invalidValue.c_str(), invalidValue.size());
    analysis::dvvp::message::StatusInfo status;
    EXPECT_EQ(PROFILING_FAILED, dev_tran->HandlePacket(req, status));
    // donot need to free req, because HandlePacket will free input req memory;
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, HandlePacketWillreturnFailWhenInputEmptyMsgPacket)
{
    GlobalMockObject::verify();

    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");
    EXPECT_NE(nullptr, dev_tran);
    ctrl_tran = std::make_shared<HDCTransport>(client);
    dev_tran->ctrlTran_ = ctrl_tran;
    SHARED_PTR_ALIA<analysis::dvvp::proto::Response> res;
    MSVP_MAKE_SHARED0_VOID(res, analysis::dvvp::proto::Response);
    res->set_jobid("0");
    res->set_status(analysis::dvvp::proto::SUCCESS);
    res->set_message("");
    std::string encoded = analysis::dvvp::message::EncodeMessage(res);
    size_t totalSize = encoded.size() + sizeof(struct tlv_req);
    TLV_REQ_PTR req = (TLV_REQ_PTR)malloc(totalSize);
    if (req == nullptr) {
        std::cout << "Failed to malloc";
        return;
    }
    req->type = static_cast<cmd_class>(0);
    req->dev_id = 0;
    req->len = encoded.size();
    (void)memcpy_s(req->value, req->len, encoded.c_str(), encoded.size());
    analysis::dvvp::message::StatusInfo retStatus;
    EXPECT_EQ(PROFILING_FAILED, dev_tran->HandlePacket(req, retStatus));
    // donot need to free req, because HandlePacket will free input req memory;
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, HandlePacketWillreturnFailWhenInputFailStatusPacket)
{
    GlobalMockObject::verify();

    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");
    EXPECT_NE(nullptr, dev_tran);
    ctrl_tran = std::make_shared<HDCTransport>(client);
    dev_tran->ctrlTran_ = ctrl_tran;
    analysis::dvvp::message::StatusInfo statusInfo;
    statusInfo.status = analysis::dvvp::message::ERR;
    SHARED_PTR_ALIA<analysis::dvvp::proto::Response> res;
    MSVP_MAKE_SHARED0_VOID(res, analysis::dvvp::proto::Response);
    res->set_jobid("0");
    res->set_status(analysis::dvvp::proto::FAILED);
    res->set_message(statusInfo.ToString());
    std::string encoded = analysis::dvvp::message::EncodeMessage(res);
    size_t totalSize = encoded.size() + sizeof(struct tlv_req);
    TLV_REQ_PTR req = (TLV_REQ_PTR)malloc(totalSize);
    if (req == nullptr) {
        std::cout << "Failed to malloc";
        return;
    }
    req->type = static_cast<cmd_class>(0);
    req->dev_id = 0;
    req->len = encoded.size();
    (void)memcpy_s(req->value, req->len, encoded.c_str(), encoded.size());
    analysis::dvvp::message::StatusInfo retStatus;
    EXPECT_EQ(PROFILING_FAILED, dev_tran->HandlePacket(req, retStatus));
    // donot need to free req, because HandlePacket will free input req memory;
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, HandlePacketWillreturnSuccWhenInputValidPacket)
{
    GlobalMockObject::verify();

    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");
    EXPECT_NE(nullptr, dev_tran);
    ctrl_tran = std::make_shared<HDCTransport>(client);
    dev_tran->ctrlTran_ = ctrl_tran;
    analysis::dvvp::message::StatusInfo statusInfo;
    statusInfo.status = analysis::dvvp::message::SUCCESS;
    SHARED_PTR_ALIA<analysis::dvvp::proto::Response> res;
    MSVP_MAKE_SHARED0_VOID(res, analysis::dvvp::proto::Response);
    res->set_jobid("0");
    res->set_status(analysis::dvvp::proto::SUCCESS);
    res->set_message(statusInfo.ToString());
    std::string encoded = analysis::dvvp::message::EncodeMessage(res);
    size_t totalSize = encoded.size() + sizeof(struct tlv_req);
    TLV_REQ_PTR req = (TLV_REQ_PTR)malloc(totalSize);
    if (req == nullptr) {
        std::cout << "Failed to malloc";
        return;
    }
    req->type = static_cast<cmd_class>(0);
    req->dev_id = 0;
    req->len = encoded.size();
    (void)memcpy_s(req->value, req->len, encoded.c_str(), encoded.size());
    analysis::dvvp::message::StatusInfo retStatus;
    EXPECT_EQ(PROFILING_SUCCESS, dev_tran->HandlePacket(req, retStatus));
    EXPECT_EQ(statusInfo.status, retStatus.status);
    // donot need to free req, because HandlePacket will free input req memory;
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, HandleShakeWillReturnFailWhenCreateConnFail)
{
    GlobalMockObject::verify();

    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");
    EXPECT_NE(nullptr, dev_tran);
    MOCKER_CPP(&analysis::dvvp::transport::DeviceTransport::CreateConn)
        .stubs()
        .will(returnValue(SHARED_PTR_ALIA<analysis::dvvp::transport::AdxTransport>(nullptr)));
    SHARED_PTR_ALIA<analysis::dvvp::proto::CtrlChannelHandshake> ctrlMessage;
    MSVP_MAKE_SHARED0_VOID(ctrlMessage, analysis::dvvp::proto::CtrlChannelHandshake);
    EXPECT_EQ(PROFILING_FAILED, dev_tran->HandleShake(ctrlMessage, true));
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, HandleShakeWillReturnFailWhenSendBufferFail)
{
    GlobalMockObject::verify();

    HDC_SESSION session = (HDC_SESSION)0x12345678;
    auto transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
        new analysis::dvvp::transport::HDCTransport(session));
    MOCKER_CPP_VIRTUAL(*transport.get(), &HDCTransport::SendBuffer, int(HDCTransport::*)(const void *, int))
        .stubs()
        .will(returnValue(PROFILING_FAILED));

    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");
    EXPECT_NE(nullptr, dev_tran);
    // input nullpre msg, so SendBuffer will return fail;
    SHARED_PTR_ALIA<analysis::dvvp::proto::CtrlChannelHandshake> ctrlMessage;
    MSVP_MAKE_SHARED0_VOID(ctrlMessage, analysis::dvvp::proto::CtrlChannelHandshake);
    EXPECT_EQ(PROFILING_FAILED, dev_tran->HandleShake(nullptr, true)); // ctrlTran_
    SHARED_PTR_ALIA<analysis::dvvp::proto::DataChannelHandshake> dataMessage;
    MSVP_MAKE_SHARED0_VOID(dataMessage, analysis::dvvp::proto::DataChannelHandshake);
    EXPECT_EQ(PROFILING_FAILED, dev_tran->HandleShake(dataMessage, false)); // dataTran_
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, HandleShakeWillReturnFailWhenRecvPacketReturnNotSupport)
{
    GlobalMockObject::verify();

    HDC_SESSION session = (HDC_SESSION)0x12345678;
    auto transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
        new analysis::dvvp::transport::HDCTransport(session));
    MOCKER_CPP_VIRTUAL(*transport.get(), &HDCTransport::SendBuffer, int(HDCTransport::*)(const void *, int))
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    struct tlv_req* packet = nullptr;
    MOCKER_CPP_VIRTUAL(*transport.get(), &HDCTransport::RecvPacket)
        .stubs()
            .with(outBoundP(&packet))
            .will(returnValue(PROFILING_NOTSUPPORT));

    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");
    EXPECT_NE(nullptr, dev_tran);
    // input nullpre msg, so SendBuffer will return fail;
    SHARED_PTR_ALIA<analysis::dvvp::proto::CtrlChannelHandshake> ctrlMessage;
    MSVP_MAKE_SHARED0_VOID(ctrlMessage, analysis::dvvp::proto::CtrlChannelHandshake);
    EXPECT_EQ(PROFILING_NOTSUPPORT, dev_tran->HandleShake(nullptr, true));
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, HandleShakeWillReturnFailWhenRecvPacketReturnFail)
{
    GlobalMockObject::verify();

    HDC_SESSION session = (HDC_SESSION)0x12345678;
    auto transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
        new analysis::dvvp::transport::HDCTransport(session));
    MOCKER_CPP_VIRTUAL(*transport.get(), &HDCTransport::SendBuffer, int(HDCTransport::*)(const void *, int))
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    struct tlv_req* packet = nullptr;
    MOCKER_CPP_VIRTUAL(*transport.get(), &HDCTransport::RecvPacket)
        .stubs()
            .with(outBoundP(&packet))
            .will(returnValue(PROFILING_FAILED));

    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");
    EXPECT_NE(nullptr, dev_tran);
    // input nullpre msg, so SendBuffer will return fail;
    SHARED_PTR_ALIA<analysis::dvvp::proto::CtrlChannelHandshake> ctrlMessage;
    MSVP_MAKE_SHARED0_VOID(ctrlMessage, analysis::dvvp::proto::CtrlChannelHandshake);
    EXPECT_EQ(PROFILING_FAILED, dev_tran->HandleShake(nullptr, true));
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, HandleShakeWillReturnFailWhenRecvPacketReturnNullPacket)
{
    GlobalMockObject::verify();

    HDC_SESSION session = (HDC_SESSION)0x12345678;
    auto transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
        new analysis::dvvp::transport::HDCTransport(session));
    MOCKER_CPP_VIRTUAL(*transport.get(), &HDCTransport::SendBuffer, int(HDCTransport::*)(const void *, int))
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    struct tlv_req* packet = nullptr;
    MOCKER_CPP_VIRTUAL(*transport.get(), &HDCTransport::RecvPacket)
        .stubs()
            .with(outBoundP(&packet))
            .will(returnValue(PROFILING_SUCCESS));

    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");
    EXPECT_NE(nullptr, dev_tran);
    // input nullpre msg, so SendBuffer will return fail;
    SHARED_PTR_ALIA<analysis::dvvp::proto::CtrlChannelHandshake> ctrlMessage;
    MSVP_MAKE_SHARED0_VOID(ctrlMessage, analysis::dvvp::proto::CtrlChannelHandshake);
    EXPECT_EQ(PROFILING_FAILED, dev_tran->HandleShake(nullptr, true));
}

TEST_F(HOST_PROF_DEVICE_TRANSPORT_UTEST, HandleShakeWillReturnSuccWhenHandlePacketReturnValidPacket)
{
    GlobalMockObject::verify();

    HDC_SESSION session = (HDC_SESSION)0x12345678;
    auto transport = std::shared_ptr<analysis::dvvp::transport::HDCTransport>(
        new analysis::dvvp::transport::HDCTransport(session));
    MOCKER_CPP_VIRTUAL(*transport.get(), &HDCTransport::SendBuffer, int(HDCTransport::*)(const void *, int))
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));

    // fake recv packet, so HandlePacket will return succ
    analysis::dvvp::message::StatusInfo statusInfo;
    statusInfo.status = analysis::dvvp::message::SUCCESS;
    SHARED_PTR_ALIA<analysis::dvvp::proto::Response> res;
    MSVP_MAKE_SHARED0_VOID(res, analysis::dvvp::proto::Response);
    res->set_jobid("0");
    res->set_status(analysis::dvvp::proto::SUCCESS);
    res->set_message(statusInfo.ToString());
    std::string encoded = analysis::dvvp::message::EncodeMessage(res);
    size_t totalSize = encoded.size() + sizeof(struct tlv_req);
    TLV_REQ_PTR req = (TLV_REQ_PTR)malloc(totalSize);
    if (req == nullptr) {
        std::cout << "Failed to malloc";
        return;
    }
    req->type = static_cast<cmd_class>(0);
    req->dev_id = 0;
    req->len = encoded.size();
    (void)memcpy_s(req->value, req->len, encoded.c_str(), encoded.size());
    MOCKER_CPP_VIRTUAL(*transport.get(), &HDCTransport::RecvPacket)
        .stubs()
            .with(outBoundP(&req))
            .will(returnValue(PROFILING_SUCCESS));

    dev_tran = std::make_shared<DeviceTransport>(client, dev_id, "123", "def_mode");
    EXPECT_NE(nullptr, dev_tran);
    // input nullpre msg, so SendBuffer will return fail;
    SHARED_PTR_ALIA<analysis::dvvp::proto::CtrlChannelHandshake> ctrlMessage;
    MSVP_MAKE_SHARED0_VOID(ctrlMessage, analysis::dvvp::proto::CtrlChannelHandshake);
    EXPECT_EQ(PROFILING_SUCCESS, dev_tran->HandleShake(nullptr, true));
}