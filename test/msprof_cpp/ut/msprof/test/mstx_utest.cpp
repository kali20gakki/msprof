/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: UT for mstx module
 * Author: Huawei Technologies Co., Ltd.
 */
#include <thread>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "prof_callback.h"
#include "mstx_data_handler.h"
#include "mstx_inject.h"
#include "msprof_tx_manager.h"

using namespace Collector::Dvvp::Mstx;
using namespace Msprof::MsprofTx;
using namespace analysis::dvvp::common::utils;

std::vector<MsprofAdditionalInfo> g_txdataList_;

int32_t MstxAddiInfoReporterCallbackStub(uint32_t agingFlag, CONST_VOID_PTR data, uint32_t len)
{
    g_txdataList_.push_back(*(reinterpret_cast<const MsprofAdditionalInfo *>(data)));
    return PROFILING_SUCCESS;
}

class MstxUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        g_txdataList_.clear();
        MsprofTxManager::instance()->SetReporterCallback(MstxAddiInfoReporterCallbackStub);
        MsprofTxManager::instance()->Init();
    }
    virtual void TearDown()
    {
        MsprofTxManager::instance()->UnInit();
        g_txdataList_.clear();
    }
};

MstxFuncPointer g_mstxMarkAPtr;
MstxFuncPointer g_mstxRangeStartAPtr;
MstxFuncPointer g_mstxRangeEndPtr;
MstxFuncPointer g_mstxDomainCreateAPtr;
MstxFuncPointer g_mstxDomainDestroyPtr;
MstxFuncPointer g_mstxDomainMarkAPtr;
MstxFuncPointer g_mstxDomainRangeStartAPtr;
MstxFuncPointer g_mstxDomainRangeEndPtr;

struct MstxContext {
    MstxFuncPointer* mstxCoreFuncTable[MSTX_FUNC_END + 1];
    MstxFuncPointer* mstxCore2FuncTable[MSTX_FUNC_DOMAIN_END + 1];
};

struct MstxContext g_ctx = {
    {
        0,
        &g_mstxMarkAPtr,
        &g_mstxRangeStartAPtr,
        &g_mstxRangeEndPtr,
        0,
    },
    {
        0,
        &g_mstxDomainCreateAPtr,
        &g_mstxDomainDestroyPtr,
        &g_mstxDomainMarkAPtr,
        &g_mstxDomainRangeStartAPtr,
        &g_mstxDomainRangeEndPtr,
        0
    }
};

void MstxApiThreadFunc()
{
    uint64_t testDomain = 0;
    uint64_t markEventId = 1;
    uint64_t rangeEventId = 2;
    MstxDataHandler::instance()->SaveMstxData("test_mark", 1, MstxDataType::DATA_MARK, testDomain);
    MstxDataHandler::instance()->SaveMstxData("test_range_start", rangeEventId,
        MstxDataType::DATA_RANGE_START, testDomain);
    MstxDataHandler::instance()->SaveMstxData(nullptr, rangeEventId, MstxDataType::DATA_RANGE_END);
}

uint64_t CalculateHash(const std::string &data)
{
    static const uint32_t UINT32_BITS = 32;  // the number of unsigned int bits
    uint32_t prime[2] = {29, 131};  // hash step size,
    uint32_t hash[2] = {0};

    for (char d : data) {
        hash[0] = hash[0] * prime[0] + static_cast<uint32_t>(d);
        hash[1] = hash[1] * prime[1] + static_cast<uint32_t>(d);
    }

    return (((static_cast<uint64_t>(hash[0])) << UINT32_BITS) | hash[1]);
}

int GetFuncTableReturnFailStub(MstxFuncModule module, MstxFuncTable *outTable, unsigned int *outSize)
{
    return MSTX_FAIL;
}

int GetFuncTableReturnInvalidOutsizeStub(MstxFuncModule module, MstxFuncTable *outTable, unsigned int *outSize)
{
    return MSTX_SUCCESS;
}

int GetFuncTableReturnValidOutsizeStub(MstxFuncModule module, MstxFuncTable *outTable, unsigned int *outSize)
{
    switch (module) {
        case MSTX_API_MODULE_CORE:
            *outTable = g_ctx.mstxCoreFuncTable;
            *outSize = MSTX_FUNC_END;
            break;
        case MSTX_API_MODULE_CORE_DOMAIN:
            *outTable = g_ctx.mstxCore2FuncTable;
            *outSize = MSTX_FUNC_DOMAIN_END;
            break;
        default:
            return MSTX_FAIL;
    }
    return MSTX_SUCCESS;
}

TEST_F(MstxUtest, GetModuleTableFuncWillReturnSuccWhenCallgetFuncTableFail)
{
    EXPECT_EQ(MSTX_SUCCESS, MsprofMstxApi::GetModuleTableFunc(GetFuncTableReturnFailStub));
}

TEST_F(MstxUtest, GetModuleTableFuncWillReturnSuccWhenGetFuncTableReturnInvalidOutsize)
{
    EXPECT_EQ(MSTX_SUCCESS, MsprofMstxApi::GetModuleTableFunc(GetFuncTableReturnInvalidOutsizeStub));
}

TEST_F(MstxUtest, GetModuleTableFuncWillReturnSuccWhenGetFuncTableReturnValidOutsize)
{
    EXPECT_EQ(MSTX_SUCCESS, MsprofMstxApi::GetModuleTableFunc(GetFuncTableReturnValidOutsizeStub));
}

TEST_F(MstxUtest, MstxDataHandlerWillSaveDataWhileRunSucc)
{
    GlobalMockObject::verify();
    EXPECT_EQ(PROFILING_SUCCESS, MstxDataHandler::instance()->Start());
    EXPECT_EQ(true, MstxDataHandler::instance()->IsStart());
    std::thread t(MstxApiThreadFunc);
    t.join();
    EXPECT_EQ(PROFILING_SUCCESS, MstxDataHandler::instance()->Stop());
    EXPECT_EQ(2, g_txdataList_.size()); // 2: mstx data size
    for (size_t i = 0; i < g_txdataList_.size(); i++) {
        EXPECT_EQ(MSPROF_REPORT_TX_LEVEL, g_txdataList_[i].level);
        EXPECT_EQ(sizeof(MsprofTxInfo), g_txdataList_[i].dataLen);
    }
}

TEST_F(MstxUtest, MstxDataHandlerStartWillReturnSuccWhileRepeateStart)
{
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    EXPECT_EQ(PROFILING_SUCCESS, MstxDataHandler::instance()->Start());
    MstxDataHandler::instance()->Stop();
}

TEST_F(MstxUtest, MstxDataHandlerReturnFailWhileSaveInvalidRangeEndId)
{
    GlobalMockObject::verify();
    uint64_t invaildId = 10;
    EXPECT_EQ(PROFILING_FAILED, MstxDataHandler::instance()->SaveMstxData("test", invaildId,
        MstxDataType::DATA_RANGE_END));
}

TEST_F(MstxUtest, MstxMarkAFuncWillReturnWhenMstxDataHandlerNotStartYet)
{
    GlobalMockObject::verify();
    const char* msg = "xxx";
    MsprofMstxApi::MstxMarkAFunc(msg, nullptr);
    EXPECT_EQ(0, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxMarkAFuncWillReturnWhenMsgIsNull)
{
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    MsprofMstxApi::MstxMarkAFunc(nullptr, nullptr);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(0, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxMarkAFuncWillReturnWhenMsgIsLongerThanMaxLen)
{
    GlobalMockObject::verify();
    std::string msg(MAX_MESSAGE_LEN, 'x');
    MstxDataHandler::instance()->Start();
    MsprofMstxApi::MstxMarkAFunc(msg.c_str(), nullptr);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(0, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxMarkAFuncWillReturnInputDataStartsWithInvalidChar)
{
    // 校验框架内置通信打点数据
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    std::string msg = "+";
    MsprofMstxApi::MstxMarkAFunc(msg.c_str(), nullptr);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(0, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxMarkAFuncWillReturnWhenInputCommunicationDataMsgLengthLargerThanMaxValue)
{
    // 校验框架内置通信打点数据
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    aclrtStream stream;
    std::string msg = "{\\\"count\\\": \\\"16\\\",\\\"dataType\\\": \\\"fp32\\\",\
                        \\\"groupName\\\": \\\"hccl_world_groupxxxxxxxxxx\\\",\\\"opName\\\": \\\"HcclSend\\\",\
                        \\\"DestRank\\\": \\\"10000\\\",\
                        \\\"streamId\\\": \\\"0\\\"}";
    MsprofMstxApi::MstxMarkAFunc(msg.c_str(), nullptr);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(0, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxMarkAFuncWillReturnWhenInputCommunicationDataMsgLengthSmallerThanMaxValue)
{
    // 校验框架内置通信打点数据
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    aclrtStream stream;
    std::string msg = "{\\\"count\\\": \\\"16\\\",\\\"dataType\\\": \\\"fp32\\\",\
                        \\\"groupName\\\": \\\"group\\\",\\\"opName\\\": \\\"HcclAllreduce\\\",\
                        \\\"streamId\\\": \\\"0\\\"}";
    MsprofMstxApi::MstxMarkAFunc(msg.c_str(), nullptr);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(1, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxMarkAFuncWillReturnWhenRtProfilerTraceExFail)
{
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    aclrtStream stream = (aclrtStream)0x12345678;
    const char* msg = "record";
    MOCKER_CPP(&Collector::Dvvp::Plugin::RuntimePlugin::MsprofRtProfilerTraceEx)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MsprofMstxApi::MstxMarkAFunc(msg, stream);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(0, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxMarkAFuncWillReturnWhenSaveMstxDataFail)
{
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    aclrtStream stream = (aclrtStream)0x12345678;
    const char* msg = "record";
    MOCKER_CPP(&Collector::Dvvp::Plugin::RuntimePlugin::MsprofRtProfilerTraceEx)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&Collector::Dvvp::Mstx::MstxDataHandler::SaveMstxData)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    MsprofMstxApi::MstxMarkAFunc(msg, stream);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(0, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxMarkAFuncWillSaveMstxDataWhenSaveMstxDataSuccWithInputStream)
{
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    aclrtStream stream = (aclrtStream)0x12345678;
    const char* msg = "record";
    MOCKER_CPP(&Collector::Dvvp::Plugin::RuntimePlugin::MsprofRtProfilerTraceEx)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MsprofMstxApi::MstxMarkAFunc(msg, stream);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(1, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxMarkAFuncWillSaveMstxDataWhenSaveMstxDataSuccWithNoInputStream)
{
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    const char* msg = "record";
    MsprofMstxApi::MstxMarkAFunc(msg, nullptr);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(1, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxRangeStartAFuncWillReturnZeroWhenMstxDataHandlerNotStartYet)
{
    GlobalMockObject::verify();
    const char* msg = "xxx";
    EXPECT_EQ(MSTX_INVALID_RANGE_ID, MsprofMstxApi::MstxRangeStartAFunc(msg, nullptr));
}

TEST_F(MstxUtest, MstxRangeStartAFuncWillReturnZeroWhenMsgIsNull)
{
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    EXPECT_EQ(MSTX_INVALID_RANGE_ID, MsprofMstxApi::MstxRangeStartAFunc(nullptr, nullptr));
    MstxDataHandler::instance()->Stop();
}

TEST_F(MstxUtest, MstxRangeStartAFuncWillReturnZeroWhenMsgIsLongerThanMaxLen)
{
    GlobalMockObject::verify();
    std::string msg(MAX_MESSAGE_LEN, 'x');
    MstxDataHandler::instance()->Start();
    EXPECT_EQ(MSTX_INVALID_RANGE_ID, MsprofMstxApi::MstxRangeStartAFunc(msg.c_str(), nullptr));
    MstxDataHandler::instance()->Stop();
}

TEST_F(MstxUtest, MstxRangeStartAFuncWillReturnZeroWhenRtProfilerTraceExFail)
{
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    aclrtStream stream = (aclrtStream)0x12345678;
    const char* msg = "record";
    MOCKER_CPP(&Collector::Dvvp::Plugin::RuntimePlugin::MsprofRtProfilerTraceEx)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(MSTX_INVALID_RANGE_ID, MsprofMstxApi::MstxRangeStartAFunc(msg, stream));
    MstxDataHandler::instance()->Stop();
}

TEST_F(MstxUtest, MstxRangeStartAFuncWillReturnZeroWhenSaveMstxDataFail)
{
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    aclrtStream stream = (aclrtStream)0x12345678;
    const char* msg = "record";
    MOCKER_CPP(&Collector::Dvvp::Plugin::RuntimePlugin::MsprofRtProfilerTraceEx)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&Collector::Dvvp::Mstx::MstxDataHandler::SaveMstxData)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(MSTX_INVALID_RANGE_ID, MsprofMstxApi::MstxRangeStartAFunc(msg, stream));
    MstxDataHandler::instance()->Stop();
}

TEST_F(MstxUtest, MstxRangeStartAFuncWillNotSaveMstxDataWhenMstxRangeEndIsNotCalledWithInputStream)
{
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    aclrtStream stream = (aclrtStream)0x12345678;
    const char* msg = "record";
    MOCKER_CPP(&Collector::Dvvp::Plugin::RuntimePlugin::MsprofRtProfilerTraceEx)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_NE(MSTX_INVALID_RANGE_ID, MsprofMstxApi::MstxRangeStartAFunc(msg, stream));
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(0, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxRangeStartAFuncWillNotSaveMstxDataWhenMstxRangeEndIsNotCalledWithNotInputStream)
{
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    const char* msg = "record";
    EXPECT_NE(MSTX_INVALID_RANGE_ID, MsprofMstxApi::MstxRangeStartAFunc(msg, nullptr));
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(0, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxRangeStartAFuncWillSaveMstxDataWhenMstxRangeEndIsCalledWithInputStream)
{
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    aclrtStream stream = (aclrtStream)0x12345678;
    const char* msg = "record";
    MOCKER_CPP(&Collector::Dvvp::Plugin::RuntimePlugin::MsprofRtProfilerTraceEx)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    uint64_t id = MsprofMstxApi::MstxRangeStartAFunc(msg, stream);
    EXPECT_NE(MSTX_INVALID_RANGE_ID, id);
    MsprofMstxApi::MstxRangeEndFunc(id);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(1, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxRangeStartAFuncWillSaveMstxDataWhenMstxRangeEndIsCalledWithNotInputStream)
{
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    const char* msg = "record";
    uint64_t id = MsprofMstxApi::MstxRangeStartAFunc(msg, nullptr);
    EXPECT_NE(MSTX_INVALID_RANGE_ID, id);
    MsprofMstxApi::MstxRangeEndFunc(id);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(1, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxRangeEndFuncWillReturnWhenMstxDataHandlerNotStartYet)
{
    GlobalMockObject::verify();
    uint64_t id = 0;
    MsprofMstxApi::MstxRangeEndFunc(id);
    EXPECT_EQ(0, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxRangeEndFuncWillReturnWhenInputInvalidRangeId)
{
    GlobalMockObject::verify();
    uint64_t id = 0;
    MstxDataHandler::instance()->Start();
    MsprofMstxApi::MstxRangeEndFunc(id);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(0, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxRangeEndFuncWillNotSaveMstxDataWhenSaveMstxDataFail)
{
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    MOCKER_CPP(&Collector::Dvvp::Mstx::MstxDataHandler::SaveMstxData)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS)) // succ for range start
        .then(returnValue(PROFILING_FAILED)); // fail for range end
    const char* msg = "record";
    uint64_t id = MsprofMstxApi::MstxRangeStartAFunc(msg, nullptr);
    EXPECT_NE(MSTX_INVALID_RANGE_ID, id);

    MsprofMstxApi::MstxRangeEndFunc(id);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(0, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxRangeEndFuncWillSaveMstxDataWhenSaveMstxDataSuccWithNotInputStream)
{
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    const char* msg = "record";
    uint64_t id = MsprofMstxApi::MstxRangeStartAFunc(msg, nullptr);
    EXPECT_NE(MSTX_INVALID_RANGE_ID, id);

    MsprofMstxApi::MstxRangeEndFunc(id);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(1, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxRangeEndFuncWillSaveMstxDataWhenSaveMstxDataSuccWithInputStream)
{
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    aclrtStream stream = (aclrtStream)0x12345678;
    const char* msg = "record";
    MOCKER_CPP(&Collector::Dvvp::Plugin::RuntimePlugin::MsprofRtProfilerTraceEx)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    uint64_t id = MsprofMstxApi::MstxRangeStartAFunc(msg, stream);
    EXPECT_NE(MSTX_INVALID_RANGE_ID, id);

    MsprofMstxApi::MstxRangeEndFunc(id);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(1, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxDomainCreateAFuncWillReturnNullWhenInputInvalidMsg)
{
    GlobalMockObject::verify();
    EXPECT_EQ(nullptr, MsprofMstxApi::MstxDomainCreateAFunc(nullptr));
}

TEST_F(MstxUtest, MstxDomainCreateAFuncWillReturnHandleWhenInputValidMsg)
{
    GlobalMockObject::verify();
    auto handle = MsprofMstxApi::MstxDomainCreateAFunc(nullptr);
    EXPECT_EQ(nullptr, handle);
    EXPECT_EQ(0, MstxDomainMgr::domainHandleMap_.size());
    
    handle = MsprofMstxApi::MstxDomainCreateAFunc("+");
    EXPECT_EQ(nullptr, handle);
    EXPECT_EQ(0, MstxDomainMgr::domainHandleMap_.size());

    handle = MsprofMstxApi::MstxDomainCreateAFunc("test");
    EXPECT_NE(nullptr, handle);
    EXPECT_EQ(1, MstxDomainMgr::domainHandleMap_.size());

    auto handle2 = MsprofMstxApi::MstxDomainCreateAFunc("test");
    EXPECT_EQ(handle, handle2);
    EXPECT_EQ(1, MstxDomainMgr::domainHandleMap_.size());
    MsprofMstxApi::MstxDomainDestroyFunc(handle);
}

TEST_F(MstxUtest, MstxDomainDestroyFuncWillRunWhenInputValidDomain)
{
    GlobalMockObject::verify();
    auto handle = MsprofMstxApi::MstxDomainCreateAFunc("test");
    EXPECT_EQ(1, MstxDomainMgr::domainHandleMap_.size());
    MsprofMstxApi::MstxDomainDestroyFunc(nullptr);
    EXPECT_EQ(1, MstxDomainMgr::domainHandleMap_.size());
    MsprofMstxApi::MstxDomainDestroyFunc(handle);
    EXPECT_EQ(0, MstxDomainMgr::domainHandleMap_.size());
}

TEST_F(MstxUtest, GetDomainNameHashByHandleWillReturnFalseWhenInputInvalidDomain)
{
    GlobalMockObject::verify();
    auto handle = MsprofMstxApi::MstxDomainCreateAFunc("test");
    uint64_t domain = 0;
    EXPECT_EQ(false, MstxDomainMgr::instance()->GetDomainNameHashByHandle(nullptr, domain));
    EXPECT_EQ(0, domain);
    MsprofMstxApi::MstxDomainDestroyFunc(handle);
}

TEST_F(MstxUtest, GetDomainNameHashByHandleWillReturnSuccWhenInputValidDomain)
{
    GlobalMockObject::verify();
    auto handle = MsprofMstxApi::MstxDomainCreateAFunc("test");
    uint64_t domain = 0;
    EXPECT_EQ(true, MstxDomainMgr::instance()->GetDomainNameHashByHandle(handle, domain));
    EXPECT_NE(0, domain);
    MsprofMstxApi::MstxDomainDestroyFunc(handle);
}

TEST_F(MstxUtest, GetDefaultDomainNameHashWillReturnDefaultName)
{
    GlobalMockObject::verify();
    uint64_t expectValue = CalculateHash("default");
    EXPECT_EQ(expectValue, MstxDomainMgr::instance()->GetDefaultDomainNameHash());
}

TEST_F(MstxUtest, MstxDomainMarkAFuncWillNotMarkWhenMstxDataHandlerNotStart)
{
    auto handle = MsprofMstxApi::MstxDomainCreateAFunc("test");
    MsprofMstxApi::MstxDomainMarkAFunc(handle, "test", nullptr);
    EXPECT_EQ(0, g_txdataList_.size());
    MsprofMstxApi::MstxDomainDestroyFunc(handle);
}

TEST_F(MstxUtest, MstxDomainMarkAFuncWillNotMarkWhenInputInvalidMsg)
{
    auto handle = MsprofMstxApi::MstxDomainCreateAFunc("test");
    MstxDataHandler::instance()->Start();
    MsprofMstxApi::MstxDomainMarkAFunc(handle, nullptr, nullptr);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(0, g_txdataList_.size());
    MsprofMstxApi::MstxDomainDestroyFunc(handle);
}

TEST_F(MstxUtest, MstxDomainMarkAFuncWillNotMarkWhenInputInvalidDomain)
{
    MstxDataHandler::instance()->Start();
    MsprofMstxApi::MstxDomainMarkAFunc(nullptr, "test", nullptr);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(0, g_txdataList_.size());
}

TEST_F(MstxUtest, MstxDomainMarkAFuncWillNotMarkWhenSaveMstxDataFail)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Collector::Dvvp::Mstx::MstxDataHandler::SaveMstxData)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    auto handle = MsprofMstxApi::MstxDomainCreateAFunc("test");
    MstxDataHandler::instance()->Start();
    MsprofMstxApi::MstxDomainMarkAFunc(handle, "test", nullptr);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(0, g_txdataList_.size());
    MsprofMstxApi::MstxDomainDestroyFunc(handle);
}

TEST_F(MstxUtest, MstxDomainMarkAFuncWillMarkWhenSaveMstxDataSucc)
{
    GlobalMockObject::verify();
    auto handle = MsprofMstxApi::MstxDomainCreateAFunc("test");
    MstxDataHandler::instance()->Start();
    MsprofMstxApi::MstxDomainMarkAFunc(handle, "test", nullptr);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(1, g_txdataList_.size());
    MsprofMstxApi::MstxDomainDestroyFunc(handle);
}

TEST_F(MstxUtest, MstxDomainMarkAFuncWillNotMarkWhenMsprofRtProfilerTraceExFail)
{
    GlobalMockObject::verify();
    aclrtStream stream = (aclrtStream)0x12345678;
    MOCKER_CPP(&Collector::Dvvp::Plugin::RuntimePlugin::MsprofRtProfilerTraceEx)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    auto handle = MsprofMstxApi::MstxDomainCreateAFunc("test");
    MstxDataHandler::instance()->Start();
    MsprofMstxApi::MstxDomainMarkAFunc(handle, "test", stream);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(0, g_txdataList_.size());
    MsprofMstxApi::MstxDomainDestroyFunc(handle);
}

TEST_F(MstxUtest, MstxDomainMarkAFuncWillNotMarkWhenMsprofRtProfilerTraceExSucc)
{
    GlobalMockObject::verify();
    aclrtStream stream = (aclrtStream)0x12345678;
    MOCKER_CPP(&Collector::Dvvp::Plugin::RuntimePlugin::MsprofRtProfilerTraceEx)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    auto handle = MsprofMstxApi::MstxDomainCreateAFunc("test");
    MstxDataHandler::instance()->Start();
    MsprofMstxApi::MstxDomainMarkAFunc(handle, "test", stream);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(1, g_txdataList_.size());
    MsprofMstxApi::MstxDomainDestroyFunc(handle);
}
