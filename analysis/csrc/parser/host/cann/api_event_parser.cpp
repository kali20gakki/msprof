/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : api_event_parser.cpp
 * Description        : api event数据解析
 * Author             : msprof team
 * Creation Date      : 2023/11/9
 * *****************************************************************************
 */

#include "analysis/csrc/parser/host/cann/api_event_parser.h"

#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/parser/chunk_generator.h"
#include "analysis/csrc/utils/utils.h"

namespace Analysis {
namespace Parser {
namespace Host {
namespace Cann {
using namespace Analysis::Utils;
namespace {
std::shared_ptr<MsprofApi> CreateMsprofApi(const MsprofEvent *startEvent, const MsprofEvent *endEvent)
{
    if (!startEvent || !endEvent) {
        ERROR("Event data is null.");
        return nullptr;
    }
    std::shared_ptr<MsprofApi> apiData;
    MAKE_SHARED0_RETURN_VALUE(apiData, MsprofApi, nullptr);

    apiData->level = startEvent->level;
    apiData->type = startEvent->type;
    apiData->threadId = startEvent->threadId;
    apiData->reserve = startEvent->requestId;
    apiData->beginTime = startEvent->timeStamp;
    apiData->endTime = endEvent->timeStamp;
    apiData->itemId = startEvent->itemId;
    return apiData;
}
}  // namespace

ApiEventParser::ApiEventParser(const std::string &path) : BaseParser(path, "ApiEventParser")
{
    MAKE_SHARED_NO_OPERATION(chunkProducer_, ChunkGenerator, sizeof(MsprofApi), path_, filePrefix_);
}

template<>
std::vector<std::shared_ptr<MsprofApi>> ApiEventParser::GetData()
{
    return apiData_;
}

template<>
std::vector<std::shared_ptr<MsprofEvent>> ApiEventParser::GetData()
{
    return eventData_;
}

int ApiEventParser::ProduceData()
{
    std::map<std::tuple<uint16_t, uint32_t, uint32_t, uint32_t, uint64_t>, MsprofEvent*> startEventMap;
    if (!Reserve(apiData_, chunkProducer_->Size())) {
        ERROR("%: Reserve data failed", parserName_);
        return ANALYSIS_ERROR;
    }
    while (!chunkProducer_->Empty()) {
        auto event = ReinterpretConvert<MsprofEvent*>(chunkProducer_->Pop());
        if (!event) {
            ERROR("%: Pop chunk failed.", parserName_);
            return ANALYSIS_ERROR;
        }
        if (event->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM) {
            ERROR("%: The last %th data check failed.", parserName_, chunkProducer_->Size());
            delete event;
            continue;
        }
        if (event->reserve != MSPROF_EVENT_FLAG) {
            // api data
            apiData_.emplace_back(std::shared_ptr<MsprofApi>(ReinterpretConvert<MsprofApi*>(event)));
            continue;
        }
        // event data，根据level, type, threadId, requestId, itemId合并start event和end event
        auto key = std::make_tuple(event->level, event->type, event->threadId, event->requestId, event->itemId);
        auto iter = startEventMap.find(key);
        if (iter == startEventMap.end()) {
            startEventMap[key] = event;
            continue;
        }
        auto startEvent = iter->second;
        auto apiData = CreateMsprofApi(startEvent, event);
        startEventMap.erase(iter);
        delete startEvent;
        delete event;
        if (!apiData) {
            ERROR("Api data is null.");
            return ANALYSIS_ERROR;
        }
        apiData_.emplace_back(apiData);
    }
    if (!startEventMap.empty()) {
        ERROR("There is remaining start event.");
    }
    return ANALYSIS_OK;
}
}  // namespace Cann
}  // namespace Host
}  // namespace Parser
}  // namespace Analysis
