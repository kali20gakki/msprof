/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: UT for msprof reporter manager
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2023-05-08
 */
#include <mutex>
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "message.h"
#include "singleton/singleton.h"
#include "msprof_callback_handler.h"
#include "errno/error_code.h"
#include "hash_data.h"
#include "uploader_mgr.h"
#include "msprof_reporter_mgr.h"
#include "prof_acl_mgr.h"
 
using namespace analysis::dvvp::proto;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::transport;
using namespace Msprof::Engine;
 
class MsprofReporterMgrUtest : public testing::Test {
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};
 
 
TEST_F(MsprofReporterMgrUtest, StartReporters)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::IsInited)
        .stubs()
        .will(returnValue(false))
        .then(returnValue(true));
    EXPECT_EQ(PROFILING_FAILED, MsprofReporterMgr::instance()->StartReporters());
 
    EXPECT_EQ(PROFILING_SUCCESS, MsprofReporterMgr::instance()->StartReporters());
    
    // repeat start reporters
    EXPECT_EQ(PROFILING_SUCCESS, MsprofReporterMgr::instance()->StartReporters());
}
 
TEST_F(MsprofReporterMgrUtest, GenHashId)
{
    GlobalMockObject::verify();
    MsprofReporterMgr::instance()->GetHashId("TestHashInfo");
}
 
TEST_F(MsprofReporterMgrUtest, ReportApiData)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::IsInited)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_SUCCESS, MsprofReporterMgr::instance()->StartReporters());
    
    MsprofApi data;
    EXPECT_EQ(PROFILING_SUCCESS, MsprofReporterMgr::instance()->ReportData(true, data));
}
 
TEST_F(MsprofReporterMgrUtest, ReportEventData)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::IsInited)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_SUCCESS, MsprofReporterMgr::instance()->StartReporters());
    
    MsprofEvent data;
    EXPECT_EQ(PROFILING_SUCCESS, MsprofReporterMgr::instance()->ReportData(true, data));
}
 
TEST_F(MsprofReporterMgrUtest, ReportCompactData)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::IsInited)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_SUCCESS, MsprofReporterMgr::instance()->StartReporters());
    
    MsprofCompactInfo data;
    EXPECT_EQ(PROFILING_SUCCESS, MsprofReporterMgr::instance()->ReportData(true, data));
}
 
TEST_F(MsprofReporterMgrUtest, ReportAdditionalData)
{
    GlobalMockObject::verify();
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::IsInited)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_SUCCESS, MsprofReporterMgr::instance()->StartReporters());
    
    MsprofAdditionalInfo data;
    EXPECT_EQ(PROFILING_SUCCESS, MsprofReporterMgr::instance()->ReportData(true, data));
}
 
TEST_F(MsprofReporterMgrUtest, RegReportTypeInfo)
{
    GlobalMockObject::verify();
    uint16_t level = 0;
    uint32_t typeId = 0;
    std::string typeName = "test";
    MsprofReporterMgr::instance()->RegReportTypeInfo(level, typeId, typeName);
    EXPECT_EQ(typeName, MsprofReporterMgr::instance()->reportTypeInfoMap_[level][typeId]);
    
    // reg typeinfo for same level and type id
    MsprofReporterMgr::instance()->RegReportTypeInfo(level, typeId, typeName);
}
 
TEST_F(MsprofReporterMgrUtest, GetRegReportTypeInfo)
{
    GlobalMockObject::verify();
    EXPECT_EQ("", MsprofReporterMgr::instance()->GetRegReportTypeInfo(1, 1));
    uint16_t level = 0;
    uint32_t typeId = 0;
    std::string typeName = "test";
    MsprofReporterMgr::instance()->RegReportTypeInfo(level, typeId, typeName);
    EXPECT_EQ(typeName, MsprofReporterMgr::instance()->GetRegReportTypeInfo(level, typeId));
}
 
 
TEST_F(MsprofReporterMgrUtest, StopReporters)
{
    GlobalMockObject::verify();
    // stop before start
    EXPECT_EQ(PROFILING_SUCCESS, MsprofReporterMgr::instance()->StopReporters());
 
    // stop after start
    MOCKER_CPP(&Msprofiler::Api::ProfAclMgr::IsInited)
        .stubs()
        .will(returnValue(true));
    EXPECT_EQ(PROFILING_SUCCESS, MsprofReporterMgr::instance()->StartReporters());
    MsprofReporterMgr::instance()->RegReportTypeInfo(0, 0, "test");
    MsprofReporterMgr::instance()->RegReportTypeInfo(1, 1, "test");
    MOCKER_CPP(&MsprofCallbackHandler::StopReporter)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    MOCKER_CPP(&UploaderMgr::UploadData)
        .stubs()
        .will(returnValue(PROFILING_FAILED))
        .then(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_FAILED, MsprofReporterMgr::instance()->StopReporters());
    EXPECT_EQ(PROFILING_SUCCESS, MsprofReporterMgr::instance()->StopReporters());
    EXPECT_EQ(PROFILING_SUCCESS, MsprofReporterMgr::instance()->StopReporters());
}
test/msprof_cpp/ut/msprof/test/msprof_utest.cpp
1
0
