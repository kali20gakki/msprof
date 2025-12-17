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
#include "mockcpp/mockcpp.hpp"
#include "gtest/gtest.h"
#include <iostream>
#include "errno/error_code.h"
#include "prof_task.h"

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