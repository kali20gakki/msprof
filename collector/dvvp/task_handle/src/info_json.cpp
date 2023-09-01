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
#include "proto/msprofiler.pb.h"
#include "config/config.h"
#include "config/config_manager.h"
#include "errno/error_code.h"
#include "message/codec.h"
#include "msprof_dlog.h"
#include "prof_manager.h"
#include "securec.h"
#include "utils/utils.h"
#include "platform/platform.h"
#include "task_relationship_mgr.h"
#include "mmpa_api.h"

namespace analysis {
namespace dvvp {
namespace host {
using namespace analysis::dvvp::proto;
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

    SHARED_PTR_ALIA<InfoMain> infoMain = nullptr;
    MSVP_MAKE_SHARED0_RET(infoMain, InfoMain, PROFILING_FAILED);

    if (InitDeviceIds() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to init devices of info.json");
        return PROFILING_FAILED;
    }
    SetPidInfo(infoMain, hostpid_);
    SetRankId(infoMain);

    if (AddHostInfo(infoMain) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to add host info to json.info.");
        return PROFILING_FAILED;
    }

    if (AddDeviceInfo(infoMain) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to add device info to json.info.");
        return PROFILING_FAILED;
    }

    if (AddOtherInfo(infoMain) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to add other info to json.info.");
        return PROFILING_FAILED;
    }

    content =  analysis::dvvp::message::EncodeJson(infoMain, true, false);

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
            int devIndexId = std::stoi(devicesVec.at(i));
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

void InfoJson::AddSysConf(SHARED_PTR_ALIA<InfoMain> infoMain)
{
#if (defined(linux) || defined(__linux__))
    long tck = sysconf(_SC_CLK_TCK);
    if (tck == -1) {
        MSPROF_LOGW("Get system clock failed, err=%d.", MmGetErrorCode());
        return;
    }
    infoMain->set_sysclockfreq(tck);
    long cpu = sysconf(_SC_NPROCESSORS_CONF);
    if (cpu == -1) {
        MSPROF_LOGW("Get system cpu num failed, err=%d.", MmGetErrorCode());
        return;
    }
    infoMain->set_cpunums(cpu);
#endif
}

void InfoJson::AddSysTime(SHARED_PTR_ALIA<InfoMain> infoMain)
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
        infoMain->set_uptime(line);
    }
    fin.close();
#endif
}

void InfoJson::AddMemTotal(SHARED_PTR_ALIA<InfoMain> infoMain)
{
#if (defined(linux) || defined(__linux__))
    std::string line;
    std::ifstream fin;

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
        if (Utils::CheckStringIsNonNegativeIntNum(result)) {
            infoMain->set_memorytotal(std::stoi(result));
        }
        break;
    }
    fin.close();
#endif
}

void InfoJson::AddNetCardInfo(SHARED_PTR_ALIA<InfoMain> infoMain)
{
#if (defined(linux) || defined(__linux__))
    std::string line;
    std::ifstream fin;

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
        if (std::getline(fin, line) && (line.length() > 0 && line[0] != '-')
                && Utils::CheckStringIsNonNegativeIntNum(line)) {
            auto netCardInfo = infoMain->add_netcard();
            netCardInfo->set_netcardname(*it);
            netCardInfo->set_speed(std::stoi(line));
        }
        fin.close();
    }
#endif
}

void InfoJson::AddCycleToTimeInfo(SHARED_PTR_ALIA<InfoMain> infoMain)
{
#if (defined(linux) || defined(__linux__))
    double freq = Utils::StatCpuRealFreq();
    uint64_t cycleCnt = Utils::GetCPUCycleCounter();
    uint64_t monotonicRaw = Utils::GetClockMonotonicRaw();

    auto cycleToTime = infoMain->add_cycletotime();
    cycleToTime->set_realcpufreq(freq);
    cycleToTime->set_calibration(static_cast<uint64_t>(cycleCnt / freq) - monotonicRaw);
#endif
}

void InfoJson::SetRankId(SHARED_PTR_ALIA<InfoMain> infoMain)
{
#if (defined(linux) || defined(__linux__))
    int32_t rankId = Utils::GetRankId();
    infoMain->set_rank_id(rankId);
#endif
}

int InfoJson::AddHostInfo(SHARED_PTR_ALIA<InfoMain> infoMain)
{
    if (Platform::instance()->RunSocSide()) {
        // host info is not availible on soc
        return PROFILING_SUCCESS;
    }
    // fetch and set OS
    MSPROF_LOGI("Begin to AddHostInfo in info.json, devices: %s.", devices_.c_str());
    char str[MMPA_MAX_PATH] = {0};
    int ret = MmGetOsVersion(str, MMPA_MAX_PATH);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGW("mmGetOsVersion failed");
    }
    std::string os(str);
    infoMain->set_os(os);

    // fetch and set hostname
    (void)memset_s(str, MMPA_MAX_PATH, 0, MMPA_MAX_PATH);
    ret = MmGetOsName(str, MMPA_MAX_PATH);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGW("mmGetOsName failed");
    }
    std::string hostName(str);
    infoMain->set_hostname(hostName);

    // fetch and set memory, clock freq, uptime, netcard info, logical cpu nums
    AddMemTotal(infoMain);
    AddSysConf(infoMain);
    AddSysTime(infoMain);
    AddNetCardInfo(infoMain);
    AddCycleToTimeInfo(infoMain);

    // fetch and set cpu infos
    MmCpuDesc *cpuInfo = nullptr;
    int32_t cpuNum = 0;
    ret = MmGetCpuInfo(&cpuInfo, &cpuNum);
    if (ret != PROFILING_SUCCESS || cpuNum <= 0) {
        MSPROF_LOGE("mmGetCpuInfo failed");
        return PROFILING_FAILED;
    }
    infoMain->set_hwtype(cpuInfo[0].arch);
    infoMain->set_cpucores(cpuNum);
    for (int32_t i = 0; i < cpuNum; i++) {
        auto infoCpu = infoMain->add_cpu();
        infoCpu->set_id(i);
        infoCpu->set_name(cpuInfo[i].manufacturer);
        infoCpu->set_type(cpuInfo[i].version);
        infoCpu->set_frequency(GetCpuFrequency());
        infoCpu->set_logical_cpu_count(cpuInfo[i].nthreads == 0 ? cpuInfo[i].ncounts : cpuInfo[i].nthreads);
    }
    MmCpuInfoFree(cpuInfo, cpuNum);
    MSPROF_LOGI("End to AddHostInfo in info.json, devices: %s.", devices_.c_str());
    return PROFILING_SUCCESS;
}

uint32_t InfoJson::GetCpuFrequency() const
{
    const uint32_t FREQUENCY_GHZ_TO_MHZ = 1000;
    return FREQUENCY_GHZ_TO_MHZ;
}

int InfoJson::GetCtrlCpuInfo(uint32_t devId, struct DeviceInfo &devInfo)
{
    int ret = analysis::dvvp::driver::DrvGetCtrlCpuId(devId, devInfo.ctrl_cpu_id);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetCtrlCpuId, deviceId=%d", devId);
        return PROFILING_FAILED;
    }
    ret = analysis::dvvp::driver::DrvGetCtrlCpuCoreNum(devId, devInfo.ctrl_cpu_core_num);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetCtrlCpuCoreNum, deviceId=%d", devId);
        return PROFILING_FAILED;
    }
    ret = analysis::dvvp::driver::DrvGetCtrlCpuEndianLittle(devId, devInfo.ctrl_cpu_endian_little);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetCtrlCpuEndianLittle, deviceId=%d", devId);
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int InfoJson::GetDevInfo(int deviceId, struct DeviceInfo &devInfo)
{
    uint32_t devId = static_cast<uint32_t>(deviceId);
    int ret = analysis::dvvp::driver::DrvGetEnvType(devId, devInfo.env_type);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetEnvType, deviceId=%d", deviceId);
        return PROFILING_FAILED;
    }
    ret = GetCtrlCpuInfo(devId, devInfo);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to GetCtrlCpuInfo, deviceId=%d", deviceId);
        return PROFILING_FAILED;
    }
    ret = analysis::dvvp::driver::DrvGetAiCpuCoreNum(devId, devInfo.ai_cpu_core_num);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetAiCpuCoreNum, deviceId=%d", deviceId);
        return PROFILING_FAILED;
    }
    ret = analysis::dvvp::driver::DrvGetAivNum(devId, devInfo.ai_vector_num);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetAivNum, deviceId=%d", deviceId);
        return PROFILING_FAILED;
    }
    ret = analysis::dvvp::driver::DrvGetAiCpuCoreId(devId, devInfo.ai_cpu_core_id);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetAiCpuCoreId, deviceId=%d", deviceId);
        return PROFILING_FAILED;
    }
    ret = analysis::dvvp::driver::DrvGetAiCpuOccupyBitmap(devId, devInfo.aicpu_occupy_bitmap);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetAiCpuOccupyBitmap, deviceId=%d", deviceId);
        return PROFILING_FAILED;
    }
    ret = analysis::dvvp::driver::DrvGetTsCpuCoreNum(devId, devInfo.ts_cpu_core_num);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetTsCpuCoreNum, deviceId=%d", deviceId);
        return PROFILING_FAILED;
    }
    ret = analysis::dvvp::driver::DrvGetAiCoreId(devId, devInfo.ai_core_id);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetAiCoreId, deviceId=%d", deviceId);
        return PROFILING_FAILED;
    }
    ret = analysis::dvvp::driver::DrvGetAiCoreNum(devId, devInfo.ai_core_num);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to DrvGetAiCoreNum, deviceId=%d", deviceId);
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("Succeeded to drvGetDevInfo, deviceId=%d", deviceId);
    return PROFILING_SUCCESS;
}

int InfoJson::AddDeviceInfo(SHARED_PTR_ALIA<InfoMain> infoMain)
{
    MSPROF_LOGI("Begin to AddDeviceInfo in info.json, devIds: %s.", devices_.c_str());
    std::vector<int> hostIds;
    for (size_t i = 0; i < hostIds_.size() && i < devIds_.size(); i++) {
        int devIndexId = devIds_.at(i);
        int hostId = hostIds_.at(i);
        hostIds.push_back(hostId);
        struct DeviceInfo devInfo;
        if (GetDevInfo(devIndexId, devInfo) != PROFILING_SUCCESS) {
            MSPROF_LOGE("GetDevInfo Device(%d) failed.", devIndexId);
            return PROFILING_FAILED;
        }
        auto infoDevice(infoMain->add_deviceinfo());
        infoDevice->set_id(hostId);
        infoDevice->set_env_type(devInfo.env_type);
        infoDevice->set_ctrl_cpu_core_num(devInfo.ctrl_cpu_core_num);
        infoDevice->set_ctrl_cpu_endian_little(devInfo.ctrl_cpu_endian_little);
        infoDevice->set_ts_cpu_core_num(devInfo.ts_cpu_core_num);
        infoDevice->set_ai_cpu_core_num(devInfo.ai_cpu_core_num);
        infoDevice->set_ai_core_num(devInfo.ai_core_num);
        infoDevice->set_aiv_num(devInfo.ai_vector_num);
        infoDevice->set_ai_cpu_core_id(devInfo.ai_cpu_core_id);
        infoDevice->set_ai_core_id(devInfo.ai_core_id);
        infoDevice->set_aicpu_occupy_bitmap(devInfo.aicpu_occupy_bitmap);
        SetCtrlCpuId(*infoDevice, devInfo.ctrl_cpu_id);
        std::string ctrlCpu;
        for (int64_t i = 0; i < devInfo.ctrl_cpu_core_num; i++) {
            ctrlCpu.append((i == 0) ? std::to_string(0) : ("," + std::to_string(i)));
        }

        infoDevice->set_ctrl_cpu(ctrlCpu);
        std::string aiCpu;
        for (int64_t i = devInfo.ai_cpu_core_id; i < devInfo.ctrl_cpu_core_num + devInfo.ai_cpu_core_num; i++) {
            aiCpu.append((i == devInfo.ai_cpu_core_id) ?
                std::to_string(devInfo.ai_cpu_core_id) : ("," + std::to_string(i)));
        }
        infoDevice->set_ai_cpu(aiCpu);
        SetHwtsFrequency(*infoDevice);
        std::string freq = Analysis::Dvvp::Driver::DrvGeAicFrq(devIndexId);
        infoDevice->set_aic_frequency(freq);
        infoDevice->set_aiv_frequency(freq);
    }
    infoMain->set_devices(hostIdSerial_);
    MSPROF_LOGI("End to AddDeviceInfo in info.json, hostIds: %s.", hostIdSerial_.c_str());

    return PROFILING_SUCCESS;
}

void InfoJson::SetCtrlCpuId(analysis::dvvp::proto::InfoDeviceInfo &infoDeviceInfo, const int64_t cpuId)
{
    const std::map<int, std::string> cpuTypes = {
        {0x41d03, "ARMv8_Cortex_A53"},
        {0x41d05, "ARMv8_Cortex_A55"},
        {0x41d07, "ARMv8_Cortex_A57"},
        {0x41d08, "ARMv8_Cortex_A72"},
        {0x41d09, "ARMv8_Cortex_A73"},
        {0x48d01, "TaishanV110"}
    };
    auto iterator = cpuTypes.find(cpuId);
    if (iterator != cpuTypes.end()) {
        infoDeviceInfo.set_ctrl_cpu_id(iterator->second);
    }
}

void InfoJson::SetHwtsFrequency(analysis::dvvp::proto::InfoDeviceInfo &infoDeviceInfo)
{
    std::string hwtsFrq = Analysis::Dvvp::Common::Config::ConfigManager::instance()->GetFrequency();
    MSPROF_LOGD("hwtsFrq:%s", hwtsFrq.c_str());
    infoDeviceInfo.set_hwts_frequency(hwtsFrq);
}

int InfoJson::AddOtherInfo(SHARED_PTR_ALIA<InfoMain> infoMain)
{
    if (jobInfo_.empty()) {
        jobInfo_ = "NA";
    }
    infoMain->set_jobinfo(jobInfo_);
    // mac
    std::string mac;
    if (analysis::dvvp::common::utils::Utils::GetMac(mac) != PROFILING_SUCCESS) {
        MSPROF_LOGW("GetMac failed.");
    }
    infoMain->set_mac(mac);
    SetPlatFormVersion(infoMain);
    SetVersionInfo(infoMain);
    return PROFILING_SUCCESS;
}

void InfoJson::SetPidInfo(SHARED_PTR_ALIA<InfoMain> infoMain, int pid)
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
        std::ifstream processInfo(processInfoPath);
        if (processInfo.is_open()) {
            std::getline(processInfo, pidName);
        } else {
            MSPROF_LOGE("Set pid_name failed(failed to open the file for pid_name info), pid=%d", pid);
        }
        // The length of searching tag "Name:\t" is 6. This constant is used to locate the position of pid_name.
        constexpr size_t searchTagLength = 6;
        size_t pidNamePos = pidName.find("Name:\t") + searchTagLength;
        if ((pidNamePos - searchTagLength) != std::string::npos) {
            pidName = pidName.substr(pidNamePos, pidName.size() - pidNamePos);
        } else {
            MSPROF_LOGE("Set pid_name failed, pid=%d", pid);
        }
#endif
    }
    infoMain->set_pid(pidtmp);
    infoMain->set_pid_name(pidName);
    return;
}

void InfoJson::SetPlatFormVersion(SHARED_PTR_ALIA<InfoMain> infoMain)
{
    std::string chipId = Analysis::Dvvp::Common::Config::ConfigManager::instance()->GetChipIdStr();
    infoMain->set_platform_version(chipId);
}

void InfoJson::SetVersionInfo(SHARED_PTR_ALIA<InfoMain> infoMain) const
{
    infoMain->set_version(PROF_VERSION_INFO);
}

InfoJson::~InfoJson()
{
}
}  // namespace host
}  // namespace dvvp
}  // namespace analysis
