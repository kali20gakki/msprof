/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_HOST_WRITE_INFO_H
#define ANALYSIS_DVVP_HOST_WRITE_INFO_H

#include <map>
#include <memory>
#include <vector>
#include "ai_drv_dsmi_api.h"
#include "ai_drv_dev_api.h"
#include "mmpa_api.h"
namespace analysis {
namespace dvvp {
namespace proto {
    class InfoMain;
    class InfoDeviceInfo;
}
namespace host {
const char * const PLATFORM_CLOUD = "cloud";

const std::string INFO_FILE_NAME = "info.json";

struct DeviceInfo {
    int64_t env_type; /**< 0, FPGA  1, EMU 2, ESL*/
    int64_t ctrl_cpu_id;
    int64_t ctrl_cpu_core_num;
    int64_t ctrl_cpu_endian_little;
    int64_t ts_cpu_core_num;
    int64_t ai_cpu_core_num;
    int64_t ai_core_num;
    int64_t ai_cpu_core_id;
    int64_t ai_core_id;
    int64_t aicpu_occupy_bitmap;
    DeviceInfo()
        : env_type(0), ctrl_cpu_id(0), ctrl_cpu_core_num(0), ctrl_cpu_endian_little(0),
          ts_cpu_core_num(0), ai_cpu_core_num(0), ai_core_num(0), ai_cpu_core_id(0), ai_core_id(0),
          aicpu_occupy_bitmap(0)
    {
    }
};

class InfoJson {
public:
    InfoJson(const std::string &jobInfo, const std::string &devices, int hostpid);
    virtual ~InfoJson();
    int Generate(std::string &content);

private:
    int InitDeviceIds();
    int AddHostInfo(SHARED_PTR_ALIA<analysis::dvvp::proto::InfoMain> message);
    int AddDeviceInfo(SHARED_PTR_ALIA<analysis::dvvp::proto::InfoMain> message);
    int AddOtherInfo(SHARED_PTR_ALIA<analysis::dvvp::proto::InfoMain> message);
    void SetHwtsFrequency(analysis::dvvp::proto::InfoDeviceInfo &message);
    int GetCtrlCpuInfo(uint32_t devId, struct DeviceInfo &devInfo);
    int GetDevInfo(int deviceId, struct DeviceInfo &devInfo);
    void SetPlatFormVersion(SHARED_PTR_ALIA<analysis::dvvp::proto::InfoMain> infoMain);
    void SetCtrlCpuId(analysis::dvvp::proto::InfoDeviceInfo &infoDeviceInfo, const int64_t cpuId);
    void SetPidInfo(SHARED_PTR_ALIA<analysis::dvvp::proto::InfoMain> infoMain, int pid);
    void AddSysConf(SHARED_PTR_ALIA<analysis::dvvp::proto::InfoMain> infoMain);
    void AddSysTime(SHARED_PTR_ALIA<analysis::dvvp::proto::InfoMain> infoMain);
    void AddMemTotal(SHARED_PTR_ALIA<analysis::dvvp::proto::InfoMain> infoMain);
    void AddNetCardInfo(SHARED_PTR_ALIA<analysis::dvvp::proto::InfoMain> infoMain);
    void AddRankId(SHARED_PTR_ALIA<analysis::dvvp::proto::InfoMain> infoMain);

private:
    std::string jobInfo_;
    std::string devices_;
    std::vector<int> devIds_;
    std::vector<int> hostIds_;
    std::string hostIdSerial_;
    int hostpid_;
};
}  // namespace host
}  // namespace dvvp
}  // namespace analysis

#endif
