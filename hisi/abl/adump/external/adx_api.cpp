/**
* @file adx_api.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "adx_api.h"
#include "mmpa_api.h"
#include "ascend_hal.h"
#include "protocol/adx_msg_proto.h"
#include "commopts/hdc_comm_opt.h"
#include "log/adx_log.h"
#include "common/file_utils.h"
#include "component/adx_get_file.h"
#include "hdc_api.h"
#include "device/adx_dsmi.h"
using namespace Adx;
static const int32_t BLOCK_RETURN_CODE = 4;
/**
 * @brief        hdc connect core dump server
 * @param  [out] client     : send file communication client handle
 * @param  [in]  devId      : send file device id
 * @return       CommHandle : client handle
 */
static CommHandle AdxHdcConnect(CommHandle &client, uint16_t devId)
{
    std::map<std::string, std::string> info;
    info[OPT_DEVICE_KEY] = std::to_string(devId);
    info[OPT_SERVICE_KEY] = std::to_string(HDC_SERVICE_TYPE_IDE_FILE_TRANS);
    CommHandle session = {OptType::COMM_HDC, ADX_OPT_INVALID_HANDLE};
    std::unique_ptr<AdxCommOpt> opt (new(std::nothrow) HdcCommOpt());
    IDE_CTRL_VALUE_FAILED(opt != nullptr, return session,
        "create hdc commopt exception");
    bool ret = AdxCommOptManager::Instance().CommOptsRegister(opt);
    IDE_CTRL_VALUE_FAILED(ret, return client, "register hdc failed");
    client = AdxCommOptManager::Instance().OpenClient(OptType::COMM_HDC, info);
    IDE_CTRL_VALUE_FAILED(client.session != ADX_OPT_INVALID_HANDLE, return client,
        "open client failed");
    session = AdxCommOptManager::Instance().Connect(client, info);
    if (session.session == ADX_OPT_INVALID_HANDLE) {
        (void)AdxCommOptManager::Instance().CloseClient(client);
    }
    return session;
}

/**
 * @brief  send file common opreate
 * @param  [in]  handle     : send file communication handle
 * @param  [in]  srcFile    : send file source file with path
 * @return IDE_DAEMON_OK(0) : send file success, IDE_DAEMON_ERROR(-1) : send file failed
 */
static int32_t AdxCommonSendFile(const CommHandle &handle, IdeString srcFile)
{
    IDE_CTRL_VALUE_FAILED(srcFile != nullptr, return IDE_DAEMON_ERROR, "source file input invalid");
    int32_t fd = mmOpen2(srcFile, M_RDONLY | M_BINARY, S_IREAD);
    char errBuf[MAX_ERRSTR_LEN + 1] = {0};
    IDE_CTRL_VALUE_FAILED(fd >= 0, return IDE_DAEMON_ERROR, "open send file failed, info : %s", \
                          mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN));
    // send file to host id(0)
    MsgCode err = AdxMsgProto::SendFile(handle, IDE_FILE_GETD_REQ, 0, fd);
    mmClose(fd);
    fd = -1;
    return err;
}

/**
 * @brief  send file common opreate
 * @param  [in]  handle     : send file communication handle
 * @param  [in]  srcFile    : send file source file with path
 * @return IDE_DAEMON_OK(0) : send file success, IDE_DAEMON_ERROR(-1) : send file failed
 */
static int32_t AdxCommonGetFile(const CommHandle &handle, const std::string &srcFile)
{
    IDE_CTRL_VALUE_FAILED(!srcFile.empty(), return IDE_DAEMON_ERROR, "source file input invalid");
    // create dir if not exist
    std::string saveDirName = FileUtils::GetFileDir(srcFile);
    if (FileUtils::IsFileExist(saveDirName) == false) {
        if (FileUtils::CreateDir(saveDirName) != IDE_DAEMON_NONE_ERROR) {
            IDE_LOGE("create dir failed path: %s", saveDirName.c_str());
            return IDE_DAEMON_ERROR;
        }
    }
    // get file
    int32_t fd = mmOpen2(srcFile.c_str(), M_RDWR | M_CREAT | M_BINARY | M_TRUNC, M_IRUSR | M_IWRITE);
    char errBuf[MAX_ERRSTR_LEN + 1] = {0};
    IDE_CTRL_VALUE_FAILED(fd >= 0, return IDE_DAEMON_ERROR, "open file exception, info : %s",
                          mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN));

    int32_t err = AdxMsgProto::RecvFile(handle, fd);
    if (err != IDE_DAEMON_NONE_ERROR && err != IDE_DAEMON_CHANNEL_ERROR) {
        mmClose(fd);
        fd = -1;
        IDE_LOGE("receive file failed, info : %s", srcFile.c_str());
        (void)remove(srcFile.c_str());
        return IDE_DAEMON_ERROR;
    }

    if (err == IDE_DAEMON_CHANNEL_ERROR) {
        IDE_LOGE("send file receive responce failed,\n" \
            "maybe multiple msnpureports executed at the same time, which is not supported,\n" \
            "please execute msnpureport only once per time later if needed");
    }

    mmClose(fd);
    fd = -1;
    return IDE_DAEMON_OK;
}

/**
 * @brief  hdc send file
 * @param  [in]  srcFile    : send file source file with path
 * @param  [in]  desFile    : send file to destination path
 * @return IDE_DAEMON_OK(0) : send file success, IDE_DAEMON_ERROR(-1) : send file failed
 */
int32_t AdxHdcSendFile(IdeString srcFile, IdeString desFile)
{
    uint16_t devId = 0;
    int32_t err = IDE_DAEMON_ERROR;
    IDE_CTRL_VALUE_FAILED(srcFile != nullptr && desFile != nullptr, return err,
        "send file input parameter invalied");
    IDE_EVENT("core dump file to %s", desFile);
    CommHandle client = {OptType::COMM_HDC, ADX_OPT_INVALID_HANDLE};
    CommHandle handle = AdxHdcConnect(client, devId);
    IDE_CTRL_VALUE_FAILED(handle.session != ADX_OPT_INVALID_HANDLE, return err,
        "send file hdc client connect failed");
    MsgCode code = AdxMsgProto::SendMsgData(handle, IDE_FILE_GETD_REQ, MsgStatus::MSG_STATUS_NONE_ERROR,
                                            desFile, strlen(desFile) + 1);
    IDE_CTRL_VALUE_FAILED(code == IDE_DAEMON_NONE_ERROR, goto DO_ERROR, "send file hand shake failed");
    code = AdxMsgProto::RecvResponse(handle);
    IDE_CTRL_VALUE_FAILED(code == IDE_DAEMON_NONE_ERROR, goto DO_ERROR, "send file shake responce failed");

    err = AdxCommonSendFile(handle, srcFile);
DO_ERROR:
    (void)AdxCommOptManager::Instance().Close(handle);
    (void)AdxCommOptManager::Instance().CloseClient(client);
    return err;
}

/**
 * @brief  get device file
 * @param  [in]  devId    : device id
 * @param  [in]  desPath    : send file to destination path
 * @param  [in]  logType    : file type, stackcore, slog, bbox, message
 * @return IDE_DAEMON_OK(0) : get file success, IDE_DAEMON_ERROR(-1) : get file failed
 */
int32_t AdxGetDeviceFile(uint16_t devId, IdeString desPath, IdeString logType)
{
    int32_t err = IDE_DAEMON_ERROR;
    IDE_CTRL_VALUE_FAILED(desPath != nullptr && logType != nullptr, return err,
        "send file input parameter invalied");
    IDE_EVENT("get device file to %s", desPath);
    CommHandle client = {OptType::COMM_HDC, ADX_OPT_INVALID_HANDLE};

    uint32_t logId = 0;
    err = AdxGetLogIdByPhyId(devId, &logId);
    IDE_CTRL_VALUE_FAILED(err == IDE_DAEMON_OK, return err,
        "get device logic id failed");

    CommHandle handle = AdxHdcConnect(client, logId);
    IDE_CTRL_VALUE_FAILED(handle.session != ADX_OPT_INVALID_HANDLE, return IDE_DAEMON_ERROR,
        "send file hdc client connect failed");

    int runEnv = 0;
    err = IdeGetRunEnvBySession((HDC_SESSION)handle.session, &runEnv);
    if (err != IDE_DAEMON_OK || (runEnv != NON_DOCKER && runEnv != VM_NON_DOCKER)) {
        return BLOCK_RETURN_CODE;
    }

    std::string basePath = desPath;
    std::string value;
    MsgCode code = AdxMsgProto::SendMsgData(handle, IDE_FILE_GETD_REQ, MsgStatus::MSG_STATUS_NONE_ERROR,
                                            logType, strlen(logType) + 1);
    IDE_CTRL_VALUE_FAILED(code == IDE_DAEMON_NONE_ERROR, goto GET_ERROR, "send file hand shake failed");
    do {
        code = AdxMsgProto::GetStringMsgData(handle, value);
        IDE_CTRL_VALUE_FAILED(code == IDE_DAEMON_NONE_ERROR, goto GET_ERROR, "get file shake responce failed");
        if (value.compare(0, SEND_END_MSG.length(), SEND_END_MSG, 0, SEND_END_MSG.length()) == 0) {
            break;
        }
        IDE_LOGD("receive file relative path and name is %s", value.c_str());
        value = basePath + OS_SPLIT_STR + value;
#if (OS_TYPE != LINUX)
        value = FileUtils::ReplaceAll(value, "/", "\\");
#endif
        err = AdxCommonGetFile(handle, value);
        IDE_CTRL_VALUE_FAILED(code == IDE_DAEMON_OK, continue, "get file process failed");
        (void)mmChmod(value.c_str(), M_IRUSR); // readonly, 400
    } while (true);

GET_ERROR:
    (void)AdxCommOptManager::Instance().Close(handle);
    (void)AdxCommOptManager::Instance().CloseClient(client);
    return IDE_DAEMON_OK;
}
