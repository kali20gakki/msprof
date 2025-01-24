/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : api_event_dumper.h
 * Description        : ApiEventDumper定义
 * Author             : msprof team
 * Creation Date      : 2023/11/30
 * *****************************************************************************
 */


#ifndef ANALYSIS_VIEWER_DATABASE_DRAFTS_API_EVENT_DB_DUMPER_H
#define ANALYSIS_VIEWER_DATABASE_DRAFTS_API_EVENT_DB_DUMPER_H

#include <thread>
#include <unordered_map>
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/domain/services/persistence/host/base_dumper.h"
#include "analysis/csrc/domain/entities/tree/include/event.h"

namespace Analysis {
namespace Domain {
using EventData = std::vector<std::tuple<std::string, std::string, std::string, uint32_t, std::string,
        uint64_t, uint64_t, uint32_t>>;

class ApiEventDBDumper final : public BaseDumper<ApiEventDBDumper> {
    using ApiData = std::vector<std::shared_ptr<Domain::Event>>;
public:
    explicit ApiEventDBDumper(const std::string &hostFilePath);
    EventData GenerateData(const ApiData &apiTraces);
};
} // Analysis
} // Drafts
#endif // ANALYSIS_VIEWER_DATABASE_DRAFTS_API_EVENT_DB_DUMPER_H