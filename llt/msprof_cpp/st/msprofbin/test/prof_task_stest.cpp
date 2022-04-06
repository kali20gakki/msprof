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

class PROF_TASK_UTEST : public testing::Test {
protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

TEST_F(PROF_TASK_UTEST, RpcTaskTest) {
    GlobalMockObject::verify();

    std::shared_ptr<analysis::dvvp::message::ProfileParams> params(
    new analysis::dvvp::message::ProfileParams);   
    SHARED_PTR_ALIA<ProfRpcTask> task(new ProfRpcTask(0, params));
    task->Init();
    task->Stop();
    task->PostSyncDataCtrl();
    task->UnInit();

}