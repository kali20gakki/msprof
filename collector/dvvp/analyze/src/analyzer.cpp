/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: handle profiling data
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2020-10-24
 */
#include "analyzer.h"
#include "config/config.h"
#include "data_struct.h"
#include "errno/error_code.h"
#include "op_desc_parser.h"
#include "message/codec.h"
#include "prof_channel_manager.h"
#include "proto/msprofiler.pb.h"
#include "uploader.h"
#include "uploader_mgr.h"
#include "toolchain/prof_acl_api.h"
#include "transport/hash_data.h"

namespace Analysis {
namespace Dvvp {
namespace Analyze {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::transport;

Analyzer::Analyzer(SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> uploader)
    : resultCount_(0),
      profileMode_(PROFILE_MODE_STEP_TRACE),
      flushedChannel_(false),
      flushQueueLen_(0),
      uploader_(uploader),
      inited_(false)
{
}

Analyzer::~Analyzer() = default;

int Analyzer::Init()
{
    MSVP_MAKE_SHARED0_RET(analyzerGe_, AnalyzerGe, PROFILING_FAILED);
    MSVP_MAKE_SHARED0_RET(analyzerHwts_, AnalyzerHwts, PROFILING_FAILED);
    MSVP_MAKE_SHARED0_RET(analyzerTs_, AnalyzerTs, PROFILING_FAILED);
    MSVP_MAKE_SHARED0_RET(analyzerFfts_, AnalyzerFfts, PROFILING_FAILED);
    MSVP_MAKE_SHARED0_RET(analyzerRt_, AnalyzerRt, PROFILING_FAILED);

    inited_ = true;
    if ((analyzerHwts_->InitFrequency() != PROFILING_SUCCESS) ||
        (analyzerTs_->InitFrequency() != PROFILING_SUCCESS) ||
        (analyzerFfts_->InitFrequency() != PROFILING_SUCCESS)) {
        inited_ = false;
        MSPROF_LOGE("Analyzer InitFrequency failed. inited_ = false");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}


void Analyzer::Flush()
{
    if (uploader_ != nullptr) {
        uploader_->Flush();
    }
}

void Analyzer::PrintStats()
{
    MSPROF_EVENT("total_size_analyze, upload time: %llu", resultCount_);
    PrintDeviceStats();
    PrintHostStats();
}

void Analyzer::PrintDeviceStats()
{
    analyzerHwts_->PrintStats();
    analyzerFfts_->PrintStats();
    analyzerTs_->PrintStats();
}

void Analyzer::PrintHostStats()
{
    analyzerGe_->PrintStats();
    analyzerRt_->PrintStats();
}

void Analyzer::TsDataPostProc()
{
    if (profileMode_ == PROFILE_MODE_INVALID) {
        if (!analyzerTs_->keypointOpInfo_.empty()) {
            MSPROF_EVENT("set profile mode: PROFILE_MODE_STEP_TRACE.");
            profileMode_ = PROFILE_MODE_STEP_TRACE;
        } else if (!analyzerTs_->opTimes_.empty()) {
            MSPROF_EVENT("set profile mode: PROFILE_MODE_SINGLE_OP.");
            profileMode_ = PROFILE_MODE_SINGLE_OP;
        }
    }

    if (profileMode_ == PROFILE_MODE_STEP_TRACE && IsNeedUpdateIndexId()) {
        MSPROF_LOGI("received new keypoint op end info.");
        UpdateOpIndexId(analyzerTs_->opTimes_);
        UpdateOpIndexId(analyzerHwts_->opTimes_);
        UploadAppOp(analyzerHwts_->opTimes_);
    }

    UploadAppOp(analyzerTs_->opTimes_);
    UploadKeypointOp();
}

void Analyzer::HwtsDataPostProc()
{
    if (profileMode_ == PROFILE_MODE_INVALID && !analyzerHwts_->opTimes_.empty() && !flushedChannel_) {
        Analysis::Dvvp::JobWrapper::ProfChannelManager::instance()->FlushChannel();
        MSPROF_LOGI("flush channel data ...");
        flushedChannel_ = true;

        SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> uploader = nullptr;
        analysis::dvvp::transport::UploaderMgr::instance()->GetUploader(devIdStr_, uploader);
        if (uploader == nullptr) {
            MSPROF_LOGE("GetUploader failed. devIdStr_: %s. inited_ = false", devIdStr_.c_str());
            inited_ = false;
            return;
        }
        flushQueueLen_ = uploader->GetQueueSize();
        if (flushQueueLen_ == 0) {
            MSPROF_EVENT("set profile mode: PROFILE_MODE_SINGLE_OP.");
            profileMode_ = PROFILE_MODE_SINGLE_OP;
        }
    }

    UpdateOpIndexId(analyzerHwts_->opTimes_);
    UploadAppOp(analyzerHwts_->opTimes_);
}

void Analyzer::FftsDataPostProc()
{
    // profileMode_ default is PROFILE_MODE_STEP_TRACE, no need check.
    UpdateOpIndexId(analyzerFfts_->opTimes_);
    UploadAppOp(analyzerFfts_->opTimes_);
}

void Analyzer::UploadAppOp(std::multimap<std::string, OpTime> &opTimes)
{
    if (profileMode_ == PROFILE_MODE_STEP_TRACE) {
        for (auto iter = opTimes.begin(); iter != opTimes.end();) {
            if (iter->second.indexId == 0) {
                iter++;
                continue;
            }
            std::string key0 = iter->first + KEY_SEPARATOR + "0";
            std::string key1 = iter->first + KEY_SEPARATOR + std::to_string(iter->second.indexId);
            bool key0_res = analyzerGe_->IsOpInfoCompleted(key0);
            bool key1_res = analyzerGe_->IsOpInfoCompleted(key1);
            if (key0_res && key1_res) {
                MSPROF_LOGE("GE data error. %s %s", key0.c_str(), key1.c_str());
                iter = opTimes.erase(iter);
            } else if (analyzerGe_->IsOpInfoCompleted(key0)) {
                ConstructAndUploadData(key0, iter->second);
                iter = opTimes.erase(iter);
            } else if (analyzerGe_->IsOpInfoCompleted(key1)) {
                ConstructAndUploadData(key1, iter->second);
                iter = opTimes.erase(iter);
            } else {
                iter++;
            }
        }
    } else if (profileMode_ == PROFILE_MODE_SINGLE_OP) {
        for (auto iter = opTimes.begin(); iter != opTimes.end();) {
            std::string key = iter->first + KEY_SEPARATOR + "0";
            if (analyzerGe_->IsOpInfoCompleted(key)) {
                ConstructAndUploadData(key, iter->second);
                iter = opTimes.erase(iter);
            } else {
                iter++;
            }
        }
    }
}

void Analyzer::UploadKeypointOp()
{
    auto &tsKeypointOp = analyzerTs_->keypointOpInfo_;

    for (size_t i = 0; i < tsKeypointOp.size(); i++) {
        if (tsKeypointOp[i].uploaded) {
            continue;
        }

        if (tsKeypointOp[i].endTime == 0) {
            continue;
        }

        std::string nullStr;
        OpTime opTime = {0, 0, 0, 0, 0, 0, ACL_SUBSCRIBE_OP};
        opTime.start = tsKeypointOp[i].startTime;
        opTime.end = tsKeypointOp[i].endTime;
        opTime.indexId = analyzerGe_->GetModelId(tsKeypointOp[i].modelId);
        ConstructAndUploadData(nullStr, opTime);
        tsKeypointOp[i].uploaded = true;
    }
}

uint64_t Analyzer::GetOpIndexId(uint64_t opTimeStamp)
{
    auto &tsKeypointOp = analyzerTs_->keypointOpInfo_;

    size_t opNum = tsKeypointOp.size();
    if ((opNum == 0) || ((opNum == 1) && (tsKeypointOp[0].endTime == 0))) {
        return 0;
    }

    size_t endIndex = opNum - 1;
    if (tsKeypointOp[endIndex].endTime == 0 && endIndex >= 1) {
        // last keypoint op not completed, so index more offset 1
        endIndex--;
    }
    if (opTimeStamp > tsKeypointOp[endIndex].endTime) {
        // the latest keypoint op's time less than opTimeStamp, need waiting later keypoint op
        return 0;
    }
    for (size_t i = 0; i <= endIndex; i++) {
        if ((opTimeStamp > tsKeypointOp[endIndex - i].startTime) &&
            (opTimeStamp < tsKeypointOp[endIndex - i].endTime)) {
            tsKeypointOp[endIndex - i].findSuccTimes += 1;
            return tsKeypointOp[endIndex - i].indexId;
        }
    }
    MSPROF_LOGI("Unable to get OpIndexID. opNum %d, opTimeStamp %llu.", opNum, opTimeStamp);

    return 0;
}

void Analyzer::UpdateOpIndexId(std::multimap<std::string, OpTime> &opTimes)
{
    uint64_t maxIndexId = 0;
    for (auto iter = opTimes.begin(); iter != opTimes.end(); iter++) {
        if (iter->second.indexId != 0) {
            continue;
        }
        iter->second.indexId = GetOpIndexId(iter->second.end);
        if (iter->second.indexId > maxIndexId) {
            maxIndexId = iter->second.indexId;
        }
    }

    // clean useless keypoint op
    auto &tsKeypointOp = analyzerTs_->keypointOpInfo_;
    for (auto iter = tsKeypointOp.begin(); iter != tsKeypointOp.end();) {
        if (!iter->uploaded) {
            iter++;
            continue;
        }
        if (iter->indexId < maxIndexId) {
            MSPROF_LOGI("delete keypoint. indexId:%llu findSuccTimes:%llu",
                        iter->indexId, iter->findSuccTimes);
            iter = tsKeypointOp.erase(iter);
        } else {
            iter++;
        }
    }
}

bool Analyzer::IsNeedUpdateIndexId()
{
    auto &tsKeypointOp = analyzerTs_->keypointOpInfo_;

    if (tsKeypointOp.empty()) {
        return false;
    }

    if (tsKeypointOp.back().endTime != 0 &&
        !(tsKeypointOp.back().uploaded)) {
        return true;
    } else {
        return false;
    }
}

void Analyzer::ConstructAndUploadData(const std::string &opId, OpTime &opTime)
{
    if (opTime.start > opTime.end || opTime.startAicore > opTime.endAicore) {
        MSPROF_LOGE("End timestamp is less then start. op:%s start:%llu end:%llu startAicore:%llu endAicore:%llu",
                    opId.c_str(), opTime.start, opTime.end, opTime.startAicore, opTime.endAicore);
        return;
    }

    ProfOpDesc opDesc;
    auto ret = memset_s(&opDesc, sizeof(ProfOpDesc), 0, sizeof(ProfOpDesc));
    if (ret != EOK) {
        MSPROF_LOGE("memset failed ret:%d", ret);
        return;
    }
    bool opIdEmpty = (opId.length() == 0);
    std::string opName = opIdEmpty ? KEYPOINT_OP_NAME : analyzerGe_->GetOpName(opId);
    std::string opType = opIdEmpty ? KEYPOINT_OP_TYPE : analyzerGe_->GetOpType(opId);
    opDesc.modelId = opIdEmpty ? opTime.indexId : analyzerGe_->GetModelId(opId);
    uint64_t opIndex = OpDescParser::instance()->SetOpTypeAndOpName(opType, opName);
    if (opIndex == 0) {
        return;
    }
    opDesc.opIndex = opIndex;
    opDesc.duration = opTime.end - opTime.start;
    opDesc.start = opTime.start;
    opDesc.end = opTime.end;
    opDesc.flag = opTime.flag;
    if (opType == "FFTS_PLUS") {
        opDesc.flag = ACL_SUBSCRIBE_SUBGRAPH;
    }
    opDesc.threadId = opTime.threadId;
    opDesc.executionTime = opTime.endAicore - opTime.startAicore; // chipId 0 only
    auto opDescIntPtr = reinterpret_cast<uint8_t *>(&opDesc);
    if (opDescIntPtr == nullptr) {
        MSPROF_LOGE("Failed to call reinterpret_cast.");
        return;
    }
    opDesc.signature = analysis::dvvp::common::utils::Utils::GenerateSignature(
        opDescIntPtr + sizeof(uint32_t), sizeof(ProfOpDesc) - sizeof(uint32_t));
    if (uploader_ == nullptr) {
        MSPROF_LOGE("Analyzer::uploader_ is nullptr");
        return;
    }
    MSPROF_LOGD("Upload old data. modelId: %u, threadId: %u, opName: %s, opType: %s, start: %llu, "
        "end: %llu, duration: %llu, startAicore: %llu, endAicore: %llu, flag: %u", opDesc.modelId,
        opDesc.threadId, opName.c_str(), opType.c_str(), opDesc.start, opDesc.end, opDesc.duration,
        opTime.startAicore, opTime.endAicore, opDesc.flag);
    auto opDescCharPtr = reinterpret_cast<CHAR_PTR>(&opDesc);
    uploader_->UploadData(opDescCharPtr, sizeof(ProfOpDesc));
    resultCount_++;
}

void Analyzer::OnOptimizeData(CONST_VOID_PTR data, uint32_t len)
{
    if (!inited_) {
        MSPROF_LOGE("Analyzer is not been inited!");
        return;
    }

    auto decoded = analysis::dvvp::message::DecodeMessage2(data, len);
    auto message = std::dynamic_pointer_cast<analysis::dvvp::proto::FileChunkReq>(decoded);
    if (message == nullptr || message->filename().empty()) {
        MSPROF_LOGW("Analyzer OnOptimizeData is not data for analyzing.");
        return;
    }
    if (message->datamodule() == FileChunkDataModule::PROFILING_IS_CTRL_DATA) {
        if (message->filename() == "end_info") {
            ClearAnalyzerData();
        }
        return;
    }

    DispatchOptimizeData(message);
}

void Analyzer::DispatchOptimizeData(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message)
{
    MSPROF_LOGD("Start to analyze file: %s, tag: %s", message->filename().c_str(), message->tag().c_str());
    if (analyzerGe_->IsGeApiOrEventData(message->filename())) {
        analyzerGe_->GeApiAndEventParse(message);
    } else if (analyzerGe_->IsGeCompactData(message->tag())) {
        analyzerGe_->GeCompactParse(message);
    } else if (analyzerGe_->IsGeGraphIdMapData(message->tag())) {
        analyzerGe_->GeGraphIdMapParse(message);
    } else if (analyzerGe_->IsGeContextData(message->tag())) {
        analyzerGe_->GeContextParse(message);
    } else if (analyzerRt_->IsRtCompactData(message->tag())) {
        analyzerRt_->RtCompactParse(message);
    } else if (analyzerHwts_->IsHwtsData(message->filename())) {
        analyzerHwts_->HwtsParse(message);
    } else if (analyzerFfts_->IsFftsData(message->filename())) {
        analyzerFfts_->FftsParse(message);
    } else if (analyzerTs_->IsTsData(message->filename())) {
        analyzerTs_->Parse(message);
        TsDataPostProc();
    } else {
        MSPROF_LOGD("Analyzer drop data, fileName: %s.", message->filename().c_str());
        return;
    }
    UploadProfOpDescProc();
}

void Analyzer::UploadProfOpDescProc()
{
    std::unique_lock<std::mutex> lk(AnalyzerBase::opDescInfoMtx_);
    for (auto &it : AnalyzerBase::opDescInfos_) {
        if (uploader_ == nullptr) {
            MSPROF_LOGE("uploader is nullptr in upload optimize data");
            return;
        }
        MSPROF_LOGD("Upload opt data pop from vector. modelId: %u, threadId: %u, start: %llu, end: %llu"
            " duration: %llu, flag: %u, exetime: %llu", it.modelId, it.threadId, it.start, it.end, it.duration,
            it.flag, it.executionTime);
        uploader_->UploadData(reinterpret_cast<CHAR_PTR>(&it), sizeof(ProfOpDesc));
        resultCount_++;
    }
    AnalyzerBase::opDescInfos_.clear();
}

void Analyzer::SetDevId(const std::string &devIdStr)
{
    devIdStr_ = devIdStr;
    MSPROF_LOGI("Analyzer::SetDevId devIdStr_: %s", devIdStr_.c_str());
}
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis
