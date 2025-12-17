/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "utils/utils.h"
#include "errno/error_code.h"
#include "collection_register.h"

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::message;
using namespace Analysis::Dvvp::JobWrapper;

static int g_processResult = PROFILING_FAILED;
static int g_uninitResult = PROFILING_FAILED;
static int g_globalJobLevel = false;

class UtestCollectionJob : public ICollectionJob
{
public:
    UtestCollectionJob();
    virtual ~UtestCollectionJob();
    int Init(const std::shared_ptr<CollectionJobCfg> cfg);
    int Process();
    int Uninit();
    bool IsGlobalJobLevel()
    {
        return g_globalJobLevel;
    }
};

UtestCollectionJob::UtestCollectionJob()
{
}

UtestCollectionJob::~UtestCollectionJob()
{
}

int UtestCollectionJob::Init(const std::shared_ptr<CollectionJobCfg> cfg)
{
    return PROFILING_SUCCESS;
}

int UtestCollectionJob::Process()
{
    return g_processResult;
}

int UtestCollectionJob::Uninit()
{
    return g_uninitResult;
}

class CollectionRegisterUtest: public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown()
    {
        GlobalMockObject::verify();
        g_processResult = PROFILING_FAILED;
        g_uninitResult = PROFILING_FAILED;
        g_globalJobLevel = false;
    }

};

TEST_F(CollectionRegisterUtest, CollectionJobRegisterAndRunWillReturnFailWhenInputInvalidParams)
{
    CollectionRegisterMgr mgr;
    std::shared_ptr<ICollectionJob> instance = std::make_shared<UtestCollectionJob>();

    int ret = mgr.CollectionJobRegisterAndRun(0, NR_MAX_COLLECTION_JOB, instance);
    EXPECT_EQ(PROFILING_FAILED, ret);

    ret = mgr.CollectionJobRegisterAndRun(-1, DDR_DRV_COLLECTION_JOB, instance);
    EXPECT_EQ(PROFILING_FAILED, ret);

    ret = mgr.CollectionJobRegisterAndRun(0, DDR_DRV_COLLECTION_JOB, nullptr);
    EXPECT_EQ(PROFILING_FAILED, ret);
}

TEST_F(CollectionRegisterUtest, CollectionJobRegisterAndRunWillReturnFailWhenInsertCollectionJobFail)
{
    CollectionRegisterMgr mgr;
    std::shared_ptr<ICollectionJob> instance = std::make_shared<UtestCollectionJob>();

    MOCKER_CPP(&CollectionRegisterMgr::CheckCollectionJobIsNoRegister)
        .stubs()
        .will(returnValue(false));
    int ret = mgr.CollectionJobRegisterAndRun(0, DDR_DRV_COLLECTION_JOB, instance);
    EXPECT_EQ(PROFILING_FAILED, ret);
}

TEST_F(CollectionRegisterUtest, CollectionJobRegisterAndRunWillReturnFailWhenJobProcessFail)
{
    CollectionRegisterMgr mgr;
    std::shared_ptr<ICollectionJob> instance = std::make_shared<UtestCollectionJob>();

    MOCKER_CPP(&CollectionRegisterMgr::CheckCollectionJobIsNoRegister)
        .stubs()
        .will(returnValue(true));
    g_processResult = PROFILING_FAILED;
    int ret = mgr.CollectionJobRegisterAndRun(0, DDR_DRV_COLLECTION_JOB, instance);
    EXPECT_EQ(PROFILING_FAILED, ret);
}

TEST_F(CollectionRegisterUtest, CollectionJobRegisterAndRunWillReturnSuccWhenJobProcessSucc)
{
    CollectionRegisterMgr mgr;
    std::shared_ptr<ICollectionJob> instance = std::make_shared<UtestCollectionJob>();

    MOCKER_CPP(&CollectionRegisterMgr::CheckCollectionJobIsNoRegister)
        .stubs()
        .will(returnValue(true));
    g_processResult = PROFILING_SUCCESS;
    int ret = mgr.CollectionJobRegisterAndRun(0, DDR_DRV_COLLECTION_JOB, instance);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(CollectionRegisterUtest, CollectionJobRunWillReturnFailWhenInputInvalidParams)
{
    CollectionRegisterMgr mgr;

    int ret = mgr.CollectionJobRun(-1, DDR_DRV_COLLECTION_JOB);
    EXPECT_EQ(PROFILING_FAILED, ret);

    ret = mgr.CollectionJobRun(0, NR_MAX_COLLECTION_JOB);
    EXPECT_EQ(PROFILING_FAILED, ret);
}

TEST_F(CollectionRegisterUtest, CollectionJobRunWillReturnFailWhenJobNotRegistered)
{
    CollectionRegisterMgr mgr;

    std::map<ProfCollectionJobE, SHARED_PTR_ALIA<ICollectionJob>> fakeJobs;
    mgr.collectionJobs_[0] = fakeJobs;
    int ret = mgr.CollectionJobRun(0, DDR_DRV_COLLECTION_JOB);
    EXPECT_EQ(PROFILING_FAILED, ret);
}

TEST_F(CollectionRegisterUtest, CollectionJobRunWillReturnFailWhenProcessFail)
{
    CollectionRegisterMgr mgr;
    g_processResult = PROFILING_FAILED;
    std::shared_ptr<ICollectionJob> instance = std::make_shared<UtestCollectionJob>();
    std::map<ProfCollectionJobE, SHARED_PTR_ALIA<ICollectionJob>> fakeJobs;
    fakeJobs[DDR_DRV_COLLECTION_JOB] = instance;
    mgr.collectionJobs_[0] = fakeJobs;
    int ret = mgr.CollectionJobRun(0, DDR_DRV_COLLECTION_JOB);
    EXPECT_EQ(PROFILING_FAILED, ret);
}

TEST_F(CollectionRegisterUtest, CollectionJobRunWillReturnSuccWhenProcessSucc)
{
    CollectionRegisterMgr mgr;
    g_processResult = PROFILING_SUCCESS;
    std::shared_ptr<ICollectionJob> instance = std::make_shared<UtestCollectionJob>();
    std::map<ProfCollectionJobE, SHARED_PTR_ALIA<ICollectionJob>> fakeJobs;
    fakeJobs[DDR_DRV_COLLECTION_JOB] = instance;
    mgr.collectionJobs_[0] = fakeJobs;
    int ret = mgr.CollectionJobRun(0, DDR_DRV_COLLECTION_JOB);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(CollectionRegisterUtest, CollectionJobUnregisterAndStopWillReturnFailWhenInputInvalidParams)
{
    CollectionRegisterMgr mgr;

    int ret = mgr.CollectionJobUnregisterAndStop(0, NR_MAX_COLLECTION_JOB);
    EXPECT_EQ(PROFILING_FAILED, ret);

    ret = mgr.CollectionJobUnregisterAndStop(-1, DDR_DRV_COLLECTION_JOB);
    EXPECT_EQ(PROFILING_FAILED, ret);
}

TEST_F(CollectionRegisterUtest, CollectionJobUnregisterAndStopWillReturnFailWhenGetAndDelCollectionJobFail)
{
    CollectionRegisterMgr mgr;

    MOCKER_CPP(&CollectionRegisterMgr::GetAndDelCollectionJob)
        .stubs()
        .will(returnValue(false));
    int ret = mgr.CollectionJobUnregisterAndStop(0, DDR_DRV_COLLECTION_JOB);
    EXPECT_EQ(PROFILING_FAILED, ret);
}

TEST_F(CollectionRegisterUtest, CollectionJobUnregisterAndStopWillReturnFailWhenUninitFail)
{
    CollectionRegisterMgr mgr;

    std::shared_ptr<ICollectionJob> instance = std::make_shared<UtestCollectionJob>();
    mgr.InsertCollectionJob(0, DDR_DRV_COLLECTION_JOB, instance);
    int ret = mgr.CollectionJobUnregisterAndStop(0, DDR_DRV_COLLECTION_JOB);
    EXPECT_EQ(PROFILING_FAILED, ret);
}

TEST_F(CollectionRegisterUtest, CollectionJobUnregisterAndStopWillReturnSuccWhenUninitSucc)
{
    CollectionRegisterMgr mgr;

    std::shared_ptr<ICollectionJob> instance = std::make_shared<UtestCollectionJob>();
    g_uninitResult = PROFILING_SUCCESS;
    mgr.InsertCollectionJob(0, DDR_DRV_COLLECTION_JOB, instance);
    int ret = mgr.CollectionJobUnregisterAndStop(0, DDR_DRV_COLLECTION_JOB);
    EXPECT_EQ(PROFILING_SUCCESS, ret);
}

TEST_F(CollectionRegisterUtest, InsertCollectionJobWillReturnFalseWhenInputInvalidParams)
{
    CollectionRegisterMgr mgr;

    std::shared_ptr<ICollectionJob> instance = std::make_shared<UtestCollectionJob>();

    bool ret = mgr.InsertCollectionJob(-1, DDR_DRV_COLLECTION_JOB, instance);
    EXPECT_EQ(false, ret);

    ret = mgr.InsertCollectionJob(0, NR_MAX_COLLECTION_JOB, instance);
    EXPECT_EQ(false, ret);

    ret = mgr.InsertCollectionJob(0, DDR_DRV_COLLECTION_JOB, nullptr);
    EXPECT_EQ(false, ret);
}

TEST_F(CollectionRegisterUtest, InsertCollectionJobWillReturnFailWhenJobNotRegistered)
{
    CollectionRegisterMgr mgr;

    std::shared_ptr<ICollectionJob> instance = std::make_shared<UtestCollectionJob>();
    MOCKER_CPP(&CollectionRegisterMgr::CheckCollectionJobIsNoRegister)
        .stubs()
        .will(returnValue(false));

    bool ret = mgr.InsertCollectionJob(0, DDR_DRV_COLLECTION_JOB, instance);
    EXPECT_EQ(false, ret);
}

TEST_F(CollectionRegisterUtest, InsertCollectionJobWillReturnTrueWhenJobRegistered)
{
    CollectionRegisterMgr mgr;

    std::shared_ptr<ICollectionJob> instance = std::make_shared<UtestCollectionJob>();
    MOCKER_CPP(&CollectionRegisterMgr::CheckCollectionJobIsNoRegister)
        .stubs()
        .will(returnValue(true));

    bool ret = mgr.InsertCollectionJob(0, DDR_DRV_COLLECTION_JOB, instance);
    EXPECT_EQ(true, ret);
}

TEST_F(CollectionRegisterUtest, GetAndDelCollectionJobWillReturnFalseWhenInputInvalidParams)
{
    CollectionRegisterMgr mgr;
    std::shared_ptr<ICollectionJob> instance = std::make_shared<UtestCollectionJob>();
    bool ret = mgr.GetAndDelCollectionJob(0, NR_MAX_COLLECTION_JOB, instance);
    EXPECT_EQ(false, ret);

    ret = mgr.GetAndDelCollectionJob(-1, DDR_DRV_COLLECTION_JOB, instance);
    EXPECT_EQ(false, ret);
}

TEST_F(CollectionRegisterUtest, GetAndDelCollectionJobWillReturnFalseWhenCheckCollectionJobNoRegister)
{
    CollectionRegisterMgr mgr;
    std::shared_ptr<ICollectionJob> instance = std::make_shared<UtestCollectionJob>();
    MOCKER_CPP(&CollectionRegisterMgr::CheckCollectionJobIsNoRegister)
        .stubs()
        .will(returnValue(true));
    bool ret = mgr.GetAndDelCollectionJob(0, DDR_DRV_COLLECTION_JOB, instance);
    EXPECT_EQ(false, ret);
}

TEST_F(CollectionRegisterUtest, GetAndDelCollectionJobWillReturnFalseWhenCheckIdNotEqualToInputDevId)
{
    CollectionRegisterMgr mgr;
    std::shared_ptr<ICollectionJob> instance = std::make_shared<UtestCollectionJob>();
    g_globalJobLevel = true;
    std::map<ProfCollectionJobE, SHARED_PTR_ALIA<ICollectionJob>> fakeJobs;
    fakeJobs[DDR_DRV_COLLECTION_JOB] = instance;
    mgr.collectionJobs_[1] = fakeJobs;
    bool ret = mgr.GetAndDelCollectionJob(0, DDR_DRV_COLLECTION_JOB, instance);
    EXPECT_EQ(false, ret);
}

TEST_F(CollectionRegisterUtest, GetAndDelCollectionJobWillReturnTrueWhenInputValidParams)
{
    CollectionRegisterMgr mgr;
    std::shared_ptr<ICollectionJob> instance = std::make_shared<UtestCollectionJob>();
    g_globalJobLevel = false;
    std::map<ProfCollectionJobE, SHARED_PTR_ALIA<ICollectionJob>> fakeJobs;
    fakeJobs[DDR_DRV_COLLECTION_JOB] = instance;
    mgr.collectionJobs_[0] = fakeJobs;
    bool ret = mgr.GetAndDelCollectionJob(0, DDR_DRV_COLLECTION_JOB, instance);
    EXPECT_EQ(true, ret);
}

TEST_F(CollectionRegisterUtest, CheckCollectionJobIsNoRegisterWillReturnFalseWhenInputInvalidParams)
{
    CollectionRegisterMgr mgr;
    int devId = 0;
    bool ret = mgr.CheckCollectionJobIsNoRegister(devId, NR_MAX_COLLECTION_JOB);
    EXPECT_EQ(false, ret);
    devId = -1;
    ret = mgr.CheckCollectionJobIsNoRegister(devId, DDR_DRV_COLLECTION_JOB);
    EXPECT_EQ(false, ret);
}
