/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : fusion_op.cpp
 * Description        : 组合fusion_op数据
 * Author             : msprof team
 * Creation Date      : 2025/5/12
 * *****************************************************************************
 */

#include "analysis/csrc/application/summary/fusion_op_assembler.h"
#include <sstream>
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/domain/data_process/ai_task/hash_init_processor.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Utils;
using namespace Analysis::Domain;
using namespace Analysis::Viewer::Database;

FusionOpAssembler::FusionOpAssembler(const std::string &name, const std::string &profPath)
    : SummaryAssembler(name, profPath)
    {
        headers_ = {"Device_id", "Model Name", "Model ID", "Fusion Op", "Original Ops", "Memory Input(KB)",
                "Memory Output(KB)", "Memory Weight(KB)", "Memory Workspace(KB)", "Memory Total(KB)"};
    }


void FusionOpAssembler::GenerateFusionOp(std::vector<FusionOpInfo> &fusionOpData,
                                         std::vector<ModelName> &modelNameData,
                                         std::unordered_map<std::string, std::string> &hashMap)
{
    for (auto& name : modelNameData) {
        name.modelName = hashMap.count(name.modelName) > 0 ? hashMap[name.modelName] : name.modelName;
    }
    std::unordered_map<uint32_t, std::string> ids;
    for (auto& name : modelNameData) {
        ids[name.modelId] = name.modelName;
    }
    for (auto& data : fusionOpData) {
        data.fusionName = hashMap.count(data.fusionName) > 0 ? hashMap[data.fusionName] : data.fusionName;

        std::stringstream ss(data.opNames);
        std::string item;
        std::vector<std::string> items;
        while (std::getline(ss, item, ',')) {
            items.push_back(hashMap.count(item) ? hashMap[item] : item);
        }

        data.opNames = Join(items, ";");
        std::vector<std::string> row = {"host", ids[data.modelId], std::to_string(data.modelId), data.fusionName,
                                        data.opNames, data.memoryInput, data.memoryOutput, data.memoryWeight,
                                        data.memoryWorkspace, data.memoryTotal};
        res_.emplace_back(row);
    }
}

uint8_t FusionOpAssembler::AssembleData(DataInventory &dataInventory)
{
    auto fusionOpData = dataInventory.GetPtr<std::vector<FusionOpInfo>>();
    auto modelNameData = dataInventory.GetPtr<std::vector<ModelName>>();
    auto hashMap = dataInventory.GetPtr<GeHashMap>();
    if (fusionOpData == nullptr || modelNameData == nullptr || hashMap == nullptr) {
        WARN("fusion op data not exist.");
        return DATA_NOT_EXIST;
    }
    GenerateFusionOp(*fusionOpData, *modelNameData, *hashMap);
    std::string fileName = File::PathJoin({profPath_, OUTPUT_PATH, FUSION_OP_NAME});
    WriteToFile(fileName, {});
    return ASSEMBLE_SUCCESS;
}

}
}