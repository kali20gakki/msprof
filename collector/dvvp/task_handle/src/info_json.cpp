/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#include "info_json.h"
#include <cstdio>
#include <fstream>
#if (defined(linux) || defined(__linux__))
#include <unistd.h>
#endif
#include <string>
#include "ai_drv_dev_api.h"
#include "config/config.h"
#include "config/config_manager.h"
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "prof_manager.h"
#include "securec.h"
#include "utils/utils.h"
#include "platform/platform.h"
#include "task_relationship_mgr.h"
#include "mmpa_api.h"
#include "transport/hash_data.h"

namespace analysis {
namespace dvvp {
namespace host {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::Common::Platform;
using namespace Analysis::Dvvp::Common::Config;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;

const char * const PROF_NET_CARD = "/sys/class/net";
const char * const PROF_PROC_MEM = "/proc/meminfo";
const char * const PROF_PROC_UPTIME = "/proc/uptime";

const char * const PROC_MEM_TOTAL = "MemTotal";
const char * const PROC_NET_SPEED = "speed";

InfoJson::InfoJson(const std::string &jobInfo, const std::string &devices, int hostpid)
    : jobInfo_(jobInfo), devices_(devices), hostpid_(hostpid)
{
}

int InfoJson::Generate(std::string &content)
{
    MSPROF_LOGI("Begin to generate info.json, devices: %s.", devices_.c_str());

    if (InitDeviceIds() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to init devices of info.json");
        return PROFILING_FAILED;
    }
    SetPidInfo(hostpid_);
    SetRankId();

    if (AddHostInfo() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to add host info to json.info.");
        return PROFILING_FAILED;
    }

    if (AddDeviceInfo() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to add device info to json.info.");
        return PROFILING_FAILED;
    }

    if (AddOtherInfo() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to add other info to json.info.");
        return PROFILING_FAILED;
    }

    nlohmann::json root;
    infoMain_.ToObject(root);
    content = root.dump();

    MSPROF_LOGI("End to generate info.json, devices: %s.", devices_.c_str());
    return PROFILING_SUCCESS;
}

int InfoJson::InitDeviceIds()
{
    devIds_.clear();
    hostIds_.clear();
    auto devicesVec = analysis::dvvp::common::utils::Utils::Split(devices_, false, "", ",");
    for (size_t i = 0; i < devicesVec.size(); i++) {
        try {
            int32_t devIndexId;
            if (Utils::StrToInt(devIndexId, devicesVec.at(i)) == PROFILING_FAILED) {
                return PROFILING_FAILED;
            }
            if (devIndexId < 0 || devIndexId >= MSVP_MAX_DEV_NUM) {
                continue;
            }
            devIds_.push_back(devIndexId);
            int hostId = Analysis::Dvvp::TaskHandle::TaskRelationshipMgr::instance()->GetHostIdByDevId(devIndexId);
            hostIds_.push_back(hostId);
            MSPROF_LOGI("Init devices in info.json, devId: %d, hostId: %d", devIndexId, hostId);
        } catch (...) {
            MSPROF_LOGE("Failed to transfer Device(%s) to integer.", devicesVec.at(i).c_str());
            return PROFILING_FAILED;
        }
    }
    analysis::dvvp::common::utils::UtilsStringBuilder<int> intBuilder;
    hostIdSerial_ = intBuilder.Join(hostIds_, ",");
    return PROFILING_SUCCESS;
}

void InfoJson::AddSysConf()
{
#if (defined(linux) || defined(__linux__))
    long tck = sysconf(_SC_CLK_TCK);
    if (tck == -1) {
        MSPROF_LOGW("Get system clock failed, err=%d.", MmGetErrorCode());
        return;
    }
    infoMain_.sysClockFreq = tck;
    long cpu = sysconf(_SC_NPROCESSORS_CONF);
    if (cpu == -1) {
        MSPROF_LOGW("Get system cpu num failed, err=%d.", MmGetErrorCode());
        return;
    }
    infoMain_.cpuNums = cpu;
#endif
}

void InfoJson::AddSysTime()
{
#if (defined(linux) || defined(__linux__))
    std::string line;
    std::ifstream fin;

    long long len = Utils::GetFileSize(PROF_PROC_UPTIME);
    if (len < 0 || len > MSVP_LARGE_FILE_MAX_LEN) {
        MSPROF_LOGW("[AddSysTime] Proc file(%s) size(%lld)", PROF_PROC_UPTIME, len);
        return;
    }
    fin.open(PROF_PROC_UPTIME, std::ifstream::in);
    if (!fin.is_open()) {
        MSPROF_LOGW("Open file %s failed.", PROF_PROC_UPTIME);
        return;
    }
    if (std::getline(fin, line)) {
        infoMain_.upTime = line;
    }
    fin.close();
#endif
}

void InfoJson::AddMemTotal()
{
#if (defined(linux) || defined(__linux__))
    std::string line;
    std::ifstream fin;
    uint32_t memoryTotal;

    long long len = Utils::GetFileSize(PROF_PROC_MEM);
    if (len < 0 || len > MSVP_LARGE_FILE_MAX_LEN) {
        MSPROF_LOGW("[AddMemTotal] Proc file(%s) size(%lld)", PROF_PROC_MEM, len);
        return;
    }
    fin.open(PROF_PROC_MEM, std::ifstream::in);
    if (!fin.is_open()) {
        MSPROF_LOGW("Open file %s failed.", PROF_PROC_MEM);
        return;
    }
    while (std::getline(fin, line)) {
        if (line.find(PROC_MEM_TOTAL) == std::string::npos) {
            continue;
        }
        unsigned start = 0;
        unsigned end = 0;
        for (unsigned i = 0; i < line.size(); i++) {
            char c = line.at(i);
            if (c >= '0' && c <= '9') {
                start = i;
                break;
            }
        }
        for (unsigned i = line.size() - 1; i > start; i--) {
            char c = line.at(i);
            if (c >= '0' && c <= '9') {
                end = i;
                break;
            }
        }
        if (start == 0 || end == 0) {
            MSPROF_LOGW("Parse MemTotal %s failed.", line.c_str());
            break;
        }
        std::string result = line.substr(start, end - start + 1);
        if (Utils::StrToUint32(memoryTotal, result) == PROFILING_SUCCESS) {
            infoMain_.memoryTotal = memoryTotal;
        }
        break;
    }
    fin.close();
#endif
}

void InfoJson::AddNetCardInfo()
{
#if (defined(linux) || defined(__linux__))
    std::string line;
    std::ifstream fin;
    uint32_t speed;

    std::vector<std::string> netCards;
    Utils::GetChildFilenames(PROF_NET_CARD, netCards);
    if (netCards.empty()) {
        MSPROF_LOGW("Scandir dir %s, no child dirs.", PROF_NET_CARD);
        return;
    }
    std::vector<std::string>::iterator it;
    for (it = netCards.begin(); it != netCards.end(); it++) {
        std::string srcFile(PROF_NET_CARD);
        srcFile += MSVP_SLASH + *it + MSVP_SLASH + PROC_NET_SPEED;
        long long len = Utils::GetFileSize(srcFile);
        if (len < 0 || len > MSVP_LARGE_FILE_MAX_LEN) {
            MSPROF_LOGW("[AddMemTotal] Proc file(%s) size(%lld)", srcFile.c_str(), len);
            continue;
        }
        fin.open(srcFile, std::ifstream::in);
        if (!fin.is_open()) {
            MSPROF_LOGW("Open file %s failed.", srcFile.c_str());
            continue;
        }
        if (std::getline(fin, line) && (line.length() > 0 && line[0] != '-') &&
            Utils::StrToUint32(speed, line) == PROFILING_SUCCESS) {
            NetCardInfo info;
            info.netCardName = *it;
            info.speed = speed;
            infoMain_.netCard.emplace_back(info);
        }
        fin.close();
    }
#endif
}

void InfoJson::SetRankId()
{
#if (defined(linux) || defined(__linux__))
    int32_t rankId = Utils::GetRankId();
    infoMain_.rank_id = rankId;
#endif
}

int InfoJson::AddHostInfo()
{
    // fetch and set OS
    MSPROF_LOGI("Begin to AddHostInfo in info.json, devices: %s.", devices_.c_str());
    char str[MMPA_MAX_PATH] = {0};
    int ret = MmGetOsVersion(str, MMPA_MAX_PATH);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGW("mmGetOsVersion failed");
    }
    std::string os(str);
    infoMain_.OS = os;

    // fetch and set hostname and hostuid
    if (memset_s(str, MMPA_MAX_PATH, 0, MMPA_MAX_PATH) != EOK) {
        MSPROF_LOGE("memset failed");
        return PROFILING_FAILED;
    }
    ret = MmGetOsName(str, MMPA_MAX_PATH);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGW("mmGetOsName failed");
    }
    std::string hostName(str);
    infoMain_.hostname = hostName;
    std::string macStr = analysis::dvvp::common::utils::Utils::GetHostMacStr();
    uint64_t macStrHashId = analysis::dvvp::transport::HashData::instance()->GenHashId(macStr);
    infoMain_.hostUid = std::to_string(macStrHashId);

    // fetch and set memory, clock freq, uptime, netcard info, logical cpu nums
    AddMemTotal();
    AddSysConf();
    AddSysTime();
    AddNetCardInfo();

    // fetch and set cpu infos
    MmCpuDesc *cpuInfo = nullptr;
    int32_t cpuNum = 0;
    ret = MmGetCpuInfo(&cpuInfo, &cpuNum);
    if (ret != PROFILING_SUCCESS || cpuNum <= 0) {
        MSPROF_LOGE("mmGetCpuInfo failed");
        return PROFILING_FAILED;
    }
    infoMain_.hwtype = cpuInfo[0].arch;
    infoMain_.cpuCores = cpuNum;
    for (int32_t i = 0; i < cpuNum; i++) {
        InfoCpu cpu;
        cpu.Id = i;
        cpu.Name = std::string(cpuInfo[i].manufacturer);
        cpu.Frequency = GetHostOscFrequency();
        cpu.Logical_CPU_Count = cpuInfo[i].nthreads == 0 ? cpuInfo[i].ncounts : cpuInfo[i].nthreads;
        cpu.Type = std::string(cpuInfo[i].version);
        infoMain_.CPU.emplace_back(cpu);
    }
    MmCpuInfoFree(cpuInfo, cpuNum);
    MSPROF_LOGI("End to AddHostInfo in info.json, devices: %s.", devices_.c_str());
    return PROFILING_SUCCESS;
}

std::string InfoJson::GetHostOscFrequency() const
{
    return Analysis::Dvvp::Common::Platform::Platform::instance()->PlatformGetHostOscFreq();
}

std::string InfoJson::GetDeviceOscFrequency(uint32_t deviceId, const std::string &freq)
{
    return Analysis::Dvvp::Common::Platform::Platform::instance()->PlatformGetDeviceOscFreq(deviceId, freq);
}

int InfoJson::GetCtrlCpuInfo(uint32_t devId, struct InfoDeviceInfo &devInfo)
{
    int64_t ctrlCpuId = 0;
    int ret = analysis::dvvp::driver::DrvGetCtrlCpuId(devId, ctrlCpuId);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetCtrlCpuId, deviceId=%d", devId);
        return PROFILING_FAILED;
    }
    SetCtrlCpuId(devInfo, ctrlCpuId);
    int64_t val = 0;
    ret = analysis::dvvp::driver::DrvGetCtrlCpuCoreNum(devId, val);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetCtrlCpuCoreNum, deviceId=%d", devId);
        return PROFILING_FAILED;
    }
    devInfo.ctrl_cpu_core_num = static_cast<uint32_t>(val);
    ret = analysis::dvvp::driver::DrvGetCtrlCpuEndianLittle(devId, val);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetCtrlCpuEndianLittle, deviceId=%d", devId);
        return PROFILING_FAILED;
    }
    devInfo.ctrl_cpu_endian_little = static_cast<uint32_t>(val);
    return PROFILING_SUCCESS;
}

int InfoJson::GetDevInfo(int deviceId, struct InfoDeviceInfo &deviceInfo)
{
    uint32_t devId = static_cast<uint32_t>(deviceId);
    int64_t val = 0;
    if (analysis::dvvp::driver::DrvGetEnvType(devId, val) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetEnvType, deviceId=%d", deviceId);
        return PROFILING_FAILED;
    }
    deviceInfo.env_type = static_cast<uint32_t>(val);
    if (GetCtrlCpuInfo(devId, deviceInfo) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to GetCtrlCpuInfo, deviceId=%d", deviceId);
        return PROFILING_FAILED;
    }
    if (analysis::dvvp::driver::DrvGetAiCpuCoreNum(devId, val) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetAiCpuCoreNum, deviceId=%d", deviceId);
        return PROFILING_FAILED;
    }
    deviceInfo.ai_cpu_core_num = static_cast<uint32_t>(val);
    if (analysis::dvvp::driver::DrvGetAivNum(devId, val) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetAivNum, deviceId=%d", deviceId);
        return PROFILING_FAILED;
    }
    deviceInfo.aiv_num = static_cast<uint32_t>(val);
    if (deviceInfo.ai_cpu_core_num != 0 && analysis::dvvp::driver::DrvGetAiCpuCoreId(devId, val) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetAiCpuCoreId, deviceId=%d", deviceId);
        return PROFILING_FAILED;
    }
    deviceInfo.ai_cpu_core_id = static_cast<uint32_t>(val);
    if (analysis::dvvp::driver::DrvGetAiCpuOccupyBitmap(devId, val) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetAiCpuOccupyBitmap, deviceId=%d", deviceId);
        return PROFILING_FAILED;
    }
    deviceInfo.aicpu_occupy_bitmap = static_cast<uint32_t>(val);
    if (analysis::dvvp::driver::DrvGetTsCpuCoreNum(devId, val) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetTsCpuCoreNum, deviceId=%d", deviceId);
        return PROFILING_FAILED;
    }
    deviceInfo.ts_cpu_core_num = static_cast<uint32_t>(val);
    if (analysis::dvvp::driver::DrvGetAiCoreId(devId, val) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetAiCoreId, deviceId=%d", deviceId);
        return PROFILING_FAILED;
    }
    deviceInfo.ai_core_id = static_cast<uint32_t>(val);
    if (analysis::dvvp::driver::DrvGetAiCoreNum(devId, val) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetAiCoreNum, deviceId=%d", deviceId);
        return PROFILING_FAILED;
    }
    deviceInfo.ai_core_num = static_cast<uint32_t>(val);
    MSPROF_LOGI("Succeeded to drvGetDevInfo, deviceId=%d", deviceId);
    return PROFILING_SUCCESS;
}

int InfoJson::AddDeviceInfo()
{
    MSPROF_LOGI("Begin to AddDeviceInfo in info.json, devIds: %s.", devices_.c_str());
    std::vector<int> hostIds;
    for (size_t i = 0; i < hostIds_.size() && i < devIds_.size(); i++) {
        int devIndexId = devIds_.at(i);
        int hostId = hostIds_.at(i);
        hostIds.push_back(hostId);

        struct InfoDeviceInfo devInfo;
        if (GetDevInfo(devIndexId, devInfo) != PROFILING_SUCCESS) {
            MSPROF_LOGE("GetDevInfo Device(%d) failed.", devIndexId);
            return PROFILING_FAILED;
        }
        devInfo.id = hostId;
        std::string ctrlCpu;
        for (int64_t i = 0; i < devInfo.ctrl_cpu_core_num; i++) {
            ctrlCpu.append((i == 0) ? std::to_string(0) : ("," + std::to_string(i)));
        }

        devInfo.ctrl_cpu = ctrlCpu;
        std::string aiCpu;
        for (int64_t i = devInfo.ai_cpu_core_id; i < devInfo.ctrl_cpu_core_num + devInfo.ai_cpu_core_num; i++) {
            aiCpu.append((i == devInfo.ai_cpu_core_id) ?
                std::to_string(devInfo.ai_cpu_core_id) : ("," + std::to_string(i)));
        }
        devInfo.ai_cpu = aiCpu;
        SetHwtsFrequency(static_cast<uint32_t>(devIndexId), devInfo);
        std::string freq = Analysis::Dvvp::Driver::DrvGeAicFrq(devIndexId);
        devInfo.aic_frequency = freq;
        devInfo.aiv_frequency = freq;
        infoMain_.DeviceInfo.emplace_back(devInfo);
    }
    infoMain_.devices = hostIdSerial_;
    MSPROF_LOGI("End to AddDeviceInfo in info.json, hostIds: %s.", hostIdSerial_.c_str());

    return PROFILING_SUCCESS;
}

void InfoJson::SetCtrlCpuId(struct InfoDeviceInfo &devInfo, int64_t ctrlCpuId)
{
    const std::map<int64_t, std::string> cpuTypes = {
        {0x41d03, "ARMv8_Cortex_A53"},
        {0x41d05, "ARMv8_Cortex_A55"},
        {0x41d07, "ARMv8_Cortex_A57"},
        {0x41d08, "ARMv8_Cortex_A72"},
        {0x41d09, "ARMv8_Cortex_A73"},
        {0x48d01, "TaishanV110"}
    };
    auto iterator = cpuTypes.find(ctrlCpuId);
    if (iterator != cpuTypes.end()) {
        devInfo.ctrl_cpu_id = iterator->second;
    }
}

void InfoJson::SetHwtsFrequency(uint32_t deviceId, struct InfoDeviceInfo &devInfo)
{
    std::string hwtsFrq = GetDeviceOscFrequency(deviceId, ConfigManager::instance()->GetFrequency());
    MSPROF_LOGD("hwtsFrq:%s", hwtsFrq.c_str());
    devInfo.hwts_frequency = hwtsFrq;
}

int InfoJson::AddOtherInfo()
{
    if (jobInfo_.empty()) {
        jobInfo_ = "NA";
    }
    infoMain_.jobInfo = jobInfo_;
    // mac
    std::string mac;
    if (analysis::dvvp::common::utils::Utils::GetMac(mac) != PROFILING_SUCCESS) {
        MSPROF_LOGW("GetMac failed.");
    }
    infoMain_.mac = mac;
    SetPlatFormVersion();
    SetVersionInfo();
    SetDrvVersion();
    return PROFILING_SUCCESS;
}

void InfoJson::SetPidInfo(int pid)
{
    std::string pidtmp;
    std::string pidName = "NA";
    std::string processInfoPath;
    if (pid == HOST_PID_DEFAULT) {
        pidtmp = "NA";
    } else {
        pidtmp = std::to_string(pid);
#if (defined(linux) || defined(__linux__))
        processInfoPath = "/proc/" + pidtmp + "/status";
        long long len = Utils::GetFileSize(processInfoPath);
        if (len < 0 || len > MSVP_LARGE_FILE_MAX_LEN) {
            MSPROF_LOGW("[SetPidInfo] Proc file(%s) size(%lld)", processInfoPath.c_str(), len);
        } else {
            std::ifstream processInfo(processInfoPath);
            if (processInfo.is_open()) {
                std::getline(processInfo, pidName);
            } else {
                MSPROF_LOGE("Set pid_name failed(failed to open the file for pid_name info), pid=%d", pid);
            }
            processInfo.close();
            // The length of searching tag "Name:\t" is 6. This constant is used to locate the position of pid_name.
            constexpr size_t searchTagLength = 6;
            size_t pidNamePos = pidName.find("Name:\t") + searchTagLength;
            if ((pidNamePos - searchTagLength) != std::string::npos) {
                pidName = pidName.substr(pidNamePos, pidName.size() - pidNamePos);
            } else {
                MSPROF_LOGE("Set pid_name failed, pid=%d", pid);
            }
        }
#endif
    }
    infoMain_.pid = pidtmp;
    infoMain_.pid_name = pidName;
    return;
}

void InfoJson::SetPlatFormVersion()
{
    std::string chipId = Analysis::Dvvp::Common::Config::ConfigManager::instance()->GetChipIdStr();
    infoMain_.platform_version = chipId;
}

void InfoJson::SetVersionInfo()
{
    infoMain_.version = Analysis::Dvvp::Common::Platform::PROF_VERSION_INFO;
}

void InfoJson::SetDrvVersion()
{
    infoMain_.drvVersion = analysis::dvvp::driver::DrvGetApiVersion();
}

InfoJson::~InfoJson()
{
}
}  // namespace host
}  // namespace dvvp
}  // namespace analysis
