/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : api_event_dumper.cpp
 * Description        : ApiEventDumper实现
 * Author             : msprof team
 * Creation Date      : 2023/11/16
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/drafts/api_event_db_dumper.h"
#include "analysis/csrc/utils/file.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/parser/host/cann/hash_data.h"
#include "analysis/csrc/parser/host/cann/type_data.h"
#include "analysis/csrc/viewer/database/drafts/number_mapping.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Drafts {
namespace {
    const uint32_t TWO_BYTES = 16;
}
using HashData = Analysis::Parser::Host::Cann::HashData;
using TypeData = Analysis::Parser::Host::Cann::TypeData;

ApiEventDBDumper::ApiEventDBDumper(const std::string &hostFilePath) : BaseDumper<ApiEventDBDumper>(
        hostFilePath, "ApiEventData")
{
    MAKE_SHARED0_NO_OPERATION(database_, ApiEventDB);
}

EventData ApiEventDBDumper::GenerateData(const ApiData &apiTraces)
{
    EventData data;
    if (!Utils::Reserve(data, apiTraces.size())) {
        return data;
    }
    for (auto &traceEvent : apiTraces) {
        std::string structType;
        std::string id;
        auto trace = traceEvent->apiPtr;
        // 除了acl层数据，其他数据的id设置为"0"
        // 对于acl层数据，type这个字段，高16位表示类型，低16位表示API的hash，所以这里右移
        if (trace->level == MSPROF_REPORT_ACL_LEVEL) {
            structType = NumberMapping::Get(NumberMapping::MappingType::ACL_API_TAG,
                                            trace->type >> TWO_BYTES);
            id = TypeData::GetInstance().Get(trace->level, trace->type);
        } else {
            structType = TypeData::GetInstance().Get(trace->level, trace->type);
            id = "0";
        }
        std::string itemId = trace->itemId == 0 ? "0": HashData::GetInstance().Get(trace->itemId);
        data.emplace_back(structType, id,
                          NumberMapping::Get(NumberMapping::MappingType::LEVEL, trace->level),
                          trace->threadId,
                          itemId, trace->beginTime, trace->endTime,
                          traceEvent->id);
    }
    return data;
}
} // Drafts
} // Database
} // Viewer
} // Analysis