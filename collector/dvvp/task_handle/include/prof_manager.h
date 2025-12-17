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
#ifndef ANALYSIS_DVVP_HOST_PROF_MANAGER_H
#define ANALYSIS_DVVP_HOST_PROF_MANAGER_H

#include <map>
#include "message/prof_params.h"
#include "prof_task.h"
#include "singleton/singleton.h"
#include "transport/prof_channel.h"
#include "app/application.h"
#include "utils/utils.h"

namespace analysis {
namespace dvvp {
namespace host {
class ProfManager : public analysis::dvvp::common::singleton::Singleton<ProfManager> {
    friend analysis::dvvp::common::singleton::Singleton<ProfManager>;

public:
    int AclInit();
    int AclUinit();

    int Handle(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);

public:
    SHARED_PTR_ALIA<ProfTask> GetTask(const std::string &jobId);
    int StopTask(const std::string &jobId);
    int OnTaskFinished(const std::string &jobId);
    int WriteCtrlDataToFile(const std::string &absolutePath, const std::string &data, int dataLen);
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> HandleProfilingParams(
        const std::string &sampleConfig);
    int IdeCloudProfileProcess(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    bool CheckIfDevicesOnline(const std::string paramsDevices, std::string &statusInfo) const;

protected:
    ProfManager() : isInited_(false)
    {
    }
    virtual ~ProfManager()
    {
    }

private:
    bool CreateSampleJsonFile(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
                              const std::string &resultDir);
    bool CheckHandleSuc(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
                        analysis::dvvp::message::StatusInfo &statusInfo);
    int ProcessHandleFailed(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);

private:
    int LaunchTask(SHARED_PTR_ALIA<ProfTask> task, const std::string &jobId, std::string &info);
    SHARED_PTR_ALIA<ProfTask> GetTaskNoLock(const std::string &jobId);
    bool IsDeviceProfiling(const std::vector<std::string> &devices);
    bool CreateDoneFile(const std::string &absolutePath, const std::string &fileSize) const;
    bool PreGetDeviceList(std::vector<int> &devIds) const;

private:
    bool isInited_;
    std::mutex taskMtx_;
    std::map<std::string, SHARED_PTR_ALIA<ProfTask> > _tasks;  // taskptr, task
};
}  // namespace host
}  // namespace dvvp
}  // namespace analysis

#endif
