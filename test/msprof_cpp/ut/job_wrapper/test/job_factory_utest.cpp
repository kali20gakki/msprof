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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "job_factory.h"

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::message;
using namespace Analysis::Dvvp::JobWrapper;

class JOB_FACTORY_UTEST: public testing::Test {
protected:
    virtual void SetUp() {
    }
    virtual void TearDown() {

    }
};


TEST_F(JOB_FACTORY_UTEST, JobSocFactory_CreateJobAdapter) {
    std::shared_ptr<JobSocFactory> jobFactory;
    MSVP_MAKE_SHARED0_VOID(jobFactory, JobSocFactory);
    EXPECT_NE(nullptr, jobFactory);
    EXPECT_NE(nullptr, jobFactory->CreateJobAdapter(0));
}
