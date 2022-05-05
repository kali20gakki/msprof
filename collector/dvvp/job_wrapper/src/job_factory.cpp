/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: the job factory class.
 * Create: 2019.11.30
 */
#include "job_factory.h"
#include "job_device_soc.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
using namespace Analysis::Dvvp::JobWrapper;

JobFactory::JobFactory()
{
}

JobFactory::~JobFactory()
{
}

JobSocFactory::JobSocFactory()
{}
JobSocFactory::~JobSocFactory()
{}

SHARED_PTR_ALIA<JobAdapter> JobSocFactory::CreateJobAdapter(int devIndexId)
{
    SHARED_PTR_ALIA<JobDeviceSoc> job = nullptr;
    MSVP_MAKE_SHARED1_RET(job, JobDeviceSoc, devIndexId, nullptr);
    return job;
}
}}}