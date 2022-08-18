/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: the job factory class.
 * Create: 2019.11.30
 */
#ifndef ANALYSIS_DVVP_JOB_FACTORY_H
#define ANALYSIS_DVVP_JOB_FACTORY_H

#include "job_adapter.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
class JobFactory {
public:
    JobFactory();
    virtual ~JobFactory();
};

class JobSocFactory : public JobFactory {
public:
    JobSocFactory();
    ~JobSocFactory() override;
public:
    SHARED_PTR_ALIA<JobAdapter> CreateJobAdapter(int devIndexId);
};
}}}

#endif
