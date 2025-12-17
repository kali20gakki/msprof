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