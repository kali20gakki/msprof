/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: creat tx manager
 * Author:
 * Create: 2021-11-22
 */

#include "msprof_tx_manager.h"

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
MsprofTxManager::MsprofTxManager() : isInit_(false), reporter_(nullptr), stampPool_(nullptr)
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

void MsprofTxManager::SetReporterCallback(const MsprofReporterCallback func)
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
    MSPROF_LOGI("[SetCategoryName] category is %d, categoryName is %s", category, categoryName.c_str());
    static const std::string MSPROF_TX_CATEGORY_DICT = "category_dict";

    std::string hashData = std::to_string(category) + HASH_DIC_DELIMITER + categoryName + "\n";

    ReporterData reporterData;
    memset_s(&reporterData, sizeof(ReporterData), 0, sizeof(ReporterData));
    reporterData.deviceId = DEFAULT_HOST_ID;
    auto err = memcpy_s(reporterData.tag, static_cast<size_t>(MSPROF_ENGINE_MAX_TAG_LEN),
        MSPROF_TX_CATEGORY_DICT.c_str(), MSPROF_TX_CATEGORY_DICT.size());
    if (err != EOK) {
        MSPROF_LOGE("[SetCategoryName] memcpy tag failed. ret is %u", err);
        return PROFILING_FAILED;
    }

    reporterData.dataLen = hashData.size();
    reporterData.data = (unsigned char *)hashData.c_str();

    auto ret = reporter_->Report(reporterData);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[ReportStampData] report profiling data failed.");
        return PROFILING_FAILED;
    }

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

    stamp->stampInfo.category = category;
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

    stamp->stampInfo.payloadType = type;

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

    static const int MAX_MSG_LEN = 128;
    if (msgLen >= MAX_MSG_LEN) {
        MSPROF_LOGE("[SetStampTraceMessage]msg len(%u) is invalid, must less then 128", msgLen);
        std::string errorReason = "msg len should be less than" + std::to_string(MAX_MSG_LEN);
        MSPROF_INPUT_ERROR("EK0001", std::vector<std::string>({"value", "param", "reason"}),
            std::vector<std::string>({std::to_string(msgLen), "stamp", errorReason}));
        return PROFILING_FAILED;
    }
    auto ret = memset_s(stamp->stampInfo.message, MAX_MSG_LEN, 0, MAX_MSG_LEN);
    if (ret != EOK) {
        MSPROF_LOGE("[SetStampTraceMessage]memset_s message failed, ret is %u", ret);
        return PROFILING_FAILED;
    }
    ret = strncpy_s(stamp->stampInfo.message, MAX_MSG_LEN - 1, msg, msgLen);
    if (ret != EOK) {
        MSPROF_LOGE("[SetStampTraceMessage]strcpy_s message failed, ret is %u", ret);
        return PROFILING_FAILED;
    }
    MSPROF_LOGD("[SetStampTraceMessage]stamp set trace message:%s success.", stamp->stampInfo.message);
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

    stamp->stampInfo.startTime = static_cast<uint64_t>(Utils::GetClockMonotonicRaw());
    stamp->stampInfo.endTime = stamp->stampInfo.startTime;
    stamp->stampInfo.eventType = static_cast<uint32_t>(EventType::MARK);
    return ReportStampData(stamp->stampInfo);
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

    stamp->stampInfo.startTime = static_cast<uint64_t>(Utils::GetClockMonotonicRaw());
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
    auto &stampInfo = stamp->stampInfo;
    stampInfo.endTime = static_cast<uint64_t>(Utils::GetClockMonotonicRaw());
    stampInfo.eventType = static_cast<uint32_t>(EventType::PUSH_OR_POP);
    return ReportStampData(stampInfo);
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
    auto &stampInfo = stamp->stampInfo;
    stampInfo.startTime = static_cast<uint64_t>(Utils::GetClockMonotonicRaw());
    *rangeId = stampPool_->GetIdByStamp(stamp);
    MSPROF_LOGD("[RangeStart] save stamp success, rangeId is %u", *rangeId);
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
    auto stampInfo = stamp->stampInfo;
    stampInfo.endTime = static_cast<uint64_t>(Utils::GetClockMonotonicRaw());
    stampInfo.eventType = static_cast<uint32_t>(EventType::START_OR_STOP);
    return ReportStampData(stampInfo);
}

int MsprofTxManager::ReportStampData(MsprofStampInfo &stamp) const
{
    if (!isInit_) {
        MSPROF_LOGE("[ReportStampData]MsprofTxManager is not inited yet");
        return PROFILING_FAILED;
    }
    static const std::string MSPROF_TX_REPORTER_TAG = "msproftx";

    stamp.processId = static_cast<uint32_t>(MmGetPid());
    stamp.threadId = static_cast<uint32_t>(MmGetTid());
    stamp.dataTag = MSPROF_MSPROFTX_DATA_TAG;
    stamp.magicNumber = static_cast<uint16_t>(MSPROF_DATA_HEAD_MAGIC_NUM);

    ReporterData reporterData;
    memset_s(&reporterData, sizeof(ReporterData), 0, sizeof(ReporterData));
    reporterData.deviceId = DEFAULT_HOST_ID;
    auto err = memcpy_s(reporterData.tag, static_cast<size_t>(MSPROF_ENGINE_MAX_TAG_LEN),
        MSPROF_TX_REPORTER_TAG.c_str(), MSPROF_TX_REPORTER_TAG.size());
    if (err != EOK) {
        MSPROF_LOGE("[ReportStampData] memcpy tag failed. ret is %u", err);
        return PROFILING_FAILED;
    }

    reporterData.data = reinterpret_cast<unsigned char *>(&stamp);
    reporterData.dataLen = sizeof(stamp);

    auto ret = reporter_->Report(reporterData);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[ReportStampData] report profiling data failed.");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}
}
}
