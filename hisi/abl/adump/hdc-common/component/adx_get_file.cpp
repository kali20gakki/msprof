/**
 * @file adx_get_file.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "adx_get_file.h"
#include "mmpa_api.h"
#include "common/utils.h"
#include "file_utils.h"
#include "string_utils.h"
#include "hdc_api.h"
namespace Adx {
static const std::string DEVICE_SLOG_PATH = "/var/log/npu/slog/";
static const std::string DEVICE_STACKCORE_PATH = "/var/log/npu/coredump/";
static const std::string DEVICE_BBOX_PATH = "/var/log/npu/hisi_logs/";
static const std::string DEVICE_MESSAGE_ORI_PATH = "/var/log/";
static const std::string DEVICE_MESSAGE_PATH = "/home/HwHiAiUser/ide_daemon/";
static constexpr int32_t MESSAGES_MAX_NUM = 99;

int32_t AdxGetFile::Init()
{
    return IDE_DAEMON_OK;
}

int32_t AdxGetFile::TransferFile(const CommHandle &handle, const std::string &filePath, int pid)
{
    std::string tmpSuffix = "." + std::to_string(pid) + ".tmp";
    std::string tmpFilePath = filePath + tmpSuffix;

    if (FileUtils::CopyFileAndRename(filePath, tmpFilePath) != IDE_DAEMON_OK) {
        IDE_LOGE("copy target file to tmp file failed, %s", filePath.c_str());
        return IDE_DAEMON_ERROR;
    }

    int32_t fd = mmOpen2(tmpFilePath.c_str(), M_RDONLY | M_BINARY, S_IREAD);
    if (fd < 0) {
#ifndef ADX_LIB_HOST_DRV // can't define errBuf if IDE_LOGW is empty
        char errBuf[MAX_ERRSTR_LEN + 1] = {0};
#endif
        IDE_LOGW("open send file failed, info : %s, open file is %s",
                 mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN), tmpFilePath.c_str());
        (void)remove(tmpFilePath.c_str());
        return IDE_DAEMON_ERROR;
    }

    // remove /var/log/npu/slog or /var/log/npu/coredump or /var/log/npu/hisi_logs
    std::string matchStr;
    if (GetPathPrefix(tmpFilePath, matchStr) != IDE_DAEMON_OK) {
        (void)remove(tmpFilePath.c_str());
        mmClose(fd);
        fd = -1;
        return IDE_DAEMON_ERROR;
    }
    // remove .tmp and head path
    std::string cutPrefixTmpFilePath = tmpFilePath.substr(matchStr.length(),
        tmpFilePath.length() - tmpSuffix.length() - matchStr.length());
    MsgCode code = AdxMsgProto::SendMsgData(handle, IDE_FILE_GETD_REQ, MsgStatus::MSG_STATUS_NONE_ERROR,
        (IdeSendBuffT)(cutPrefixTmpFilePath.c_str()), cutPrefixTmpFilePath.length() + 1);
    if (code != IDE_DAEMON_NONE_ERROR) {
        IDE_LOGW("send cutPrefixTmpFilePath msg failed, %s", cutPrefixTmpFilePath.c_str());
        (void)remove(tmpFilePath.c_str());
        mmClose(fd);
        fd = -1;
        return IDE_DAEMON_ERROR;
    }

    // send file to host id(0)
    code = AdxMsgProto::SendFile(handle, IDE_FILE_GETD_REQ, 0, fd);
    if (code != IDE_DAEMON_NONE_ERROR) {
        IDE_LOGW("send exact file failed, start to send ctrl msg to notice host");
        // send ctrl msg
        AdxMsgProto::SendResponse(handle, IDE_FILE_GETD_REQ, 0, MsgStatus::MSG_STATUS_LOAD_ERROR);
    }
    mmClose(fd);
    fd = -1;
    (void)remove(tmpFilePath.c_str());
    IDE_LOGD("remove tmp file: %s", tmpFilePath.c_str());
    return IDE_DAEMON_OK;
}

int32_t AdxGetFile::Process(const CommHandle &handle, const SharedPtr<MsgProto> &proto)
{
    IDE_CTRL_VALUE_FAILED(proto->msgType == MsgType::MSG_DATA, return IDE_DAEMON_OK, "reveice non data message");
    std::string logType((IdeString)proto->data);
    std::vector<std::string> list;
    int pid = 0;
    int err = IdeGetPidBySession((HDC_SESSION)handle.session, &pid);
    if (err != IDE_DAEMON_OK) {
        IDE_LOGE("get pid failed, %d", err);
        return IDE_DAEMON_ERROR;
    }

    if (GetFileList(logType, list) != IDE_DAEMON_OK) {
        return IDE_DAEMON_ERROR;
    }

    for (auto it : list) {
        IDE_LOGI("get file from list, %s", it.c_str());
        if (logType == "message") {
            err = PrepareMessagesFile(it);
            if (err == IDE_DAEMON_DONE) {
                continue;
            }
            if (err != IDE_DAEMON_OK) {
                IDE_LOGE("Prepare message file failed, %s", it.c_str());
                continue;
            }
            IDE_LOGI("prepare message file finished, %s", it.c_str());
        }

        err = TransferFile(handle, it, pid);
        if (err != IDE_DAEMON_OK) {
            continue;
        }

        if (logType == "message") {
            (void)remove(it.c_str());
            IDE_LOGD("remove prepared message file: %s", it.c_str());
        }
    }

    MsgCode code = AdxMsgProto::SendMsgData(handle, IDE_FILE_GETD_REQ, MsgStatus::MSG_STATUS_NONE_ERROR,
        (IdeSendBuffT)(SEND_END_MSG.c_str()), SEND_END_MSG.length() + 1);
    IDE_CTRL_VALUE_FAILED(code == IDE_DAEMON_NONE_ERROR, return IDE_DAEMON_ERROR,
            "send end msg failed");

    return IDE_DAEMON_OK;
}

int32_t AdxGetFile::UnInit()
{
    return IDE_DAEMON_OK;
}

int32_t AdxGetFile::GetFileList(const std::string &logType, std::vector<std::string> &list)
{
    if (logType.empty()) {
        IDE_LOGE("logType is empty");
        return IDE_DAEMON_ERROR;
    }
    std::string basePath;
    std::string fileNamePrefix;
    bool isRecursive = true;
    if (logType == "slog") {
        basePath = DEVICE_SLOG_PATH;
    } else if (logType == "stackcore") {
        basePath = DEVICE_STACKCORE_PATH;
    } else if (logType == "bbox") {
        basePath = DEVICE_BBOX_PATH;
    } else if (logType == "message") {
        basePath = DEVICE_MESSAGE_ORI_PATH;
        fileNamePrefix = "messages";
        isRecursive = false;
    } else {
        IDE_LOGE("Invalid log type, %s", logType.c_str());
        return IDE_DAEMON_ERROR;
    }
    if (FileUtils::GetDirFileList(basePath, list, nullptr, fileNamePrefix, isRecursive) == false) {
        IDE_LOGE("get file list failed.");
        return IDE_DAEMON_ERROR;
    }
    return IDE_DAEMON_OK;
}

int32_t AdxGetFile::GetPathPrefix(const std::string &tmpFilePath, std::string &matchStr)
{
    std::string::size_type idx = tmpFilePath.find(DEVICE_SLOG_PATH);
    if (idx != std::string::npos) {
        matchStr = DEVICE_SLOG_PATH;
        return IDE_DAEMON_OK;
    }
    idx = tmpFilePath.find(DEVICE_STACKCORE_PATH);
    if (idx != std::string::npos) {
        matchStr = DEVICE_STACKCORE_PATH;
        return IDE_DAEMON_OK;
    }
    idx = tmpFilePath.find(DEVICE_BBOX_PATH);
    if (idx != std::string::npos) {
        matchStr = DEVICE_BBOX_PATH;
        return IDE_DAEMON_OK;
    }
    idx = tmpFilePath.find(DEVICE_MESSAGE_PATH);
    if (idx != std::string::npos) {
        matchStr = DEVICE_MESSAGE_PATH;
        return IDE_DAEMON_OK;
    }
    IDE_LOGE("Invalid device file path prefix, %s", tmpFilePath.c_str());
    return IDE_DAEMON_ERROR;
}

int32_t AdxGetFile::PrepareMessagesFile(std::string &messagesFilePath)
{
    std::string::size_type idx = messagesFilePath.rfind(".");
    if (idx == std::string::npos) {
        IDE_LOGI("not aging file, check pass: %s", messagesFilePath.c_str());
    } else {
        std::string agingNumStr = messagesFilePath.substr(idx + 1);
        if (!StringUtils::IsIntDigital(agingNumStr)) {
            IDE_LOGE("agingNumStr is not a number, %s, skip", agingNumStr .c_str());
            return IDE_DAEMON_DONE;
        }

        int32_t agingNum = std::stoi(agingNumStr);
        if (agingNum > MESSAGES_MAX_NUM) {
            IDE_LOGW("exceed messages max num, %d, skip", agingNum);
            return IDE_DAEMON_DONE;
        }
    }

    std::string messagesFileName;
    IDE_CTRL_VALUE_FAILED(FileUtils::GetFileName(messagesFilePath, messagesFileName) == IDE_DAEMON_OK,
        return IDE_DAEMON_ERROR, "get file name failed");
    std::string messagesNewFilePath = DEVICE_MESSAGE_PATH + messagesFileName;
    std::string cmdCopyStr = "sudo cp -RPf " + messagesFilePath + " " + DEVICE_MESSAGE_PATH;
    std::string cmdChownStr = "sudo chown -RPf HwHiAiUser:HwHiAiUser " + messagesNewFilePath;
    IDE_LOGI("cp command : %s", cmdCopyStr.c_str());
    IDE_LOGI("chown command : %s", cmdChownStr.c_str());
    FILE *fstream = Popen(cmdCopyStr.c_str(), "r");
    IDE_CTRL_VALUE_FAILED(fstream != nullptr, return IDE_DAEMON_ERROR, "Popen copy failed");
    int ret = Pclose(fstream);
    if (ret != 0) {
        IDE_LOGW("pclose exit failed");
    }

    fstream = Popen(cmdChownStr.c_str(), "r");
    IDE_CTRL_VALUE_FAILED(fstream != nullptr, return IDE_DAEMON_ERROR, "Popen chown failed");
    ret = Pclose(fstream);
    if (ret != 0) {
        IDE_LOGW("pclose exit failed");
    }

    if (!FileUtils::IsFileExist(messagesNewFilePath)) {
        IDE_LOGE("copy message file failed, %s", messagesNewFilePath.c_str());
        return IDE_DAEMON_ERROR;
    }

    // switch /var/log/messages to /home/HwHiAiUser/ide-daemon/messages
    messagesFilePath = messagesNewFilePath;
    return IDE_DAEMON_OK;
}
}