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
#ifndef ANALYSIS_DVVP_JOB_WRAPPER_PROF_TIMER_H
#define ANALYSIS_DVVP_JOB_WRAPPER_PROF_TIMER_H

#include <bitset>
#include <map>
#include <unordered_map>
#include <mutex>
#include <fstream>
#include <csignal>
#include "memory/chunk_pool.h"
#include "message/prof_params.h"
#include "singleton/singleton.h"
#include "thread/thread.h"
#include "transport/uploader.h"
#include "utils/utils.h"


namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
using namespace analysis::dvvp::common::utils;
const char * const PROF_PROC_STAT = "/proc/stat";
const char * const PROF_PROC_MEM = "/proc/meminfo";
const char * const PROF_NET_STAT = "/proc/net/dev";
const char * const PROF_PROC_UPTIME = "/proc/uptime";

enum TimerHandlerTag {
    PROF_SYS_STAT,
    PROF_SYS_MEM,
    PROF_SYS_ALL_PID,
    PROF_HOST_PID_CPU,
    PROF_HOST_PID_MEM,
    PROF_HOST_SYS_CPU,
    PROF_HOST_SYS_MEM,
    PROF_HOST_ALL_PID_CPU,
    PROF_HOST_ALL_PID_MEM,
    PROF_HOST_SYS_NETWORK,
    PROF_NETDEV_STATS
};

class TimerHandler {
public:
    explicit TimerHandler(TimerHandlerTag tag);
    virtual ~TimerHandler();
public:
    virtual int Execute() = 0;
    virtual int Init() = 0;
    virtual int Uinit() = 0;
    TimerHandlerTag GetTag();
public:
    TimerHandlerTag tag_;
};

class ProcTimerHandler : public TimerHandler {
public:
    ProcTimerHandler(TimerHandlerTag tag, unsigned int devId, unsigned int bufSize,
            unsigned sampleIntervalMs, const std::string &srcFileName, const std::string &retFileName,
            SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param,
            SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
            SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader);
    ~ProcTimerHandler() override;

public:
    int Execute() override;
    int Init() override;
    int Uinit() override;
protected:
    virtual void ParseProcFile(std::ifstream &ifs, std::string &data) = 0;

protected:
    void PacketData(std::string &dest, std::string &data, unsigned int headSize);
    void StoreData(std::string &data);
    void SendData(CONST_UNSIGNED_CHAR_PTR buf, unsigned int size);
    void FlushBuf();
    bool IsValidData(std::ifstream &ifs, std::string &data) const;
    bool CheckFileSize(const std::string &file) const;

protected:
    analysis::dvvp::common::memory::Chunk          buf_;
    std::ifstream   if_;

private:
    volatile bool   isInited_;
    unsigned long long    prevTimeStamp_;
    unsigned long long    sampleIntervalNs_;
    unsigned int    index_;
    std::string     srcFileName_;
    std::string     retFileName_;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param_;
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx_;
    SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader_;
};

class ProcHostCpuHandler : public ProcTimerHandler {
public:
    ProcHostCpuHandler(TimerHandlerTag tag, unsigned int bufSize,
            unsigned sampleIntervalMs, const std::string &retFileName,
            SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param,
            SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
            SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader);
    ~ProcHostCpuHandler() override;

private:
    void ParseProcFile(std::ifstream &ifs, std::string &data) override;
    void ParseProcTidStat(std::string &data);
    void ParseSysTime(std::string &data);

private:
    std::string sysTimeSrc_;
    std::string taskSrc_;
};

class ProcHostMemHandler : public ProcTimerHandler {
public:
    ProcHostMemHandler(TimerHandlerTag tag, unsigned int bufSize,
            unsigned sampleIntervalMs, const std::string &retFileName,
            SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param,
            SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
            SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader);
    ~ProcHostMemHandler() override;

private:
    void ParseProcFile(std::ifstream &ifs, std::string &data) override;
    void ParseProcMemUsage(std::string &data);

private:
    std::string statmSrc_;
};

class ProcHostNetworkHandler : public ProcTimerHandler {
public:
    ProcHostNetworkHandler(TimerHandlerTag tag, unsigned int bufSize,
            unsigned sampleIntervalMs, const std::string &retFileName,
            SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param,
            SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
            SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader);
    ~ProcHostNetworkHandler() override;

private:
    void ParseProcFile(std::ifstream &ifs, std::string &data) override;
    void ParseNetStat(std::string &data);
};

class ProcStatFileHandler : public ProcTimerHandler {
public:
    ProcStatFileHandler(TimerHandlerTag tag, unsigned int devId, unsigned int bufSize,
            unsigned sampleIntervalMs, const std::string &srcFileName, const std::string &retFileName,
            SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param,
            SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
            SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader);
    ~ProcStatFileHandler() override;

private:
    void ParseProcFile(std::ifstream &ifs, std::string &data) override;
};

class ProcPidStatFileHandler : public ProcTimerHandler {
public:
    ProcPidStatFileHandler(TimerHandlerTag tag, unsigned int devId, unsigned int bufSize,
            unsigned sampleIntervalMs, const std::string &srcFileName, const std::string &retFileName,
            SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param,
            SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
            SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader,
            unsigned int pid);
    ~ProcPidStatFileHandler() override;

private:
    void ParseProcFile(std::ifstream &ifs, std::string &data) override;
    void GetProcessName(std::string &processName);
private:
    unsigned int pid_;
    std::ifstream ifStat_;
};

class ProcMemFileHandler : public ProcTimerHandler {
public:
    ProcMemFileHandler(TimerHandlerTag tag, unsigned int devId, unsigned int bufSize,
            unsigned sampleIntervalMs, const std::string &srcFileName, const std::string &retFileName,
            SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param,
            SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
            SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader);
    ~ProcMemFileHandler() override;

private:
    void ParseProcFile(std::ifstream &ifs, std::string &data) override;
};

class ProcPidMemFileHandler : public ProcTimerHandler {
public:
    ProcPidMemFileHandler(TimerHandlerTag tag, unsigned int devId, unsigned int bufSize,
            unsigned sampleIntervalMs, const std::string &srcFileName, const std::string &retFileName,
            SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param,
            SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
            SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader,
            unsigned int pid);
    ~ProcPidMemFileHandler() override;

private:
    void ParseProcFile(std::ifstream &ifs, std::string &data) override;
    void GetProcessName(std::string &processName);

private:
    unsigned int pid_;
};

class ProcPidFileHandler {
public:
    ProcPidFileHandler() {}
    virtual ~ProcPidFileHandler() {}
    void Execute();

public:
    SHARED_PTR_ALIA<ProcPidMemFileHandler>  memHandler_;
    SHARED_PTR_ALIA<ProcPidStatFileHandler> statHandler_;
};

class ProcAllPidsFileHandler : public TimerHandler {
public:
    ProcAllPidsFileHandler(TimerHandlerTag tag, unsigned int devId,
            unsigned sampleIntervalNs, SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param,
            SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
            SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader);
    ~ProcAllPidsFileHandler() override;

public:
    int Execute() override;
    int Init() override;
    int Uinit() override;

public:
    static void GetProcessName(unsigned int pid, std::string &processName);

private:
    void ParseProcFile(const std::ifstream &ifs, const std::string &data) const;
    void GetCurPids(std::vector<unsigned int> &curPids);
    void GetNewExitPids(std::vector<unsigned int> &curPids, std::vector<unsigned int> &prevPids,
            std::vector<unsigned int> &newPids, std::vector<unsigned int> &exitPids);
    void HandleNewPidsCpu(std::vector<unsigned int> &newPids);
    void HandleNewPidsMem(std::vector<unsigned int> &newPids);
    void HandleExitPids(std::vector<unsigned int> &exitPids);

private:
    unsigned int devId_;
    unsigned long long prevTimeStamp_;
    unsigned long long sampleIntervalNs_;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param_;
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx_;
    SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader_;

    std::map<unsigned int, SHARED_PTR_ALIA<ProcPidFileHandler> > pidsMap_;
    std::vector<unsigned int> prevPids_;
};

class NetDevStatsHandler : public TimerHandler {
public:
    NetDevStatsHandler(TimerHandlerTag tag, size_t bufSize, uint64_t sampleIntervalNs,
        const std::string &retFileName, SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx);
    ~NetDevStatsHandler() override = default;

    int Execute() override;
    int Init() override;
    int Uinit() override;
    int RegisterDevTask(uint32_t devId);
    int RemoveDevTask(uint32_t devId);
    size_t GetCurDevTaskCount();

private:
    void StoreData(uint32_t devId, std::string data);
    void SendData(uint32_t devId, CONST_UNSIGNED_CHAR_PTR buf, size_t size);
    bool GetDcmiCardDevId(uint32_t devId, int &dcmiCardId, int &dcmiDevId);
    std::unordered_map<uint32_t, std::pair<int, int>> GetCurDcmiCardDevIdMap();

private:
    volatile bool isInited_;
    unsigned long long prevTimeStamp_;
    size_t bufSize_;
    uint64_t sampleIntervalNs_;
    std::string retFileName_;
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx_;
    std::mutex devTaskMtx_;
    std::unordered_map<uint32_t, std::pair<int, int>> dcmiCardDevIdMap_;
    std::unordered_map<uint32_t, SHARED_PTR_ALIA<analysis::dvvp::common::memory::Chunk>> devTaskBufs_;
};

struct TimerParam {
    explicit TimerParam(unsigned long interval)
        : intervalUsec(interval)
    {}
    unsigned long intervalUsec;
};

class ProfTimer : public analysis::dvvp::common::thread::Thread {
public:
    explicit ProfTimer(SHARED_PTR_ALIA<struct TimerParam>);
    ~ProfTimer() override;

public:
    int RegisterTimerHandler(TimerHandlerTag tag, SHARED_PTR_ALIA<TimerHandler> handler);
    int RemoveTimerHandler(TimerHandlerTag tag);
    SHARED_PTR_ALIA<TimerHandler> GetTimerHandler(TimerHandlerTag tag);

public:
    int Init();
    int Uinit();
    int Start() override;
    int Stop() override;

protected:
    void Run(const struct error_message::Context &errorContext) override;

private:
    int Handler();

private:
    volatile bool isStarted_;
    SHARED_PTR_ALIA<struct TimerParam> timerParam_;
    std::mutex mtx_;
    std::map<enum TimerHandlerTag, SHARED_PTR_ALIA<TimerHandler> > handlerMap_;
};

class TimerManager : public analysis::dvvp::common::singleton::Singleton<TimerManager> {
    friend analysis::dvvp::common::singleton::Singleton<TimerManager>;
public:
    void StartProfTimer();
    void StopProfTimer();
    void RegisterProfTimerHandler(TimerHandlerTag tag, SHARED_PTR_ALIA<TimerHandler> handler);
    void RemoveProfTimerHandler(TimerHandlerTag tag);
    SHARED_PTR_ALIA<TimerHandler> GetProfTimerHandler(TimerHandlerTag tag);

protected:
    TimerManager();
    virtual ~TimerManager();

private:
    volatile int profTimerCnt_;
    std::mutex profTimerMtx_;
    SHARED_PTR_ALIA<Analysis::Dvvp::JobWrapper::ProfTimer> profTimer_;
};
} // namespace JobWrapper
} // namespace Dvvp
} // namespace Analysis
#endif // ANALYSIS_DVVP_JOB_WRAPPER_PROF_TIMER_H
