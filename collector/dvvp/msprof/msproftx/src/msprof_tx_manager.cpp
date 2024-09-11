/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2024. All rights reserved.
 * Description: creat tx manager
 * Author:
 * Create: 2021-11-22
 */

#include "msprof_tx_manager.h"
#include "msprof_stamp_pool.h"

#include "config/config.h"
#include "errno/error_code.h"
#include "mmpa_api.h"
#include "msprof_dlog.h"
#include "msprof_error_manager.h"
#include "utils.h"

namespace Msprof {
namespace MsprofTx {
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;
MsprofTxManager::MsprofTxManager() : isInit_(false), reporter_(nullptr), stampPool_(nullptr), eventId_(1)
{
}

MsprofTxManager::~MsprofTxManager()
{
    reporter_.reset();
    stampPool_.reset();
}


int MsprofTxManager::Init()
{
    std::lock_guard<std::mutex> lk(mtx_);
    if (reporter_ == nullptr) {
        MSPROF_LOGW("[Init] MsprofTxManager reporter not register, please check if register callback success.");
        return PROFILING_FAILED;
    }
    if (isInit_) {
        MSPROF_LOGW("[Init] MsprofTxManager already inited.");
        return PROFILING_SUCCESS;
    }

    MSVP_MAKE_SHARED0_RET(stampPool_, MsprofStampPool, PROFILING_FAILED);
    auto ret = stampPool_->Init(CURRENT_STAMP_SIZE);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[Init]init stamp pool failed, ret is %d", ret);
        MSPROF_INNER_ERROR("EK9999", "init stamp pool failed, ret is %d", ret);
        return PROFILING_FAILED;
    }

    ret = reporter_->Init();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[Init]reporter init failed!");
        stampPool_->UnInit();
        return PROFILING_FAILED;
    }
    isInit_ = true;
    return PROFILING_SUCCESS;
}

void MsprofTxManager::UnInit()
{
    std::lock_guard<std::mutex> lk(mtx_);
    if (isInit_) {
        stampPool_->UnInit();
        reporter_->UnInit();
        isInit_ = false;
        MSPROF_LOGI("[UnInit] TxManager unInit success.");
    }
}

void MsprofTxManager::SetReporterCallback(const MsprofAddiInfoReporterCallback func)
{
    if (reporter_ == nullptr) {
        MSVP_MAKE_SHARED0_VOID(reporter_, MsprofTxReporter);
    }
    reporter_->SetReporterCallback(func);
}

/*!
 * get stamp pointer from memory pool
 * @return
 */
ACL_PROF_STAMP_PTR MsprofTxManager::CreateStamp() const
{
    if (!isInit_) {
        MSPROF_LOGE("[CreateStamp]MsprofTxManager is not inited yet");
        return nullptr;
    }

    return stampPool_->CreateStamp();
}

/*!
 *
 * @param stamp
 */
void MsprofTxManager::DestroyStamp(ACL_PROF_STAMP_PTR stamp) const
{
    if (stamp == nullptr) {
        MSPROF_LOGE("[DestroyStamp]aclprofStamp is nullptr");
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"nullptr", "stamp", "stamp can not be nullptr when destroy"}));
        return;
    }
    if (!isInit_) {
        MSPROF_LOGE("[DestroyStamp]MsprofTxManager is not inited yet");
        return;
    }

    stampPool_->DestroyStamp(stamp);
}

// save category and name relation
int MsprofTxManager::SetCategoryName(uint32_t category, std::string categoryName) const
{
    if (!isInit_) {
        MSPROF_LOGE("[SetCategoryName]MsprofTxManager is not inited yet");
        return PROFILING_FAILED;
    }
    static const std::string MSPROF_TX_CATEGORY_DICT = "category_dict";
    static const uint32_t tagLen = MSPROF_TX_CATEGORY_DICT.size();

    std::string hashData = std::to_string(category) + HASH_DIC_DELIMITER + categoryName + "\n";

    ReporterData reporterData;
    reporterData.deviceId = DEFAULT_HOST_ID;
    auto ret = memcpy_s(reporterData.tag, MSPROF_ENGINE_MAX_TAG_LEN - 1, MSPROF_TX_CATEGORY_DICT.c_str(), tagLen);
    if (ret != EOK) {
        MSPROF_LOGE("[SetCategoryName] memcpy tag failed. ret=%u len=%u", ret, tagLen);
        return PROFILING_FAILED;
    }
    reporterData.tag[tagLen] = '\0';
    reporterData.dataLen = hashData.size();
    reporterData.data = (unsigned char *)hashData.c_str();
    return PROFILING_SUCCESS;
}

// stamp message manage
int MsprofTxManager::SetStampCategory(ACL_PROF_STAMP_PTR stamp, uint32_t category) const
{
    if (!isInit_) {
        MSPROF_LOGE("[SetStampCategory]MsprofTxManager is not inited yet");
        return PROFILING_FAILED;
    }
    if (stamp == nullptr) {
        MSPROF_LOGE("aclprofStamp is nullptr");
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"nullptr", "stamp", "stamp can not be nullptr when set category"}));
        return PROFILING_FAILED;
    }
    stamp->txInfo.value.stampInfo.category = category;
    return PROFILING_SUCCESS;
}

int MsprofTxManager::SetStampPayload(ACL_PROF_STAMP_PTR stamp, const int32_t type, void *value) const
{
    if (!isInit_) {
        MSPROF_LOGE("[SetStampPayload]MsprofTxManager is not inited yet");
        return PROFILING_FAILED;
    }
    if (stamp == nullptr) {
        MSPROF_LOGE("aclprofStamp is nullptr");
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"nullptr", "stamp", "stamp can not be nullptr when set payload"}));
        return PROFILING_FAILED;
    }
    if (value == nullptr) {
        MSPROF_LOGE("[SetStampPayload]value is nullptr");
        return PROFILING_FAILED;
    }

    stamp->txInfo.value.stampInfo.payloadType = type;

    return PROFILING_SUCCESS;
}

int MsprofTxManager::SetStampTraceMessage(ACL_PROF_STAMP_PTR stamp, CONST_CHAR_PTR msg, uint32_t msgLen) const
{
    if (!isInit_) {
        MSPROF_LOGE("[SetStampTraceMessage]MsprofTxManager is not inited yet");
        return PROFILING_FAILED;
    }
    if (stamp == nullptr || msg == nullptr) {
        MSPROF_LOGE("[SetStampTraceMessage]aclprofStamp or msg is nullptr");
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"nullptr", "stamp", "stamp and msg can not be nullptr when set traceMessage"}));
        return PROFILING_FAILED;
    }
    auto ret = memcpy_s(stamp->txInfo.value.stampInfo.message, sizeof(stamp->txInfo.value.stampInfo.message) - 1,
                        msg, msgLen);
    if (ret != EOK) {
        MSPROF_LOGE("[SetStampTraceMessage]strcpy_s message failed, ret=%u msgLen=%u", ret, msgLen);
        return PROFILING_FAILED;
    }
    stamp->txInfo.value.stampInfo.message[msgLen] = 0;
    MSPROF_LOGD("[SetStampTraceMessage]stamp set trace message:%s success.", stamp->txInfo.value.stampInfo.message);
    return PROFILING_SUCCESS;
}

// mark stamp
int MsprofTxManager::Mark(ACL_PROF_STAMP_PTR stamp) const
{
    if (!isInit_) {
        MSPROF_LOGE("[Mark]MsprofTxManager is not inited yet");
        return PROFILING_FAILED;
    }
    if (stamp == nullptr) {
        MSPROF_LOGE("[Mark]aclprofStamp is nullptr");
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"nullptr", "stamp", "stamp can not be nullptr when mark"}));
        return PROFILING_FAILED;
    }

    stamp->txInfo.value.stampInfo.startTime = static_cast<uint64_t>(Utils::GetClockRealtimeOrCPUCycleCounter());
    stamp->txInfo.value.stampInfo.endTime = stamp->txInfo.value.stampInfo.startTime;
    stamp->txInfo.value.stampInfo.eventType = static_cast<uint32_t>(EventType::MARK);
    return ReportStampData(stamp);
}

int MsprofTxManager::MarkEx(CONST_CHAR_PTR msg, size_t msgLen, aclrtStream stream)
{
    uint64_t markIdx = GetEventId();
    static uint32_t pid = static_cast<uint32_t>(MmGetPid());
    static thread_local uint32_t tid = static_cast<uint32_t>(MmGetTid());
    if (!isInit_) {
        MSPROF_LOGE("[MarkEx]MsprofTxManager is not inited yet");
        return PROFILING_FAILED;
    }
    if (msg == nullptr || stream == nullptr || strnlen(msg, msgLen) != msgLen) {
        MSPROF_LOGE("[MarkEx]Invalid input param for markEx");
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"nullptr", "message", "Invalid input param for markEx"}));
        return PROFILING_FAILED;
    }
    if (msgLen > static_cast<size_t>(MAX_MESSAGE_LEN - 1) || msgLen < 1) {
        MSPROF_LOGE("[MarkEx]The length of input message should be in range of 1~%zu",
                    static_cast<size_t>(MAX_MESSAGE_LEN - 1));
        return PROFILING_FAILED;
    }
    // create MsprofTxInfo info
    MsprofTxInfo info;
    info.infoType = 1;
    info.value.stampInfo.processId = pid;
    info.value.stampInfo.threadId = tid;
    info.value.stampInfo.eventType = static_cast<uint16_t>(EventType::MARK_EX);
    info.value.stampInfo.startTime = static_cast<uint64_t>(Utils::GetClockRealtimeOrCPUCycleCounter());
    info.value.stampInfo.endTime = info.value.stampInfo.startTime;
    // rtProfilerTraceEx
    if (RuntimePlugin::instance()->MsprofRtProfilerTraceEx(markIdx, MARKEX_MODEL_ID, MARKEX_TAG_ID, stream) !=
        PROFILING_SUCCESS) {
        MSPROF_LOGE("[MarkEx]Failed to run mstx task, mark idx=%u", markIdx);
        return PROFILING_FAILED;
    }
    info.value.stampInfo.markId = markIdx;
    // copy message
    if (memcpy_s(info.value.stampInfo.message, MAX_MESSAGE_LEN - 1, msg, msgLen) != EOK) {
        MSPROF_LOGE("[MarkEx]memcpy_s message failed, msgLen %zu", msgLen);
        return PROFILING_FAILED;
    }
    info.value.stampInfo.message[msgLen] = '\0';
    // report data
    if (reporter_->Report(info) != PROFILING_SUCCESS) {
        MSPROF_LOGE("[MarkEx] report profiling data failed.");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

// stamp stack manage
int MsprofTxManager::Push(ACL_PROF_STAMP_PTR stamp) const
{
    if (!isInit_) {
        MSPROF_LOGE("[Push]MsprofTxManager is not inited yet");
        return PROFILING_FAILED;
    }
    if (stamp == nullptr) {
        MSPROF_LOGE("[Push]aclprofStamp is nullptr");
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"nullptr", "stamp", "stamp can not be nullptr when push"}));
        return PROFILING_FAILED;
    }

    stamp->txInfo.value.stampInfo.startTime = static_cast<uint64_t>(Utils::GetClockRealtimeOrCPUCycleCounter());
    return stampPool_->MsprofStampPush(stamp);
}


int MsprofTxManager::Pop() const
{
    if (!isInit_) {
        MSPROF_LOGE("[Pop]MsprofTxManager is not inited yet");
        return PROFILING_FAILED;
    }
    auto stamp = stampPool_->MsprofStampPop();
    if (stamp == nullptr) {
        MSPROF_LOGE("[Pop]stampPool pop failed ,stamp is null!");
        return PROFILING_FAILED;
    }
    stamp->txInfo.value.stampInfo.endTime = static_cast<uint64_t>(Utils::GetClockRealtimeOrCPUCycleCounter());
    stamp->txInfo.value.stampInfo.eventType = static_cast<uint32_t>(EventType::PUSH_OR_POP);
    return ReportStampData(stamp);
}

// stamp map manage
int MsprofTxManager::RangeStart(ACL_PROF_STAMP_PTR stamp, uint32_t *rangeId) const
{
    if (!isInit_) {
        MSPROF_LOGE("[RangeStart]MsprofTxManager is not inited yet");
        return PROFILING_FAILED;
    }
    if (stamp == nullptr) {
        MSPROF_LOGE("[RangeStart] stamp pointer is nullptr!");
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"nullptr", "stamp", "stamp can not be nullptr when RangeStart"}));
        return PROFILING_FAILED;
    }
    if (rangeId == nullptr) {
        MSPROF_LOGE("[RangeStart] rangeId pointer is nullptr!");
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({"nullptr", "rangeId", "rangeId can not be nullptr when RangeStart"}));
        return PROFILING_FAILED;
    }
    auto &stampInfo = stamp->txInfo.value.stampInfo;
    stampInfo.startTime = static_cast<uint64_t>(Utils::GetClockRealtimeOrCPUCycleCounter());
    *rangeId = stampPool_->GetIdByStamp(stamp);
    return PROFILING_SUCCESS;
}

int MsprofTxManager::RangeStop(uint32_t rangeId) const
{
    if (!isInit_) {
        MSPROF_LOGE("[RangeStop]MsprofTxManager is not inited yet");
        return PROFILING_FAILED;
    }
    auto stamp = stampPool_->GetStampById(rangeId);
    if (stamp == nullptr) {
        MSPROF_LOGE("[RangeStop] Get stamp by rangeId failed, rangeId is %u!", rangeId);
        std::string errorReason = "Get stamp by rangeId failed, this rangeId was not set in profAclRangeStart";
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({std::to_string(rangeId), "rangeId", errorReason}));
        return PROFILING_FAILED;
    }
    stamp->txInfo.value.stampInfo.endTime = static_cast<uint64_t>(Utils::GetClockRealtimeOrCPUCycleCounter());
    stamp->txInfo.value.stampInfo.eventType = static_cast<uint32_t>(EventType::START_OR_STOP);
    return ReportStampData(stamp);
}

int MsprofTxManager::ReportStampData(MsprofStampInstance *stamp) const
{
    static thread_local uint32_t tid = static_cast<uint32_t>(MmGetTid());
    stamp->txInfo.value.stampInfo.threadId = tid;
    if (reporter_->Report(stamp->txInfo) != PROFILING_SUCCESS) {
        MSPROF_LOGE("[ReportStampData] report profiling data failed.");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

void MsprofTxManager::ReportData(MsprofTxInfo &data)
{
    if (reporter_ == nullptr) {
        MSPROF_LOGE("[ReportData] reporter is nullptr!");
        return;
    }
    if (reporter_->Report(data) != PROFILING_SUCCESS) {
        MSPROF_LOGE("[ReportData] Report data failed.");
    }
}

uint64_t MsprofTxManager::GetEventId()
{
    return eventId_++;
}
}
}