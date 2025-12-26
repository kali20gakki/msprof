/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

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
    bool ProcessSingleDevice(CommunicationData& communicationData, const std::string& devicePath,
                             std::vector<KfcTaskData>& resTask, std::vector<KfcOpData>& resOp);
};
}
}


#endif // ANALYSIS_DOMAIN_KFC_COMM_PROCESSOR_H
