/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : mc2_comm_info_processor.h
 * Description        : 处理Mc2CommInfo表数据
 * Author             : msprof team
 * Creation Date      : 2024/11/18
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_MC2_COMM_INFO_PROCESSOR_H
#define ANALYSIS_DOMAIN_MC2_COMM_INFO_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/mc2_comm_info_data.h"

namespace Analysis {
namespace Domain {
// "aicpu_kfc_stream_id", "comm_stream_ids"
using OriMc2InfoData = std::vector<std::tuple<uint16_t, std::string>>;

class Mc2CommInfoProcessor : public DataProcessor {
public:
    Mc2CommInfoProcessor() = default;
    explicit Mc2CommInfoProcessor(const std::string &profPath);
private:
    bool Process(DataInventory& dataInventory) override;
    std::vector<MC2CommInfoData> LoadOriData(const DBInfo &mc2DB, const std::string &dbPath);
};
}
}
#endif // ANALYSIS_DOMAIN_MC2_COMM_INFO_PROCESSOR_H
