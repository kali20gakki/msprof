/**
* @file task_manager.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ANALYSIS_DVVP_DEVICE_TASK_MGR_H
#define ANALYSIS_DVVP_DEVICE_TASK_MGR_H


#include <map>
#include <memory>
#include <mutex>
#include <string>
#include "prof_job_handler.h"
#include "singleton/singleton.h"

namespace analysis {
namespace dvvp {
namespace device {
class TaskManager : public analysis::dvvp::common::singleton::Singleton<TaskManager> {
public:
    TaskManager();
    virtual ~TaskManager();
    int Init();
    int Uninit();
    SHARED_PTR_ALIA<ProfJobHandler> CreateTask(int hostId, const std::string &jobId,
        SHARED_PTR_ALIA<analysis::dvvp::transport::ITransport> transport);
    SHARED_PTR_ALIA<ProfJobHandler> GetTask(const std::string &jobId);
    bool DeleteTask(const std::string &jobId);
    void ClearTask();
    void ConnectionReset(SHARED_PTR_ALIA<analysis::dvvp::transport::ITransport> transport);
private:
    bool isInited_;
    std::map<std::string, SHARED_PTR_ALIA<ProfJobHandler>> taskMap_;
    std::mutex mtx_;
};
} // device
} // dvvp
} // analysis
#endif
