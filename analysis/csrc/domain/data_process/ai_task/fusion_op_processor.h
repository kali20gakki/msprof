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

#ifndef ANALYSIS_DOMAIN_FUSION_OP_PROCESSOR_H
#define ANALYSIS_DOMAIN_FUSION_OP_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/fusion_op_data.h"

namespace Analysis {
namespace Domain {
// model_id, fusion_name, fusion_op_nums, op_names, memory_input,
// memory_output, memory_weight, memory_workspace, memory_total
using OriFusionOpDataFormat = std::vector<std::tuple<uint64_t, std::string, uint64_t, std::string,
        std::string, std::string, std::string, std::string, std::string>>;

class FusionOpProcessor : public DataProcessor {
public:
    FusionOpProcessor() = default;
    explicit FusionOpProcessor(const std::string &profPath);

private:
    bool Process(DataInventory& dataInventory) override;
    OriFusionOpDataFormat LoadData(const DBInfo &fusionOpDB, const std::string &dbPath);
    std::vector<FusionOpInfo> FormatData(const OriFusionOpDataFormat &oriData);
};
}
}
#endif // ANALYSIS_DOMAIN_FUSION_OP_PROCESSOR_H