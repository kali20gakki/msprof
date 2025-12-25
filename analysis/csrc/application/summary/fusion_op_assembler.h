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

#ifndef ANALYSIS_APPLICATION_SUMMARY_FUSION_OP_ASSEMBLE_H
#define ANALYSIS_APPLICATION_SUMMARY_FUSION_OP_ASSEMBLE_H

#include <cstdint>
#include <unordered_map>
#include "analysis/csrc/application/summary/summary_assembler.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/fusion_op_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/model_name_data.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Infra;
using namespace Analysis::Domain;
class FusionOpAssembler : public SummaryAssembler {
public:
    FusionOpAssembler() = default;
    FusionOpAssembler(const std::string &name, const std::string &profPath);
private:
    uint8_t AssembleData(DataInventory &dataInventory);
    void GenerateFusionOp(std::vector<FusionOpInfo> &fusionOpData,
                        std::vector<ModelName> &modelNameData, std::unordered_map<std::string, std::string> &hashMap);
};
}
}
#endif // ANALYSIS_APPLICATION_SUMMARY_FUSION_OP_ASSEMBLE_H