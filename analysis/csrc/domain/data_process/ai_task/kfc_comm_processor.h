/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : kfc_comm_processor.cpp
 * Description        : kfc通信数据导出流程
 * Author             : msprof team
 * Creation Date      : 2024/8/30
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_KFC_COMM_PROCESSOR_H
#define ANALYSIS_DOMAIN_KFC_COMM_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/kfc_turn_data.h"
#include "analysis/csrc/domain/data_process/ai_task/communication_info_processor.h"

namespace Analysis {
namespace Domain {
class KfcCommProcessor : public CommunicationInfoProcessor {
public:
    KfcCommProcessor() = default;
    explicit KfcCommProcessor(const std::string &profPath);

private:
    static bool ConvertTaskData(std::vector<KfcTaskData> &resTask, const std::vector<CommunicationTaskData> &taskData);
    static bool ConvertOpData(std::vector<KfcOpData> &resOp, const std::vector<CommunicationOpData> &opData);
    bool Process(DataInventory& dataInventory) override;
    OriOpDataFormat LoadOpData(const DBInfo &hcclSingleDeviceDB) override;
    OriTaskDataFormat LoadTaskData(const DBInfo& hcclSingleDeviceDB) override;
    bool SaveData(std::vector<KfcTaskData> &resTask, std::vector<KfcOpData> &resOp, DataInventory& dataInventory);
};
}
}


#endif // ANALYSIS_DOMAIN_KFC_COMM_PROCESSOR_H
