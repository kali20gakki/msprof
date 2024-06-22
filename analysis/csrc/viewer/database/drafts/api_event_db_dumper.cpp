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
const std::string GE_STEP_INFO_API_TYPE = "step_info";

ApiEventDBDumper::ApiEventDBDumper(const std::string &hostFilePath) : BaseDumper<ApiEventDBDumper>(
    hostFilePath, "ApiData")
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
        std::string itemId;
        if (trace->level == MSPROF_REPORT_MODEL_LEVEL ||
            (structType == GE_STEP_INFO_API_TYPE && trace->level == MSPROF_REPORT_NODE_LEVEL)) {
            // node层的step_info以及model层, 原来的itemId出现了60000， 60001等异常（预期为1）情况，所以使用structType作为判断依据
            itemId = std::to_string(trace->itemId);
        } else {
            itemId = (trace->itemId == 0) ? "0" : HashData::GetInstance().Get(trace->itemId);
        }
        if (trace->beginTime <= 0) {
            // 过滤开始时间小于0的数据
            WARN("struct_type: %, item_id: %, id: % begin time is %, it will be filtered out",
                 structType, itemId, id, trace->beginTime);
            continue;
        }
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