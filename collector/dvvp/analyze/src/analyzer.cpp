/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#include "analyzer.h"
#include "config/config.h"
#include "data_struct.h"
#include "errno/error_code.h"
#include "op_desc_parser.h"
#include "prof_channel_manager.h"
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

void Analyzer::UploadAppOp(std::multimap<std::string, OpTime> &opTimes)
{
    if (profileMode_ == PROFILE_MODE_STEP_TRACE) {
        UploadAppOpModeStepTrace(opTimes);
    } else if (profileMode_ == PROFILE_MODE_STATIC_SHAPE) {
        UploadAppOpModeStaticShape(opTimes);
    } else if (profileMode_ == PROFILE_MODE_SINGLE_OP) {
        UploadAppOpModeSingleOp(opTimes);
    }
}

void Analyzer::UploadAppOpModeStepTrace(std::multimap<std::string, OpTime> &opTimes)
{
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
}

void Analyzer::UploadAppOpModeStaticShape(std::multimap<std::string, OpTime> &opTimes)
{
    bool isAllStatic = analyzerGe_->GetIsAllStaticShape();
    if (isAllStatic) {
        // in final solution only this branch is needed,need to query high16bit taskid from stream info
        for (auto iter = opTimes.begin(); iter != opTimes.end();) {
            std::string key0 = iter->first + KEY_SEPARATOR + "0";
            bool key0Res = analyzerGe_->IsOpInfoCompleted(key0);
            if (key0Res) {
                ConstructAndUploadData(key0, iter->second);
                iter = opTimes.erase(iter);
                MSPROF_LOGD("Op info is Constructed, %s", key0.c_str());
            } else {
                iter++;
                MSPROF_LOGD("Op info is incomplete, %s", key0.c_str());
            }
        }
    } else {
        MSPROF_LOGD("Try to construct Op info, is not all static");
        for (auto iter = opTimes.begin(); iter != opTimes.end();) {  // tmp solution, discard dynamic shape task
            uint32_t streamType;
            if (!analyzerGe_->GetStreamType(iter->second.streamId, streamType)) {
                MSPROF_LOGI("Op Stream info hasn't been received, stream id is %u", iter->second.streamId);
                iter++;
                continue;
            }
 
            if (streamType == UNKNOWN_SHAPE_STREAM) {
                MSPROF_LOGI("Op belong to unknown shape stream, not supported yet");
                iter = opTimes.erase(iter);
                continue;
            }
 
            std::string key0 = iter->first + KEY_SEPARATOR + "0";
            bool key0Res = analyzerGe_->IsOpInfoCompleted(key0);
            if (key0Res) {
                ConstructAndUploadData(key0, iter->second);
                iter = opTimes.erase(iter);
                MSPROF_LOGD("Op info is Constructed, %s", key0.c_str());
            } else {
                iter++;
                MSPROF_LOGD("Op info is incomplete, %s", key0.c_str());
            }
        }
    }
}

void Analyzer::UploadAppOpModeSingleOp(std::multimap<std::string, OpTime> &opTimes)
{
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

void Analyzer::UploadKeypointOp()
{
    auto &tsKeypointOp = analyzerTs_->keypointOpInfo_;

    if (profileMode_ == PROFILE_MODE_STATIC_SHAPE) {
        for (auto iter = tsKeypointOp.begin(); iter != tsKeypointOp.end();) {
            if (iter->second.endTime == 0) {
                ++iter;
                MSPROF_LOGD("Key point end is not ready");
                continue;
            }
            std::string nullStr;
            OpTime opTime = {0, 0, 0, 0, 0, 0, ACL_SUBSCRIBE_OP, 0};
            opTime.start = iter->second.startTime;
            opTime.end = iter->second.endTime;
            opTime.indexId = analyzerGe_->GetModelId(iter->second.modelId);
            ConstructAndUploadData(nullStr, opTime);
            iter = tsKeypointOp.erase(iter);  // static shape, no need to keep uploaded keypoint
        }
    } else {
        for (auto iter = tsKeypointOp.begin(); iter != tsKeypointOp.end(); ++iter) {
            if (iter->second.uploaded) {
                continue;
            }

            if (iter->second.endTime == 0) {
                continue;
            }

            std::string nullStr;
            OpTime opTime = {0, 0, 0, 0, 0, 0, ACL_SUBSCRIBE_OP, 0};
            opTime.start = iter->second.startTime;
            opTime.end = iter->second.endTime;
            opTime.indexId = analyzerGe_->GetModelId(iter->second.modelId);
            ConstructAndUploadData(nullStr, opTime);
            iter->second.uploaded = true;
        }
    }
}

uint64_t Analyzer::GetOpIndexId(uint64_t opTimeStamp)
{
    auto &tsKeypointOp = analyzerTs_->keypointOpInfo_;

    size_t opNum = tsKeypointOp.size();
    if ((opNum == 0) || ((opNum == 1) && (tsKeypointOp.begin()->second.endTime == 0))) {
        MSPROF_LOGD("Get OpIndex failed");
        return 0;
    }

    for (auto iter = tsKeypointOp.begin(); iter != tsKeypointOp.end(); ++iter) {
        if (iter->second.endTime == 0) {
            MSPROF_LOGI("Incomplete keypoint");
            continue;
        }
        if ((opTimeStamp > iter->second.startTime) && (opTimeStamp < iter->second.endTime)) {
            iter->second.findSuccTimes += 1;
            return iter->second.indexId;
        }
    }

    MSPROF_LOGI("Unable to get OpIndexID. opNum %d, opTimeStamp %llu.", opNum, opTimeStamp);
    return 0;
}

void Analyzer::UpdateOpIndexId(std::multimap<std::string, OpTime> &opTimes)
{
    if (profileMode_ == PROFILE_MODE_STATIC_SHAPE) {
        MSPROF_LOGI("Static shape scene, no need to update op index");
        return;
    } else {
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
            if (!iter->second.uploaded) {
                iter++;
                continue;
            }
            if (iter->second.indexId < maxIndexId) {
                MSPROF_LOGI("delete keypoint. indexId:%llu findSuccTimes:%llu",
                    iter->second.indexId,
                    iter->second.findSuccTimes);
                iter = tsKeypointOp.erase(iter);
            } else {
                iter++;
            }
        }
    }
}

bool Analyzer::IsNeedUpdateIndexId()
{
    auto &tsKeypointOp = analyzerTs_->keypointOpInfo_;

    if (tsKeypointOp.empty()) {
        return false;
    }

    for (auto iter = tsKeypointOp.begin(); iter != tsKeypointOp.end(); ++iter) {
        if (iter->second.endTime != 0 && !(iter->second.uploaded)) {
            return true;
        }
    }
    return false;
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

void Analyzer::OnOptimizeData(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq)
{
    if (!inited_) {
        MSPROF_LOGE("Analyzer is not been inited!");
        return;
    }
    if (fileChunkReq == nullptr || fileChunkReq->fileName.empty()) {
        MSPROF_LOGW("Analyzer OnOptimizeData is not data for analyzing.");
        return;
    }
    if (fileChunkReq->chunkModule == FileChunkDataModule::PROFILING_IS_CTRL_DATA) {
        if (fileChunkReq->fileName == "end_info") {
            ClearAnalyzerData();
        }
        return;
    }

    DispatchOptimizeData(fileChunkReq);
}

void Analyzer::DispatchOptimizeData(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq)
{
    MSPROF_LOGD("Start to analyze file: %s", fileChunkReq->fileName.c_str());
    if (analyzerGe_->IsGeApiOrEventData(fileChunkReq->fileName)) {
        analyzerGe_->GeApiAndEventParse(fileChunkReq);
    } else if (analyzerGe_->IsGeCompactData(fileChunkReq->fileName)) {
        analyzerGe_->GeCompactParse(fileChunkReq);
    } else if (analyzerGe_->IsGeGraphIdMapData(fileChunkReq->fileName)) {
        analyzerGe_->GeGraphIdMapParse(fileChunkReq);
    } else if (analyzerGe_->IsGeContextData(fileChunkReq->fileName)) {
        analyzerGe_->GeContextParse(fileChunkReq);
    } else if (analyzerRt_->IsRtCompactData(fileChunkReq->fileName)) {
        analyzerRt_->RtCompactParse(fileChunkReq);
    } else if (analyzerHwts_->IsHwtsData(fileChunkReq->fileName)) {
        analyzerHwts_->HwtsParse(fileChunkReq);
    } else if (analyzerFfts_->IsFftsData(fileChunkReq->fileName)) {
        analyzerFfts_->FftsParse(fileChunkReq);
    } else if (analyzerTs_->IsTsData(fileChunkReq->fileName)) {
        analyzerTs_->Parse(fileChunkReq);
        TsDataPostProc();
    } else {
        MSPROF_LOGD("Analyzer drop data, fileName: %s.", fileChunkReq->fileName.c_str());
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
