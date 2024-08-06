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
#include "msprof_tx_manager.h"

using namespace Collector::Dvvp::Mstx;
using namespace Msprof::MsprofTx;
using namespace analysis::dvvp::common::utils;

std::vector<ReporterData> g_txdataList_;
int32_t MstxReporterCallbackStub(uint32_t moduleId, uint32_t type, VOID_PTR data, uint32_t len)
{
    if (type == MsprofReporterCallbackType::MSPROF_REPORTER_REPORT) {
        g_txdataList_.push_back(*(reinterpret_cast<ReporterData *>(data)));
    }
    return PROFILING_SUCCESS;
}

class MstxUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
        MsprofTxManager::instance()->SetReporterCallback(MstxReporterCallbackStub);
        MsprofTxManager::instance()->Init();
    }
    virtual void TearDown()
    {
        MsprofTxManager::instance()->UnInit();
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
        EXPECT_EQ(64, g_txdataList_[i].deviceId); // 64: host id
        EXPECT_EQ(sizeof(MsprofTxExInfo), g_txdataList_[i].dataLen);
        EXPECT_EQ("msproftx_ex", std::string(g_txdataList_[i].tag));
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