/**
 * @file adx_log_hdc.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "adx_log_hdc.h"
#include "mmpa_api.h"
#include "common/utils.h"
#include "file_utils.h"
#include "log_session_manage.h"
namespace Adx {
static const std::string INSERT_MSG = "###[HDC_MSG]_DEVICE_FRAMEWORK_START_###";
static const std::string DELETE_MSG = "###[HDC_MSG]_DEVICE_FRAMEWORK_END_###";

int32_t AdxLogHdc::Init()
{
    return IDE_DAEMON_OK;
}

int32_t AdxLogHdc::Process(const CommHandle &handle, const SharedPtr<MsgProto> &proto)
{
    IDE_CTRL_VALUE_FAILED(proto->msgType == MsgType::MSG_DATA, return IDE_DAEMON_OK, "reveice non data message");
    IDE_CTRL_VALUE_FAILED(handle.session != ADX_OPT_INVALID_HANDLE,
        return IDE_DAEMON_ERROR, "handle or handle.session invalid");
    LogNotifyMsg *msg = reinterpret_cast<LogNotifyMsg *>(proto->data);
    std::string actionType((IdeString)msg->data);

    int devId = 0;
    int err = IdeGetDevIdBySession((HDC_SESSION)handle.session, &devId);
    if (err != IDE_DAEMON_OK) {
        IDE_LOGE("get device id failed, %d", err);
        return IDE_DAEMON_ERROR;
    }

    // get pid from session
    int pid = 0;
    err = IdeGetPidBySession((HDC_SESSION)handle.session, &pid);
    if (err != IDE_DAEMON_OK) {
        IDE_LOGE("get pid failed, %d", err);
        return IDE_DAEMON_ERROR;
    }

    // update session-devId-pid data struct
    if (actionType == INSERT_MSG) {
        IDE_CTRL_VALUE_FAILED(InsertSessionNode(handle.session, pid, devId) == IDE_DAEMON_OK, return IDE_DAEMON_ERROR,
        "insert log session node failed");
    } else if (actionType == DELETE_MSG) {
        SessionNode *node = GetSessionNode(pid, devId);
        IDE_CTRL_VALUE_FAILED(node != NULL, return IDE_DAEMON_ERROR, "get session node failed");
        node->timeout = msg->timeout;
        IDE_CTRL_VALUE_FAILED(DeleteSessionNode(node->session, pid, devId) == IDE_DAEMON_OK, return IDE_DAEMON_ERROR,
        "delete log session node failed");
        CommHandle closeHandle;
        closeHandle.session = handle.session;
        closeHandle.type = handle.type;
        (void)AdxCommOptManager::Instance().Close(closeHandle);
        closeHandle.session = ADX_OPT_INVALID_HANDLE;
    } else {
        IDE_LOGE("invalid session node operate type, %s", actionType.c_str());
        return IDE_DAEMON_ERROR;
    }
    return IDE_DAEMON_OK;
}

int32_t AdxLogHdc::UnInit()
{
    return IDE_DAEMON_OK;
}
}
