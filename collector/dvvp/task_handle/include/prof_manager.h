/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_HOST_PROF_MANAGER_H
#define ANALYSIS_DVVP_HOST_PROF_MANAGER_H

#include <map>
#include "message/prof_params.h"
#include "prof_task.h"
#include "singleton/singleton.h"
#include "transport/transport.h"
#include "transport/hdc/device_transport.h"
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
    void SendFailedStatusInfo(const analysis::dvvp::message::StatusInfo &statusInfo,
        const std::string &jobId);

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
