/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : FusionOpDumper.cpp
 * Description        : FusionOp落盘
 * Author             : msprof team
 * Creation Date      : 2024/3/6
 * *****************************************************************************
 */
#include "collector/inc/toolchain/prof_common.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/parser/host/cann/hash_data.h"
#include "analysis/csrc/parser/host/cann/type_data.h"
#include "analysis/csrc/viewer/database/drafts/number_mapping.h"
#include "analysis/csrc/viewer/database/drafts/fusion_op_dumper.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Drafts {
using namespace Analysis::Parser::Host::Cann;

FusionOpDumper::FusionOpDumper(const std::string &hostFilePath)
    : BaseDumper<FusionOpDumper>(hostFilePath, "FusionOPInfo")
{
    MAKE_SHARED0_NO_OPERATION(database_, FusionOpInfoDB);
}
std::vector<FusionOpsDumpData> FusionOpDumper::GenerateData(const FusionOps &fusionOps)
{
    std::vector<FusionOpsDumpData> res;
    if (!Utils::Reserve(res, fusionOps.size())) {
        return res;
    }
    for (auto &fusionOp: fusionOps) {
        auto fusionStruct = Utils::ReinterpretConvert<ProfFusionOpInfo *>(fusionOp->data);
        std::vector<std::string> fusionOpNames;
        if (!Utils::Reserve(fusionOpNames, fusionStruct->fusionOpNum)) {
            break;
        }
        for (uint32_t i = 0; i < fusionStruct->fusionOpNum; i++) {
            fusionOpNames.emplace_back(HashData::GetInstance().Get(fusionStruct->fusionOpId[i]));
        }
        res.emplace_back(
            NumberMapping::Get(NumberMapping::MappingType::LEVEL, fusionOp->level),
            TypeData::GetInstance().Get(fusionOp->level, fusionOp->type),
            fusionOp->threadId,
            fusionOp->timeStamp,
            HashData::GetInstance().Get(fusionStruct->opName),
            fusionStruct->fusionOpNum,
            std::to_string(fusionStruct->inputMemsize),
            std::to_string(fusionStruct->outputMemsize),
            std::to_string(fusionStruct->weightMemSize),
            std::to_string(fusionStruct->workspaceMemSize),
            std::to_string(fusionStruct->totalMemSize),
            Utils::Join(fusionOpNames, ",")
        );
    }
    return res;
}
} // Drafts
} // Database
} // Viewer
} // Analysis
