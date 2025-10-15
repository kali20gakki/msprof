/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_HOST_WRITE_INFO_H
#define ANALYSIS_DVVP_HOST_WRITE_INFO_H

#include <map>
#include <memory>
#include <vector>
#include "errno/error_code.h"
#include "ai_drv_dsmi_api.h"
#include "ai_drv_dev_api.h"
#include "mmpa_api.h"
#include "message/message.h"
namespace analysis {
namespace dvvp {
namespace host {
const char * const PLATFORM_CLOUD = "cloud";
const std::string INFO_FILE_NAME = "info.json";

struct InfoCpu : analysis::dvvp::message::BaseInfo {
    int32_t Id;
    std::string Name;
    std::string Frequency;
    int32_t Logical_CPU_Count;
    std::string Type;

    void ToObject(nlohmann::json &object) override
    {
        SET_VALUE(object, Id);
        SET_VALUE(object, Name);
        SET_VALUE(object, Frequency);
        SET_VALUE(object, Logical_CPU_Count);
        SET_VALUE(object, Type);
    }

    void FromObject(const nlohmann::json &object) override {}
};

struct InfoDeviceInfo : analysis::dvvp::message::BaseInfo {
    int id;
    uint32_t env_type;
    std::string ctrl_cpu_id;
    uint32_t ctrl_cpu_core_num;
    uint32_t ctrl_cpu_endian_little;
    uint32_t ts_cpu_core_num;
    uint32_t ai_cpu_core_num;
    uint32_t ai_core_num;
    uint32_t ai_cpu_core_id;
    uint32_t ai_core_id;
    uint32_t aicpu_occupy_bitmap;
    std::string ctrl_cpu;
    std::string ai_cpu;
    uint32_t aiv_num;
    std::string hwts_frequency;
    std::string aic_frequency;
    std::string aiv_frequency;

    void ToObject(nlohmann::json &object) override
    {
        SET_VALUE(object, id);
        SET_VALUE(object, env_type);
        SET_VALUE(object, ctrl_cpu_id);
        SET_VALUE(object, ctrl_cpu_core_num);
        SET_VALUE(object, ctrl_cpu_endian_little);
        SET_VALUE(object, ts_cpu_core_num);
        SET_VALUE(object, ai_cpu_core_num);
        SET_VALUE(object, ai_core_num);
        SET_VALUE(object, ai_cpu_core_id);
        SET_VALUE(object, ai_core_id);
        SET_VALUE(object, aicpu_occupy_bitmap);
        SET_VALUE(object, ctrl_cpu);
        SET_VALUE(object, ai_cpu);
        SET_VALUE(object, aiv_num);
        SET_VALUE(object, hwts_frequency);
        SET_VALUE(object, aic_frequency);
        SET_VALUE(object, aiv_frequency);
    }

    void FromObject(const nlohmann::json &object) override {}
};

struct NetCardInfo : analysis::dvvp::message::BaseInfo {
    std::string netCardName;
    uint32_t speed;

    void ToObject(nlohmann::json &object) override
    {
        SET_VALUE(object, netCardName);
        SET_VALUE(object, speed);
    }

    void FromObject(const nlohmann::json &object) override {}
};

struct InfoMain : analysis::dvvp::message::BaseInfo {
    std::string jobInfo;
    std::string version;
    std::string mac;
    std::string OS;
    int32_t cpuCores = 0;
    std::string hostname;
    std::string hwtype;
    std::string devices;
    std::string platform;
    std::vector<struct InfoCpu> CPU;
    std::vector<struct InfoDeviceInfo> DeviceInfo;
    std::string platform_version;
    std::string pid;
    uint32_t memoryTotal = 0;
    long cpuNums = 0;
    long sysClockFreq = 0;
    std::string upTime;
    uint32_t netCardNums = 0;
    std::vector<struct NetCardInfo> netCard;
    int32_t rank_id = 0;
    std::string pid_name;
    uint32_t drvVersion = 0;
    std::string hostUid;

    void ToObject(nlohmann::json &object) override
    {
        SET_VALUE(object, jobInfo);
        SET_VALUE(object, version);
        SET_VALUE(object, mac);
        SET_VALUE(object, OS);
        SET_VALUE(object, cpuCores);
        SET_VALUE(object, hostname);
        SET_VALUE(object, hwtype);
        SET_VALUE(object, devices);
        SET_VALUE(object, platform);
        SET_VALUE(object, platform_version);
        SET_VALUE(object, pid);
        SET_VALUE(object, memoryTotal);
        SET_VALUE(object, cpuNums);
        SET_VALUE(object, sysClockFreq);
        SET_VALUE(object, upTime);
        SET_VALUE(object, netCardNums);
        SET_VALUE(object, rank_id);
        SET_VALUE(object, pid_name);
        SET_VALUE(object, drvVersion);
        SET_VALUE(object, hostUid);
        SET_ARRAY_OBJECT_VALUE(object, CPU, "CPU");
        SET_ARRAY_OBJECT_VALUE(object, DeviceInfo, "DeviceInfo");
        SET_ARRAY_OBJECT_VALUE(object, netCard, "netCard");
    }

    void FromObject(const nlohmann::json &object) override {}
};

class InfoJson {
public:
    InfoJson(const std::string &jobInfo, const std::string &devices, int hostpid);
    virtual ~InfoJson();
    int Generate(std::string &content);

private:
    int InitDeviceIds();
    int AddHostInfo();
    int AddDeviceInfo();
    int AddOtherInfo();
    void SetHwtsFrequency(uint32_t deviceId, struct InfoDeviceInfo &devInfo);
    int GetCtrlCpuInfo(uint32_t devId, struct InfoDeviceInfo &devInfo);
    int GetDevInfo(int deviceId, struct InfoDeviceInfo &devInfo);
    void SetPlatFormVersion();
    void SetCtrlCpuId(struct InfoDeviceInfo &devInfo, int64_t ctrlCpuId);
    void SetPidInfo(int pid);
    void AddSysConf();
    void AddSysTime();
    void AddMemTotal();
    void AddNetCardInfo();
    void SetRankId();
    void SetVersionInfo();
    void SetDrvVersion();
    std::string GetHostOscFrequency() const;
    std::string GetDeviceOscFrequency(uint32_t deviceId, const std::string &freq);

private:
    std::string jobInfo_;
    std::string devices_;
    std::vector<int> devIds_;
    std::vector<int> hostIds_;
    std::string hostIdSerial_;
    int hostpid_;
    struct InfoMain infoMain_;
};
}  // namespace host
}  // namespace dvvp
}  // namespace analysis

#endif
