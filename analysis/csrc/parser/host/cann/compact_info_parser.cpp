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

#include "compact_info_parser.h"

#include "prof_common.h"
#include "chunk_generator.h"
#include "log.h"
#include "flip.h"
#include "type_data.h"
#include "context.h"
#include "utils.h"

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
    chunkConsumers_ = {
        {typeid(MsprofCompactInfo).hash_code(), chunkProducer_},
    };
}

int CompactInfoParser::ConsumeChunk(std::shared_ptr<void> &chunk, const std::shared_ptr<ChunkGenerator> &chunkConsumer)
{
    chunk = chunkConsumer->Pop();
    if (!chunk) {
        ERROR("chunk is null.");
        return ANALYSIS_ERROR;
    }
    // 避免FlipTask数据进行5A5A校验
    if (chunkConsumer->GetChunkSize() == sizeof(FlipTask)) {
        return ANALYSIS_OK;
    }
    auto data = ChunkGenerator::ToObj<MsprofCompactInfo>(chunk);
    if (data->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM) {
        ERROR("Check 5A5A failed in place &.", chunkConsumer->Size());
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

TaskTrackParser::TaskTrackParser(const std::string &path) : CompactInfoParser(path)
{
    chunkProducer_ = MakeShared<ChunkGenerator>(sizeof(MsprofCompactInfo), path_, filePrefix_);
    chunkConsumers_ = {
        {typeid(MsprofCompactInfo).hash_code(), MakeShared<ChunkGenerator>(sizeof(MsprofCompactInfo))},
        {typeid(FlipTask).hash_code(), MakeShared<ChunkGenerator>(sizeof(FlipTask))},
    };
}

int TaskTrackParser::ProduceChunk()
{
    if (chunkProducer_->ReadChunk() != ANALYSIS_OK) {
        ERROR("Read Chunk failed.");
        return ANALYSIS_ERROR;
    }
    const std::string taskFlip = "FLIP_TASK";
    const std::string taskMaintenance = "MAINTENANCE";
    auto &taskTrackConsumer = chunkConsumers_[typeid(MsprofCompactInfo).hash_code()];
    auto &flipTaskConsumer = chunkConsumers_[typeid(FlipTask).hash_code()];
    std::vector<std::shared_ptr<MsprofCompactInfo>> taskTracks;
    std::vector<std::shared_ptr<FlipTask>> flipTasks;
    while (!chunkProducer_->ChunkEmpty()) {
        auto chunk = ChunkGenerator::ToObj<MsprofCompactInfo>(chunkProducer_->Pop());
        if (!chunk) {
            ERROR("Chunk is null.");
            continue;
        }
        if (TypeData::GetInstance().Get(chunk->level, chunk->data.runtimeTrack.taskType) == taskFlip) {
            std::shared_ptr<FlipTask> flipTask = Flip::CreateFlipTask(chunk);
            if (!flipTask) {
                ERROR("FlipTask make_shared failed.");
                return ANALYSIS_ERROR;
            }
            flipTasks.emplace_back(flipTask);
            flipTaskConsumer->Push(ChunkGenerator::ToChunk<FlipTask>(flipTask));
            continue;
        }
        if (TypeData::GetInstance().Get(chunk->level, chunk->data.runtimeTrack.taskType) == taskMaintenance) {
            continue;
        }
        taskTracks.emplace_back(chunk);
    }
    if (Context::GetInstance().IsAllExport()) {
        Flip::ComputeBatchId(taskTracks, flipTasks);
    }
    for (const auto &task : taskTracks) {
        taskTrackConsumer->Push(ChunkGenerator::ToChunk<MsprofCompactInfo>(task));
    }
    return ANALYSIS_OK;
}
}  // namespace Cann
}  // namespace Host
}  // namespace Parser
}  // namespace Analysis
