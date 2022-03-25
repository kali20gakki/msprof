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
