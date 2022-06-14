/**
 * @file hdc_api.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef IDE_HDC_API_H
#define IDE_HDC_API_H
#include "ascend_hal.h"
#include "extra_config.h"
#include "adx_log.h"

enum cmd_class {
    IDE_EXEC_COMMAND_REQ = 0, /**< 执行device命令请求\n */
    IDE_SEND_FILE_REQ,        /**< 发送文件到device命令请求\n */
    IDE_DEBUG_REQ,            /**< Debug命令请求\n */
    IDE_BBOX_REQ,             /**< Bbox命令请求\n */
    IDE_LOG_REQ,              /**< Log命令请求\n */
    IDE_PROFILING_REQ,        /**< Profiling命令请求\n */
    IDE_OME_DUMP_REQ,         /**< Ome dump命令请求\n */
    IDE_FILE_SYNC_REQ,        /**< 发送文件到AiHost 命令请求\n */
    IDE_EXEC_API_REQ,         /**< 执行AiHost Api命令请求\n */
    IDE_EXEC_HOSTCMD_REQ,     /**< 执行AiHost 命令命令请求\n */
    IDE_DETECT_REQ,           /**< 执行AiHost 通路命令请求\n */
    IDE_FILE_GET_REQ,         /**< 获取AiHost侧文件命令请求\n */
    IDE_NV_REQ,               /**< 执行AiHost Nv命令请求\n */
    IDE_DUMP_REQ,             /**< Dump命令请求\n */
    IDE_FILE_GETD_REQ,        /**< 获取Device侧文件命令请求\n */
    IDE_INVALID_REQ,          /**< 无效命令请求\n */
    NR_IDE_CMD_CLASS,         /**< 标识命令请求最大值\n */
};
typedef enum cmd_class CmdClassT;

struct tlv_req {
    enum cmd_class type; /**< command type */
    int dev_id;          /**< device id */
    int len;             /**< data len */
    char value[0];       /**< data */
};

typedef struct tlv_req TlvReqT;
typedef TlvReqT* IdeTlvReq;
typedef const TlvReqT* IdeTlvConReq;
typedef IdeTlvReq* IdeTlvReqAddr;

namespace Analysis {
namespace Dvvp {
namespace Adx {
using IdeTlvReq = TlvReqT*;
using IdeTlvConReq = const TlvReqT*;
using IdeTlvReqAddr = IdeTlvReq*;

using DRV_HDC_MSG_T_PTR = struct drvHdcMsg *;
using IDE_HDC_PACKET_T_PTR = struct IdeHdcPacket *;
using HDC_CLIENT_PTR = HDC_CLIENT *;
using HDC_SESSION_PTR = HDC_SESSION *;

enum class IdeLastPacket {
    IDE_NOT_LAST_PACK = 0,
    IDE_LAST_PACK = 1
};
enum IdeDaemonPackageType {
    IDE_DAEMON_LITTLE_PACKAGE = 0xB0,
    IDE_DAEMON_BIG_PACKAGE
};

int HdcClientDestroy(HDC_CLIENT client);
int HdcClientInit(HDC_CLIENT_PTR client);

struct IdeHdcPacket {
    uint32_t len;
    enum IdeDaemonPackageType type;     // package type : big package,little package
    int8_t isLast;                 // only 0:is not last package; 1:last package
    char value[0];
};
struct IoVec {
    IdeBuffT base;
    uint32_t len;
};

HDC_CLIENT HdcClientCreate(enum drvHdcServiceType type);
int32_t HdcClientDestroy(HDC_CLIENT client);
HDC_SERVER HdcServerCreate(int32_t logDevId, enum drvHdcServiceType type);
void HdcServerDestroy(HDC_SERVER server);
HDC_SESSION HdcServerAccept(HDC_SERVER server);

int32_t HdcRead(HDC_SESSION session, IdeRecvBuffT buf, IdeI32Pt recvLen);
int32_t HdcReadNb(HDC_SESSION session, IdeRecvBuffT buf, IdeI32Pt recvLen);
int32_t HdcWrite(HDC_SESSION session, IdeSendBuffT buf, int32_t len);
int32_t HdcWriteNb(HDC_SESSION session, IdeSendBuffT buf, int32_t len);
int32_t HdcSessionConnect(int32_t peerNode, int32_t peerDevId,
    HDC_CLIENT client, HDC_SESSION_PTR session);
int32_t HalHdcSessionConnect(int32_t peerNode, int32_t peerDevId,
    int32_t hostPid, HDC_CLIENT client, HDC_SESSION_PTR session);
int32_t HdcSessionDestroy(HDC_SESSION session);
int32_t HdcSessionClose(HDC_SESSION session);
int HdcGetDeviceBasePath(int32_t peerNode, int32_t peerDevid, IdeStringBuffer path, uint32_t pathLength);
int32_t HdcCapacity(IdeU32Pt segment);

/* ide-cmd与host-daemon通信使用的接口 */
int32_t IdeSockWriteData(IdeSession sockDesc, IdeSendBuffT buf, int32_t len);
int32_t IdeSockReadData(IdeSession sockDesc, IdeRecvBuffT readBuf, IdeI32Pt recvLen);
IdeSession IdeSockDupCreate(IdeSession sock);
void IdeSockDupDestroy(IdeSession sock);
void IdeSockDestroy(IdeSession sock);
HDC_CLIENT GetIdeDaemonHdcClient();
int32_t IdeCreatePacket(CmdClassT type, IdeString value, uint32_t valueLen,
    IdeRecvBuffT buf, IdeI32Pt bufLen);
void IdeFreePacket(IdeBuffT buf);
int32_t IdeGetDevIdBySession(HDC_SESSION session, IdeI32Pt devId);
void IdeDeviceStartupRegister(int (*devStartupNotifier)(uint32_t num, IdeU32Pt dev));
int32_t HdcSessionWrite(HDC_SESSION session, IdeSendBuffT buf, int32_t len, int32_t flag);
int HdcStorePackage(const IdeHdcPacket &packet, struct IoVec &ioVec);
}   // namespace Adx
}   // namespace Dvvp
}   // namespace Analysis

#endif
