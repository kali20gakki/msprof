/**
* @file adx_operate_log_level.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef ADX_SET_LOG_LEVEL_H
#define ADX_SET_LOG_LEVEL_H
#include "hdc_api.h"
#include "ide_tlv.h"
#include "memory_utils.h"
#include "log/adx_log.h"
namespace Adx {
constexpr int32_t TOOL_MAX_SLEEP_MILLSECOND = 4294967;
constexpr int32_t TOOL_ONE_THOUSAND         = 1000;
constexpr int32_t MSG_MAX_LEN               = 1024;
constexpr const char *DEVICE_LEVEL_SETTING_SUCCESS = "set device_%d level success.";
constexpr const char *LEVEL_SETTING_SUCCESS        = "++OK++";
constexpr const char *COMPUTE_POWER_GROUP = "multiple hosts use one device, prohibit operating log level";
constexpr int32_t GETLOGLEVEL = 1;
#define XFREE(ps)           \
    do {                    \
        if ((ps) != NULL) { \
            free((ps));     \
            (ps) = NULL;    \
        }                   \
    } while (0)

#define IDE_FREE_PACKET_AND_SET_NULL(ptr) do {            \
    IdeXfree(ptr);                                        \
    ptr = nullptr;                                        \
} while (0)


int OperateDeviceLevel(const HDC_CLIENT client, const struct tlv_req *req,\
                       IdeStringBuffer logLevelResult, int logLevelResultLength, int logOperatonType);
int CreatePacket(int devId, IdeString value, uint32_t valueLen, IdeRecvBuffT buf, IdeI32Pt bufLen);
int ToolSleep(UINT32 milliSecond);
int ReadSettingResult(HDC_SESSION session, IdeStringBuffer const resultBuf);
}
#endif // ADX_SET_LOG_LEVEL_H

