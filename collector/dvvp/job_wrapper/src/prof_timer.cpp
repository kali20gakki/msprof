/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: class timer
 * Author: lixubo
 * Create: 2019-04-23
 */

#include "prof_timer.h"
#include <algorithm>
#include <cstdio>
#include <cctype>
#include <string>
#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "config/config.h"
#include "msprof_dlog.h"
#include "proto/profiler.pb.h"
#include "utils/utils.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::utils;
using namespace Analysis::Dvvp::MsprofErrMgr;

const char * const PROC_FILE = "/proc";
const char * const PROC_COMM_FILE = "comm";
const char * const PROC_CPU = "cpu";
const char * const PROC_SELF = "self";
const char * const PROC_PID_STAT = "stat";
const char * const PROC_TID_STAT = "stat";
const char * const PROC_PID_MEM = "statm";

const char * const PROC_TASK = "task";

const char * const PROF_PID_MEM_FILE = "Memory.data";
const char * const PROF_PID_STAT_FILE = "CpuUsage.data";

const char * const PROF_DATA_TIMER_STAMP = "TimeStamp:";
const char * const PROF_DATA_INDEX = "\nIndex:";
const char * const PROF_DATA_LEN = "\nDataLen:";
const char * const PROF_DATA_PROCESSNAME = "ProcessName:";

TimerHandler::TimerHandler(TimerHandlerTag tag)
    : tag_(tag)
{
}

TimerHandler::~TimerHandler()
{
}

TimerHandlerTag TimerHandler::GetTag()
{
    return tag_;
}

ProcTimerHandler::ProcTimerHandler(TimerHandlerTag tag, unsigned int devId /* = 0 */, unsigned int bufSize,
                                   unsigned sampleIntervalNs,
                                   const std::string &srcFileName, const std::string &retFileName,
                                   SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param,
                                   SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
                                   SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader)
    : TimerHandler(tag),
      buf_(bufSize),
      isInited_(false),
      prevTimeStamp_(0),
      sampleIntervalNs_(sampleIntervalNs),
      index_(0),
      srcFileName_(srcFileName),
      retFileName_("data/" + retFileName), param_(param), jobCtx_(jobCtx), upLoader_(upLoader)
{
    UNUSED(devId);
}

ProcTimerHandler::~ProcTimerHandler()
{
}

int ProcTimerHandler::Init()
{
    int ret = PROFILING_FAILED;
    do {
        if (isInited_) {
            MSPROF_LOGE("The Handler is inited");
            MSPROF_INNER_ERROR("EK9999", "The Handler is inited");
            break;
        }

        if (!buf_.Init()) {
            MSPROF_LOGE("Buf init failed");
            MSPROF_INNER_ERROR("EK9999", "Buf init failed");
            break;
        }

        prevTimeStamp_ = 0;
        isInited_ = true;
        ret = PROFILING_SUCCESS;
    } while (0);

    return ret;
}

int ProcTimerHandler::Uinit()
{
    if (!isInited_) {
        return PROFILING_SUCCESS;
    }

    FlushBuf();

    upLoader_.reset();
    buf_.Uninit();

    isInited_ = false;
    return PROFILING_SUCCESS;
}

int ProcTimerHandler::Execute()
{
    static const unsigned int PROF_DATE_HEADER_SIZE = 100; // header size:100B
    do {
        if (!isInited_) {
            MSPROF_LOGE("ProcTimerHandler is not inited: %s", retFileName_.c_str());
            MSPROF_INNER_ERROR("EK9999", "ProcTimerHandler is not inited: %s", retFileName_.c_str());
            break;
        }

        unsigned long long curTimeStamp = analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw();
        if ((curTimeStamp - prevTimeStamp_) < sampleIntervalNs_ && (prevTimeStamp_ != 0)) {
            continue;
        }
        std::string src;
        std::string dest;
        prevTimeStamp_ = curTimeStamp;
        if (!srcFileName_.empty()) {
            if (!CheckFileSize(srcFileName_)) {
                break;
            }
            if_.open(srcFileName_, std::ifstream::in);
            if (!if_.is_open()) {
                MSPROF_LOGW("Open file %s failed", srcFileName_.c_str());
                break;
            }
            ParseProcFile(if_, src);
            if_.close();

            PacketData(dest, src, PROF_DATE_HEADER_SIZE);
        } else {
            ParseProcFile(if_, src);
            dest = src;
        }

        StoreData(dest);
        index_++;
    } while (0);

    return PROFILING_SUCCESS;
}

void ProcTimerHandler::PacketData(std::string &dest, std::string &data, unsigned int headSize)
{
    if (data.size() == 0) {
        MSPROF_LOGW("data is empty, fileName:%s", srcFileName_.c_str());
        return;
    }

    dest.reserve(data.size() + headSize);

    dest += PROF_DATA_TIMER_STAMP;
    dest += std::to_string(prevTimeStamp_);

    dest += PROF_DATA_INDEX;
    dest += std::to_string(index_);

    dest += PROF_DATA_LEN;
    dest += std::to_string(data.size());
    dest += "\n";

    dest += data;
    dest += "\n";
}

void ProcTimerHandler::StoreData(std::string &data)
{
    if (data.size() == 0) {
        return;
    }

    if (buf_.GetFreeSize() < data.size() && buf_.GetUsedSize() != 0) {
        // send buf data
        MSPROF_LOGD("StoreData %d", static_cast<int>(buf_.GetUsedSize()));
        SendData(buf_.GetBuffer(), buf_.GetUsedSize());
        buf_.SetUsedSize(0);
    }

    if (buf_.GetFreeSize() < data.size()) {
        // send data
        SendData(reinterpret_cast<CONST_UNSIGNED_CHAR_PTR>(data.c_str()), data.size());
        return;
    }

    unsigned int spaceSize = buf_.GetFreeSize();
    unsigned int usedSize = buf_.GetUsedSize();
    auto dataBuf = buf_.GetBuffer();
    if (dataBuf == nullptr) {
        return;
    }
    errno_t err = memcpy_s(static_cast<void *>(const_cast<UNSIGNED_CHAR_PTR>(buf_.GetBuffer() + usedSize)),
                           spaceSize, data.c_str(), data.size());
    if (err != EOK) {
        MSPROF_LOGE("memcpy stat data failed: %d", static_cast<int>(err));
        MSPROF_INNER_ERROR("EK9999", "memcpy stat data failed: %d", static_cast<int>(err));
    } else {
        buf_.SetUsedSize(usedSize + data.size());
    }

    if (buf_.GetFreeSize() <= (buf_.GetBufferSize() * 1) / 4) { // 1 / 4 space size
        // send buf data
        SendData(buf_.GetBuffer(), buf_.GetUsedSize());
        buf_.SetUsedSize(0);
    }
}

void ProcTimerHandler::SendData(CONST_UNSIGNED_CHAR_PTR buf, unsigned int size)
{
    if (buf == nullptr) {
        MSPROF_LOGE("buf to be sent is nullptr");
        MSPROF_INNER_ERROR("EK9999", "buf to be sent is nullptr");
        return;
    }
    TimerHandlerTag tag = GetTag();
    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunk;
    MSVP_MAKE_SHARED0_VOID(fileChunk, analysis::dvvp::proto::FileChunkReq);

    fileChunk->set_filename(retFileName_);
    fileChunk->set_offset(-1);
    fileChunk->set_chunk(buf, size);
    fileChunk->set_chunksizeinbytes(size);
    fileChunk->set_islastchunk(false);
    fileChunk->set_needack(false);
    fileChunk->mutable_hdr()->set_job_ctx(jobCtx_->ToString());
    if (tag >= PROF_HOST_PROC_CPU && tag <= PROF_HOST_SYS_NETWORK) {
        fileChunk->set_datamodule(analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_FROM_MSPROF_HOST);
    } else {
        fileChunk->set_datamodule(analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_FROM_DEVICE);
    }

    std::string encoded = analysis::dvvp::message::EncodeMessage(fileChunk);
    if (upLoader_ == nullptr) {
        MSPROF_LOGE("[ProcTimerHandler::SendData] upLoader_ is null");
        MSPROF_INNER_ERROR("EK9999", "upLoader_ is null");
        return;
    }
    int ret = upLoader_->UploadData(static_cast<void *>(const_cast<CHAR_PTR>(encoded.c_str())),
        (uint32_t)encoded.size());
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[ProcTimerHandler::SendData] Upload Data Failed");
        MSPROF_INNER_ERROR("EK9999", "Upload Data Failed");
    }
}

void ProcTimerHandler::FlushBuf()
{
    MSPROF_LOGI("FlushBuf %s, the total index :%lu", retFileName_.c_str(), index_);

    size_t usedSize = buf_.GetUsedSize();
    MSPROF_LOGI("FlushBuf, isInited_:%d, bufUsedSize:%d", isInited_ ? 1 : 0, usedSize);
    if (isInited_ && usedSize > 0) {
        SendData(buf_.GetBuffer(), usedSize);
        MSPROF_LOGI("FlushBuf running %d", static_cast<int>(usedSize));
        buf_.SetUsedSize(0);
    }
}

bool ProcTimerHandler::CheckFileSize(const std::string &file) const
{
    long long len = analysis::dvvp::common::utils::Utils::GetFileSize(file);
    if (len < 0 || len > MSVP_LARGE_FILE_MAX_LEN) {
        MSPROF_LOGW("[ProcTimerHandler] Proc file(%s) size(%lld)", file.c_str(), len);
        return false;
    }
    return true;
}

bool ProcTimerHandler::IsValidData(std::ifstream &ifs, std::string &data) const
{
    bool isValid = false;
    std::string buf;
    while (std::getline(ifs, buf)) {
        data += buf;
        data += "\n";
        isValid = true;
    }
    return isValid;
}

ProcHostCpuHandler::ProcHostCpuHandler(TimerHandlerTag tag, unsigned int bufSize,
                                       unsigned sampleIntervalNs, const std::string &retFileName,
                                       SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param,
                                       SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
                                       SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader)
    : ProcTimerHandler(tag, 0, bufSize, sampleIntervalNs, "", retFileName, param, jobCtx, upLoader)
{
    // "/proc/{pid}/task"
    taskSrc_ += PROC_FILE;
    taskSrc_ += MSVP_SLASH;
    taskSrc_ += std::to_string(param->host_sys_pid);
    taskSrc_ += MSVP_SLASH;
    taskSrc_ += PROC_TASK;
}

ProcHostCpuHandler::~ProcHostCpuHandler()
{
}

void ProcHostCpuHandler::ParseProcFile(std::ifstream &ifs /* = ios::in */, std::string &data)
{
    UNUSED(ifs);
    static const unsigned int PROC_STAT_USELESS_DATA_SIZE = 512;
    data.reserve(PROC_STAT_USELESS_DATA_SIZE);

    // collect process thread cpu usage data
    std::string procData;
    ParseProcTidStat(procData);
    if (procData.empty()) {
        return;
    }

    data += "time ";
    unsigned long long time = Utils::GetClockMonotonicRaw();
    data += std::to_string(time);
    data += "\n";
    ParseSysTime(data);
    data += procData;
}

void ProcHostCpuHandler::ParseSysTime(std::string &data)
{
    std::string line;
    std::ifstream fin;

    if (!CheckFileSize(PROF_PROC_UPTIME)) {
        return;
    }
    fin.open(PROF_PROC_UPTIME, std::ifstream::in);
    if (!fin.is_open()) {
        MSPROF_LOGW("Open file %s failed.", PROF_PROC_UPTIME);
        return;
    }
    if (std::getline(fin, line)) {
        data += line;
        data += "\n";
    }
    fin.close();
}

void ProcHostCpuHandler::ParseProcTidStat(std::string &data)
{
    std::string line;
    std::ifstream fin;

    std::vector<std::string> tidDirs;
    Utils::GetChildDirs(taskSrc_, false, tidDirs);
    if (tidDirs.empty()) {
        return;
    }
    std::vector<std::string>::iterator it;
    for (it = tidDirs.begin(); it != tidDirs.end(); ++it) {
        std::string statFile = *it + MSVP_SLASH + PROC_TID_STAT;
        if (!CheckFileSize(statFile)) {
            continue;
        }
        fin.open(statFile, std::ifstream::in);
        if (!fin.is_open()) {
            MSPROF_LOGW("Open file %s failed.", statFile.c_str());
            continue;
        }
        while (std::getline(fin, line)) {
            data += line + "\n";
        }
        fin.close();
    }
}

ProcHostMemHandler::ProcHostMemHandler(TimerHandlerTag tag, unsigned int bufSize,
                                       unsigned sampleIntervalNs, const std::string &retFileName,
                                       SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param,
                                       SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
                                       SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader)
    : ProcTimerHandler(tag, 0, bufSize, sampleIntervalNs, "", retFileName, param, jobCtx, upLoader)
{
    // "/proc/{pid}/statm"
    statmSrc_ += PROC_FILE;
    statmSrc_ += MSVP_SLASH;
    statmSrc_ += std::to_string(param->host_sys_pid);
    statmSrc_ += MSVP_SLASH;
    statmSrc_ += PROC_PID_MEM;
}

ProcHostMemHandler::~ProcHostMemHandler()
{
}

void ProcHostMemHandler::ParseProcFile(std::ifstream &ifs /* = ios::in */, std::string &data)
{
    UNUSED(ifs);
    static const unsigned int PROC_STAT_USELESS_DATA_SIZE = 512;
    data.reserve(PROC_STAT_USELESS_DATA_SIZE);

    // collect process memory usage data
    std::string procData;
    ParseProcMemUsage(procData);
    if (procData.empty()) {
        return;
    }

    unsigned long long time = Utils::GetClockMonotonicRaw();
    data += std::to_string(time);
    data += " ";
    data += procData;
}

void ProcHostMemHandler::ParseProcMemUsage(std::string &data)
{
    std::string line;
    std::ifstream fin;

    if (!CheckFileSize(statmSrc_)) {
        return;
    }
    fin.open(statmSrc_, std::ifstream::in);
    if (!fin.is_open()) {
        MSPROF_LOGW("Open file %s failed.", statmSrc_.c_str());
        return;
    }
    if (std::getline(fin, line)) {
        data += line + "\n";
    }
    fin.close();
}

ProcHostNetworkHandler::ProcHostNetworkHandler(TimerHandlerTag tag, unsigned int bufSize,
                                               unsigned sampleIntervalNs, const std::string &retFileName,
                                               SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param,
                                               SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
                                               SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader)
    : ProcTimerHandler(tag, 0, bufSize, sampleIntervalNs, "", retFileName, param, jobCtx, upLoader)
{
}

ProcHostNetworkHandler::~ProcHostNetworkHandler()
{
}

void ProcHostNetworkHandler::ParseProcFile(std::ifstream &ifs /* = ios::in */, std::string &data)
{
    UNUSED(ifs);
    static const unsigned int PROC_STAT_USELESS_DATA_SIZE = 512;
    data.reserve(PROC_STAT_USELESS_DATA_SIZE);

    data += "time ";
    unsigned long long time = Utils::GetClockMonotonicRaw();
    data += std::to_string(time);
    data += "\n";
    ParseNetStat(data);
}

void ProcHostNetworkHandler::ParseNetStat(std::string &data)
{
    std::string line;
    std::ifstream fin;

    if (!CheckFileSize(PROF_NET_STAT)) {
        return;
    }
    fin.open(PROF_NET_STAT, std::ifstream::in);
    if (!fin.is_open()) {
        MSPROF_LOGW("Open file %s failed", PROF_NET_STAT);
        return;
    }
    while (std::getline(fin, line)) {
        if (line.find(":") != std::string::npos) {
            data += line + "\n";
        }
    }
    fin.close();
}

ProcStatFileHandler::ProcStatFileHandler(TimerHandlerTag tag, unsigned int devId, unsigned int bufSize,
                                         unsigned sampleIntervalNs,
                                         const std::string &srcFileName,
                                         const std::string &retFileName,
                                         SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param,
                                         SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
                                         SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader)
    : ProcTimerHandler(tag, devId, bufSize, sampleIntervalNs, srcFileName,
        retFileName, param, jobCtx, upLoader)
{
}

ProcStatFileHandler::~ProcStatFileHandler()
{
}

void ProcStatFileHandler::ParseProcFile(std::ifstream &ifs, std::string &data)
{
    static const unsigned int PROC_STAT_USELESS_DATA_SIZE = 512;

    data.reserve(PROC_STAT_USELESS_DATA_SIZE);

    std::string buf;

    while (std::getline(ifs, buf)) {
        std::transform (buf.begin(), buf.end(), buf.begin(), (int (*)(int))std::tolower);

        if (buf.find(PROC_CPU) != std::string::npos) {
            data += buf;
            data += "\n";
        } else {
            break;
        }
    }
}

ProcPidStatFileHandler::ProcPidStatFileHandler(TimerHandlerTag tag, unsigned int devId, unsigned int bufSize,
                                               unsigned sampleIntervalNs,
                                               const std::string &srcFileName,
                                               const std::string &retFileName,
                                               SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param,
                                               SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
                                               SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader,
                                               unsigned int pid)
    : ProcTimerHandler(tag, devId, bufSize, sampleIntervalNs, srcFileName, std::to_string(pid) + "-" + retFileName,
        param, jobCtx, upLoader),
      pid_(pid)
{
}

ProcPidStatFileHandler::~ProcPidStatFileHandler()
{
}

void ProcPidStatFileHandler::ParseProcFile(std::ifstream &ifs, std::string &data)
{
    static const unsigned int PROC_PID_STAT_DATA_SIZE = 512;
    data.reserve(PROC_PID_STAT_DATA_SIZE);
    if (IsValidData(ifs, data)) {
        // pid name
        std::string processName(PROC_SELF);
        ProcAllPidsFileHandler::GetProcessName(pid_, processName);

        data += PROF_DATA_PROCESSNAME;
        data += processName;
        data += "\n";
        std::string buf;
        ifStat_.open(PROF_PROC_STAT, std::ifstream::in);
        if (!ifStat_.is_open()) {
            MSPROF_LOGW("Open file %s failed", PROF_PROC_STAT);
            return;
        }
        while (std::getline(ifStat_, buf)) {
            std::transform (buf.begin(), buf.end(), buf.begin(), (int (*)(int))std::tolower);

            if (buf.find(PROC_CPU) != std::string::npos) {
                data += buf;
                data += "\n";
                break;
            }
        }

        ifStat_.close();
    }
}

ProcMemFileHandler::ProcMemFileHandler(TimerHandlerTag tag, unsigned int devId, unsigned int bufSize,
    unsigned sampleIntervalNs, const std::string &srcFileName, const std::string &retFileName,
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param,
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
    SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader)
    : ProcTimerHandler(tag, devId, bufSize, sampleIntervalNs, srcFileName, retFileName, param, jobCtx, upLoader)
{
}

ProcMemFileHandler::~ProcMemFileHandler()
{
}

void ProcMemFileHandler::ParseProcFile(std::ifstream &ifs, std::string &data)
{
    static const unsigned int PROC_MEM_USELESS_DATA_SIZE = 1536;

    data.reserve(PROC_MEM_USELESS_DATA_SIZE);

    std::string buf;
    while (std::getline(ifs, buf)) {
        data += buf;
        data += "\n";
    }
}

ProcPidMemFileHandler::ProcPidMemFileHandler(TimerHandlerTag tag, unsigned int devId, unsigned int bufSize,
                                             unsigned sampleIntervalNs,
                                             const std::string &srcFileName, const std::string &retFileName,
                                             SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param,
                                             SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
                                             SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader,
                                             unsigned int pid)
    : ProcTimerHandler(tag, devId, bufSize, sampleIntervalNs, srcFileName,
        std::to_string(pid) + "-" + retFileName, param, jobCtx, upLoader),
      pid_(pid)
{
}

ProcPidMemFileHandler::~ProcPidMemFileHandler()
{
}

void ProcPidMemFileHandler::ParseProcFile(std::ifstream &ifs, std::string &data)
{
    static const unsigned int PROC_PID_MEM_DATA_SIZE = 32;
    data.reserve(PROC_PID_MEM_DATA_SIZE);
    if (IsValidData(ifs, data)) {
        // pid name
        std::string processName(PROC_SELF);
        ProcAllPidsFileHandler::GetProcessName(pid_, processName);

        data += PROF_DATA_PROCESSNAME;
        data += processName;
        data += "\n";
    }
}

void ProcPidFileHandler::Execute()
{
    if (memHandler_) {
        memHandler_->Execute();
    }
    if (statHandler_) {
        statHandler_->Execute();
    }
}

ProcAllPidsFileHandler::ProcAllPidsFileHandler(TimerHandlerTag tag, unsigned int devId,
                                               unsigned sampleIntervalNs,
                                               SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param,
                                               SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx,
                                               SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader)
    : TimerHandler(tag),
      devId_(devId),
      prevTimeStamp_(0), sampleIntervalNs_(sampleIntervalNs),
      param_(param), jobCtx_(jobCtx), upLoader_(upLoader)
{
}

ProcAllPidsFileHandler::~ProcAllPidsFileHandler()
{
}

int ProcAllPidsFileHandler::Init()
{
    MSPROF_LOGI("ProcAllPidsFileHandler Init");
    GetCurPids(prevPids_);
    HandleNewPids(prevPids_);

    return PROFILING_SUCCESS;
}

int ProcAllPidsFileHandler::Uinit()
{
    for (auto iter = pidsMap_.begin(); iter != pidsMap_.end(); iter++) {
        if (iter->second->memHandler_ != nullptr) {
            iter->second->memHandler_->Uinit();
        }

        if (iter->second->statHandler_ != nullptr) {
            iter->second->statHandler_->Uinit();
        }
    }

    upLoader_.reset();
    pidsMap_.clear();
    prevPids_.clear();

    return PROFILING_SUCCESS;
}

void ProcAllPidsFileHandler::GetProcessName(unsigned int pid, std::string &processName)
{
    std::string fileName(PROC_FILE);
    fileName += "/";
    fileName += std::to_string(pid);
    fileName += "/";
    fileName += PROC_COMM_FILE;

    long long len = analysis::dvvp::common::utils::Utils::GetFileSize(fileName);
    if (len < 0 || len > MSVP_SMALL_FILE_MAX_LEN) {
        MSPROF_LOGE("proc comm file size is invalid");
        MSPROF_INNER_ERROR("EK9999", "proc comm file size is invalid");
        return;
    }

    std::ifstream ifs(fileName, std::ifstream::in);

    if (!ifs.is_open()) {
        MSPROF_LOGW("Open file %s failed", fileName.c_str());
        return;
    }

    std::getline(ifs, processName);

    ifs.close();
}

void ProcAllPidsFileHandler::GetCurPids(std::vector<unsigned int> &curPids)
{
    static const unsigned int PROC_PID_NUM = 128;

    std::vector<std::string> pidDirs;
    pidDirs.reserve(PROC_PID_NUM);
    analysis::dvvp::common::utils::Utils::GetChildDirs(PROC_FILE, false, pidDirs);

    size_t size = pidDirs.size();
    curPids.reserve(size);
    curPids.clear();
    for (size_t i = 0; i < size; i++) {
        unsigned int pid = strtoul(pidDirs[i].c_str() + strlen(PROC_FILE) + strlen("/"), nullptr, 0);
        if (pid != 0) {
            curPids.push_back(pid);
        }
    }

    std::sort(curPids.begin(), curPids.end());
}

void ProcAllPidsFileHandler::GetNewExitPids(std::vector<unsigned int> &curPids, std::vector<unsigned int> &prevPids,
                                            std::vector<unsigned int> &newPids, std::vector<unsigned int> &exitPids)
{
    size_t curPidsSize = curPids.size();
    size_t prevPidsSize = prevPids.size();
    size_t i = 0;
    size_t j = 0;

    newPids.clear();
    exitPids.clear();

    for (i = 0, j = 0; i < prevPidsSize && j < curPidsSize;) {
        if (prevPids[i] == curPids[j]) {
            i++;
            j++;
        } else if (prevPids[i] < curPids[j]) {
            MSPROF_LOGI("exit Pid %d", prevPids[i]);
            exitPids.push_back(prevPids[i++]);
        } else {
            MSPROF_LOGI("New Pid %d", curPids[j]);
            newPids.push_back(curPids[j++]);
        }
    }

    while (i < prevPidsSize) {
        exitPids.push_back(prevPids[i++]);
    }

    while (j < curPidsSize) {
        newPids.push_back(curPids[j++]);
    }
}

void ProcAllPidsFileHandler::HandleNewPids(std::vector<unsigned int> &newPids)
{
    SHARED_PTR_ALIA<ProcPidFileHandler> pidFileHandler = nullptr;
    SHARED_PTR_ALIA<ProcPidMemFileHandler> pidMemHandler = nullptr;
    SHARED_PTR_ALIA<ProcPidStatFileHandler> pidStatHandler = nullptr;

    size_t size = newPids.size();
    for (size_t i = 0; i < size; i++) {
        pidFileHandler = std::make_shared<ProcPidFileHandler>();

        std::string str;

        std::string pidSrcMemFileName =  std::string(PROC_FILE) + "/"
                                         + std::to_string(newPids[i]) + "/" + PROC_PID_MEM;
        std::string pidSrcStatFileName = std::string(PROC_FILE) + "/"
                                         + std::to_string(newPids[i]) + "/" + PROC_PID_STAT;

        std::string pidRetMemFileName = PROF_PID_MEM_FILE;
        pidRetMemFileName += str;
        std::string pidStatMemFileName = PROF_PID_STAT_FILE;
        pidStatMemFileName += str;

        const unsigned int procPidMemBufSize = (1 << 12); // 1 << 12, the size of per data is about 100Byte
        const unsigned int procPidStatBufSize = (1 << 12); // 1 << 12, the size of per stat data is about 300Byte

        pidMemHandler = std::make_shared<ProcPidMemFileHandler>(GetTag(), devId_, procPidMemBufSize,
                                                                sampleIntervalNs_, pidSrcMemFileName,
                                                                pidRetMemFileName, param_,
                                                                jobCtx_, upLoader_, newPids[i]);
        pidStatHandler = std::make_shared<ProcPidStatFileHandler>(GetTag(), devId_, procPidStatBufSize,
            sampleIntervalNs_, pidSrcStatFileName,
            pidStatMemFileName, param_, jobCtx_, upLoader_, newPids[i]);
        if (pidMemHandler->Init() == PROFILING_SUCCESS &&
            pidStatHandler->Init() == PROFILING_SUCCESS) {
            pidFileHandler->memHandler_ = pidMemHandler;
            pidFileHandler->statHandler_ = pidStatHandler;

            pidsMap_.insert(std::pair<unsigned int, SHARED_PTR_ALIA<ProcPidFileHandler> >(newPids[i], pidFileHandler));
        }
    }
}

void ProcAllPidsFileHandler::HandleExitPids(std::vector<unsigned int> &exitPids)
{
    size_t size = exitPids.size();
    for (size_t i = 0; i < size; i++) {
        auto iter = pidsMap_.find(exitPids[i]);
        if (iter != pidsMap_.end()) {
            if (iter->second->memHandler_) {
                iter->second->memHandler_->Uinit();
            }

            if (iter->second->statHandler_) {
                iter->second->statHandler_->Uinit();
            }

            pidsMap_.erase(iter);
        }
    }
}

int ProcAllPidsFileHandler::Execute()
{
    unsigned long long curTimeStamp = analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw();
    if ((curTimeStamp - prevTimeStamp_) >= sampleIntervalNs_ || (prevTimeStamp_ == 0)) {
        prevTimeStamp_ = curTimeStamp;

        std::vector<unsigned int> curPids;
        GetCurPids(curPids);

        static const unsigned int CHANGE_PIDS_NUM = 16;
        std::vector<unsigned int> newPids(curPids.size() > prevPids_.size() ?
            curPids.size() - prevPids_.size() : CHANGE_PIDS_NUM);
        std::vector<unsigned int> exitPids(curPids.size() > prevPids_.size() ?
            curPids.size() - prevPids_.size() : CHANGE_PIDS_NUM);

        GetNewExitPids(curPids, prevPids_, newPids, exitPids);
        HandleExitPids(exitPids);
        HandleNewPids(newPids);

        prevPids_.swap(curPids);
        for (auto iter = pidsMap_.begin(); iter != pidsMap_.end(); iter++) {
            iter->second->Execute();
        }
    }
    return PROFILING_SUCCESS;
}

void ProcAllPidsFileHandler::ParseProcFile(const std::ifstream &ifs /* = ios::in */,
    const std::string &data /* = "" */) const
{
    UNUSED(ifs);
    UNUSED(data);
}

ProfTimer::ProfTimer(SHARED_PTR_ALIA<TimerParam> timerParam)
    : isStarted_(false), timerParam_(timerParam)
{
}

ProfTimer::~ProfTimer()
{
    Stop();
}


int ProfTimer::Handler()
{
    std::lock_guard<std::mutex> lk(mtx_);
    for (auto iter = handlerMap_.begin(); iter != handlerMap_.end(); iter++) {
        iter->second->Execute();
    }
    return static_cast<int>(handlerMap_.size());
}

int ProfTimer::RegisterTimerHandler(TimerHandlerTag tag, SHARED_PTR_ALIA<TimerHandler> handler)
{
    MSPROF_LOGI("ProfTimer RegisterTimerHandler tag %u", tag);

    std::lock_guard<std::mutex> lk(mtx_);
    handlerMap_[tag] = handler;

    return PROFILING_SUCCESS;
}

int ProfTimer::RemoveTimerHandler(TimerHandlerTag tag)
{
    MSPROF_LOGI("ProfTimer RemoveTimerHandler tag %u begin", tag);
    std::lock_guard<std::mutex> lk(mtx_);
    auto iter = handlerMap_.find(tag);
    if (iter != handlerMap_.end()) {
        iter->second->Uinit();

        handlerMap_.erase(tag);
    }
    MSPROF_LOGI("ProfTimer RemoveTimerHandler tag %u succ", tag);

    return PROFILING_SUCCESS;
}

int ProfTimer::Start()
{
    int ret = PROFILING_FAILED;

    MSPROF_LOGI("Start ProfTimer begin");
    std::lock_guard<std::mutex> lk(mtx_);
    do {
        if (isStarted_) {
            MSPROF_LOGE("ProfTimer is running");
            MSPROF_INNER_ERROR("EK9999", "ProfTimer is running");
            break;
        }

        isStarted_ = true;
        analysis::dvvp::common::thread::Thread::SetThreadName(
            analysis::dvvp::common::config::MSVP_COLLECT_PROF_TIMER_THREAD_NAME);
        (void)analysis::dvvp::common::thread::Thread::Start();

        ret = PROFILING_SUCCESS;
        MSPROF_LOGI("Start ProfTimer succ");
    } while (0);

    MSPROF_LOGI("Start ProfTimer end");
    return ret;
}

int ProfTimer::Stop()
{
    int ret = PROFILING_SUCCESS;
    MSPROF_LOGI("Stop ProfTimer begin");
    do {
        if (!isStarted_) {
            break;
        }
        isStarted_ = false;

        (void)analysis::dvvp::common::thread::Thread::Stop();

        mtx_.lock();
        for (auto iter = handlerMap_.begin(); iter != handlerMap_.end(); iter++) {
            iter->second->Uinit();
        }
        handlerMap_.clear();
        mtx_.unlock();

        MSPROF_LOGI("Stop ProfTimer succ");
    } while (0);

    MSPROF_LOGI("Stop ProfTimer end");

    return ret;
}

void ProfTimer::Run(const struct error_message::Context &errorContext)
{
    MsprofErrorManager::instance()->SetErrorContext(errorContext);
    do {
        Handler();
        analysis::dvvp::common::utils::Utils::UsleepInterupt(timerParam_->intervalUsec);
    } while (isStarted_);
}

TimerManager::TimerManager()
    : profTimerCnt_(0)
{
}

TimerManager::~TimerManager()
{
    profTimer_.reset();
    profTimerCnt_ = 0;
}

void TimerManager::StartProfTimer()
{
    std::lock_guard<std::mutex> lk(profTimerMtx_);
    if (profTimerCnt_ == 0) {
        static const unsigned long long PROF_TIMER_INTERVAL_US = 1000; // 1000 us
        SHARED_PTR_ALIA<TimerParam> timerParam = nullptr;
        MSVP_MAKE_SHARED1_VOID(timerParam, TimerParam, PROF_TIMER_INTERVAL_US);
        MSVP_MAKE_SHARED1_VOID(profTimer_, ProfTimer, timerParam);
        int ret = profTimer_->Start();
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("StartProfTimer failed");
            MSPROF_INNER_ERROR("EK9999", "StartProfTimer failed");
            return;
        }
        MSPROF_LOGI("StartProfTimer end");
    }
    profTimerCnt_++;
}

void TimerManager::StopProfTimer()
{
    std::lock_guard<std::mutex> lk(profTimerMtx_);
    profTimerCnt_--;
    if (profTimer_ != nullptr && profTimerCnt_ == 0) {
        MSPROF_LOGI("StopProfTimer begin");
        int ret = profTimer_->Stop();
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("StopProfTimer failed");
            MSPROF_INNER_ERROR("EK9999", "StopProfTimer failed");
        }
        MSPROF_LOGI("StopProfTimer end");
    }
}

void TimerManager::RegisterProfTimerHandler(TimerHandlerTag tag,
    SHARED_PTR_ALIA<TimerHandler> handler)
{
    std::lock_guard<std::mutex> lk(profTimerMtx_);
    if (profTimer_ != nullptr && handler != nullptr) {
        (void)profTimer_->RegisterTimerHandler(tag, handler);
    }
}

void TimerManager::RemoveProfTimerHandler(TimerHandlerTag tag)
{
    std::lock_guard<std::mutex> lk(profTimerMtx_);
    if (profTimer_ != nullptr) {
        (void)profTimer_->RemoveTimerHandler(tag);
    }
}
}
}
}
