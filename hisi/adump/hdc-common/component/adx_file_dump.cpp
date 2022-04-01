/**
 * @file adx_file_dump.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "adx_file_dump.h"
#include "mmpa_api.h"
#include "common/utils.h"
#include "file_utils.h"
namespace Adx {
int32_t AdxFileDump::Init()
{
    return IDE_DAEMON_OK;
}

int32_t AdxFileDump::Process(const CommHandle &handle, const SharedPtr<MsgProto> &proto)
{
    IDE_CTRL_VALUE_FAILED(proto->msgType == MsgType::MSG_DATA, return IDE_DAEMON_OK, "reveice non data message");
    AdxMsgProto::SendResponse(handle, proto->reqType, proto->devId, MsgStatus::MSG_STATUS_NONE_ERROR);
    std::string file((IdeString)proto->data);
    if (FileUtils::IsValidDirChar(file) == false || FileUtils::CheckNonCrossPath(file) == false) {
        IDE_LOGE("check file valid char failed");
        return IDE_DAEMON_ERROR;
    }

    int32_t fd = mmOpen2(file.c_str(), M_RDWR | M_CREAT | M_BINARY | M_TRUNC, M_IRUSR | M_IWRITE);
    char errBuf[MAX_ERRSTR_LEN + 1] = {0};
    IDE_CTRL_VALUE_FAILED(fd >= 0, return IDE_DAEMON_ERROR, "open file exception, info : %s",
                          mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN));

    if (AdxMsgProto::RecvFile(handle, fd) != IDE_DAEMON_NONE_ERROR) {
        mmClose(fd);
        fd = -1;
        IDE_LOGE("receive file failed, info : %s", file.c_str());
        (void)remove(file.c_str());
        return IDE_DAEMON_ERROR;
    }

    mmClose(fd);
    fd = -1;
    return IDE_DAEMON_OK;
}

int32_t AdxFileDump::UnInit()
{
    return IDE_DAEMON_OK;
}
}