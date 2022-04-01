/**
 * @file adx_msg_proto.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "adx_msg_proto.h"
#include "securec.h"
#include "mmpa_api.h"
#include "memory_utils.h"
#include "log/adx_log.h"
namespace Adx {
static const uint32_t MAX_PROTO_FILE_BUFFER_SIZE    = 512000;    // 500kb

MsgProto *AdxMsgProto::CreateMsgPacket(CmdClassT type, uint16_t devId, IdeSendBuffT data, uint32_t length)
{
    MsgProto *msg = AdxMsgProto::CreateDataMsg(data, length);
    IDE_CTRL_VALUE_FAILED(msg != nullptr, return nullptr, "create message failed");
    msg->devId = devId;
    msg->reqType = type;
    return msg;
}

MsgProto *AdxMsgProto::CreateMsgByType(MsgType type, IdeSendBuffT data, uint32_t length)
{
    MsgProto *msg = nullptr;
    if (length > UINT32_MAX - sizeof(MsgProto)) {
        return nullptr;
    }

    if (type == MsgType::MSG_CTRL || type == MsgType::MSG_DATA) {
        uint32_t mallocLen = length + sizeof(MsgProto);
        msg = reinterpret_cast<MsgProto *>(IdeXmalloc(mallocLen));
        IDE_CTRL_VALUE_FAILED(msg != nullptr, return nullptr, "malloc memory failed");
        if (data != nullptr) { // send buffer(data) not nullptr copy data to message
            int32_t ret = memcpy_s(msg->data, length, data, length);
            if (ret != EOK) {
                IDE_LOGE("create msg mem copy failed");
                IDE_XFREE_AND_SET_NULL(msg);
                return nullptr;
            }
        }

        msg->msgType = type;
        msg->sliceLen = length;
        msg->totalLen = length;
        IDE_LOGI("adx_msg_proto CreateMsgByType length is %u", length);
        msg->headInfo = ADX_PROTO_MAGIC_VALUE;
        msg->headVer = ADX_PROTO_VERSION;
        msg->order = 0;
        return msg;
    }

    return nullptr;
}

MsgProto *AdxMsgProto::CreateDataMsg(IdeSendBuffT data, uint32_t length)
{
    return CreateMsgByType(MsgType::MSG_DATA, data, length);
}

int32_t AdxMsgProto::CreateCtrlMsg(MsgProto &proto, MsgStatus status)
{
    proto.headInfo = ADX_PROTO_MAGIC_VALUE;
    proto.headVer = ADX_PROTO_VERSION;
    proto.order = 0;
    proto.sliceLen = 0;
    proto.totalLen = 0;
    proto.msgType = MsgType::MSG_CTRL;
    proto.status = status;
    return IDE_DAEMON_OK;
}

MsgCode AdxMsgProto::SendMsgData(const CommHandle &handle, CmdClassT type, MsgStatus status,
    IdeSendBuffT data, uint32_t length)
{
    MsgProto *msg = AdxMsgProto::CreateMsgPacket(type, 0, data, length);
    IDE_CTRL_VALUE_FAILED(msg != nullptr, return IDE_DAEMON_MALLOC_ERROR, "create message failed");
    std::unique_ptr<MsgProto, decltype(&IdeXfree)> sendDataMsgPtr(msg, IdeXfree);
    sendDataMsgPtr->status = status;
    msg = nullptr;
    int32_t ret = AdxCommOptManager::Instance().Write(handle, sendDataMsgPtr.get(),
        sendDataMsgPtr->sliceLen + sizeof(MsgProto), COMM_OPT_BLOCK);
    if (ret != IDE_DAEMON_OK) {
        char errBuf[MAX_ERRSTR_LEN + 1] = {0};
        IDE_LOGE("send message failed ret(%d), please check server is alive, info : %s", ret,
                 mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN));
        return IDE_DAEMON_CHANNEL_ERROR;
    }

    return IDE_DAEMON_NONE_ERROR;
}

MsgCode AdxMsgProto::GetStringMsgData(const CommHandle &handle, std::string &value)
{
    MsgProto *req = nullptr;
    int32_t length = 0;

    int32_t ret = AdxCommOptManager::Instance().Read(handle, (IdeRecvBuffT)&req, length, COMM_OPT_NOBLOCK);
    if (ret == IDE_DAEMON_ERROR || req == nullptr || length <= 0) {
        IDE_LOGE("reveice request failed ret %d, length(%d)", ret, length);
        return IDE_DAEMON_CHANNEL_ERROR;
    }

    SharedPtr<MsgProto> msgPtr(req, IdeXfree);
    req = nullptr;
    if (msgPtr->sliceLen + sizeof(MsgProto) != (uint32_t)length) {
        IDE_LOGE("reveice request package(%u) length(%d) exception", msgPtr->sliceLen, length);
        return IDE_DAEMON_UNKNOW_ERROR;
    }

    value = std::string((IdeStringBuffer)msgPtr->data, msgPtr->sliceLen);
    return IDE_DAEMON_NONE_ERROR;
}

MsgCode AdxMsgProto::SendFile(const CommHandle &handle, CmdClassT type, uint16_t devId, int32_t fd)
{
    IDE_CTRL_VALUE_FAILED(fd >= 0, return IDE_DAEMON_INVALID_PARAM_ERROR, "create message failed");
    MsgProto *msg = AdxMsgProto::CreateMsgPacket(type, devId, nullptr, MAX_PROTO_FILE_BUFFER_SIZE);
    IDE_CTRL_VALUE_FAILED(msg != nullptr, return IDE_DAEMON_MALLOC_ERROR, "create message failed");
    std::unique_ptr<MsgProto, decltype(&IdeXfree)> sendDataMsgPtr(msg, IdeXfree);
    msg = nullptr;
    mmSsize_t fileLength = mmLseek(fd, 0L, SEEK_END);
    IDE_CTRL_VALUE_FAILED(fileLength >= 0, return IDE_DAEMON_UNKNOW_ERROR, "lseek end failed");

    mmSsize_t pos = mmLseek(fd, 0L, SEEK_SET);
    IDE_CTRL_VALUE_FAILED(pos >= 0, return IDE_DAEMON_UNKNOW_ERROR, "lseek set failed");
    mmSsize_t resLen = fileLength;
    sendDataMsgPtr->totalLen = fileLength;
    sendDataMsgPtr->offset = 0;
    while (resLen > 0) {
        mmSsize_t readLen = resLen > MAX_PROTO_FILE_BUFFER_SIZE ? MAX_PROTO_FILE_BUFFER_SIZE : resLen;
        mmSsize_t len = mmRead(fd, sendDataMsgPtr->data, readLen);
        char errBuf[MAX_ERRSTR_LEN + 1] = {0};
        IDE_CTRL_VALUE_FAILED(len >= 0, return IDE_DAEMON_UNKNOW_ERROR,
            "read file failed : info [%s]", mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN));
        if (len > 0 && len <= readLen) {
            sendDataMsgPtr->sliceLen = (uint32_t)len;
            int32_t ret = AdxCommOptManager::Instance().Write(handle, sendDataMsgPtr.get(),
            sendDataMsgPtr->sliceLen + sizeof(MsgProto), COMM_OPT_BLOCK);
            IDE_CTRL_VALUE_FAILED(ret == IDE_DAEMON_OK, return IDE_DAEMON_CHANNEL_ERROR,
                "hand shake failed ret %d, please check server is alive", ret);
        }
        sendDataMsgPtr->offset += (uint32_t)len;
        resLen -= len;
    }
    RecvResponse(handle);
    return IDE_DAEMON_NONE_ERROR;
}

MsgCode AdxMsgProto::RecvFile(const CommHandle &handle, int32_t fd)
{
    IDE_CTRL_VALUE_FAILED(fd >= 0, return IDE_DAEMON_INVALID_PARAM_ERROR, "create message failed");
    MsgProto *msg = nullptr;
    int32_t length = 0;
    while (true) {
        int32_t ret = AdxCommOptManager::Instance().Read(handle, (IdeRecvBuffT)&msg,
            length, COMM_OPT_NOBLOCK);
            IDE_CTRL_VALUE_FAILED(ret == IDE_DAEMON_OK && msg != nullptr, return IDE_DAEMON_CHANNEL_ERROR,
                "hand shake failed ret %d, read failed or timeout", ret);
        if (msg->msgType == MsgType::MSG_CTRL) { // check the message is ctrl or not
            IDE_LOGW("receive ctrl msg from device, stop receiving this file");
            IDE_XFREE_AND_SET_NULL(msg);
            return IDE_DAEMON_NONE_ERROR;
        }
        if (msg->sliceLen != 0 && msg->sliceLen <= MAX_PROTO_FILE_BUFFER_SIZE) {
            mmSsize_t len = mmWrite(fd, msg->data, msg->sliceLen);
            if (len < 0) {
                char errBuf[MAX_ERRSTR_LEN + 1] = {0};
                IDE_LOGE("write file failed : info [%s]",
                         mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN));
                IDE_XFREE_AND_SET_NULL(msg);
                return IDE_DAEMON_UNKNOW_ERROR;
            }
        }

        if (msg->totalLen == msg->sliceLen + msg->offset) {
            break;
        }

        IDE_XFREE_AND_SET_NULL(msg);
    }

    if (SendResponse(handle, msg->reqType, msg->devId, MsgStatus::MSG_STATUS_NONE_ERROR) !=
        IDE_DAEMON_NONE_ERROR) {
        IDE_LOGW("send response exception");
        IDE_XFREE_AND_SET_NULL(msg);
        return IDE_DAEMON_CHANNEL_ERROR;
    }

    IDE_XFREE_AND_SET_NULL(msg);
    return IDE_DAEMON_NONE_ERROR;
}

MsgCode AdxMsgProto::SendResponse(const CommHandle &handle, uint16_t type,
    uint16_t devId, MsgStatus status)
{
    MsgProto msg;
    (void)memset_s(&msg, sizeof(msg), 0, sizeof(msg));
    (void)CreateCtrlMsg(msg, status);
    msg.reqType = type;
    msg.devId = devId;
    int32_t ret = AdxCommOptManager::Instance().Write(handle, (IdeSendBuffT)&msg, sizeof(MsgProto), COMM_OPT_BLOCK);
    if (ret != IDE_DAEMON_OK) {
        IDE_LOGE("send respone failed ret %d, please check peer is alive", ret);
        return IDE_DAEMON_CHANNEL_ERROR;
    }

    IDE_LOGI("device(%d) cmd(%d) respone success", devId, type);
    return IDE_DAEMON_NONE_ERROR;
}

MsgCode AdxMsgProto::RecvResponse(const CommHandle &handle)
{
    MsgProto *recvBuf = nullptr;
    int32_t length = 0;
    int32_t ret = AdxCommOptManager::Instance().Read(handle, (IdeRecvBuffT)&recvBuf, length, COMM_OPT_BLOCK);
    if (ret != IDE_DAEMON_OK || recvBuf == nullptr) {
        return IDE_DAEMON_CHANNEL_ERROR;
    }

    if (recvBuf->msgType == MsgType::MSG_CTRL && recvBuf->status == MsgStatus::MSG_STATUS_NONE_ERROR) {
        IDE_XFREE_AND_SET_NULL(recvBuf);
        return IDE_DAEMON_NONE_ERROR;
    }

    if (recvBuf->msgType == MsgType::MSG_CTRL && recvBuf->status == MsgStatus::MSG_STATUS_CACHE_FULL_ERROR) {
        IDE_XFREE_AND_SET_NULL(recvBuf);
        return IDE_DAEMON_DUMP_QUEUE_FULL;
    }

    IDE_XFREE_AND_SET_NULL(recvBuf);
    return IDE_DAEMON_UNKNOW_ERROR;
}
}