/**
* @file running_mode.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef COLLECTOR_DVVP_MSPROFBIN_RUNNING_MODE_H
#define COLLECTOR_DVVP_MSPROFBIN_RUNNING_MODE_H
#include <set>
#include <vector>
#include "message/prof_params.h"
#include "utils/utils.h"
#include "prof_task.h"

namespace Collector {
namespace Dvvp {
namespace Msprofbin {

enum class ParseDataType {
    DATA_PATH_INVALID = 0,
    DATA_PATH_NON_CLUSTER,
    DATA_PATH_CLUSTER
};

class RunningMode {
public:
    // check params dependence in specific mode
    virtual int ModeParamsCheck() = 0;
    virtual int RunModeTasks() = 0;
    virtual void UpdateOutputDirInfo();
    void StopRunningTasks() const;
    void RemoveRecordFile(const std::string &fileName) const;
    SHARED_PTR_ALIA<Analysis::Dvvp::Msprof::ProfTask> GetRunningTask(const std::string &jobId);

    // marked when unexcepted quit
    bool isQuit_;
    std::string jobResultDir_;
    std::set<std::string> jobResultDirList_;
protected:
    RunningMode(std::string preCheckParams, std::string modeName,
        SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    virtual ~RunningMode();
    int CheckForbiddenParams() const;
    int CheckNeccessaryParams() const;
    void OutputUselessParams() const;
    int StartParseTask();
    int StartQueryTask();
    int StartExportTask();
    int RunExportSummaryTask(const analysis::dvvp::common::utils::ExecCmdParams &execCmdParams,
        std::vector<std::string> &envsV, int &exitCode);
    int RunExportTimelineTask(const analysis::dvvp::common::utils::ExecCmdParams &execCmdParams,
        std::vector<std::string> &envsV, int &exitCode);
    int CheckAnalysisEnv();
    int WaitRunningProcess(std::string processUsage) const;
    // for parse, export, query mode
    int GetOutputDirInfoFromParams();
    // for app, system mode
    int GetOutputDirInfoFromRecord();
    int HandleProfilingParams() const;

    std::string modeName_;
    // In any time, at most one child task Process is running in all the mode except system
    MmProcess taskPid_;
    std::string taskName_;
    std::string preCheckParams_;
    // forbidden params
    std::set<int> blackSet_;
    // useful params
    std::set<int> whiteSet_;
    std::set<int> neccessarySet_;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params_;
    std::map<std::string, SHARED_PTR_ALIA<Analysis::Dvvp::Msprof::ProfTask>> taskMap_;

private:
    std::string ConvertParamsSetToString(std::set<int>& srcSet) const;
    void SetEnvList(std::vector<std::string> &envsV);
    int CheckCurrentDataPathValid(const std::string &path) const;
    int CheckParseDataPathValid(const std::string &parseDataPath) const;
    ParseDataType GetParseDataType(const std::string &parseDataPath) const;

    std::string analysisPath_;
};

class AppMode : public RunningMode {
public:
    AppMode(std::string preCheckParams, SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    ~AppMode() override;
    int ModeParamsCheck() override;
    int RunModeTasks() override;
private:
    int StartAppTask(bool needWait = true);
    int StartAppTaskForDynProf();
};

class SystemMode : public RunningMode {
public:
    SystemMode(std::string preCheckParams, SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    ~SystemMode() override;
    int ModeParamsCheck() override;
    int RunModeTasks() override;
private:
    bool DataWillBeCollected() const;
    int CheckIfDeviceOnline() const;
    int CheckHostSysParams() const;
    int StartSysTask();
    int WaitSysTask() const;
    void StopTask();
    bool IsDeviceJob() const;
    int StartDeviceJobs(const std::string& device);
    int StartHostJobs();
    int CreateJobDir(std::string device, std::string &resultDir) const;
    int RecordOutPut() const;
    int StartHostTask(const std::string resultDir, const std::string device);
    int StartDeviceTask(const std::string resultDir, const std::string device);
    int CreateUploader(const std::string &jobId, const std::string &resultDir) const;
    bool CreateSampleJsonFile(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
        const std::string &resultDir);
    bool CreateDoneFile(const std::string &absolutePath, const std::string &fileSize) const;
    int WriteCtrlDataToFile(const std::string &absolutePath, const std::string &data, int dataLen) const;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> GenerateHostParam(
        SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> GenerateDeviceParam(
        SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params) const;

    std::string baseDir_;
};

class ParseMode : public RunningMode {
public:
    ParseMode(std::string preCheckParams, SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    ~ParseMode() override;
    int ModeParamsCheck() override;
    int RunModeTasks() override;
    void UpdateOutputDirInfo() override;
};

class QueryMode : public RunningMode {
public:
    QueryMode(std::string preCheckParams, SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    ~QueryMode() override;
    int ModeParamsCheck() override;
    int RunModeTasks() override;
    void UpdateOutputDirInfo() override;
};

class ExportMode : public RunningMode {
public:
    ExportMode(std::string preCheckParams, SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    ~ExportMode() override;
    int ModeParamsCheck() override;
    int RunModeTasks() override;
    void UpdateOutputDirInfo() override;
};

}
}
}


#endif