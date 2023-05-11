/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author:
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_DEVICE_AI_DRV_API_H
#define ANALYSIS_DVVP_DEVICE_AI_DRV_API_H

#include <vector>
#include <map>
#include <string>
#include "message/prof_params.h"
#include "ascend_hal.h"
#include "utils/utils.h"
#include "singleton/singleton.h"
#if (defined(linux) || defined(__linux__))
using UNSIGNED_INT_PTR = unsigned int *;
int __attribute__((weak)) halProfDataFlush(unsigned int deviceId, unsigned int channelId, UNSIGNED_INT_PTR bufSize);
#endif
namespace analysis {
namespace dvvp {
namespace driver {
using namespace analysis::dvvp::common::utils;

enum AI_DRV_CHANNEL {
    PROF_CHANNEL_UNKNOWN = 0,
    PROF_CHANNEL_HBM     = CHANNEL_HBM, // 1
    PROF_CHANNEL_PCIE    = CHANNEL_PCIE, // 3
    PROF_CHANNEL_NIC     = CHANNEL_NIC, // 4
    PROF_CHANNEL_DVPP    = CHANNEL_DVPP, // 6
    PROF_CHANNEL_DDR     = CHANNEL_DDR, // 7
    PROF_CHANNEL_LLC     = CHANNEL_LLC, // 8
    PROF_CHANNEL_HCCS    = CHANNEL_HCCS, // 9
    PROF_CHANNEL_TS_CPU  = CHANNEL_TSCPU, // 10
    // instr profiling group(0~9) channelId(11~41)
    PROF_CHANNEL_INSTR_GROUP0_AIC = CHANNEL_INSTR_GROUP0_AIC,   // 11
    PROF_CHANNEL_INSTR_GROUP0_AIV0 = CHANNEL_INSTR_GROUP0_AIV0,
    PROF_CHANNEL_INSTR_GROUP0_AIV1 = CHANNEL_INSTR_GROUP0_AIV1,
    PROF_CHANNEL_INSTR_GROUP1_AIC = CHANNEL_INSTR_GROUP1_AIC,
    PROF_CHANNEL_INSTR_GROUP1_AIV0 = CHANNEL_INSTR_GROUP1_AIV0,
    PROF_CHANNEL_INSTR_GROUP1_AIV1 = CHANNEL_INSTR_GROUP1_AIV1,
    PROF_CHANNEL_INSTR_GROUP2_AIC = CHANNEL_INSTR_GROUP2_AIC,
    PROF_CHANNEL_INSTR_GROUP2_AIV0 = CHANNEL_INSTR_GROUP2_AIV0,
    PROF_CHANNEL_INSTR_GROUP2_AIV1 = CHANNEL_INSTR_GROUP2_AIV1,
    PROF_CHANNEL_INSTR_GROUP3_AIC = CHANNEL_INSTR_GROUP3_AIC,
    PROF_CHANNEL_INSTR_GROUP3_AIV0 = CHANNEL_INSTR_GROUP3_AIV0,
    PROF_CHANNEL_INSTR_GROUP3_AIV1 = CHANNEL_INSTR_GROUP3_AIV1,
    PROF_CHANNEL_INSTR_GROUP4_AIC = CHANNEL_INSTR_GROUP4_AIC,
    PROF_CHANNEL_INSTR_GROUP4_AIV0 = CHANNEL_INSTR_GROUP4_AIV0,
    PROF_CHANNEL_INSTR_GROUP4_AIV1 = CHANNEL_INSTR_GROUP4_AIV1,
    PROF_CHANNEL_INSTR_GROUP5_AIC = CHANNEL_INSTR_GROUP5_AIC,
    PROF_CHANNEL_INSTR_GROUP5_AIV0 = CHANNEL_INSTR_GROUP5_AIV0,
    PROF_CHANNEL_INSTR_GROUP5_AIV1 = CHANNEL_INSTR_GROUP5_AIV1,
    PROF_CHANNEL_INSTR_GROUP6_AIC = CHANNEL_INSTR_GROUP6_AIC,
    PROF_CHANNEL_INSTR_GROUP6_AIV0 = CHANNEL_INSTR_GROUP6_AIV0,
    PROF_CHANNEL_INSTR_GROUP6_AIV1 = CHANNEL_INSTR_GROUP6_AIV1,
    PROF_CHANNEL_INSTR_GROUP7_AIC = CHANNEL_INSTR_GROUP7_AIC,
    PROF_CHANNEL_INSTR_GROUP7_AIV0 = CHANNEL_INSTR_GROUP7_AIV0,
    PROF_CHANNEL_INSTR_GROUP7_AIV1 = CHANNEL_INSTR_GROUP7_AIV1,
    PROF_CHANNEL_INSTR_GROUP8_AIC = CHANNEL_INSTR_GROUP8_AIC,
    PROF_CHANNEL_INSTR_GROUP8_AIV0 = CHANNEL_INSTR_GROUP8_AIV0,
    PROF_CHANNEL_INSTR_GROUP8_AIV1 = CHANNEL_INSTR_GROUP8_AIV1,
    PROF_CHANNEL_INSTR_GROUP9_AIC = CHANNEL_INSTR_GROUP9_AIC,
    PROF_CHANNEL_INSTR_GROUP9_AIV0 = CHANNEL_INSTR_GROUP9_AIV0,
    PROF_CHANNEL_INSTR_GROUP9_AIV1 = CHANNEL_INSTR_GROUP9_AIV1, // 41
    PROF_CHANNEL_AI_CORE  = CHANNEL_AICORE, // 43
    PROF_CHANNEL_TS_FW    = CHANNEL_TSFW, // 44
    PROF_CHANNEL_HWTS_LOG = CHANNEL_HWTS_LOG, // 45
    PROF_CHANNEL_FMK      = CHANNEL_KEY_POINT, // 46
    PROF_CHANNEL_L2_CACHE = CHANNEL_TSFW_L2, // 47
    PROF_CHANNEL_AIV_HWTS_LOG = CHANNEL_HWTS_LOG1, // 48
    PROF_CHANNEL_AIV_TS_FW   = CHANNEL_TSFW1, // 49
    PROF_CHANNEL_STARS_SOC_LOG = CHANNEL_STARS_SOC_LOG_BUFFER,   // 50
    PROF_CHANNEL_STARS_BLOCK_LOG = CHANNEL_STARS_BLOCK_LOG_BUFFER, // 51
    PROF_CHANNEL_STARS_SOC_PROFILE = CHANNEL_STARS_SOC_PROFILE_BUFFER, // 52
    PROF_CHANNEL_FFTS_PROFILIE_TASK = CHANNEL_FFTS_PROFILE_BUFFER_TASK, // 53
    PROF_CHANNEL_FFTS_PROFILIE_SAMPLE = CHANNEL_FFTS_PROFILE_BUFFER_SAMPLE, // 54
    // instr profiling group(10~17) channelId(60~83)
    PROF_CHANNEL_INSTR_GROUP10_AIC = CHANNEL_INSTR_GROUP10_AIC,     // 60
    PROF_CHANNEL_INSTR_GROUP10_AIV0 = CHANNEL_INSTR_GROUP10_AIV0,
    PROF_CHANNEL_INSTR_GROUP10_AIV1 = CHANNEL_INSTR_GROUP10_AIV1,
    PROF_CHANNEL_INSTR_GROUP11_AIC = CHANNEL_INSTR_GROUP11_AIC,
    PROF_CHANNEL_INSTR_GROUP11_AIV0 = CHANNEL_INSTR_GROUP11_AIV0,
    PROF_CHANNEL_INSTR_GROUP11_AIV1 = CHANNEL_INSTR_GROUP11_AIV1,
    PROF_CHANNEL_INSTR_GROUP12_AIC = CHANNEL_INSTR_GROUP12_AIC,
    PROF_CHANNEL_INSTR_GROUP12_AIV0 = CHANNEL_INSTR_GROUP12_AIV0,
    PROF_CHANNEL_INSTR_GROUP12_AIV1 = CHANNEL_INSTR_GROUP12_AIV1,
    PROF_CHANNEL_INSTR_GROUP13_AIC = CHANNEL_INSTR_GROUP13_AIC,
    PROF_CHANNEL_INSTR_GROUP13_AIV0 = CHANNEL_INSTR_GROUP13_AIV0,
    PROF_CHANNEL_INSTR_GROUP13_AIV1 = CHANNEL_INSTR_GROUP13_AIV1,
    PROF_CHANNEL_INSTR_GROUP14_AIC = CHANNEL_INSTR_GROUP14_AIC,
    PROF_CHANNEL_INSTR_GROUP14_AIV0 = CHANNEL_INSTR_GROUP14_AIV0,
    PROF_CHANNEL_INSTR_GROUP14_AIV1 = CHANNEL_INSTR_GROUP14_AIV1,
    PROF_CHANNEL_INSTR_GROUP15_AIC = CHANNEL_INSTR_GROUP15_AIC,
    PROF_CHANNEL_INSTR_GROUP15_AIV0 = CHANNEL_INSTR_GROUP15_AIV0,
    PROF_CHANNEL_INSTR_GROUP15_AIV1 = CHANNEL_INSTR_GROUP15_AIV1,
    PROF_CHANNEL_INSTR_GROUP16_AIC = CHANNEL_INSTR_GROUP16_AIC,
    PROF_CHANNEL_INSTR_GROUP16_AIV0 = CHANNEL_INSTR_GROUP16_AIV0,
    PROF_CHANNEL_INSTR_GROUP16_AIV1 = CHANNEL_INSTR_GROUP16_AIV1,
    PROF_CHANNEL_INSTR_GROUP17_AIC = CHANNEL_INSTR_GROUP17_AIC,
    PROF_CHANNEL_INSTR_GROUP17_AIV0 = CHANNEL_INSTR_GROUP17_AIV0,
    PROF_CHANNEL_INSTR_GROUP17_AIV1 = CHANNEL_INSTR_GROUP17_AIV1,   // 83
    PROF_CHANNEL_AIV_CORE = CHANNEL_AIV, // 85
    // instr profiling group(18~24) channelId(86~106)
    PROF_CHANNEL_INSTR_GROUP18_AIC = CHANNEL_INSTR_GROUP18_AIC,     // 86
    PROF_CHANNEL_INSTR_GROUP18_AIV0 = CHANNEL_INSTR_GROUP18_AIV0,
    PROF_CHANNEL_INSTR_GROUP18_AIV1 = CHANNEL_INSTR_GROUP18_AIV1,
    PROF_CHANNEL_INSTR_GROUP19_AIC = CHANNEL_INSTR_GROUP19_AIC,
    PROF_CHANNEL_INSTR_GROUP19_AIV0 = CHANNEL_INSTR_GROUP19_AIV0,
    PROF_CHANNEL_INSTR_GROUP19_AIV1 = CHANNEL_INSTR_GROUP19_AIV1,
    PROF_CHANNEL_INSTR_GROUP20_AIC = CHANNEL_INSTR_GROUP20_AIC,
    PROF_CHANNEL_INSTR_GROUP20_AIV0 = CHANNEL_INSTR_GROUP20_AIV0,
    PROF_CHANNEL_INSTR_GROUP20_AIV1 = CHANNEL_INSTR_GROUP20_AIV1,
    PROF_CHANNEL_INSTR_GROUP21_AIC = CHANNEL_INSTR_GROUP21_AIC,
    PROF_CHANNEL_INSTR_GROUP21_AIV0 = CHANNEL_INSTR_GROUP21_AIV0,
    PROF_CHANNEL_INSTR_GROUP21_AIV1 = CHANNEL_INSTR_GROUP21_AIV1,
    PROF_CHANNEL_INSTR_GROUP22_AIC = CHANNEL_INSTR_GROUP22_AIC,
    PROF_CHANNEL_INSTR_GROUP22_AIV0 = CHANNEL_INSTR_GROUP22_AIV0,
    PROF_CHANNEL_INSTR_GROUP22_AIV1 = CHANNEL_INSTR_GROUP22_AIV1,
    PROF_CHANNEL_INSTR_GROUP23_AIC = CHANNEL_INSTR_GROUP23_AIC,
    PROF_CHANNEL_INSTR_GROUP23_AIV0 = CHANNEL_INSTR_GROUP23_AIV0,
    PROF_CHANNEL_INSTR_GROUP23_AIV1 = CHANNEL_INSTR_GROUP23_AIV1,
    PROF_CHANNEL_INSTR_GROUP24_AIC = CHANNEL_INSTR_GROUP24_AIC,
    PROF_CHANNEL_INSTR_GROUP24_AIV0 = CHANNEL_INSTR_GROUP24_AIV0,
    PROF_CHANNEL_INSTR_GROUP24_AIV1 = CHANNEL_INSTR_GROUP24_AIV1,   // 106
    PROF_CHANNEL_ROCE     = CHANNEL_ROCE, // 129
    PROF_CHANNEL_NPU_APP_MEM = CHANNEL_NPU_APP_MEM, // 130
    PROF_CHANNEL_NPU_MEM = CHANNEL_NPU_MEM, // 131
    PROF_CHANNEL_LP = CHANNEL_LP, // 132
    PROF_CHANNEL_DVPP_VENC = CHANNEL_DVPP_VENC, // 135
    PROF_CHANNEL_DVPP_JPEGE = CHANNEL_DVPP_JPEGE, // 136
    PROF_CHANNEL_DVPP_VDEC = CHANNEL_DVPP_VDEC, // 137
    PROF_CHANNEL_DVPP_JPEGD = CHANNEL_DVPP_JPEGD, // 138
    PROF_CHANNEL_DVPP_VPC = CHANNEL_DVPP_VPC, // 139
    PROF_CHANNEL_DVPP_PNG = CHANNEL_DVPP_PNG, // 140
    PROF_CHANNEL_DVPP_SCD = CHANNEL_DVPP_SCD, // 141
    PROF_CHANNEL_MAX      = CHANNEL_NUM, // 160
};

enum PROFILE_MODE {
    PROFILE_REAL_TIME = PROF_REAL,
};

enum TAG_TS_PROF_TYPE {
    TS_PROF_TYPE_TASK_BASE = 0,
    TS_PROF_TYPE_SAMPLE_BASE,
};

enum TAG_FFTS_PROF_TYPE {
    FFTS_PROF_TYPE_TASK_BASE = 0,
    FFTS_PROF_TYPE_SAMPLE_BASE,
    FFTS_PROF_TYPE_BLOCK,
    FFTS_PROF_TYPE_SUBTASK,
    FFTS_PROF_TYPE_BULT,
};

enum TAG_FFTS_PROF_MODE {
    FFTS_PROF_MODE_AIC = 0,
    FFTS_PROF_MODE_AIV,
    FFTS_PROF_MODE_BULT,
};

enum STARS_ACSQ_BITMODE {
    DSA_LOG_EN = 0,
    VDEC_LOG_EN,
    JPEGD_LOG_EN,
    JPEGE_LOG_EN,
    VPC_LOG_EN,
    TOPIC_LOG_EN,
    PCIE_LOG_EN,
    ROCEE_LOG_EN,
    SDMA_LOG_EN,
    CTRL_TASK_LOG_EN,
};

using AI_DRV_CHANNEL_PTR = AI_DRV_CHANNEL *;
using PROF_POLL_INFO_PTR = struct prof_poll_info *;
constexpr int PMU_EVENT_MAX_NUM = 8;

template<typename T>
using TEMPLATE_T_PTR = T *;

using TS_PROFILE_COMMAND_TYPE_T = enum TAG_TS_PROFILE_COMMAND_TYPE {
    TS_PROFILE_COMMAND_TYPE_ACK = 0,
    TS_PROFILE_COMMAND_TYPE_PROFILING_ENABLE = 1,
    TS_PROFILE_COMMAND_TYPE_PROFILING_DISABLE = 2,
    TS_PROFILE_COMMAND_TYPE_BUFFERFULL = 3,
    TS_PROFILE_COMMAND_TASK_BASE_ENABLE = 4,     // task base profiling enable
    TS_PROFILE_COMMAND_TASK_BASE_DISENABLE = 5,  // task base profiling disenable
    TS_PROFILE_COMMAND_TS_FW_ENABLE = 6,         // TS fw data enable
    TS_PROFILE_COMMAND_TS_FW_DISENABLE = 7,      // TS fw data disenable
};

using TsTsCpuProfileConfigT = struct TagTsTsCpuProfileConfig {
    uint32_t period;
    uint32_t event_num;
    uint32_t event[0];
};

using TsAiCpuProfileConfigT = struct TagTsAiCpuProfileConfig {
    uint32_t period;
    uint32_t event_num;
    uint32_t event[0];
} ;

using TsAiCoreProfileConfigT = struct TagTsAiCoreProfileConfig {
    uint32_t type;                      // 0-task base, 1-sample base
    uint32_t almost_full_threshold;     // sample base
    uint32_t period;                    // sample base
    uint32_t core_mask;                 // sample base
    uint32_t event_num;                 // public
    uint32_t event[PMU_EVENT_MAX_NUM];  // public
    uint32_t tag;                       // bit0=0 enable immediately; bit0=1 enable delay
};

using TsTsFwProfileConfigT = struct TagTsTsFwProfileConfig {
    uint32_t period;
    uint32_t ts_task_track;     // 1-enable,2-disable
    uint32_t ts_cpu_usage;      // 1-enable,2-disable
    uint32_t ai_core_status;    // 1-enable,2-disable
    uint32_t ts_timeline;       // 1-enable,2-disable
    uint32_t ai_vector_status;  // 1-enable,2-disable
    uint32_t ts_keypoint;       // 1-enable,2-disable
    uint32_t ts_memcpy;         // 1-enable,2-disable
};

using StarsSocLogConfigT = struct TagStarsSocLogConfig {
    uint32_t acsq_task;         // 1-enable,2-disable
    uint32_t acc_pmu;           // 1-enable,2-disable
    uint32_t cdqm_reg;          // 1-enable,2-disable
    uint32_t dvpp_vpc_block;    // 1-enable,2-disable
    uint32_t dvpp_jpegd_block;  // 1-enable,2-disable
    uint32_t dvpp_jpede_block;  // 1-enable,2-disable
    uint32_t ffts_thread_task;  // 1-enable,2-disable
    uint32_t ffts_block;        // 1-enable,2-disable
    uint32_t sdma_dmu;          // 1-enable,2-disable
};

using CommonSampleSwitchT = struct TagSocProfileConfig {
    uint32_t innerSwitch;   // 1-enable,2-disable
    uint32_t period;        // ms
};

using StarsSocProfileConfigT = struct TagStarsSocProfileConfig {
    CommonSampleSwitchT acc_pmu;
    CommonSampleSwitchT on_chip;
    CommonSampleSwitchT inter_die;
    CommonSampleSwitchT inter_chip;
    CommonSampleSwitchT low_power;
    CommonSampleSwitchT starsInfo;
};

using StarsAiCoreProfileConfigT = struct TagStarsAiCoreConfig {
    uint32_t type;                     // bit0：task base | bit1：sample base | bit2：blk task | bit3：sub task
    uint32_t period;                   // sample base
    uint32_t core_mask;                // sample base
    uint32_t event_num;                // public
    uint16_t event[PMU_EVENT_MAX_NUM]; // public
};

using FftsProfileConfigT = struct TagFftsProfileConfig {
    uint32_t cfgMode;      // 0-none, 1-aic, 2-aiv, 3-aic&aiv
    StarsAiCoreProfileConfigT aiEventCfg[FFTS_PROF_MODE_BULT];
};

using InstrProfileConfigT = struct TagInstrProfileConfig {
    uint32_t period;
};

using TsHwtsProfileConfigT = struct TagTsHwtsProfileConfig {
    uint32_t tag;           // bit0=0 enable immediately; bit0=1 enable delay
};

struct TagDdrProfileConfig {
    uint32_t period;
    uint32_t masterId;  // core_id
    uint32_t eventNum;
    uint32_t event[0];  // read-0,write-1
};

struct TagTsHbmProfileConfig {
    uint32_t period;
    uint32_t masterId;
    uint32_t eventNum;
    uint32_t event[0];
};

struct TagMemProfileConfig {
    uint32_t period;
    uint32_t res1;
    uint32_t event;
    uint32_t res2;
};

struct LpmConvProfileConfig {
    uint32_t period;
    uint32_t res;
};

/* LLC profile cfg data */
struct TagLlcProfileConfig {
    uint32_t period;   // sample period
    uint32_t sampleType;  // sample data type, 0 read hit rate, 1 write hit rate
};

struct TagTsL2CacheProfileConfig {
    uint32_t eventNum;
    uint32_t event[0];
};

struct DrvPeripheralProfileCfg {
    DrvPeripheralProfileCfg()
        : profDeviceId(-1),
          profSamplePeriod(0),
          profSamplePeriodHi(0),
          profChannel(PROF_CHANNEL_UNKNOWN),
          configP(nullptr),
          configSize(0),
          cfgMode(0),
          aicMode(0),
          aivMode(0) {}
    virtual ~DrvPeripheralProfileCfg() {}
    int profDeviceId;
    int profSamplePeriod;
    int profSamplePeriodHi;
    AI_DRV_CHANNEL profChannel;
    void *configP;
    uint32_t configSize;
    uint32_t cfgMode;
    uint32_t aicMode;
    uint32_t aivMode;
    std::string profDataFilePath;
    std::string profDataFile;
};

int DrvPeripheralStart(const DrvPeripheralProfileCfg &peripheralCfg);

template<typename T>
int DoProfTsCpuStart(const DrvPeripheralProfileCfg &peripheralCfg,
                     const std::vector<std::string> &profEvents,
                     TEMPLATE_T_PTR<T> configP,
                     uint32_t configSize);

int DrvTscpuStart(const DrvPeripheralProfileCfg &peripheralCfg,
                  const std::vector<std::string> &profEvents);

int DrvAicoreStart(const DrvPeripheralProfileCfg &peripheralCfg,
                   const std::vector<int> &profCores,
                   const std::vector<std::string> &profEvents);

int DrvAicoreTaskBasedStart(int profDeviceId,
    AI_DRV_CHANNEL profChannel, const std::vector<std::string> &profEvents);

int DrvL2CacheTaskStart(int profDeviceId, AI_DRV_CHANNEL profChannel,
    const std::vector<std::string> &profEvents);

int DrvSetTsCommandType(TsTsFwProfileConfigT &configP,
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> profileParams);

int DrvTsFwStart(const DrvPeripheralProfileCfg &peripheralCfg,
                 SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> profileParams);

int DrvStarsSocLogStart(const DrvPeripheralProfileCfg &peripheralCfg,
                        SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> profileParams);

int DrvFftsProfileStart(const DrvPeripheralProfileCfg &peripheralCfg, const std::vector<int> &aicCores,
                        const std::vector<std::string> &aicEvents, const std::vector<int> &aivCores,
                        const std::vector<std::string> &aivEvents);

int DrvInstrProfileStart(const uint32_t devId, const AI_DRV_CHANNEL channelId, const uint32_t sampleCycle);

int DrvHwtsLogStart(int profDeviceId, AI_DRV_CHANNEL profChannel);

int DrvFmkDataStart(int devId, AI_DRV_CHANNEL profChannel);

int DrvStop(int profDeviceId,
            AI_DRV_CHANNEL profChannel);

int DrvChannelRead(int profDeviceId,
                   AI_DRV_CHANNEL profChannel,
                   UNSIGNED_CHAR_PTR outBuf,
                   uint32_t bufSize);

int DrvChannelPoll(PROF_POLL_INFO_PTR outBuf,
                   int num,
                   int timeout);

int DrvProfFlush(unsigned int deviceId, unsigned int channelId, unsigned int &bufSize);

struct DrvProfChannelInfo {
    DrvProfChannelInfo()
        : channelId(PROF_CHANNEL_UNKNOWN),
          channelType(0) {}
    virtual ~DrvProfChannelInfo() {}
    AI_DRV_CHANNEL channelId;
    unsigned int channelType;  /* system or APP */
    std::string channelName;
};

struct DrvProfChannelsInfo {
    DrvProfChannelsInfo()
        : deviceId(-1),
          chipType(0) {}
    virtual ~DrvProfChannelsInfo() {}
    int deviceId;
    unsigned int chipType;
    std::vector<struct DrvProfChannelInfo> channels;
};

class AiDrvProfApi {
public:
    AiDrvProfApi();
    virtual ~AiDrvProfApi();
};

class DrvChannelsMgr : public analysis::dvvp::common::singleton::Singleton<DrvChannelsMgr> {
public:
    DrvChannelsMgr();
    virtual ~DrvChannelsMgr();
    int GetAllChannels(int devIndexId);
    bool ChannelIsValid(int devId, AI_DRV_CHANNEL channelId);

private:
    std::map<int, struct DrvProfChannelsInfo> devIdChannelsMap_;
    std::mutex mtxChannel_;    // mutex for channel mgr
};
}  // namespace driver
}  // namespace dvvp
}  // namespace analysis
#endif
