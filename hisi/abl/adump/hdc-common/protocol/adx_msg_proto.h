/**
 * @file adx_msg_proto.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ADX_MSG_PROTO_H
#define ADX_MSG_PROTO_H
#include <cstdint>
#include "ide_tlv.h"
#include "ide_daemon_api.h"
#include "adx_msg.h"
#include "commopts/adx_comm_opt_manager.h"
namespace Adx {
using MsgCode = IdeErrorT;
class AdxMsgProto {
public:
    static MsgProto *CreateMsgPacket(CmdClassT type, uint16_t devId, IdeSendBuffT data, uint32_t length);
    static MsgProto *CreateDataMsg(IdeSendBuffT data, uint32_t length);
    static int32_t CreateCtrlMsg(MsgProto &proto, MsgStatus status);
    static MsgCode SendMsgData(const CommHandle &handle, CmdClassT type, MsgStatus status,
        IdeSendBuffT data, uint32_t length);
    static MsgCode GetStringMsgData(const CommHandle &handle, std::string &value);
    static MsgCode SendFile(const CommHandle &handle, CmdClassT type, uint16_t devId, int32_t fd);
    static MsgCode RecvFile(const CommHandle &handle, int32_t fd);
    static MsgCode HandShake(const CommHandle &handle, CmdClassT type, uint16_t devId);
    static MsgCode SendResponse(const CommHandle &handle, uint16_t type, uint16_t devId, MsgStatus status);
    static MsgCode RecvResponse(const CommHandle &handle);
private:
    static MsgProto *CreateMsgByType(MsgType type, IdeSendBuffT data, uint32_t length);
};
}
#endif // ADX_PROTOCOL_H
