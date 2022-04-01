/**
 * @file hdc_api.cpp
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
#include "mmpa_api.h"
#include "log/adx_log.h"
#include "common/memory_utils.h"
#include "common/config.h"

#define IDE_FREE_HDC_MSG_AND_SET_NULL(ptr) do {                        \
    if ((ptr) != nullptr) {                                            \
        drvHdcFreeMsg(ptr);                                            \
        ptr = nullptr;                                                 \
    }                                                                  \
} while (0)

using namespace IdeDaemon::Common::Config;

namespace Adx {
struct DataSendMsg {
    IdeSendBuffT buf;
    int bufLen;
    uint32_t maxSendLen;
};

/**
 * @brief initial HDC client
 * @param client: HDC initialization handle
 *
 * @return
 *      IDE_DAEMON_OK:    init succ
 *      IDE_DAEMON_ERROR: init failed
 */
int32_t HdcClientInit(HDC_CLIENT *client)
{
    hdcError_t error;
    int32_t flag = 0;
    IDE_CTRL_VALUE_FAILED(client != nullptr, return IDE_DAEMON_ERROR, "client is nullptr");

    // create HDC client
    error = drvHdcClientCreate (client, MAX_SESSION_NUM, HDC_SERVICE_TYPE_IDE1, flag);
    if (error != DRV_ERROR_NONE || *client == nullptr) {
        IDE_LOGE("Hdc Client Create Failed, error: %d", error);
        return IDE_DAEMON_ERROR;
    }

    return IDE_DAEMON_OK;
}

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
    error = drvHdcClientCreate (&client, MAX_SESSION_NUM, type, flag);
    if (error != DRV_ERROR_NONE || client == nullptr) {
        IDE_LOGE("Hdc Client Create Failed, error: %d", error);
        return nullptr;
    }

    return client;
}

/**
 * @brief       destroy HDC client
 * @param [in]  client: HDC handle
 *
 * @return
 *      IDE_DAEMON_OK:    destroy succ
 *      IDE_DAEMON_ERROR: destroy failed
 */
int32_t HdcClientDestroy(HDC_CLIENT client)
{
    hdcError_t error = DRV_ERROR_NONE;

    if (client != nullptr) {
        error = drvHdcClientDestroy(client);
        if (error != DRV_ERROR_NONE) {
            IDE_LOGE("Hdc Client Destroy error: %d", error);
            return IDE_DAEMON_ERROR;
        }
    }

    return IDE_DAEMON_OK;
}

HDC_SERVER HdcServerCreate(int32_t logDevId, drvHdcServiceType type)
{
    HDC_SERVER server = nullptr;
    hdcError_t error = drvHdcServerCreate (logDevId, type, &server);
    if (error == DRV_ERROR_DEVICE_NOT_READY) {
        IDE_LOGW("logDevId %d HDC not ready", logDevId);
        return nullptr;
    } else if (error != DRV_ERROR_NONE) {
        IDE_LOGE("logDevId %d create HDC failed, error: %d", logDevId, error);
        return nullptr;
    }

    IDE_EVENT("logDevId %d create HDC server successfully", logDevId);
    return server;
}

/**
 * @brief      destroy HDC server
 * @param [in] server: HDC server handle
 *
 * @return
 *      None
 */
void HdcServerDestroy(HDC_SERVER server)
{
    const int hdcMaxTimes = 30;
    const int hdcWaitBaseSleepTime = 100;
    hdcError_t error = DRV_ERROR_NONE;
    if (server != nullptr) {
        int times = 0;
        do {
            error = drvHdcServerDestroy(server);
            if (error != DRV_ERROR_NONE) {
                IDE_LOGE("hdc server destroy error : %d, times %d", error, times);
                times++;
                mmSleep(hdcWaitBaseSleepTime);
            }
        } while (times < hdcMaxTimes && error == DRV_ERROR_CLIENT_BUSY);
    }
}

HDC_SESSION HdcServerAccept(HDC_SERVER server)
{
    HDC_SESSION session = nullptr;
    hdcError_t error = drvHdcSessionAccept(server, &session);
    if (error != DRV_ERROR_NONE || session == nullptr) {
        IDE_LOGW("hdc accept error");
        return nullptr;
    }

    if (drvHdcSetSessionReference(session) != DRV_ERROR_NONE) {
        IDE_LOGE("set reference error");
        (void)HdcSessionClose(session);
        return nullptr;
    }
    return session;
}

/**
 * @brief add iovec to list
 * @param [in] base : iovec slice of receive data
 * @param [out] ioList : iovec slice list
 */
static void IoVecAddToList(struct IoVec &base, std::list<struct IoVec> &ioList)
{
    if (base.base != nullptr && base.len > 0) {
        ioList.push_back(base);
        base.base = nullptr;
        base.len = 0;
    }
}

/**
 * @brief transfer iovec list to memory
 * @param [in] ioList : iovec slice list
 * @param [out] base : save receive data of all slice
 * @return IDE_DAEMON_OK : success
 */
static int32_t IoVecListToMem(std::list<struct IoVec> &ioList, struct IoVec &base)
{
    uint32_t offset = 0;
    for (auto it = ioList.begin(); it != ioList.end();) {
        auto eit = it++;
        if (base.len >= eit->len && offset <= base.len - eit->len) {
            errno_t err = memcpy_s((IdeU8Pt)base.base + offset, base.len - offset, eit->base, eit->len);
            if (err != EOK) {
                return IDE_DAEMON_ERROR;
            }
        }
        offset += eit->len;
    }
    return IDE_DAEMON_OK;
}

/**
 * @brief free list of iovec
 * @param [in] ioList  ide daemon hdc client
 */
static void IoVecListFree(std::list<struct IoVec> &ioList)
{
    for (auto it = ioList.begin(); it != ioList.end();) {
        auto eit = it++;
        IDE_XFREE_AND_SET_NULL(eit->base);
        ioList.erase(eit);
    }
    IDE_LOGD("iovec list free");
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
    if (packet.type == IdeDaemonPackageType::IDE_DAEMON_LITTLE_PACKAGE) {
        if ((static_cast<uint64_t>(ioVec.len) + packet.len) > UINT32_MAX) {
            IDE_LOGE("buf_len : %u, packet->len: %u", ioVec.len, packet.len);
            return IDE_DAEMON_ERROR;
        }

        uint32_t len = ioVec.len + packet.len;
        // malloc new buffer for adding new data
        newBuf = reinterpret_cast<IdeStringBuffer>(IdeXrmalloc(buf, ioVec.len, len));
        if (newBuf == nullptr) {
            IDE_LOGE("Ide Xrmalloc Failed");
            return IDE_DAEMON_ERROR;
        }
        IDE_XFREE_AND_SET_NULL(buf);
        buf = newBuf;
        // add new package data
        ret = memcpy_s(buf + ioVec.len, packet.len, packet.value, packet.len);
        if (ret != EOK) {
            IDE_LOGE("memory copy failed, ret: %d", ret);
            IDE_XFREE_AND_SET_NULL(buf);
            return IDE_DAEMON_ERROR;
        }

        ioVec.len = len;
        ioVec.base = buf;
        return IDE_DAEMON_OK;
    }

    return IDE_DAEMON_ERROR;
}

static int HdcReadPackage(struct drvHdcMsg &pmsg, IdeLastPacket &isLast,
    int32_t recvBufCount, struct IoVec &ioVec)
{
    IdeStringBuffer pBuf = nullptr;
    int32_t pBufLen = 0;
    // Traverse the descriptor and fetch the read buf
    int i = 0;
    for (; i < recvBufCount; i++) {
        hdcError_t error = drvHdcGetMsgBuffer(&pmsg, i, &pBuf, &pBufLen);
        IDE_CTRL_VALUE_FAILED(error == DRV_ERROR_NONE,
            return IDE_DAEMON_ERROR, "Hdc Get Msg Buffer, error %d", error);
        if (pBuf != nullptr && pBufLen > 0) {
            struct IdeHdcPacket *packet = reinterpret_cast<struct IdeHdcPacket*>(pBuf);
            // store package by packet type
            int err = HdcStorePackage(*packet, ioVec);
            IDE_CTRL_VALUE_FAILED(err == IDE_DAEMON_OK, return IDE_DAEMON_ERROR,
                "Hdc store package, error");
            // receive the last package is true
            if (packet->isLast == IdeLastPacket::IDE_LAST_PACK) {
                isLast = IdeLastPacket::IDE_LAST_PACK;
            }
        }
    }

    return IDE_DAEMON_OK;
}

/**
 * @brief transfer iovec list to memory of hdc read data
 * @param [in] hdcIoList : iovec slice list of hdc data
 * @param [in] bufLen : length of hdc data
 * @param [out] recvBuf : memory of hdc data
 * @param [out] recvLen : length of hdc data
 * @return IDE_DAEMON_OK : success; IDE_DAEMON_ERROR:failed
 */
int HdcReadIovecToMem(std::list <struct IoVec> &hdcIoList, uint32_t bufLen, IdeRecvBuffT recvBuf, IdeI32Pt recvLen)
{
    struct IoVec ioBase = {nullptr, 0};
    IDE_CTRL_VALUE_FAILED(recvBuf != nullptr, return IDE_DAEMON_ERROR, "recvBuf is nullptr");
    IDE_CTRL_VALUE_FAILED(recvLen != nullptr, return IDE_DAEMON_ERROR, "recvLen is nullptr");
    IDE_CTRL_VALUE_FAILED(bufLen > 0, return IDE_DAEMON_ERROR, "bufLen is nullptr");
    IdeStringBuffer buf = static_cast<IdeStringBuffer>(IdeXmalloc(bufLen));
    if (buf == nullptr) {
        IoVecListFree(hdcIoList);
        return IDE_DAEMON_ERROR;
    }
    ioBase.base = buf;
    ioBase.len = bufLen;
    if (IoVecListToMem(hdcIoList, ioBase) == IDE_DAEMON_ERROR) {
        IoVecListFree(hdcIoList);
        IDE_XFREE_AND_SET_NULL(buf);
        return IDE_DAEMON_ERROR;
    }
    IoVecListFree(hdcIoList);
    *recvBuf = buf;
    *recvLen = bufLen;
    return IDE_DAEMON_OK;
}

static inline void HdcReadBuffFree(struct drvHdcMsg *pmsg, std::list<struct IoVec> &ioList)
{
    IDE_FREE_HDC_MSG_AND_SET_NULL(pmsg);
    IoVecListFree(ioList);
}

static int HdcRecvData(HDC_SESSION session, struct drvHdcMsg *pmsg, int nbFlag, int32_t *recvBufCount)
{
    uint32_t len = 0;
    uint32_t timeout = 0;
    hdcError_t error = halHdcRecv(session, pmsg, len, nbFlag, recvBufCount, timeout);
    if (error != DRV_ERROR_NONE) {
        if (error == DRV_ERROR_NON_BLOCK) {
            return IDE_DAEMON_RECV_NODATA;
        } else if (error == DRV_ERROR_SOCKET_CLOSE) {
            IDE_LOGI("Session is closed.");
            return IDE_DAEMON_SOCK_CLOSE;
        }
        IDE_LOGE("Hdc Receive, error %d", error);
        return IDE_DAEMON_ERROR;
    }
    return IDE_DAEMON_OK;
}
/**
 * @brief get data from hdc session
 * @param session: HDC session
 * @param recv_buf: a buffer that stores the result
 * @param recv_len: the length of recv_buf
 * @param nb_flag: IDE_DAEMON_HDC_BLOCK: read by block mode; IDE_DAEMON_HDC_NOBLOCK: read by non-block mode
 *
 * @return
 *      IDE_DAEMON_OK:    read succ
 *      IDE_DAEMON_ERROR: read failed
 */
static int HdcSessionRead(HDC_SESSION session, IdeRecvBuffT recvBuf, IdeI32Pt recvLen, int nbFlag)
{
    struct drvHdcMsg *pmsg = nullptr;
    int32_t count = 1;
    int32_t recvBufCount = 0;
    IdeLastPacket isLast = IdeLastPacket::IDE_NOT_LAST_PACK;
    uint32_t bufLen = 0;
    std::list<struct IoVec> hdcIoList;
    struct IoVec ioBase = {nullptr, 0};

    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_ERROR, "session is nullptr");
    IDE_CTRL_VALUE_FAILED(recvBuf != nullptr, return IDE_DAEMON_ERROR, "recv_buf is nullptr");
    IDE_CTRL_VALUE_FAILED(recvLen != nullptr, return IDE_DAEMON_ERROR, "recv_len is nullptr");

    // 1.request alloc hdc message, count is 1
    hdcError_t error = drvHdcAllocMsg(session, &pmsg, count);
    IDE_CTRL_VALUE_FAILED((error == DRV_ERROR_NONE) && (pmsg != nullptr),
        return IDE_DAEMON_ERROR, "Hdc Alloc Msg, error %d", error);
    while (1) {
        // 2.Receive data, since the count is 1 when applying the descriptor, read up to 1 buf at a time.
        // len no use just for parameter
        int err = HdcRecvData(session, pmsg, nbFlag, &recvBufCount);
        if (err != IDE_DAEMON_OK) {
            HdcReadBuffFree(pmsg, hdcIoList);
            return err;
        }

        // 3.trans read data to Iolist
        err = HdcReadPackage(*pmsg, isLast, recvBufCount, ioBase);
        bufLen += ioBase.len;
        IoVecAddToList(ioBase, hdcIoList);
        if (err != IDE_DAEMON_OK) {
            HdcReadBuffFree(pmsg, hdcIoList);
            IDE_LOGE("Hdc receive package error");
            return IDE_DAEMON_ERROR;
        }
        // if isLast is true the last package is received, not receive again;
        if (isLast == IdeLastPacket::IDE_LAST_PACK) {
            break;
        }

        // 4.reuse hdc message
        error = drvHdcReuseMsg(pmsg);
        if (error != DRV_ERROR_NONE) {
            HdcReadBuffFree(pmsg, hdcIoList);
            IDE_LOGE("Hdc Reuse Msg, error: %d", error);
            return IDE_DAEMON_ERROR;
        }
    }

    // 5.free hdc message
    error = drvHdcFreeMsg(pmsg);
    pmsg = nullptr;
    if (error != DRV_ERROR_NONE) {
        HdcReadBuffFree(pmsg, hdcIoList);
        IDE_LOGE("Hdc Free Msg, error: %d", error);
        return IDE_DAEMON_ERROR;
    }

    return HdcReadIovecToMem(hdcIoList, bufLen, recvBuf, recvLen);
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

    return HdcSessionRead(session, recvBuf, recvLen, IDE_DAEMON_BLOCK);
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

    return HdcSessionRead(session, recvBuf, recvLen, IDE_DAEMON_NOBLOCK);
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
    struct drvHdcMsg *pmsg, struct IdeHdcPacket *packet, int flag)
{
    hdcError_t hdcError = DRV_ERROR_NONE;
    uint32_t reservedLen = dataSendMsg.bufLen;
    uint32_t totalLen = reservedLen;
    uint32_t sendLen = 0;
    IdeSendBuffT buf = dataSendMsg.buf;
    unsigned int timeout = 0;

    IDE_CTRL_VALUE_FAILED(session != nullptr, return DRV_ERROR_INVALID_VALUE, "session is nullptr");
    IDE_CTRL_VALUE_FAILED(buf != nullptr, return DRV_ERROR_INVALID_VALUE, "buf is nullptr");
    IDE_CTRL_VALUE_FAILED(pmsg != nullptr, return DRV_ERROR_INVALID_VALUE, "pmsg is nullptr");
    IDE_CTRL_VALUE_FAILED(packet != nullptr, return DRV_ERROR_INVALID_VALUE, "packet is nullptr");

    do {
        // calculate package data size for this time
        // if reserved data size is bigger than the max size which should subcontract package
        if (reservedLen > dataSendMsg.maxSendLen) {
            sendLen = dataSendMsg.maxSendLen;
            packet->isLast = IdeLastPacket::IDE_NOT_LAST_PACK;
        } else {
            sendLen = reservedLen;
            packet->isLast = IdeLastPacket::IDE_LAST_PACK;
        }
        packet->type = IdeDaemonPackageType::IDE_DAEMON_LITTLE_PACKAGE;
        packet->len = sendLen;

        errno_t ret = memcpy_s(packet->value, dataSendMsg.maxSendLen,
                               static_cast<IdeU8Pt>(const_cast<IdeBuffT>(buf)) +
                               (totalLen - reservedLen), sendLen);
        IDE_CTRL_VALUE_FAILED(ret == EOK, return DRV_ERROR_INVALID_VALUE, "memory copy failed");

        // add buffer to hdc message
        hdcError = drvHdcAddMsgBuffer(pmsg, reinterpret_cast<IdeStringBuffer>(packet),
                                      sizeof(struct IdeHdcPacket) + packet->len);
        IDE_CTRL_VALUE_FAILED(hdcError == DRV_ERROR_NONE, return hdcError, "Hdc Add Msg Buffer, error: %d", hdcError);

        // send hdc message
        hdcError = halHdcSend(session, pmsg, flag, timeout);
        IDE_CTRL_VALUE_FAILED(hdcError == DRV_ERROR_NONE, return hdcError, "Hdc Send, error: %d", hdcError);

        // reuse hdc message
        hdcError = drvHdcReuseMsg(pmsg);
        IDE_CTRL_VALUE_FAILED(hdcError == DRV_ERROR_NONE, return hdcError, "Hdc Reuse Msg, error: %d", hdcError);

        reservedLen -= sendLen;
    } while (reservedLen > 0 && hdcError == DRV_ERROR_NONE);

    return hdcError;
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
int32_t HdcSessionWrite(HDC_SESSION session, IdeSendBuffT buf, int32_t len, int32_t flag)
{
    hdcError_t hdcError;
    struct drvHdcMsg *pmsg = nullptr;
    int32_t count = 1;
    uint32_t capacity = 0;
    struct IdeHdcPacket* packet = nullptr;

    IDE_CTRL_VALUE_FAILED((session != nullptr && buf != nullptr && len > 0),
                          return IDE_DAEMON_ERROR, "Invalid Parameter");

    int err = HdcCapacity(&capacity);
    IDE_CTRL_VALUE_FAILED(err == IDE_DAEMON_OK, return IDE_DAEMON_ERROR, "Hdc Capacity Failed, err: %d", err);

    // 1.request alloc hdc message, count is 1
    hdcError = drvHdcAllocMsg(session, &pmsg, count);
    IDE_CTRL_VALUE_FAILED((hdcError == DRV_ERROR_NONE) && (pmsg != nullptr),
        return IDE_DAEMON_ERROR, "Hdc Alloc Msg, error: %d", hdcError);

    uint32_t maxDatalen = capacity - sizeof(struct IdeHdcPacket);
    size_t packetLen = sizeof(struct IdeHdcPacket) + maxDatalen;
    packet = static_cast<struct IdeHdcPacket*>(IdeXmalloc(packetLen));
    if (packet == nullptr) {
        IDE_LOGE("IdeXmalloc %lu Size failed", static_cast<unsigned long>(packetLen));
        IDE_FREE_HDC_MSG_AND_SET_NULL(pmsg);
        return IDE_DAEMON_ERROR;
    }

    struct DataSendMsg dataSendMsg = { buf, len, maxDatalen };
    hdcError = HdcWritePackage(session, dataSendMsg, pmsg, packet, flag);

    // free packet
    IDE_XFREE_AND_SET_NULL(packet);

    // 6. free hdc message
    hdcError_t ret = drvHdcFreeMsg(pmsg);
    IDE_CTRL_VALUE_FAILED(ret == DRV_ERROR_NONE, return IDE_DAEMON_ERROR, "Hdc Free Msg, error: %d", ret);
    pmsg = nullptr;

    return hdcError != DRV_ERROR_NONE ? IDE_DAEMON_ERROR : IDE_DAEMON_OK;
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
int32_t HdcWrite(HDC_SESSION session, IdeSendBuffT buf, int32_t len)
{
    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_ERROR, "session is nullptr");
    IDE_CTRL_VALUE_FAILED(buf != nullptr, return IDE_DAEMON_ERROR, "buf is nullptr");
    IDE_CTRL_VALUE_FAILED(len > 0, return IDE_DAEMON_ERROR, "len is invalid");

    return HdcSessionWrite(session, buf, len, IDE_DAEMON_BLOCK);
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

    return HdcSessionWrite(session, buf, len, IDE_DAEMON_NOBLOCK);
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
int32_t HdcSessionConnect(int32_t peerNode, int32_t peerDevid, HDC_CLIENT client, HDC_SESSION *session)
{
    IDE_CTRL_VALUE_FAILED(peerNode >= 0, return IDE_DAEMON_ERROR, "peer_node is invalid");
    IDE_CTRL_VALUE_FAILED(peerDevid >= 0, return IDE_DAEMON_ERROR, "peer_devid is invalid");
    IDE_CTRL_VALUE_FAILED(client != nullptr, return IDE_DAEMON_ERROR, "client is nullptr");
    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_ERROR, "session is nullptr");

    // hdc connect
    hdcError_t error = drvHdcSessionConnect(peerNode, peerDevid, client, session);
    if (error != DRV_ERROR_NONE || *session == nullptr) {
        IDE_LOGI("Hdc Session Connect, ret: %d", error);
        return IDE_DAEMON_ERROR;
    }

    IDE_CTRL_VALUE_FAILED(drvHdcSetSessionReference(*session) == DRV_ERROR_NONE, return IDE_DAEMON_ERROR,
        "session reference set failed");

    IDE_LOGI("connect succ, peer_node: %d, peer_devid: %d", peerNode, peerDevid);
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
    int32_t hostPid, HDC_CLIENT client, HDC_SESSION *session)
{
    IDE_CTRL_VALUE_FAILED(peerNode >= 0, return IDE_DAEMON_ERROR, "peer_node is invalid");
    IDE_CTRL_VALUE_FAILED(peerDevid >= 0, return IDE_DAEMON_ERROR, "peer_devid is invalid");
    IDE_CTRL_VALUE_FAILED(hostPid >= 0, return IDE_DAEMON_ERROR, "host_pid is invalid");
    IDE_CTRL_VALUE_FAILED(client != nullptr, return IDE_DAEMON_ERROR, "client is nullptr");
    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_ERROR, "session is nullptr");

    // hdc connect
    hdcError_t error = halHdcSessionConnectEx(peerNode, peerDevid, hostPid, client, session);
    if (error != DRV_ERROR_NONE || *session == nullptr) {
        IDE_LOGI("Hdc Session Connect, ret: %d", error);
        return IDE_DAEMON_ERROR;
    }

    IDE_CTRL_VALUE_FAILED(drvHdcSetSessionReference(*session) == DRV_ERROR_NONE, return IDE_DAEMON_ERROR,
        "session reference set failed");

    IDE_LOGI("connect succ, peer_node: %d, peer_devid: %d, host_pid: %d", peerNode, peerDevid, hostPid);
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

    return HdcSessionClose(session);
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
    hdcError_t error = drvHdcSessionClose(session);
    if (error != DRV_ERROR_NONE) {
        IDE_LOGE("Hdc Session Close Failed, error: %d", error);
        return IDE_DAEMON_ERROR;
    }

    return IDE_DAEMON_OK;
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

    error = drvHdcGetCapacity(&capacity);
    if (error != DRV_ERROR_NONE) {
        IDE_LOGE("Get Hdc Capacity Failed,error: %d", error);
        return IDE_DAEMON_ERROR;
    }
    uint32_t maxSegment = capacity.maxSegment;
    if (maxSegment > IDE_MAX_HDC_SEGMENT || maxSegment < IDE_MIN_HDC_SEGMENT) {
        IDE_LOGE("Get Hdc Capacity Segment Invaild: %u", capacity.maxSegment);
        return IDE_DAEMON_ERROR;
    }

    if (segment != nullptr) {
        *segment = capacity.maxSegment;
        return IDE_DAEMON_OK;
    }

    return IDE_DAEMON_ERROR;
}

/**
 * @brief get device id by HDC session
 * @param session: hdc session handle
 *
 * @return
 *      IDE_DAEMON_OK:    get hdc session succ
 *      IDE_DAEMON_ERROR: get hdc session failed
 */
int32_t IdeGetDevIdBySession(HDC_SESSION session, IdeI32Pt devId)
{
    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_ERROR, "session is nullptr");
    IDE_CTRL_VALUE_FAILED(devId != nullptr, return IDE_DAEMON_ERROR, "devId is nullptr");

#if (OS_TYPE != WIN)
    hdcError_t err = halHdcGetSessionAttr(session, HDC_SESSION_ATTR_DEV_ID, devId);
    if (err != DRV_ERROR_NONE) {
        IDE_LOGE("Hdc Get Session DevId Failed, err: %d", err);
        return IDE_DAEMON_ERROR;
    }
#else
    *devId = 0;
#endif

    return IDE_DAEMON_OK;
}

/**
 * @brief get env info by HDC session, is docker or not
 * @param session: hdc session handle
 * @param runEnv: 1 non-docker 2 docker
 *
 * @return
 *      IDE_DAEMON_OK:    get hdc session succ
 *      IDE_DAEMON_ERROR: get hdc session failed
 */
int32_t IdeGetRunEnvBySession(HDC_SESSION session, IdeI32Pt runEnv)
{
    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_ERROR, "session is nullptr");
    IDE_CTRL_VALUE_FAILED(runEnv != nullptr, return IDE_DAEMON_ERROR, "runEnv is nullptr");

#if (OS_TYPE != WIN)
    hdcError_t err = halHdcGetSessionAttr(session, HDC_SESSION_ATTR_RUN_ENV, runEnv);
    if (err != DRV_ERROR_NONE) {
        IDE_LOGE("Hdc Get Session runEnv Failed, err: %d", err);
        return IDE_DAEMON_ERROR;
    }
#else
    *runEnv = 1;
#endif

    return IDE_DAEMON_OK;
}

/**
 * @brief get vfid by HDC session
 * @param session: hdc session handle
 *
 * @return
 *      IDE_DAEMON_OK:    get hdc session succ
 *      IDE_DAEMON_ERROR: get hdc session failed
 */
int32_t IdeGetVfIdBySession(HDC_SESSION session, IdeI32Pt vfId)
{
    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_ERROR, "session is nullptr");
    IDE_CTRL_VALUE_FAILED(vfId != nullptr, return IDE_DAEMON_ERROR, "vfId is nullptr");

#if (OS_TYPE != WIN)
    hdcError_t err = halHdcGetSessionAttr(session, HDC_SESSION_ATTR_VFID, vfId);
    if (err != DRV_ERROR_NONE) {
        IDE_LOGE("Hdc Get Session VfId Failed, err: %d", err);
        return IDE_DAEMON_ERROR;
    }
#else
    *vfId = 0;
#endif

    return IDE_DAEMON_OK;
}

/**
 * @brief get pid info by HDC session
 * @param session: hdc session handle
 * @param pid: app pid
 *
 * @return
 *      IDE_DAEMON_OK:    get hdc session succ
 *      IDE_DAEMON_ERROR: get hdc session failed
 */
int32_t IdeGetPidBySession(HDC_SESSION session, IdeI32Pt pid)
{
    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_ERROR, "session is nullptr");
    IDE_CTRL_VALUE_FAILED(pid != nullptr, return IDE_DAEMON_ERROR, "pid is nullptr");

#if (OS_TYPE != WIN)
    hdcError_t err = halHdcGetSessionAttr(session, HDC_SESSION_ATTR_PEER_CREATE_PID, pid);
    if (err != DRV_ERROR_NONE) {
        IDE_LOGE("Hdc Get Session runEnv Failed, err: %d", err);
        return IDE_DAEMON_ERROR;
    }
#else
    *pid = 0;
#endif

    return IDE_DAEMON_OK;
}
}
