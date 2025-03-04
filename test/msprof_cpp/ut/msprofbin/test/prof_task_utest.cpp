#include "mockcpp/mockcpp.hpp"
#include "gtest/gtest.h"
#include <iostream>
#include "errno/error_code.h"
#include "prof_task.h"
#include "message/codec.h"
#include "config/config.h"
#include "config/config_manager.h"

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::Msprof;

class ProfTaskUtest : public testing::Test {
protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

TEST_F(ProfTaskUtest, RpcTaskTest)
{
    GlobalMockObject::verify();

    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    SHARED_PTR_ALIA<ProfRpcTask> task(new ProfRpcTask(0, params));
    task->Init();
    task->Stop();
    task->PostSyncDataCtrl();
    task->UnInit();
}

TEST_F(ProfTaskUtest, SocTaskTest)
{
    GlobalMockObject::verify();
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    MSVP_MAKE_SHARED0_VOID(params, analysis::dvvp::message::ProfileParams);
    params->host_profiling = true;
    SHARED_PTR_ALIA<ProfSocTask> task(new ProfSocTask(0, params));
    MOCKER_CPP(&analysis::dvvp::transport::UploaderMgr::UploadCtrlFileData)
        .stubs()
        .will(returnValue(PROFILING_SUCCESS));
    EXPECT_EQ(PROFILING_SUCCESS, task->Init());
    task->Start();
    task->Stop();
    task->Wait();
    EXPECT_EQ(PROFILING_SUCCESS, task->UnInit());
}