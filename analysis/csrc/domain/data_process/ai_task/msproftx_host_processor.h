/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : msprofTx_host_processor.h
 * Description        : 处理host侧msprofTx表数据
 * Author             : msprof team
 * Creation Date      : 2024/8/5
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_MSPROFTX_HOST_PROCESSOR_H
#define ANALYSIS_DOMAIN_MSPROFTX_HOST_PROCESSOR_H

#include <functional>
#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/msprof_tx_host_data.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Domain {
// pid, tid, category, payload_type, message_type, payload_value, start, end, event_type, message
using OriMsprofTxHostData = std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, uint64_t,
                                        uint64_t, std::string, std::string>>;
// pid, tid, mark_id, start, end, event_type, message
using OriMsprofTxExHostData = std::vector<std::tuple<uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, std::string,
                                                     std::string>>;
class MsprofTxHostProcessor : public DataProcessor {
public:
    MsprofTxHostProcessor() = default;
    explicit MsprofTxHostProcessor(const std::string &profPath);
private:
    bool Process(DataInventory& dataInventory) override;
    void FormatTxData(const OriMsprofTxHostData &oriTxData, std::vector<MsprofTxHostData> &processedData,
                      Utils::SyscntConversionParams &params, Utils::ProfTimeRecord &record);
    void FormatTxExData(const OriMsprofTxExHostData &oriTxExData, std::vector<MsprofTxHostData> &processedData,
                        Utils::SyscntConversionParams &params, Utils::ProfTimeRecord &record);

    template<typename T>
    bool ProcessData(T &oriData, DBInfo& info, bool &existFlag,
                     std::function<T(const DBInfo &dbInfo, const std::string &dbPath)> func)
    {
        std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, info.dbName});
        if (!info.ConstructDBRunner(dbPath)) {
            return false;
        }
        auto status = CheckPathAndTable(dbPath, info);
        if (status == CHECK_FAILED) {
            return false;
        } else if (status == NOT_EXIST) {
            existFlag = true;
            return true;
        }
        oriData = func(info, dbPath);
        if (oriData.empty()) {
            WARN("original data is empty. DBPath is %", dbPath);
        }
        return true;
    }
};
}
}
#endif // ANALYSIS_DOMAIN_MSPROFTX_HOST_PROCESSOR_H
