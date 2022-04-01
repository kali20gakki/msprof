/**
* @file adx_dump_record.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "adx_dump_record.h"
#include "mmpa_api.h"
#include "adx_log.h"
#include "file_utils.h"
#include "string_utils.h"
#include "memory_utils.h"
#include "dump/adx_dump_process.h"
namespace Adx {
static const int32_t RECORD_DUMP_QUEUE_DEFAULT_SIZE = 60;
static const int32_t WAIT_RECORD_FILE_FINISH_TIME   = 500;
static const std::size_t MAX_IP_LENGTH              = 16;

AdxDumpRecord::AdxDumpRecord()
    : dumpRecordFlag_(true),
      hostDumpDataInfoQueue_(RECORD_DUMP_QUEUE_DEFAULT_SIZE),
      dumpInitNum_(0)
{
}

AdxDumpRecord::~AdxDumpRecord()
{
    UnInit();
}

int32_t AdxDumpRecord::GetDumpInitNum() const
{
    return dumpInitNum_;
}

void AdxDumpRecord::UpdateDumpInitNum(bool isPlus)
{
    if (isPlus) {
        dumpInitNum_++;
    } else {
        dumpInitNum_--;
    }
}

/**
 * @brief initialize record file
 * @param [in] recordPath : record file Path
 * @return
 *      IDE_DAEMON_ERROR : falied
 *      IDE_DAEMON_OK : success
 */
int AdxDumpRecord::Init(const std::string &hostPid)
{
    // non-soc case
    if (hostPid.empty()) {
        char dumpPath[MAX_FILE_PATH_LENGTH] = {0};
        if (mmGetCwd(dumpPath, sizeof(dumpPath)) != EN_OK) {
            IDE_LOGE("get current dir failed ");
            return IDE_DAEMON_ERROR;
        }
        dumpPath_ = dumpPath;
    } else {
#if (OS_TYPE == LINUX)
        // soc case
        std::string appBin = "/proc/" + hostPid + "/exe";
        uint32_t pathSize = MMPA_MAX_PATH + 1;
        IdeStringBuffer curPath = reinterpret_cast<IdeStringBuffer>(IdeXmalloc(pathSize));
        IDE_CTRL_VALUE_FAILED(curPath != nullptr, return IDE_DAEMON_ERROR, "malloc failed");
        int len = readlink(appBin.c_str(), curPath, MMPA_MAX_PATH); // read self path of store
        if (len < 0 || len > MMPA_MAX_PATH) {
            IDE_LOGE("Can't Get app bin directory");
            IDE_XFREE_AND_SET_NULL(curPath);
            return IDE_DAEMON_ERROR;
        } else {
            IDE_LOGI("get app bin path: %s", curPath);
        }
        curPath[len] = '\0'; // add string end char
        dumpPath_ = curPath;
        std::string::size_type idx = dumpPath_.find_last_of(OS_SPLIT_STR);
        dumpPath_ = dumpPath_.substr(0, idx);
        IDE_XFREE_AND_SET_NULL(curPath);
#endif
    }
    IDE_LOGI("dumpPath prefix is %s", dumpPath_.c_str());
    if (!dumpPath_.empty() && dumpPath_.back() != OS_SPLIT_CHAR) {
        dumpPath_ += OS_SPLIT_CHAR;
    }
    IDE_LOGI("record remote dump temp path : %s", dumpPath_.c_str());
    dumpRecordFlag_ = true;
    return IDE_DAEMON_OK;
}

/**
 * @brief initialize record file
 * @param [in] recordPath : record file Path
 * @return
 *      IDE_DAEMON_ERROR : falied
 *      IDE_DAEMON_OK : success
 */
int32_t AdxDumpRecord::UnInit()
{
    IDE_LOGI("start to dump uninit");
    dumpRecordFlag_ = false;
#if !defined(__IDE_UT)
    while (!DumpDataQueueIsEmpty()) {
        mmSleep(WAIT_RECORD_FILE_FINISH_TIME);
    }
#if !defined(ANDROID)
    this->hostDumpDataInfoQueue_.Quit();
#endif
#endif
    IDE_LOGI("dump uninit success");
    return IDE_DAEMON_OK;
}

/**
 * @brief record dump data to disk
 * @param [in] dumpChunk : dump chunk
 * @return
 *      true : record dump data to disk success
 *      false : record dump data to disk failed
 */
bool AdxDumpRecord::RecordDumpDataToDisk(const DumpChunk &dumpChunk) const
{
    // dump file aging
    std::string filePath = dumpChunk.fileName;
    if (filePath.empty()) {
        IDE_LOGE("filepath of received dump chunk is empty");
        return false;
    }
    if (JudgeRemoteFalg(filePath)) {
        auto pos = filePath.find_first_of(":");
        filePath = filePath.substr(pos + 1);
        filePath = dumpPath_ + filePath;
    } else {
        if (!FileUtils::IsAbsolutePath(filePath)) {
            filePath = dumpPath_ + filePath;
        }
    }

#if (OS_TYPE != LINUX)
    filePath = FileUtils::ReplaceAll(filePath, "/", "\\");
#endif

    IDE_LOGI("start to record dump data to disk path: %s", filePath.c_str());

    std::string saveDirName = FileUtils::GetFileDir(filePath);
    if (!FileUtils::IsFileExist(saveDirName)) {
        if (FileUtils::CreateDir(saveDirName) != IDE_DAEMON_NONE_ERROR) {
            IDE_LOGE("create dir failed path: %s", filePath.c_str());
            return false;
        }
    }

    while (FileUtils::IsDiskFull(saveDirName, dumpChunk.bufLen)) {
        IDE_LOGE("don't have enough free disk %d", dumpChunk.bufLen);
        return false;
    }

    std::string realPath;
    if (FileUtils::FileNameIsReal(filePath, realPath) != IDE_DAEMON_OK) {
        IDE_LOGE("real path: %s", filePath.c_str());
        return false;
    }

    IdeErrorT err = FileUtils::WriteFile(realPath.c_str(),
        dumpChunk.dataBuf, dumpChunk.bufLen, dumpChunk.offset);
    if (err != IDE_DAEMON_NONE_ERROR) {
        (void)remove(realPath.c_str());
        IDE_LOGE("WriteFile failed, fileName: %s, err: %d", realPath.c_str(), err);
        return false;
    }
    IDE_LOGI("record dump data success: %s", realPath.c_str());
    return true;
}

/**
 * @brief judge if is remote case based on flag
 * @param [in] msg : flag msg
 * @return
 *      true : is remote case
 *      false : not remote case
 */
bool AdxDumpRecord::JudgeRemoteFalg(const std::string &msg) const
{
    std::size_t len = msg.find_first_of (":");
    if (len != std::string::npos && len < MAX_IP_LENGTH) {
        std::string ipStr = msg.substr(0, len);
        if (StringUtils::IpValid(ipStr)) {
            IDE_LOGD("remote ip info check pass: %s", ipStr.c_str());
            return true;
        }
    }
    IDE_LOGD("non remote ip case checked.");

    return false;
}

void AdxDumpRecord::SetWorkPath(const std::string &path)
{
    workPath_ = path;
}

/**
 * @brief record dump info to the file
 * @param [in] data : record data
 * @return
 *      IDE_DAEMON_ERROR : falied
 *      IDE_DAEMON_OK : success
 */
void AdxDumpRecord::RecordDumpInfo()
{
    IDE_EVENT("start dump thread, remote dump record temp path : %s", dumpPath_.c_str());
    while (dumpRecordFlag_ || !DumpDataQueueIsEmpty()) {
        IDE_LOGD("record new file");
        HostDumpDataInfo data = {nullptr, 0};
        if (!hostDumpDataInfoQueue_.Pop(data)) {
            continue;
        }

        if (data.msg == nullptr) {
            continue;
        }
        SharedPtr<MsgProto> dataInfo = data.msg;
        DumpChunk *dumpChunk = reinterpret_cast<DumpChunk*>(dataInfo->data);
        if (dumpChunk == nullptr) {
            IDE_LOGW("transfered dumpChunk is nullptr");
            continue;
        }

        IDE_LOGD("queue pop data success!, offset: %d, fileName: %s", dumpChunk->offset, dumpChunk->fileName);
        if (AdxDataDumpProcess::Instance().IsRegistered()) {
            IDE_LOGI("mindspore session!");
            std::function<int(const struct DumpChunk *, int)> messageCallback =
            AdxDataDumpProcess::Instance().GetCallbackFun();
            if (!messageCallback) {
                IDE_LOGE("Registered messageCallback function is not callable,\
                    drop this data packet, fileName:%s", dumpChunk->fileName);
                continue;
            }
            int dumpChunkLen = sizeof(struct DumpChunk) + dumpChunk->bufLen;
            int ret = messageCallback(dumpChunk, dumpChunkLen);
            if (ret != IDE_DAEMON_NONE_ERROR) {
                IDE_LOGE("failed to transmission dump data to mindspore. err = %d", ret);
            }
        } else if (!RecordDumpDataToDisk(*dumpChunk)) {
            IDE_LOGE("failed to record dump data to disk.");
        }
        IDE_LOGD("new popped data process success");
    }
    IDE_LOGI("exit record file thread");
}

/**
 * @brief record dump info to the file
 * @param [in] data : record data
 * @return
 *      IDE_DAEMON_ERROR : falied
 *      IDE_DAEMON_OK : success
 */
bool AdxDumpRecord::RecordDumpDataToQueue(HostDumpDataInfo &info)
{
    if (hostDumpDataInfoQueue_.IsFull()) {
        IDE_LOGW("the dump data queue is full. please reduce model batch, images or dump layers.");
        return false;
    } else {
        hostDumpDataInfoQueue_.Push(info);
        IDE_LOGD("Insert dump data to queue success.");
    }

    return true;
}

/**
 * @brief get dump size from the record lists
 * @param [in] tag : tag of record lists
 * @return
 *      true : success
 *     false : false
 */
bool AdxDumpRecord::DumpDataQueueIsEmpty() const
{
    return hostDumpDataInfoQueue_.IsEmpty();
}
}
