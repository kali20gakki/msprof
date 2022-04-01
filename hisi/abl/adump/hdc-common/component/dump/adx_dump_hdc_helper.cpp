/**
 * @file adx_dump_hdc_helper.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "adx_dump_hdc_helper.h"
#include "ascend_hal.h"
#include "protocol/adx_msg_proto.h"
#include "adx_dump_record.h"
#include "log/adx_log.h"
#include "common_utils.h"
#include "memory_utils.h"
#include "hdc_api.h"
#include "string_utils.h"
namespace Adx {
AdxDumpHdcHelper::AdxDumpHdcHelper() : init_(false)
{
}

AdxDumpHdcHelper::~AdxDumpHdcHelper()
{
    UnInit();
}

bool AdxDumpHdcHelper::Init()
{
    if (!init_) {
        std::unique_ptr<AdxCommOpt> opt (new(std::nothrow) HdcCommOpt());
        IDE_CTRL_VALUE_FAILED(opt != nullptr, return false, "create hdc commopt exception");
        bool ret = AdxCommOptManager::Instance().CommOptsRegister(opt);
        IDE_CTRL_VALUE_FAILED(ret, return false, "register hdc failed");
        std::map<std::string, std::string> info;
        info[OPT_SERVICE_KEY] = std::to_string(HDC_SERVICE_TYPE_DUMP);
        client_ = AdxCommOptManager::Instance().OpenClient(OptType::COMM_HDC, info);
        IDE_CTRL_VALUE_FAILED(client_.session != ADX_OPT_INVALID_HANDLE, return false,
            "open client failed");
        IDE_LOGI("dump data client init end<%x, %d>", client_.session, client_.type);
        init_ = true;
    }
    return true;
}

void AdxDumpHdcHelper::UnInit()
{
    if (init_) {
        IDE_LOGI("dump data client close <%x, %d>", client_.session, client_.type);
        HdcClientDestroy((HDC_CLIENT)client_.session);
        init_ = false;
    }
}

/**
 * @brief      parse connect info
 * @param [in] privInfo: string of connect info
 * @param [in] proto: struct of conect info
 *
 * @return
 *      IDE_DAEMON_NONE_ERROR: parse connect info success
 *      IDE_DAEMON_INVALID_PARAM_ERROR: parse connect info failed
 */
IdeErrorT AdxDumpHdcHelper::ParseConnectInfo(const std::string &privInfo, std::map<std::string, std::string> &proto)
{
    std::string hostId;
    std::string hostPid;
    bool ret = StringUtils::ParsePrivInfo(privInfo, hostId, hostPid);
    IDE_CTRL_VALUE_FAILED(ret == true, return IDE_DAEMON_INVALID_PARAM_ERROR, "ParseConnectInfo failed");
    proto[OPT_DEVICE_KEY] = hostId;
    proto[OPT_PID_KEY] =  hostPid;
    return IDE_DAEMON_NONE_ERROR;
}

IdeErrorT AdxDumpHdcHelper::HandShake(const std::string &info, IDE_SESSION &session)
{
    uint16_t devId = 0;
    std::map<std::string, std::string> proto;
    IdeErrorT err = ParseConnectInfo(info, proto);
    IDE_CTRL_VALUE_FAILED(err == IDE_DAEMON_NONE_ERROR, return err, "parse connect info failed");
    IDE_LOGI("proto: hostId: %s, hostPid: %s", proto[OPT_DEVICE_KEY].c_str(), proto[OPT_PID_KEY].c_str());
    CommHandle handle = AdxCommOptManager::Instance().Connect(client_, proto);
    IDE_CTRL_VALUE_FAILED(handle.session != ADX_OPT_INVALID_HANDLE,
        return IDE_DAEMON_INVALID_PARAM_ERROR, "dump data connect failed");
    err = AdxMsgProto::SendResponse(handle, IDE_DUMP_REQ, devId, MsgStatus::MSG_STATUS_HAND_SHAKE);
    IDE_CTRL_VALUE_FAILED(err == IDE_DAEMON_NONE_ERROR, return err, "dump data hand shake failed");

    err = AdxMsgProto::RecvResponse(handle);
    IDE_CTRL_VALUE_FAILED(err == IDE_DAEMON_NONE_ERROR, return err, "dump date shake responce failed");

    session = (HDC_SESSION)handle.session;
    IDE_LOGD("handshake success");
    return IDE_DAEMON_NONE_ERROR;
}

IdeErrorT AdxDumpHdcHelper::DataProcess(const IDE_SESSION &session, const IdeDumpChunk &dumpChunk)
{
    int err;
    uint32_t dataLen = 0;
    // malloc memory for save user data
    IDE_RETURN_IF_CHECK_ASSIGN_32U_ADD(sizeof(DumpChunk),
        dumpChunk.bufLen, dataLen, return IDE_DAEMON_INTERGER_REVERSED_ERROR);
    MsgProto *msg = AdxMsgProto::CreateMsgPacket(IDE_DUMP_REQ, 0, nullptr, dataLen);
    IDE_CTRL_VALUE_FAILED(msg != nullptr, return IDE_DAEMON_MALLOC_ERROR, "create message failed");
    std::unique_ptr<MsgProto, decltype(&IdeXfree)> sendDataMsgPtr(msg, IdeXfree);
    msg = nullptr;
    DumpChunk* data =  reinterpret_cast<DumpChunk*>(sendDataMsgPtr->data);
    // check file name length, not longger than IDE_MAX_FILE_PATH
    IDE_CTRL_VALUE_FAILED(strlen(dumpChunk.fileName) < IDE_MAX_FILE_PATH, return IDE_DAEMON_INVALID_PATH_ERROR,
        "fileName is too long, not longger than %d length", IDE_MAX_FILE_PATH);
    err = strcpy_s(data->fileName, MAX_FILE_PATH_LENGTH, dumpChunk.fileName);
    IDE_CTRL_VALUE_FAILED(err == EOK, return IDE_DAEMON_INVALID_PATH_ERROR, "copy file name failed");
    data->bufLen = dumpChunk.bufLen;
    data->flag = dumpChunk.flag;
    data->isLastChunk = dumpChunk.isLastChunk;
    data->offset = dumpChunk.offset;
    IDE_LOGI("dataLen: %u, bufLen: %u, flag: %d, isLastChunk: %u, offset: %lld, fileName: %s",
        dataLen, data->bufLen, (int32_t)data->flag, data->isLastChunk, data->offset, data->fileName);
    err = memcpy_s(data->dataBuf, data->bufLen, dumpChunk.dataBuf, dumpChunk.bufLen);
    IDE_CTRL_VALUE_FAILED(err == EOK, return IDE_DAEMON_UNKNOW_ERROR, "memcpy_s data buffer failed");
    CommHandle handle;
    handle.type = client_.type;
    handle.session = (OptHandle)session;
    uint32_t ret = AdxCommOptManager::Instance().Write(handle, sendDataMsgPtr.get(),
        sendDataMsgPtr->sliceLen + sizeof(MsgProto), COMM_OPT_BLOCK);
    IDE_CTRL_VALUE_FAILED(ret == IDE_DAEMON_OK, return IDE_DAEMON_WRITE_ERROR, "write dump file error");
    IdeErrorT code = AdxMsgProto::RecvResponse(handle);
    IDE_CTRL_VALUE_FAILED(code == IDE_DAEMON_NONE_ERROR ||
        code == IDE_DAEMON_DUMP_QUEUE_FULL, return code, "send file shake responce failed");
    IDE_LOGD("dump data process normal");
    return code;
}

IdeErrorT AdxDumpHdcHelper::Finish(IDE_SESSION &session)
{
    CommHandle handle;
    handle.type = client_.type;
    handle.session = (OptHandle)session;
    IdeErrorT err = AdxMsgProto::SendResponse(handle, IDE_DUMP_REQ, 0, MsgStatus::MSG_STATUS_DATA_END);
    IDE_CTRL_VALUE_FAILED(err == IDE_DAEMON_NONE_ERROR, return err, "dump data hand shake failed");
    AdxCommOptManager::Instance().Close(handle);
    session = nullptr;
    return IDE_DAEMON_NONE_ERROR;
}

/**
 * @brief dump start api,create a HDC session for dump
 * @param [in] privInfo: remote connect info
 *
 * @return
 *      not NULL: Handle used by hdc
 *      NULL:     dump start failed
 */
IDE_SESSION HdcDumpStart(IdeString privInfo)
{
    IDE_CTRL_VALUE_FAILED(privInfo != nullptr && strlen(privInfo) != 0,
        return nullptr, "dump start msg is empty");
    if (!Adx::AdxDumpHdcHelper::Instance().Init()) {
        return nullptr;
    }
    IDE_SESSION session = nullptr;
    std::string privInfoStr = std::string(privInfo);
    Adx::AdxDumpHdcHelper::Instance().HandShake(privInfoStr, session);
    return session;
}

/**
 * @brief dump data to remote server
 * @param [in] session: HDC session to dump data
 * @param [in] dumpChunk: Dump information
 * @return
 *      IDE_DAEMON_INVALID_PARAM_ERROR: invalid parameter
 *      IDE_DAEMON_UNKNOW_ERROR: write data failed
 *      IDE_DAEMON_NONE_ERROR:   write data succ
 */
IdeErrorT HdcDumpData(IDE_SESSION session, const IdeDumpChunk *dumpChunk)
{
    IdeErrorT ret;
    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_INVALID_PARAM_ERROR, "session is nullptr");
    IDE_CTRL_VALUE_FAILED(dumpChunk != nullptr, return IDE_DAEMON_INVALID_PARAM_ERROR, "IdeDumpChunk is nullptr");
    const uint32_t waitInsertDumpQueueTime = 100;
    int32_t retryInsertDumpQueueTimes = 3000; // 5min(3000 * 100ms)
    IDE_LOGD("dump data process entry");
    do {
        ret = Adx::AdxDumpHdcHelper::Instance().DataProcess(session, *dumpChunk);
        if (ret == IDE_DAEMON_DUMP_QUEUE_FULL) {
            (void)mmSleep(waitInsertDumpQueueTime);
            retryInsertDumpQueueTimes--;
        }
    } while (ret == IDE_DAEMON_DUMP_QUEUE_FULL && retryInsertDumpQueueTimes > 0);
    IDE_LOGD("dump data process exit");
    return ret;
}

/**
 * @brief send dump end msg
 * @param [in] session: HDC session to dump data
 * @return
 *      IDE_DAEMON_UNKNOW_ERROR: send dump end msg failed
 *      IDE_DAEMON_NONE_ERROR:   send dump end msg success
 */
IdeErrorT HdcDumpEnd(IDE_SESSION session)
{
    IDE_CTRL_VALUE_FAILED(session != nullptr, return IDE_DAEMON_INVALID_PARAM_ERROR, "session is nullptr");
    IDE_LOGI("dump data finish");
    return Adx::AdxDumpHdcHelper::Instance().Finish(session);
}
}