/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: 对外接口错误码定义
 */
#ifndef GRAPH_TUNER_GRAPH_TUNER_ERRORCODE_H_
#define GRAPH_TUNER_GRAPH_TUNER_ERRORCODE_H_

#include <cstdint>

/** Assigned SYS ID */
constexpr uint8_t SYSID_TUNE = 10;

/* * lx module ID */
constexpr uint8_t TUNE_MODID_LX = 30; // lx fusion  pass id

namespace tune {
typedef uint32_t Status;
} // namespace tune

/* *
 * Build error code
 */
#define TUNE_DEF_ERRORNO(sysid, modid, name, value, desc)                                                      \
    static constexpr tune::Status name =                                                                       \
        (((((uint32_t)(0xFF & ((uint8_t)(sysid)))) << 24) | (((uint32_t)(0xFF & ((uint8_t)(modid)))) << 16)) | \
        (0xFFFF & ((uint16_t)(value))));


#define TUNE_DEF_ERRORNO_LX(name, value, desc) TUNE_DEF_ERRORNO(SYSID_TUNE, TUNE_MODID_LX, name, value, desc)

namespace tune {
// generic
TUNE_DEF_ERRORNO(0, 0, SUCCESS, 0, "Success");
TUNE_DEF_ERRORNO(0xFF, 0xFF, FAILED, 0xFFFF, "Failed");

TUNE_DEF_ERRORNO_LX(NO_FUSION_STRATEGY, 10, "Lx fusion strategy is invalid.");
TUNE_DEF_ERRORNO_LX(UNTUNED, 20, "Untuned");
TUNE_DEF_ERRORNO_LX(NO_UPDATE_BANK, 30, "Model Bank is not updated!");
TUNE_DEF_ERRORNO_LX(HIT_FUSION_STRATEGY, 40, "Hit lx fusion strategy.");
// inferface
} // namespace tune
#endif // TUNE_COMMON_ERRORCODE_H_
