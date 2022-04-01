/**
* @file log_api.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "adx_log_api.h"
#include "adx_operate_log_level.h"
using namespace Adx;
/**
 * @brief set or get device log level
 * @param [in] uint16_t devId: device id
 * @param [in] IdeString logLevel: device log level(debug/info/warning/error/null etc.)
 * @param [in]logLevelResult: log level result
 * @param [in]logOperatonType: log operaton type
 * @return 0: success; other : failed
 */
int AdxOperateDeviceLogLevel(uint16_t devId, IdeString logLevel, IdeStringBuffer logLevelResult,
                             int logLevelResultLength, int logOperatonType)
{
    int32_t err = IDE_DAEMON_ERROR;
    IDE_CTRL_VALUE_FAILED(logLevel != nullptr, return err, "Set device log level input parameter invalied");
    // 1. create package
    uint32_t sendLen = 0;
    IdeBuffT reqBuf = nullptr;
    err = Adx::CreatePacket(devId, logLevel, strlen(logLevel), &reqBuf, reinterpret_cast<int *>(&sendLen));
    if (err != IDE_DAEMON_OK) {
        IDE_LOGE("Create Packet failed, err: %d", err);
        IDE_FREE_PACKET_AND_SET_NULL(reqBuf);
        return IDE_DAEMON_ERROR;
    }
    // 2.create client
    struct tlv_req *req = reinterpret_cast<struct tlv_req *>(reqBuf);
    HDC_CLIENT client = HdcClientCreate(HDC_SERVICE_TYPE_IDE2);
    if (client == nullptr) {
        IDE_LOGE("Hdc client create exception");
        IDE_FREE_PACKET_AND_SET_NULL(reqBuf);
        return IDE_DAEMON_ERROR;
    }
    // 3. set or get log level
    err = Adx::OperateDeviceLevel(client, req, logLevelResult, logLevelResultLength, logOperatonType);
    if (err != IDE_DAEMON_OK) {
        if (err == ADX_COMPUTE_POWER_GROUP) {
            printf("multiple hosts use one device, prohibit operating log level\n");
        } else {
            printf("%s device log level failed! req->value = %s, req->type = %d, req->len = %d\n",
                   logOperatonType == Adx::GETLOGLEVEL ? "Get" : "Set", req->value, req->type, req->len);
        }
        IDE_FREE_PACKET_AND_SET_NULL(reqBuf);
        (void)HdcClientDestroy((HDC_CLIENT)client);
        return IDE_DAEMON_ERROR;
    }

    IDE_FREE_PACKET_AND_SET_NULL(reqBuf);
    (void)HdcClientDestroy((HDC_CLIENT)client);
    return IDE_DAEMON_OK;
}