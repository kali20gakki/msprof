/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: UT for msproftx module
 * Author: Huawei Technologies Co., Ltd.
 */
#include <dlfcn.h>
#include <unistd.h>
#include <memory>
#include <thread>
#include <sstream>
#include <iostream>
#include <string>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "mmpa_api.h"
#include "errno/error_code.h"
#include "securec.h"
#include "message/codec.h"
#include "utils/utils.h"
#include "prof_reporter.h"
#include "prof_callback.h"
#include "config/config_manager.h"
#include "proto/msprofiler.pb.h"
#include "uploader_mgr.h"
#include "msprof_stamp_pool.h"
#include "msprof_tx_manager.h"
#include "msprof_reporter_mgr.h"
#include "runtime_plugin.h"

using namespace analysis::dvvp::common::error;
using namespace Msprof::Engine;
using namespace Msprof::MsprofTx;
using namespace analysis::dvvp::proto;
using namespace Collector::Dvvp::Mmpa;

int32_t MsprofAddiInfoReporterCallbackStub(uint32_t agingFlag, CONST_VOID_PTR data, uint32_t len)
{
    return PROFILING_SUCCESS;
}

int32_t MsprofAddiInfoReporterCallbackFail(uint32_t agingFlag, CONST_VOID_PTR data, uint32_t len)
{
    return PROFILING_FAILED;
}

std::vector<MsprofAdditionalInfo> g_dataList_;

int32_t MsprofAddiInfoReporterCallbackSucc(uint32_t agingFlag, CONST_VOID_PTR data, uint32_t len)
{
    g_dataList_.push_back(*(reinterpret_cast<const MsprofAdditionalInfo *>(data)));
    return PROFILING_SUCCESS;
}

class MsprofTxUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(MsprofTxUtest, MsprofTxMemPool)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofStampPool> stampPool;
    MSVP_MAKE_SHARED0_BREAK(stampPool, MsprofStampPool);

    int ret = stampPool->Init(100); // 100 test stamp pool size
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = stampPool->Init(100); // 100 test stamp pool size
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = stampPool->Init(-1);
    EXPECT_EQ(PROFILING_FAILED, ret);

    MsprofStampInstance *instance = stampPool->CreateStamp();
    EXPECT_NE(nullptr, instance);

    ret = stampPool->MsprofStampPush(instance);
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = stampPool->MsprofStampPush(nullptr);
    EXPECT_EQ(PROFILING_FAILED, ret);

    int id = stampPool->GetIdByStamp(instance);
    EXPECT_EQ(0, id);

    MsprofStampInstance *instanceTmp = stampPool->GetStampById(id);
    EXPECT_EQ(instance, instanceTmp);

    id = stampPool->GetIdByStamp(nullptr);
    EXPECT_EQ(PROFILING_FAILED, id);

    instanceTmp = stampPool->MsprofStampPop();
    EXPECT_EQ(instance, instanceTmp);

    instanceTmp = stampPool->MsprofStampPop();
    EXPECT_EQ(nullptr, instanceTmp);

    ret = stampPool->UnInit();
    EXPECT_EQ(PROFILING_FAILED, ret);

    stampPool->DestroyStamp(instance);

    ret = stampPool->UnInit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);

    ret = stampPool->UnInit();
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(MsprofTxUtest, MsprofTxManager)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofTxManager> manager;
    MSVP_MAKE_SHARED0_BREAK(manager, MsprofTxManager);

    MsprofStampInstance stampTest;

    EXPECT_EQ(PROFILING_FAILED, manager->Init());

    EXPECT_EQ(PROFILING_FAILED, manager->SetCategoryName(1, "test"));

    EXPECT_EQ(PROFILING_FAILED, manager->SetStampCategory(nullptr, 1));

    std::string msg = "Test msg";
    EXPECT_EQ(PROFILING_FAILED, manager->SetStampTraceMessage(nullptr, msg.c_str(), msg.size()));

    EXPECT_EQ(PROFILING_FAILED, manager->SetStampPayload(nullptr, 0, nullptr));

    EXPECT_EQ(PROFILING_FAILED, manager->Mark(nullptr));

    EXPECT_EQ(PROFILING_FAILED, manager->Push(nullptr));

    EXPECT_EQ(PROFILING_FAILED, manager->Pop());

    uint32_t rangeId;
    EXPECT_EQ(PROFILING_FAILED, manager->RangeStart(nullptr, &rangeId));

    EXPECT_EQ(PROFILING_FAILED, manager->RangeStop(rangeId));

    ACL_PROF_STAMP_PTR stamp = manager->CreateStamp();
    EXPECT_EQ(nullptr, stamp);

    manager->DestroyStamp(stamp);

    manager->SetReporterCallback(MsprofAddiInfoReporterCallbackStub);

    EXPECT_EQ(PROFILING_SUCCESS, manager->Init());
    
    EXPECT_EQ(PROFILING_SUCCESS, manager->Init());

    stamp = manager->CreateStamp();
    EXPECT_NE(nullptr, stamp);

    EXPECT_EQ(PROFILING_SUCCESS, manager->SetCategoryName(1, "test"));

    EXPECT_EQ(PROFILING_FAILED, manager->SetStampCategory(nullptr, 1));

    EXPECT_EQ(PROFILING_SUCCESS, manager->SetStampCategory(stamp, 1));
    
    EXPECT_EQ(PROFILING_FAILED, manager->SetStampTraceMessage(nullptr, msg.c_str(), 0));

    EXPECT_EQ(PROFILING_FAILED, manager->SetStampTraceMessage(stamp, msg.c_str(), 129)); // 129 invalid msg len

    EXPECT_EQ(PROFILING_SUCCESS, manager->SetStampTraceMessage(stamp, msg.c_str(), msg.size()));

    EXPECT_EQ(PROFILING_FAILED, manager->SetStampPayload(nullptr, 0, nullptr));

    EXPECT_EQ(PROFILING_FAILED, manager->SetStampPayload(stamp, 0, nullptr));

    EXPECT_EQ(PROFILING_SUCCESS, manager->Mark(stamp));

    EXPECT_EQ(PROFILING_FAILED, manager->Mark(nullptr));

    EXPECT_EQ(PROFILING_SUCCESS, manager->Push(stamp));

    EXPECT_EQ(PROFILING_FAILED, manager->Push(nullptr));

    EXPECT_EQ(PROFILING_SUCCESS, manager->Pop());

    EXPECT_EQ(PROFILING_SUCCESS, manager->RangeStart(stamp, &rangeId));

    EXPECT_EQ(PROFILING_SUCCESS, manager->RangeStop(rangeId));

    manager->DestroyStamp(stamp);
    manager->UnInit();
}

TEST_F(MsprofTxUtest, MsprofTxMarkExWillReturnFailWithoutInit)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofTxManager> manager;
    MSVP_MAKE_SHARED0_BREAK(manager, MsprofTxManager);
    aclrtStream streamId = (void *)0x12345; // 0x12345 fake stream addr
    std::string msg = "test";
    EXPECT_EQ(PROFILING_FAILED, manager->MarkEx(msg.c_str(), msg.size(), streamId));
}

TEST_F(MsprofTxUtest, MsprofTxMarkExWillReturnFailWithInvalidInput)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofTxManager> manager;
    MSVP_MAKE_SHARED0_BREAK(manager, MsprofTxManager);
    manager->SetReporterCallback(MsprofAddiInfoReporterCallbackStub);
    manager->Init();
    std::string msg = "test";
    EXPECT_EQ(PROFILING_FAILED, manager->MarkEx(msg.c_str(), msg.size(), nullptr));
    aclrtStream streamId = (void *)0x12345; // 0x12345 fake stream addr
    EXPECT_EQ(PROFILING_FAILED, manager->MarkEx(nullptr, msg.size(), streamId));
    EXPECT_EQ(PROFILING_FAILED, manager->MarkEx(msg.c_str(), msg.size() + 1, streamId));
    msg = ""; // msgLen < 1
    EXPECT_EQ(PROFILING_FAILED, manager->MarkEx(msg.c_str(), msg.size(), streamId));
    manager->UnInit();
}

TEST_F(MsprofTxUtest, MsprofTxMarkExWillReturnFailWithRtApiFail)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofTxManager> manager;
    MSVP_MAKE_SHARED0_BREAK(manager, MsprofTxManager);
    manager->SetReporterCallback(MsprofAddiInfoReporterCallbackStub);
    manager->Init();
    std::string msg = "test";
    aclrtStream streamId = (void *)0x12345; // 0x12345 fake stream addr
    MOCKER_CPP(&Collector::Dvvp::Plugin::RuntimePlugin::MsprofRtProfilerTraceEx)
        .stubs()
        .will(returnValue(PROFILING_FAILED));
    EXPECT_EQ(PROFILING_FAILED, manager->MarkEx(msg.c_str(), msg.size(), streamId));
    manager->UnInit();
}

TEST_F(MsprofTxUtest, MsprofTxMarkExWillReturnFailWithReportFail)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofTxManager> manager;
    MSVP_MAKE_SHARED0_BREAK(manager, MsprofTxManager);
    manager->SetReporterCallback(MsprofAddiInfoReporterCallbackFail);
    manager->Init();
    std::string msg = "test";
    aclrtStream streamId = (void *)0x12345; // 0x12345 fake stream addr
    MOCKER_CPP(&Collector::Dvvp::Plugin::RuntimePlugin::MsprofRtProfilerTraceEx)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, manager->MarkEx(msg.c_str(), msg.size(), streamId));
    manager->UnInit();
}

TEST_F(MsprofTxUtest, MsprofTxMarkExWillReturnSuccWithReportSucc)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofTxManager> manager;
    MSVP_MAKE_SHARED0_BREAK(manager, MsprofTxManager);
    manager->SetReporterCallback(MsprofAddiInfoReporterCallbackSucc);
    manager->Init();
    std::string msg = "test";
    aclrtStream streamId = (void *)0x12345; // 0x12345 fake stream addr
    MOCKER_CPP(&Collector::Dvvp::Plugin::RuntimePlugin::MsprofRtProfilerTraceEx)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, manager->MarkEx(msg.c_str(), msg.size(), streamId));
    manager->UnInit();
    g_dataList_.clear();
}

TEST_F(MsprofTxUtest, ReportDataFuncWillFailWhileReportDataCallBackFail)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofTxManager> manager;
    MSVP_MAKE_SHARED0_BREAK(manager, MsprofTxManager);
    MsprofTxInfo data;
    manager->SetReporterCallback(MsprofAddiInfoReporterCallbackFail);
    manager->Init();
    manager->ReportData(data);
    EXPECT_EQ(0, g_dataList_.size());
    manager->UnInit();
}

TEST_F(MsprofTxUtest, ReportDataFuncWillSuccWhileReportDataCallBackSucc)
{
    GlobalMockObject::verify();
    std::shared_ptr<MsprofTxManager> manager;
    MSVP_MAKE_SHARED0_BREAK(manager, MsprofTxManager);
    MsprofTxInfo data;
    manager->SetReporterCallback(MsprofAddiInfoReporterCallbackSucc);
    manager->Init();
    manager->ReportData(data);
    EXPECT_EQ(1, g_dataList_.size());
    manager->UnInit();
}