/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mc2_comm_info_processor.cpp
 * Description        : mc2_comm_info_processor，处理Mc2CommInfo表数据
 * Author             : msprof team
 * Creation Date      : 2024/11/18
 * *****************************************************************************
 */

#include "analysis/csrc/domain/data_process/ai_task/mc2_comm_info_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

Mc2CommInfoProcessor::Mc2CommInfoProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool Mc2CommInfoProcessor::Process(Analysis::Infra::DataInventory &dataInventory)
{
    DBInfo mc2DB("mc2_comm_info.db", "Mc2CommInfo");
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, mc2DB.dbName});
    if (!mc2DB.ConstructDBRunner(dbPath)) {
        return false;
    }
    // 并不是所有场景都有Mc2对应的streamId数据
    auto status = CheckPathAndTable(dbPath, mc2DB);
    if (status == CHECK_FAILED) {
        return false;
    } else if (status == NOT_EXIST) {
        return true;
    }
    auto mc2Data = LoadOriData(mc2DB, dbPath);
    if (mc2Data.empty()) {
        ERROR("LoadOriData failed, %.", PROCESSOR_MC2_COMM_INFO);
        return false;
    }
    if (!SaveToDataInventory<MC2CommInfoData>(std::move(mc2Data), dataInventory, PROCESSOR_MC2_COMM_INFO)) {
        ERROR("Save data failed, %.", PROCESSOR_MC2_COMM_INFO);
        return false;
    }
    return true;
}

std::vector<MC2CommInfoData> Mc2CommInfoProcessor::LoadOriData(const DBInfo &mc2DB, const std::string &dbPath)
{
    std::vector<MC2CommInfoData> res;
    OriMc2InfoData oriData;
    if (mc2DB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return res;
    }
    std::string sql = "SELECT aicpu_kfc_stream_id, comm_stream_ids FROM " + mc2DB.tableName;
    if (!mc2DB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Query mc2 comm info data failed , db path is %", dbPath);
        return res;
    }
    if (!Reserve(res, oriData.size())) {
        ERROR("Reserved for mc2 comm info data failed");
        return res;
    }
    MC2CommInfoData data;
    for (const auto &item: oriData) {
        std::tie(data.aiCpuKfcStreamId, data.commStreamIds) = item;
        res.push_back(data);
    }
    return res;
}
}
}
