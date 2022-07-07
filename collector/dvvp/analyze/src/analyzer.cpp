/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: handle profiling data
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2020-10-24
 */
#include "analyzer.h"
#include "analyzer_ge.h"
#include "analyzer_hwts.h"
#include "analyzer_ts.h"
#include "analyzer_ffts.h"
#include "config/config.h"
#include "data_struct.h"
#include "errno/error_code.h"
#include "op_desc_parser.h"
#include "message/codec.h"
#include "prof_channel_manager.h"
#include "proto/profiler.pb.h"
#include "uploader.h"
#include "uploader_mgr.h"

namespace Analysis {
namespace Dvvp {
namespace Analyze {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;

Analyzer::Analyzer(SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> uploader)
    : resultCount_(0),
      profileMode_(PROFILE_MODE_STEP_TRACE),
      flushedChannel_(false),
      flushQueueLen_(0)
{
    uploader_ = uploader;

    MSVP_MAKE_SHARED0_VOID(analyzerGe_, AnalyzerGe);
    MSVP_MAKE_SHARED0_VOID(analyzerHwts_, AnalyzerHwts);
    MSVP_MAKE_SHARED0_VOID(analyzerTs_, AnalyzerTs);
    MSVP_MAKE_SHARED0_VOID(analyzerFfts_, AnalyzerFfts);

    inited_ = true;
    if ((analyzerHwts_->InitFrequency() != PROFILING_SUCCESS) ||
        (analyzerTs_->InitFrequency() != PROFILING_SUCCESS) ||
        (analyzerFfts_->InitFrequency() != PROFILING_SUCCESS)) {
        inited_ = false;
        MSPROF_LOGE("Analyzer InitFrequency failed. inited_ = false");
    }
}

Analyzer::~Analyzer() {}

void Analyzer::OnNewData(CONST_VOID_PTR data, uint32_t len)
{
    if (!inited_) {
        MSPROF_LOGE("Analyzer is not been inited!");
        return;
    }

    auto decoded = analysis::dvvp::message::DecodeMessage2(data, len);
    auto message = std::dynamic_pointer_cast<analysis::dvvp::proto::FileChunkReq>(decoded);
    if (message == nullptr || message->datamodule() == FileChunkDataModule::PROFILING_IS_CTRL_DATA) {
        if (profileMode_ == PROFILE_MODE_INVALID && flushedChannel_) {
            flushQueueLen_--;
            if (flushQueueLen_ <= 0) {
                profileMode_ = PROFILE_MODE_SINGLE_OP;
                MSPROF_EVENT("set profile mode: PROFILE_MODE_SINGLE_OP.");
            }
        }
        MSPROF_LOGW("Analyzer OnNewData is not data for analyzing");
        return;
    }
    MSPROF_LOGD("Analyzer OnNewData enter");
    DispatchData(message);
    MSPROF_LOGD("Analyzer OnNewData exit");
}

void Analyzer::Flush()
{
    if (uploader_ != nullptr) {
        uploader_->Flush();
    }
}

void Analyzer::PrintStats()
{
    MSPROF_EVENT("total_size_analyze, op time: %llu", resultCount_);
    analyzerGe_->PrintStats();
    analyzerTs_->PrintStats();
    analyzerHwts_->PrintStats();
}

void Analyzer::DispatchData(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message)
{
    if (analyzerGe_->IsGeData(message->filename())) {
        analyzerGe_->Parse(message);
        UploadAppOp(analyzerTs_->opTimes_);
        UploadAppOp(analyzerHwts_->opTimes_);
    } else if (analyzerTs_->IsTsData(message->filename())) {
        analyzerTs_->Parse(message);
        TsDataPostProc();
    } else if (analyzerHwts_->IsHwtsData(message->filename())) {
        analyzerHwts_->Parse(message);
        HwtsDataPostProc();
    } else if (analyzerFfts_->IsFftsData(message->filename())) {
        analyzerFfts_->Parse(message);
        FftsDataPostProc();
    } else {
        MSPROF_LOGI("Analyzer drop data, fileName:  %s", message->filename().c_str());
    }

    if (profileMode_ == PROFILE_MODE_INVALID && flushedChannel_) {
        flushQueueLen_--;
        if (flushQueueLen_ <= 0) {
            profileMode_ = PROFILE_MODE_SINGLE_OP;
            MSPROF_EVENT("set profile mode: PROFILE_MODE_SINGLE_OP.");
            // in single op mode, need upload hwts op immediately
            UploadAppOp(analyzerHwts_->opTimes_);
        }
    }
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
    std::string opName;
    std::string opType;
    if (opId.length() == 0) {
        opDesc.modelId = opTime.indexId;
        opName = KEYPOINT_OP_NAME;
        opType = KEYPOINT_OP_TYPE;
    } else {
        opDesc.modelId = analyzerGe_->GetModelId(opId);
        opName = analyzerGe_->GetOpName(opId);
        opType = analyzerGe_->GetOpType(opId);
    }
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
    opDesc.signature = analysis::dvvp::common::utils::Utils::GenerateSignature(
        reinterpret_cast<uint8_t *>(&opDesc) + sizeof(uint32_t), sizeof(ProfOpDesc) - sizeof(uint32_t));

    uploader_->UploadData(reinterpret_cast<CHAR_PTR>(&opDesc), sizeof(ProfOpDesc));
    resultCount_++;
}

void Analyzer::SetDevId(const std::string &devIdStr)
{
    devIdStr_ = devIdStr;
    MSPROF_LOGI("Analyzer::SetDevId devIdStr_: %s", devIdStr_.c_str());
}
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis
