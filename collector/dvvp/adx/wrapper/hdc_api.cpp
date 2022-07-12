/**
 * @file adx_hdc_api.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "hdc_api.h"
#include <list>

#include "securec.h"
#include "mmpa_plugin.h"
#include "driver_plugin.h"
#include "adx_config.h"
#include "msprof_dlog.h"
#include "memory_utils.h"
#include "utils/utils.h"

#define IDE_FREE_HDC_MSG_AND_SET_NULL(ptr) do {                        \
    if ((ptr) != nullptr) {                                            \
        DriverPlugin::instance()->MsprofDrvHdcFreeMsg(ptr);                                            \
        ptr = nullptr;                                                 \
    }                                                                  \
} while (0)

namespace Analysis {
namespace Dvvp {
namespace Adx {
using namespace IdeDaemon::Common::Config;
using namespace Collector::Dvvp::Plugin;

struct DataSendMsg {
    IdeSendBuffT buf;
    int bufLen;
    uint32_t maxSendLen;
};

/**
 * @brief       crete HDC client
 * @param [in]  client : HDC initialization handle
 * @param [in]  type   : create client hdc service type
 * @return
 *      IDE_DAEMON_OK:    init succ
 *      IDE_DAEMON_ERROR: init failed
 */
HDC_CLIENT HdcClientCreate(drvHdcServiceType type)
{
    hdcError_t error;
    int32_t flag = 0;
    HDC_CLIENT client = nullptr;

    // create HDC client
    error = DriverPlugin::instance()->MsprofDrvHdcClientCreate (&client, MAX_SESSION_NUM, type, flag);
    if (error != DRV_ERROR_NONE || client == nullptr) {
        MSPROF_LOGE("Hdc Client Create Failed, error: %d", error);
        return nullptr;
    }
    return client;
}

int32_t HdcClientDestroy(HDC_CLIENT client)
{
    if (client != nullptr) {
        hdcError_t error = DriverPlugin::instance()->MsprofDrvHdcClientDestroy(client);
        if (error != DRV_ERROR_NONE) {
            MSPROF_LOGE("Hdc Client Destroy error: %d", error);
            return IDE_DAEMON_ERROR;
        }
    }
    return IDE_DAEMON_OK;
}

HDC_SERVER HdcServerCreate(int32_t logDevId, drvHdcServiceType type)
{
    MSPROF_LOGD("HdcServerCreate begin");
    HDC_SERVER server = nullptr;
    hdcError_t error = DriverPlugin::instance()->MsprofDrvHdcServerCreate (logDevId, type, &server);
    if (error == DRV_ERROR_DEVICE_NOT_READY) {
        MSPROF_LOGW("[HdcServerCreate]logDevId %u HDC not ready", logDevId);
        return nullptr;
    } else if (error != DRV_ERROR_NONE) {
        MSPROF_LOGE("logDevId %u create HDC failed, error: %d", logDevId, error);
        return nullptr;
    }
    MSPROF_EVENT("logDevId %u create HDC server successfully", logDevId);
    return server;
}

void HdcServerDestroy(HDC_SERVER server)
{
    const int hdcMaxTimes = 30;
    const int hdcWaitBaseSleepTime = 100;
    hdcError_t error = DRV_ERROR_NONE;
    if (server == nullptr) {
        return;
    }
    int times = 0;
    do {
        error = DriverPlugin::instance()->MsprofDrvHdcServerDestroy(server);
        if (error != DRV_ERROR_NONE) {
            MSPROF_LOGE("[HdcServerDestroy]hdc server destroy error : %d, times %d", error, times);
            times++;
            MmpaPlugin::instance()->MsprofMmSleep(hdcWaitBaseSleepTime);
        }
    } while (times < hdcMaxTimes && error == DRV_ERROR_CLIENT_BUSY);
}

HDC_SESSION HdcServerAccept(HDC_SERVER server)
{
    MSPROF_LOGD("HdcServerAccept begin");
    HDC_SESSION session = nullptr;
    hdcError_t error = DriverPlugin::instance()->MsprofDrvHdcSessionAccept(server, &session);
    if (error != DRV_ERROR_NONE || session == nullptr) {
        MSPROF_LOGW("[HdcServerAccept]hdc accept error");
        return nullptr;
    }

    if (DriverPlugin::instance()->MsprofDrvHdcSetSessionReference(session) != DRV_ERROR_NONE) {
        MSPROF_LOGE("[HdcServerAccept]set reference error");
        (void)HdcSessionClose(session);
        return nullptr;
    }
    return session;
}

static void IoVecAddToList(struct IoVec &base, std::list<struct IoVec> &ioList)
{
    MSPROF_LOGD("iovec list add begin");
    if (base.base != nullptr && base.len > 0) {
        ioList.push_back(base);
        base.len = 0;
        base.base = nullptr;
    }
}

static int32_t IoVecListToMem(std::list<struct IoVec> &ioList, struct IoVec &base)
{
    uint32_t offset = 0;
    for (auto it = ioList.begin(); it != ioList.end();) {
        IoVec value = *(it++);
        if (base.len < value.len) {
            offset += value.len;
            continue;
        }
        uint32_t distance = base.len - value.len;
        if (offset <= distance) {
            errno_t err = memcpy_s((IdeU8Pt)base.base + offset, base.len - offset, value.base, value.len);
            if (err != EOK) {
                return IDE_DAEMON_ERROR;
            }
        }
        offset += value.len;
    }
    return IDE_DAEMON_OK;
}

static void IoVecListFree(std::list<struct IoVec> &ioList)
{
    MSPROF_LOGD("iovec list free begin");
    for (auto it = ioList.begin(); it != ioList.end();) {
        auto eit = it++;
        IDE_XFREE_AND_SET_NULL(eit->base);
        ioList.erase(eit);
    }
    MSPROF_LOGD("iovec list free end");
}

/**
 * @brief get packet to recv_buf
 * @param packet: HDC packet
 * @param recv_buf: a buffer that stores the result
 * @param buf_len: the length of recv_buf
 *
 * @return
 *      IDE_DAEMON_OK:    store data succ
 *      IDE_DAEMON_ERROR: store data failed
 */
int HdcStorePackage(const IdeHdcPacket &packet, struct IoVec &ioVec)
{
    errno_t ret = EOK;
    IdeStringBuffer newBuf = nullptr;
    IdeStringBuffer buf = reinterpret_cast<IdeStringBuffer>(ioVec.base);
    // little packgae packet type
    if (packet.type == IDE_DAEMON_LITTLE_PACKAGE) {
        if ((uint64_t)ioVec.len + packet.len > UINT32_MAX) {
            MSPROF_LOGE("buf_len : %u, packet->len: %u", ioVec.len, packet.len);
            return IDE_DAEMON_ERROR;
        }

        uint32_t len = ioVec.len + packet.len;
        // malloc new buffer for adding new data
        newBuf = reinterpret_cast<IdeStringBuffer>(IdeXrmalloc(buf, ioVec.len, len));
        if (newBuf == nullptr) {
            MSPROF_LOGE("Ide Xrmalloc Failed");
            return IDE_DAEMON_ERROR;
        }
        IDE_XFREE_AND_SET_NULL(buf);
        buf = newBuf;
        // add new package data
        ret = memcpy_s(buf + ioVec.len, packet.len, packet.value, packet.len);
        if (ret != EOK) {
            MSPROF_LOGE("memory copy failed, ret: %d", ret);
            IDE_XFREE_AND_SET_NULL(buf);
            return IDE_DAEMON_ERROR;
        }
        ioVec.base = buf;
        ioVec.len = len;
        return IDE_DAEMON_OK;
    }
    return IDE_DAEMON_ERROR;
}

static int HdcReadPackage(struct drvHdcMsg &pmsg, IdeLastPacket &isLast, int32_t recvBufCount, struct IoVec &ioVec)
{
    IdeStringBuffer pBuf = nullptr;
    int32_t pBufLen = 0;
    // Traverse the descriptor and fetch the read buf
    for (int i = 0; i < recvBufCount; i++) {
        hdcError_t error = DriverPlugin::instance()->MsprofDrvHdcGetMsgBuffer(&pmsg, i, &pBuf, &pBufLen);
        IDE_CTRL_VALUE_FAILED(error == DRV_ERROR_NONE, return IDE_DAEMON_ERROR, "Hdc Get Msg Buffer, error %d", error);
        if (pBuf != nullptr && pBufLen > 0) {
            IdeHdcPacket* packet = (struct IdeHdcPacket*)pBuf;
            // store package by packet type
            int err = HdcStorePackage(*packet, ioVec);
            IDE_CTRL_VALUE_FAILED(err == IDE_DAEMON_OK, return IDE_DAEMON_ERROR, "Hdc store package, error");
            // receive the last package is true
            if (packet->isLast == static_cast<int8_t>(IdeLastPacket::IDE_LAST_PACK)) {
                isLast = IdeLastPacket::IDE_LAST_PACK;
            }
        }
    }
    return IDE_DAEMON_OK;
}

int HdcReadIovecToMem(std::list <struct IoVec> &hdcIoList, uint32_t bufLen, IdeRecvBuffT recvBuf, IdeI32Pt recvLen)
{
    IDE_CTRL_VALUE_FAILED(recvLen != nullptr, return IDE_DAEMON_ERROR, "recvLen is nullptr");
    IDE_CTRL_VALUE_FAILED(bufLen > 0, return IDE_DAEMON_ERROR, "bufLen is nullptr");
    IDE_CTRL_VALUE_FAILED(recvBuf != nullptr, return IDE_DAEMON_ERROR, "recvBuf is nullptr");
    IdeStringBuffer buf = (IdeStringBuffer)IdeXmalloc(bufLen);
    if (buf == nullptr) {
        IoVecListFree(hdcIoList);
        return IDE_DAEMON_ERROR;
    }
    struct IoVec ioBase = {nullptr, 0};
    ioBase.base = buf;
    ioBase.len = bufLen;
    if (IoVecListToMem(hdcIoList, ioBase) == IDE_DAEMON_ERROR) {
        IoVecListFree(hdcIoList);
        IDE_XFREE_AND_SET_NULL(buf);
        return IDE_DAEMON_ERROR;
    }
    IoVecListFree(hdcIoList);
    *recvLen = bufLen;
    *recvBuf = buf;
    return IDE_DAEMON_OK;
}

static int HdcSessionRead(HDC_SESSION session, IdeRecvBuffT recvBuf, IdeI32Pt recvLen, int nbFlag)
{
    int32_t count = 1;
    int32_t recvBufCount = 0;
    uint32_t len = 0;
    IdeLastPacket isLast = IdeLastPacket::IDE_NOT_LAST_PACK;
    uint32_t bufLen = 0;
    std::list<struct IoVec> hdcIoList;
    struct IoVec ioBase = {nullptr, 0};

    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_ERROR, "session is nullptr");
    IDE_CTRL_VALUE_FAILED(recvBuf != nullptr, return IDE_DAEMON_ERROR, "recv_buf is nullptr");
    IDE_CTRL_VALUE_FAILED(recvLen != nullptr, return IDE_DAEMON_ERROR, "recv_len is nullptr");

    // request alloc hdc message, count is 1
    struct drvHdcMsg *pmsg = nullptr;
    hdcError_t hdcError = DriverPlugin::instance()->MsprofDrvHdcAllocMsg(session, &pmsg, count);
    IDE_CTRL_VALUE_FAILED((hdcError == DRV_ERROR_NONE) && (pmsg != nullptr),
        return IDE_DAEMON_ERROR, "Hdc Alloc Msg, error %d", hdcError);
    uint32_t timeout = 0;
    while (1) {
        // Receive data, since the count is 1 when applying the descriptor, read up to 1 buf at a time.
        // len no use just for parameter
        hdcError = DriverPlugin::instance()->MsprofHalHdcRecv(session, pmsg, len, nbFlag, &recvBufCount, timeout);
        if (hdcError == DRV_ERROR_NON_BLOCK) {
            IDE_FREE_HDC_MSG_AND_SET_NULL(pmsg);
            IoVecListFree(hdcIoList);
            return IDE_DAEMON_RECV_NODATA;
        }

        if (hdcError == DRV_ERROR_SOCKET_CLOSE) {
            IDE_FREE_HDC_MSG_AND_SET_NULL(pmsg);
            IoVecListFree(hdcIoList);
            MSPROF_LOGI("[HdcSessionRead]Session is closed.");
            return IDE_DAEMON_SOCK_CLOSE;
        }
        IDE_CTRL_VALUE_FAILED(hdcError == DRV_ERROR_NONE, goto ERROR_BRANCH, "Hdc Receive, error %d", hdcError);
        int err = HdcReadPackage(*pmsg, isLast, recvBufCount, ioBase);
        bufLen += ioBase.len;
        IoVecAddToList(ioBase, hdcIoList);
        IDE_CTRL_VALUE_FAILED(err == IDE_DAEMON_OK, goto ERROR_BRANCH, "[HdcSessionRead]Hdc receive package, error");
        // if isLast is true the last package is received, not receive again;
        if (isLast == IdeLastPacket::IDE_LAST_PACK) {
            break;
        }
        // reuse hdc message
        hdcError = DriverPlugin::instance()->MsprofDrvHdcReuseMsg(pmsg);
        IDE_CTRL_VALUE_FAILED(hdcError == DRV_ERROR_NONE, goto ERROR_BRANCH, "Hdc Reuse Msg, error: %d", hdcError);
    }

    // free hdc message
    hdcError = DriverPlugin::instance()->MsprofDrvHdcFreeMsg(pmsg);
    pmsg = nullptr;
    IDE_CTRL_VALUE_FAILED(hdcError == DRV_ERROR_NONE, goto ERROR_BRANCH, "Hdc Free Msg, error: %d", hdcError);
    return Analysis::Dvvp::Adx::HdcReadIovecToMem(hdcIoList, bufLen, recvBuf, recvLen);
ERROR_BRANCH:
    IDE_FREE_HDC_MSG_AND_SET_NULL(pmsg);
    IoVecListFree(hdcIoList);
    return IDE_DAEMON_ERROR;
}

/**
 * @brief get data from hdc session by block mode
 * @param session: HDC session
 * @param recv_buf: a buffer that stores the result
 * @param recv_len: the length of recv_buf
 *
 * @return
 *      IDE_DAEMON_OK:    read succ
 *      IDE_DAEMON_ERROR: read failed
 */
int HdcRead(HDC_SESSION session, IdeRecvBuffT recvBuf, IdeI32Pt recvLen)
{
    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_ERROR, "session is nullptr");
    IDE_CTRL_VALUE_FAILED(recvBuf != nullptr, return IDE_DAEMON_ERROR, "recv_buf is nullptr");
    IDE_CTRL_VALUE_FAILED(recvLen != nullptr, return IDE_DAEMON_ERROR, "recv_len is nullptr");

    return Analysis::Dvvp::Adx::HdcSessionRead(session, recvBuf, recvLen, IDE_DAEMON_BLOCK);
}

/**
 * @brief get data from hdc session by non-block mode
 * @param session: HDC session
 * @param recv_buf: a buffer that stores the result
 * @param recv_len: the length of recv_buf
 *
 * @return
 *      IDE_DAEMON_OK:    read succ
 *      IDE_DAEMON_ERROR: read failed
 */
int HdcReadNb(HDC_SESSION session, IdeRecvBuffT recvBuf, IdeI32Pt recvLen)
{
    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_ERROR, "session is nullptr");
    IDE_CTRL_VALUE_FAILED(recvBuf != nullptr, return IDE_DAEMON_ERROR, "recv_buf is nullptr");
    IDE_CTRL_VALUE_FAILED(recvLen != nullptr, return IDE_DAEMON_ERROR, "recv_len is nullptr");

    return Analysis::Dvvp::Adx::HdcSessionRead(session, recvBuf, recvLen, IDE_DAEMON_NOBLOCK);
}

/**
 * @brief send data through hdc
 * @param session: HDC session
 * @param dataSendMsg: send buffer and length
 * @param pmsg: the pointer to send data to hdc driver
 * @param packet: the pointer of send packet
 *
 * @return
 *      DRV_ERROR_NONE: send succ
 *      others:         send failed
 */
static hdcError_t HdcWritePackage(HDC_SESSION session, DataSendMsg dataSendMsg,
    DRV_HDC_MSG_T_PTR pmsg, IDE_HDC_PACKET_T_PTR packet, int flag)
{
    hdcError_t hdcError = DRV_ERROR_NONE;
    unsigned int totalLen = static_cast<uint32_t>(dataSendMsg.bufLen);
    unsigned int reservedLen = totalLen;
    unsigned int sendLen = 0;
    unsigned int timeout = 0;
    IdeSendBuffT buf = dataSendMsg.buf;

    IDE_CTRL_VALUE_FAILED(session != nullptr, return DRV_ERROR_INVALID_VALUE, "session is nullptr");
    IDE_CTRL_VALUE_FAILED(buf != nullptr, return DRV_ERROR_INVALID_VALUE, "buf is nullptr");
    IDE_CTRL_VALUE_FAILED(packet != nullptr, return DRV_ERROR_INVALID_VALUE, "packet is nullptr");
    IDE_CTRL_VALUE_FAILED(pmsg != nullptr, return DRV_ERROR_INVALID_VALUE, "pmsg is nullptr");

    do {
        if (reservedLen > dataSendMsg.maxSendLen) {
            sendLen = dataSendMsg.maxSendLen;
            packet->isLast = static_cast<int8_t>(IdeLastPacket::IDE_NOT_LAST_PACK);
        } else {
            sendLen = reservedLen;
            packet->isLast = static_cast<int8_t>(IdeLastPacket::IDE_LAST_PACK);
        }
        packet->len = sendLen;
        packet->type = IDE_DAEMON_LITTLE_PACKAGE;

        errno_t ret = memcpy_s(packet->value, dataSendMsg.maxSendLen,
                               static_cast<IdeU8Pt>(const_cast<IdeBuffT>(buf)) + (totalLen - reservedLen),
                               static_cast<int32_t>(sendLen));
        IDE_CTRL_VALUE_FAILED(ret == EOK, return DRV_ERROR_INVALID_VALUE, "memory copy failed");

        // add buffer to hdc message
        hdcError = DriverPlugin::instance()->MsprofDrvHdcAddMsgBuffer(pmsg, reinterpret_cast<IdeStringBuffer>(packet),
            sizeof(struct IdeHdcPacket) + packet->len);
        IDE_CTRL_VALUE_FAILED(hdcError == DRV_ERROR_NONE, return hdcError, "Hdc Add Msg Buffer, error: %d", hdcError);

        // send hdc message to rpc
        hdcError = DriverPlugin::instance()->MsprofHalHdcSend(session, pmsg, flag, timeout);
        IDE_CTRL_VALUE_FAILED(hdcError == DRV_ERROR_NONE, return hdcError, "Hdc Send, error: %d", hdcError);

        // reuse hdc message
        hdcError = DriverPlugin::instance()->MsprofDrvHdcReuseMsg(pmsg);
        IDE_CTRL_VALUE_FAILED(hdcError == DRV_ERROR_NONE, return hdcError, "Hdc Reuse Msg, error: %d", hdcError);
        reservedLen = reservedLen - sendLen;
    } while (reservedLen > 0 && hdcError == DRV_ERROR_NONE);

    return hdcError;
}

int32_t HdcSessionWrite(HDC_SESSION session, IdeSendBuffT buf, int32_t len, int32_t flag)
{
    hdcError_t hdcError;
    struct drvHdcMsg *pmsg = nullptr;
    int32_t count = 1;
    struct IdeHdcPacket* packet = nullptr;

    IDE_CTRL_VALUE_FAILED((session != nullptr && buf != nullptr && len > 0),
                          return IDE_DAEMON_ERROR, "Invalid Parameter");
    uint32_t capacity = 0;
    int err = HdcCapacity(&capacity);
    IDE_CTRL_VALUE_FAILED(err == IDE_DAEMON_OK, return IDE_DAEMON_ERROR, "Hdc Capacity Failed, err: %d", err);

    // request alloc hdc message, count is 1
    hdcError = DriverPlugin::instance()->MsprofDrvHdcAllocMsg(session, &pmsg, count);
    IDE_CTRL_VALUE_FAILED((hdcError == DRV_ERROR_NONE) && (pmsg != nullptr),
        return IDE_DAEMON_ERROR, "Hdc Alloc Msg, error: %d", hdcError);

    uint32_t maxDatalen = capacity - sizeof(struct IdeHdcPacket);
    packet = (struct IdeHdcPacket*)IdeXmalloc(sizeof(struct IdeHdcPacket) + maxDatalen);
    if (packet == nullptr) {
        MSPROF_LOGE("[HdcSessionWrite]IdeXmalloc %lu Size failed", sizeof(struct IdeHdcPacket) + maxDatalen);
        IDE_FREE_HDC_MSG_AND_SET_NULL(pmsg);
        return IDE_DAEMON_ERROR;
    }

    struct DataSendMsg dataSendMsg = { buf, len, maxDatalen };
    hdcError = Analysis::Dvvp::Adx::HdcWritePackage(session, dataSendMsg, pmsg, packet, flag);

    // free packet
    IDE_XFREE_AND_SET_NULL(packet);
    // free hdc message
    hdcError_t ret = DriverPlugin::instance()->MsprofDrvHdcFreeMsg(pmsg);
    IDE_CTRL_VALUE_FAILED(ret == DRV_ERROR_NONE, return IDE_DAEMON_ERROR, "Hdc Free Msg, error: %d", ret);
    pmsg = nullptr;
    return hdcError != DRV_ERROR_NONE ? IDE_DAEMON_ERROR : IDE_DAEMON_OK;
}

int32_t HdcWrite(HDC_SESSION session, IdeSendBuffT buf, int32_t len)
{
    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_ERROR, "session is nullptr");
    IDE_CTRL_VALUE_FAILED(len > 0, return IDE_DAEMON_ERROR, "len is invalid");
    IDE_CTRL_VALUE_FAILED(buf != nullptr, return IDE_DAEMON_ERROR, "buf is nullptr");

    return Analysis::Dvvp::Adx::HdcSessionWrite(session, buf, len, IDE_DAEMON_BLOCK);
}

/**
 * @brief write data by hdc session
 * @param session: HDC session
 * @param buf: a buffer that store data to send
 * @param len: the length of buffer
 *
 * @return
 *      IDE_DAEMON_OK:    write succ
 *      IDE_DAEMON_ERROR: write failed
 */
int32_t HdcWriteNb(HDC_SESSION session, IdeSendBuffT buf, int32_t len)
{
    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_ERROR, "session is nullptr");
    IDE_CTRL_VALUE_FAILED(buf != nullptr, return IDE_DAEMON_ERROR, "buf is nullptr");
    IDE_CTRL_VALUE_FAILED(len > 0, return IDE_DAEMON_ERROR, "len is invalid");

    return Analysis::Dvvp::Adx::HdcSessionWrite(session, buf, len, IDE_DAEMON_NOBLOCK);
}

/**
 * @brief connect remote hdc server
 * @param peer_node: Node number of the node where Device is located
 * @param peer_devid: Device ID of the unified number in the host
 * @param client: HDC Client handle corresponding to the newly created Session
 * @param session: created session
 *
 * @return
 *      IDE_DAEMON_OK:    connect succ
 *      IDE_DAEMON_ERROR: connect failed
 */
int32_t HdcSessionConnect(int32_t peerNode, int32_t peerDevid, HDC_CLIENT client, HDC_SESSION_PTR session)
{
    IDE_CTRL_VALUE_FAILED(peerNode >= 0, return IDE_DAEMON_ERROR, "peer_node is invalid");
    IDE_CTRL_VALUE_FAILED(peerDevid >= 0, return IDE_DAEMON_ERROR, "peer_devid is invalid");
    IDE_CTRL_VALUE_FAILED(client != nullptr, return IDE_DAEMON_ERROR, "client is nullptr");
    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_ERROR, "session is nullptr");

    // hdc connect
    hdcError_t error = DriverPlugin::instance()->MsprofDrvHdcSessionConnect(peerNode, peerDevid, client, session);
    if (error != DRV_ERROR_NONE || *session == nullptr) {
        MSPROF_LOGI("Hdc Session Connect, ret: %d", error);
        return IDE_DAEMON_ERROR;
    }

    IDE_CTRL_VALUE_FAILED(DriverPlugin::instance()->MsprofDrvHdcSetSessionReference(*session) == DRV_ERROR_NONE,
        return IDE_DAEMON_ERROR, "session reference set failed");

    MSPROF_LOGI("connect succ, peer_node: %d, peer_devid: %d", peerNode, peerDevid);
    return IDE_DAEMON_OK;
}

/**
 * @brief connect remote hal hdc server
 * @param peer_node: Node number of the node where Device is located
 * @param peer_devid: Device ID of the unified number in the host
 * @param host_pid: pid of host app process
 * @param client: HDC Client handle corresponding to the newly created Session
 * @param session: created session
 *
 * @return
 *      IDE_DAEMON_OK:    connect succ
 *      IDE_DAEMON_ERROR: connect failed
 */
int32_t HalHdcSessionConnect(int32_t peerNode, int32_t peerDevid,
    int32_t hostPid, HDC_CLIENT client, HDC_SESSION_PTR session)
{
    IDE_CTRL_VALUE_FAILED(peerNode >= 0, return IDE_DAEMON_ERROR, "peer_node is invalid");
    IDE_CTRL_VALUE_FAILED(peerDevid >= 0, return IDE_DAEMON_ERROR, "peer_devid is invalid");
    IDE_CTRL_VALUE_FAILED(hostPid >= 0, return IDE_DAEMON_ERROR, "host_pid is invalid");
    IDE_CTRL_VALUE_FAILED(client != nullptr, return IDE_DAEMON_ERROR, "client is nullptr");
    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_ERROR, "session is nullptr");

    // hdc connect
    hdcError_t error = DriverPlugin::instance()->MsprofHalHdcSessionConnectEx(peerNode, peerDevid,
        hostPid, client, session);
    if (error != DRV_ERROR_NONE || *session == nullptr) {
        MSPROF_LOGI("Hdc Session Connect, ret: %d", error);
        return IDE_DAEMON_ERROR;
    }

    IDE_CTRL_VALUE_FAILED(DriverPlugin::instance()->MsprofDrvHdcSetSessionReference(*session) == DRV_ERROR_NONE,
        return IDE_DAEMON_ERROR, "session reference set failed");

    MSPROF_LOGI("connect succ, peer_node: %d, peer_devid: %d, host_pid: %d", peerNode, peerDevid, hostPid);
    return IDE_DAEMON_OK;
}

/**
 * @brief destroy hdc_accpet session
 * @param session: the session created by hdc_accept
 *
 * @return
 *      IDE_DAEMON_OK:    close succ
 *      IDE_DAEMON_ERROR: close failed
 */
int32_t HdcSessionClose(HDC_SESSION session)
{
    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_ERROR, "session is nullptr");
    // close hdc session, use for hdc_accept session
    hdcError_t error = DriverPlugin::instance()->MsprofDrvHdcSessionClose(session);
    if (error != DRV_ERROR_NONE) {
        MSPROF_LOGE("Hdc Session Close Failed, error: %d", error);
        return IDE_DAEMON_ERROR;
    }
    return IDE_DAEMON_OK;
}

/**
 * @brief destroy hdc_connect session
 * @param session: the session created by hdc connect
 *
 * @return
 *      IDE_DAEMON_OK:    destroy succ
 *      IDE_DAEMON_ERROR: destroy failed
 */
int32_t HdcSessionDestroy(HDC_SESSION session)
{
    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_ERROR, "session is nullptr");

    return Analysis::Dvvp::Adx::HdcSessionClose(session);
}

/**
 * @brief get hdc capacity
 * @param segment: hdc max segment size
 *
 * @return
 *      IDE_DAEMON_OK:    read succ
 *      IDE_DAEMON_ERROR: read failed
 */
int32_t HdcCapacity(IdeU32Pt segment)
{
    hdcError_t error;
    struct drvHdcCapacity capacity = {HDC_CHAN_TYPE_MAX, 0};

    error = DriverPlugin::instance()->MsprofDrvHdcGetCapacity(&capacity);
    if (error != DRV_ERROR_NONE) {
        MSPROF_LOGE("Get Hdc Capacity Failed,error: %d", error);
        return IDE_DAEMON_ERROR;
    }
    uint32_t maxSegment = capacity.maxSegment;
    if (maxSegment > (uint32_t)IDE_MAX_HDC_SEGMENT || maxSegment < (uint32_t)IDE_MIN_HDC_SEGMENT) {
        MSPROF_LOGE("Get Hdc Capacity Segment Invaild: %u", capacity.maxSegment);
        return IDE_DAEMON_ERROR;
    }
    if (segment == nullptr) {
        return IDE_DAEMON_ERROR;
    }
    *segment = capacity.maxSegment;
    return IDE_DAEMON_OK;
}

int32_t IdeGetDevIdBySession(HDC_SESSION session, IdeI32Pt devId)
{
    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_ERROR, "session is nullptr");
    IDE_CTRL_VALUE_FAILED(devId != nullptr, return IDE_DAEMON_ERROR, "devId is nullptr");

#if (defined(linux) || defined(__linux__))
    hdcError_t err = DriverPlugin::instance()->MsprofHalHdcGetSessionAttr(session, HDC_SESSION_ATTR_DEV_ID, devId);
    if (err != DRV_ERROR_NONE) {
        MSPROF_LOGE("Hdc Get Session DevId Failed, err: %d", err);
        return IDE_DAEMON_ERROR;
    }
#else
    *devId = 0;
#endif

    return IDE_DAEMON_OK;
}

/**
 * @brief Create a data frame to send tlv
 * @param type: request type
 * @param dev_id: device id
 * @param value:the value to send
 * @param value_len: the length of value
 * @param buf: the data frame to send tlv
 * @param buf_len: the data length of frame
 *
 * @return
 *        IDE_DAEMON_OK: succ
 *        IDE_DAEMON_ERROR: failed
 */
int IdeCreatePacket(CmdClassT type, IdeString value,
    uint32_t valueLen, IdeRecvBuffT buf, IdeI32Pt bufLen)
{
    if (value == nullptr || buf == nullptr || bufLen == nullptr) {
        MSPROF_LOGE("[IdeCreatePacket]input invalid parameter");
        return IDE_DAEMON_ERROR;
    }
    if ((uint64_t)valueLen + sizeof(struct tlv_req) + 1 > UINT32_MAX) {
        MSPROF_LOGE("[IdeCreatePacket]value_len: %u, tlv_len: %lu", valueLen, sizeof(struct tlv_req));
        return IDE_DAEMON_ERROR;
    }
    uint32_t mallocValueLen = valueLen + 1;
    uint32_t sendLen = sizeof(struct tlv_req) + mallocValueLen;
    IdeStringBuffer sendBuf = static_cast<IdeStringBuffer>(IdeXmalloc(sendLen));
    IDE_CTRL_VALUE_FAILED(sendBuf != nullptr, return IDE_DAEMON_ERROR, "malloc memory failed");
    IdeTlvReq req = (IdeTlvReq)sendBuf;
    req->dev_id = 0;
    req->len = static_cast<int32_t>(valueLen);
    req->type = type;
    errno_t err = memcpy_s(req->value, mallocValueLen, value, valueLen);
    if (err != EOK) {
        MSPROF_LOGE("[IdeCreatePacket]memory copy failed, err: %d", err);
        IDE_XFREE_AND_SET_NULL(req);
        return IDE_DAEMON_ERROR;
    }
    *buf = static_cast<IdeMemHandle>(sendBuf);
    *bufLen = sizeof(struct tlv_req) + valueLen;
    return IDE_DAEMON_OK;
}

/**
 * @brief free packet, which create by IdeCreatePacket
 * @param buf: packet point
 *
 * @return
 */
void IdeFreePacket(IdeBuffT buf)
{
    IdeXfree(buf);
}
}   // namespace Adx
}   // namespace Dvvp
}   // namespace Analysis
