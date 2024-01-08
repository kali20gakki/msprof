/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : compact_info_parser.h
 * Description        : compact数据解析
 * Author             : msprof team
 * Creation Date      : 2023/11/9
 * *****************************************************************************
 */

#include "analysis/csrc/parser/host/cann/compact_info_parser.h"

#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/parser/chunk_generator.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/utils/utils.h"

namespace Analysis {
namespace Parser {
namespace Host {
namespace Cann {
using namespace Analysis::Parser::Adapter;
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;

void CompactInfoParser::Init(const std::vector<std::string> &filePrefix)
{
    chunkProducer_ = MakeShared<ChunkGenerator>(sizeof(MsprofCompactInfo), path_, filePrefix);
}

template<>
std::vector<std::shared_ptr<MsprofCompactInfo>> CompactInfoParser::GetData()
{
    return compactData_;
}

template<>
std::vector<std::shared_ptr<FlipTask>> CompactInfoParser::GetData()
{
    return flipTaskData_;
}

int CompactInfoParser::ProduceData()
{
    if (!Reserve(compactData_, chunkProducer_->Size())) {
        ERROR("%: Reserve data failed", parserName_);
        return ANALYSIS_ERROR;
    }
    while (!chunkProducer_->Empty()) {
        auto compactInfo = ReinterpretConvert<MsprofCompactInfo*>(chunkProducer_->Pop());
        if (!compactInfo) {
            ERROR("%: Pop chunk failed.", parserName_);
            return ANALYSIS_ERROR;
        }
        if (compactInfo->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM) {
            ERROR("%: The last %th data check failed.", parserName_, chunkProducer_->Size());
            delete compactInfo;
            continue;
        }
        compactData_.emplace_back(std::shared_ptr<MsprofCompactInfo>(compactInfo));
    }
    return ANALYSIS_OK;
}

int NodeBasicInfoParser::ProduceData()
{
    auto staticChunkProducer = MakeShared<ChunkGenerator>(sizeof(MsprofCompactInfo), path_, staticFilePrefix_);
    if (!staticChunkProducer || staticChunkProducer->ReadChunk() != ANALYSIS_OK) {
        ERROR("%: Read Chunk failed.", parserName_);
        return ANALYSIS_ERROR;
    }
    if (!Reserve(compactData_, chunkProducer_->Size() + staticChunkProducer->Size())) {
        ERROR("%: Reserve data failed", parserName_);
        return ANALYSIS_ERROR;
    }
    std::map<OpType, std::shared_ptr<ChunkGenerator>> producerMap = {
        {OpType::STATIC_OP, staticChunkProducer},
        {OpType::DYNAMIC_OP, chunkProducer_},
    };
    for (const auto &item : producerMap) {
        const auto &opState = static_cast<uint8_t>(item.first);
        auto &producer = item.second;
        while (!producer->Empty()) {
            auto compactInfo = ReinterpretConvert<MsprofCompactInfo*>(producer->Pop());
            if (!compactInfo) {
                ERROR("%: Pop chunk failed.", parserName_);
                return ANALYSIS_ERROR;
            }
            if (compactInfo->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM) {
                ERROR("%: The last %th data check failed with opState %.", parserName_, producer->Size(), opState);
                delete compactInfo;
                continue;
            }
            compactInfo->data.nodeBasicInfo.opState = opState;
            compactData_.emplace_back(std::shared_ptr<MsprofCompactInfo>(compactInfo));
        }
    }
    return ANALYSIS_OK;
}

int TaskTrackParser::ProduceData()
{
    const uint64_t flipTaskType = 98;
    const uint64_t maintenanceTaskType = 6;
    if (!Reserve(compactData_, chunkProducer_->Size())) {
        ERROR("%: Reserve data failed", parserName_);
        return ANALYSIS_ERROR;
    }
    while (!chunkProducer_->Empty()) {
        auto compactInfo = ReinterpretConvert<MsprofCompactInfo*>(chunkProducer_->Pop());
        if (!compactInfo) {
            ERROR("%: Pop chunk failed.", parserName_);
            return ANALYSIS_ERROR;
        }
        if (compactInfo->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM) {
            ERROR("%: The last %th data check failed.", parserName_, chunkProducer_->Size());
            delete compactInfo;
            continue;
        }
        if (compactInfo->data.runtimeTrack.taskType == flipTaskType) {
            auto flipTask = Flip::CreateFlipTask(compactInfo);
            delete compactInfo;
            if (!flipTask) {
                ERROR("FlipTask is null.");
                return ANALYSIS_ERROR;
            }
            flipTaskData_.emplace_back(flipTask);
            continue;
        }
        // 因为Maintenance task标记流销毁，device侧并不会上报，也没有更多的含义，所以无需关联和展示
        // 同时Maintenance task可能会在流销毁的flip task后面，影响batch id的计算，所以过滤掉
        if (compactInfo->data.runtimeTrack.taskType == maintenanceTaskType) {
            delete compactInfo;
            continue;
        }
        compactData_.emplace_back(std::shared_ptr<MsprofCompactInfo>(compactInfo));
    }
    Sort<MsprofCompactInfo, uint64_t, &MsprofCompactInfo::timeStamp>(compactData_);
    Sort<FlipTask, uint64_t, &FlipTask::timeStamp>(flipTaskData_);
    if (Context::GetInstance().IsAllExport()) {
        Flip::ComputeBatchId(compactData_, flipTaskData_);
    }
    return ANALYSIS_OK;
}
}  // namespace Cann
}  // namespace Host
}  // namespace Parser
}  // namespace Analysis
