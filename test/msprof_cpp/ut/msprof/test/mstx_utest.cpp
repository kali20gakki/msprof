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

void MstxApiThreadFunc()
{
    MstxDataHandler::instance()->SaveMstxData("test_mark", 1, MstxDataType::DATA_MARK);
    MstxDataHandler::instance()->SaveMstxData("test_range_start", 2, MstxDataType::DATA_RANGE_START); // 2: range_id
    MstxDataHandler::instance()->SaveMstxData(nullptr, 2, MstxDataType::DATA_RANGE_END); // 2: range_id
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

TEST_F(MstxUtest, MstxMarkAFuncWillReturnWhenMsgContainsSpecialCharacter)
{
    GlobalMockObject::verify();
    MstxDataHandler::instance()->Start();
    aclrtStream stream;
    const char* msg = "record&";
    MsprofMstxApi::MstxMarkAFunc(msg, stream);
    MstxDataHandler::instance()->Stop();
    EXPECT_EQ(0, g_txdataList_.size());
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
