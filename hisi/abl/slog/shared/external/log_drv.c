/**
* @log_drv.c
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "log_drv.h"
#include "securec.h"
#include "api/driver_api.h"

#include "dlog_error_code.h"
#include "library_load.h"
#include "log_common_util.h"
#include "log_sys_package.h"
#include "print_log.h"

#define FREE_HDC_MSG_BUF(ptr) do {                    \
    if ((ptr) != NULL) {                                \
        (void)LogdrvHdcFreeMsg((struct drvHdcMsg *)(ptr)); \
        ptr = NULL;                                   \
    }                                                 \
} while (0)

static int g_platform = -1; // 0 is device side, 1 is host side

int DrvFunctionsInit(void)
{
    return LoadDriverDllFunctions();
}

int DrvFunctionsUninit(void)
{
    return UnloadDriverDllFunctions();
}

int DrvClientCreate(HDC_CLIENT *client, int clientType)
{
    ONE_ACT_NO_LOG(client == NULL, return -1);

    HDC_CLIENT hdcClient = NULL;

    hdcError_t drvErr = LogdrvHdcClientCreate(&hdcClient, MAX_HDC_SESSION_NUM, clientType, 0);
    ONE_ACT_WARN_LOG(drvErr != DRV_ERROR_NONE, return -1, "create HDC client failed, drvErr=%d, strerr=%s.",
                     drvErr, strerror(ToolGetErrorCode()));
    ONE_ACT_WARN_LOG(hdcClient == NULL, return -1, "HDC client is null.");

    *client = hdcClient;
    return 0;
}

int DrvClientRelease(HDC_CLIENT client)
{
    ONE_ACT_NO_LOG(client == NULL, return 0);

    const int hdcMaxTimes = 30;
    const int hdcWaitBaseSleepTime = 100;
    int times = 0;
    hdcError_t drvErr = DRV_ERROR_NONE;

    do {
        drvErr = LogdrvHdcClientDestroy(client);
        if (drvErr != DRV_ERROR_NONE) {
            SELF_LOG_WARN("hdc client release drvErr=%d, times=%d", drvErr, times);
            ToolSleep(hdcWaitBaseSleepTime);
        }
        times++;
    } while (times < hdcMaxTimes && drvErr == DRV_ERROR_CLIENT_BUSY);

    return 0;
}

int DrvSessionInit(HDC_CLIENT client, HDC_SESSION *session, int devId)
{
    ONE_ACT_WARN_LOG(client == NULL, return -1, "[in] hdc client is null.");
    ONE_ACT_WARN_LOG(session == NULL, return -1, "[out] hdc session is null.");
    ONE_ACT_WARN_LOG((devId < 0) || (devId >= MAX_DEV_NUM), return -1, "[in] device id is invalid.");

    int peerNode = 0;
    hdcError_t drvErr;
    HDC_SESSION hdcSession = NULL;

    drvErr = LogdrvHdcSessionConnect(peerNode, devId, client, &hdcSession);
    ONE_ACT_WARN_LOG(drvErr != DRV_ERROR_NONE, return -1, "create session failed, drvErr=%d, strerr=%s.",
                     drvErr, strerror(ToolGetErrorCode()));

    drvErr = LogdrvHdcSetSessionReference(hdcSession);
    if (drvErr != DRV_ERROR_NONE) {
        SELF_LOG_WARN("set session reference error, drvErr=%d.", drvErr);
        (void)DrvSessionRelease(hdcSession);
        return -1;
    }

    *session = hdcSession;
    return 0;
}

int DrvSessionRelease(HDC_SESSION session)
{
    ONE_ACT_WARN_LOG(session == NULL, return -1, "[input] session is null.");

    hdcError_t drvErr = LogdrvHdcSessionClose(session);
    ONE_ACT_WARN_LOG(drvErr != DRV_ERROR_NONE, return -1, "close session failed, drvErr=%d, strerr=%s.",
                     drvErr, strerror(ToolGetErrorCode()));

    return 0;
}

int DrvGetPlatformInfo(unsigned int *info)
{
    ONE_ACT_NO_LOG(info == NULL, return -1);

    if (g_platform == HOST_SIDE || g_platform == DEVICE_SIDE) {
        *info = g_platform;
        return 0;
    }

    unsigned int platform = INVALID_VALUE;

    hdcError_t drvErr = LogdrvGetPlatformInfo(&platform);
    ONE_ACT_ERR_LOG(drvErr != DRV_ERROR_NONE, return -1, "get platform info failed, drvErr=%d.", drvErr);
    if (platform != INVALID_VALUE) {
        ONE_ACT_WARN_LOG((platform != DEVICE_SIDE) && (platform != HOST_SIDE), return -1,
                         "platform info %u is invaild.", platform);
    }
    *info = platform;
    g_platform = platform;
    return 0;
}

int DrvDevIdGetBySession(HDC_SESSION session, int attr, int *value)
{
    ONE_ACT_NO_LOG(session == NULL || value == NULL, return -1);

    int temp = 0;

    hdcError_t drvErr = LogdrvHdcGetSessionAttr(session, attr, &temp);
    ONE_ACT_WARN_LOG(drvErr != DRV_ERROR_NONE, return -1, "get session attr failed, drvErr=%d, strerr=%s.", drvErr,
                     strerror(ToolGetErrorCode()));

    *value = temp;
    return 0;
}

/**
* @brief DrvCapacityInit: get capacity by hdc alloc
* @param [out]segment: capacity size
* @return: 0: success, -1: failed
*/
static int DrvCapacityInit(unsigned int *segment)
{
    ONE_ACT_NO_LOG(segment == NULL, return -1);

    struct drvHdcCapacity capacity = { HDC_CHAN_TYPE_MAX, 0 };

    hdcError_t drvErr = LogdrvHdcGetCapacity(&capacity);
    ONE_ACT_WARN_LOG(drvErr != DRV_ERROR_NONE, return -1, "alloc HDC capacity failed, drvErr=%d", drvErr);
    ONE_ACT_WARN_LOG((capacity.maxSegment == 0) || (capacity.maxSegment > HDC_RECV_MAX_LEN), return -1,
                     "HDC capacity invalid, size=%u.", capacity.maxSegment);

    *segment = capacity.maxSegment;
    return 0;
}

/**
* @brief DrvPackageWrite: subcontract and send msg to peer end
* @param [in]session: connection session
* @param [in]sendMsg: send msg info
* @param [in]packet: packet buffer
* @return: 0: success, -1: failed
*/
static int DrvPackageWrite(HDC_SESSION session, DataSendMsg sendMsg, DataPacket *packet)
{
    hdcError_t drvErr;
    struct drvHdcMsg *msg = NULL;
    unsigned int reservedLen = sendMsg.bufLen;

    packet->isLast = (~DATA_LAST_PACKET);
    packet->dataLen = sendMsg.maxSendLen - 1;
    packet->type = LOG_LITTLE_PACKAGE;

    // alloc one hdc message for receiving
    drvErr = LogdrvHdcAllocMsg(session, &msg, 1);
    ONE_ACT_WARN_LOG(drvErr != DRV_ERROR_NONE, return -1, "alloc msg failed, drvErr=%d.", drvErr);
    ONE_ACT_WARN_LOG(msg == NULL, return -1, "HDC msg is null.");

    do {
        // if msg is bigger than max packet size, need to subcontract and send
        if (reservedLen < sendMsg.maxSendLen) {
            packet->dataLen = reservedLen;
            packet->isLast = DATA_LAST_PACKET;
        }

        // copy data to packet
        unsigned int offset = sendMsg.bufLen - reservedLen;
        int ret = memcpy_s(packet->data, sendMsg.maxSendLen, sendMsg.buf + offset, packet->dataLen);
        ONE_ACT_WARN_LOG(ret != EOK, goto WRITE_ERROR, "memory copy failed, strerr=%s.", strerror(ToolGetErrorCode()));

        // add buffer to hdc message descriptor
        drvErr = LogdrvHdcAddMsgBuffer(msg, (char *)packet, sizeof(DataPacket) + packet->dataLen);
        ONE_ACT_WARN_LOG(drvErr != DRV_ERROR_NONE, goto WRITE_ERROR,
                         "add buffer to HDC msg failed, drvErr=%d.", drvErr);

        // send hdc message
        drvErr = LogdrvHdcSend(session, msg, 0, 0);
        ONE_ACT_WARN_LOG(drvErr != DRV_ERROR_NONE, goto WRITE_ERROR, "HDC send failed, drvErr=%d.", drvErr);

        // reuse message descriptor
        drvErr = LogdrvHdcReuseMsg(msg);
        ONE_ACT_WARN_LOG(drvErr != DRV_ERROR_NONE, goto WRITE_ERROR, "reuse HDC msg failed, drvErr=%d.", drvErr);

        reservedLen -= packet->dataLen;
    } while (reservedLen > 0 && drvErr == DRV_ERROR_NONE);

    FREE_HDC_MSG_BUF(msg);
    return 0;

WRITE_ERROR:
    FREE_HDC_MSG_BUF(msg);
    return -1;
}

int DrvBufWrite(HDC_SESSION session, const char *buf, int bufLen)
{
    ONE_ACT_NO_LOG(session == NULL || buf == NULL || bufLen == 0, return -1);

    unsigned int packetSize = 0;
    DataSendMsg sendMsg = { 0 };

    // calloc capacity
    int ret = DrvCapacityInit(&packetSize);
    ONE_ACT_NO_LOG(ret != 0, return -1);
    if (bufLen + sizeof(DataPacket) < packetSize) {
        packetSize = bufLen + sizeof(DataPacket) + 1;
    }

    DataPacket *packet = (DataPacket *)calloc(1, packetSize * sizeof(char));
    // cppcheck-suppress *
    ONE_ACT_WARN_LOG(packet == NULL, return -1, "calloc %u size failed, strerr=%s.",
                     packetSize, strerror(ToolGetErrorCode()));

    // write buffer to hdc
    sendMsg.buf = buf;
    sendMsg.bufLen = bufLen;
    sendMsg.maxSendLen = packetSize - sizeof(DataPacket);
    ret = DrvPackageWrite(session, sendMsg, packet);

    XFREE(packet);
    return ret;
}

/**
* @brief CopyBufData: add packet data to buf, buf may be not empty, need to realloc new memory and copy
* @param [in]packet: recv packet data
* @param [out]buf: recv buffer
* @param [out]bufLen: recv buffer length
* @return: 0: success, -1: failed
*/
static int CopyBufData(DataPacket *packet, char **buf, unsigned int *bufLen)
{
    int ret;
    int newLen;
    char *tempBuf;

    newLen = packet->dataLen + *bufLen;
    tempBuf = (char *)calloc(1, (newLen + 1) * sizeof(char));
    ONE_ACT_WARN_LOG(tempBuf == NULL, goto COPY_ERROR, "calloc failed, strerr=%s.", strerror(ToolGetErrorCode()));

    if (*buf != NULL && *bufLen != 0) {
        ret = memcpy_s(tempBuf, newLen, *buf, *bufLen);
        ONE_ACT_WARN_LOG(ret != 0, goto COPY_ERROR, "copy data failed, strerr=%s.", strerror(ToolGetErrorCode()));
    }

    ret = memcpy_s(tempBuf + *bufLen, newLen, packet->data, packet->dataLen);
    ONE_ACT_WARN_LOG(ret != 0, goto COPY_ERROR, "copy data failed, strerr=%s.", strerror(ToolGetErrorCode()));

    XFREE(*buf);
    *buf = tempBuf;
    *bufLen = newLen;
    return 0;

COPY_ERROR:
    XFREE(tempBuf);
    return -1;
}

/**
* @brief DrvPackageRead: subcontract and send msg to peer end
* @param [in]session: connection session
* @param [in]buf: send msg info
* @param [in]bufLen: packet buffer
* @return: 0: success, -1: failed
*/
static hdcError_t DrvPackageRead(HDC_SESSION session, char **buf, unsigned int *bufLen, int flag, unsigned int timeout)
{
    hdcError_t drvErr;
    int bufCount = 0; // receive buffer count
    short isLast = (~DATA_LAST_PACKET); // last packet flag
    struct drvHdcMsg *msg = NULL;

    // alloc hdc msg
    drvErr = LogdrvHdcAllocMsg(session, &msg, 1);
    ONE_ACT_WARN_LOG(drvErr != DRV_ERROR_NONE, return drvErr, "alloc msg failed, drvErr=%d.", drvErr);
    ONE_ACT_WARN_LOG(msg == NULL, return drvErr, "HDC msg is null.");

    while (isLast != DATA_LAST_PACKET) {
        // receive data from hdc
        drvErr = LogdrvHdcRecv(session, (struct drvHdcMsg *)msg, HDC_RECV_MAX_LEN, flag, &bufCount, timeout);
        ONE_ACT_NO_LOG(drvErr != DRV_ERROR_NONE, goto READ_ERROR);

        // parse hdc recv msg to buffer
        char *pBuf = NULL; // get buffer data from msg
        int pBufLen = 0; // get buffer data length from msg
        drvErr = LogdrvHdcGetMsgBuffer(msg, 0, &pBuf, &pBufLen);
        ONE_ACT_WARN_LOG(drvErr != DRV_ERROR_NONE || pBuf == NULL, goto READ_ERROR,
                         "get HDC msg buffer failed, drvErr=%d.", drvErr);

        DataPacket *packet = (DataPacket *)pBuf;
        if (packet->isLast == DATA_LAST_PACKET) {
            isLast = DATA_LAST_PACKET;
        }
        // save recv data to buf
        ONE_ACT_NO_LOG(CopyBufData(packet, buf, bufLen) != 0, goto READ_ERROR);

        // reuse message descriptor
        drvErr = LogdrvHdcReuseMsg(msg);
        ONE_ACT_WARN_LOG(drvErr != DRV_ERROR_NONE, goto READ_ERROR, "reuse HDC msg failed, drvErr=%d.", drvErr);
    }
    FREE_HDC_MSG_BUF(msg);
    return DRV_ERROR_NONE;

READ_ERROR:
    FREE_HDC_MSG_BUF(msg);
    XFREE(*buf);
    return drvErr;
}

int DrvBufRead(HDC_SESSION session, int devId, char **buf, unsigned int *bufLen, unsigned int timeout)
{
    ONE_ACT_NO_LOG(session == NULL || buf == NULL || bufLen == NULL, return -1);
    ONE_ACT_NO_LOG(devId < 0 || devId >= MAX_DEV_NUM, return -1);

    // read by HDC
    hdcError_t drvErr = DrvPackageRead(session, buf, bufLen, HDC_FLAG_WAIT_TIMEOUT, timeout);
    if (drvErr == DRV_ERROR_SOCKET_CLOSE) {
        SELF_LOG_WARN("HDC session is closed, devId=%d.", devId);
        return -1;
    } else if (drvErr == DRV_ERROR_NON_BLOCK) {
        SELF_LOG_WARN("HDC no data, devId=%d.", devId);
        return -1;
    } else if (drvErr == DRV_ERROR_WAIT_TIMEOUT) {
        SELF_LOG_WARN("HDC recv timeout, timeout=%d.", timeout);
        return -1;
    } else if (drvErr != DRV_ERROR_NONE) {
        SELF_LOG_WARN("HDC recv failed, ret=%d, devId=%d.", drvErr, devId);
        return -1;
    }

    return 0;
}
