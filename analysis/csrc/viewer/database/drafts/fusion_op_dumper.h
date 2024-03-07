/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : FusionOpDumper.h
 * Description        : FusionOp落盘
 * Author             : msprof team
 * Creation Date      : 2024/3/6
 * *****************************************************************************
 */
#ifndef ANALYSIS_CSRC_VIEWER_DATABASE_DRAFTS_FUSION_OP_DUMPER_H
#define ANALYSIS_CSRC_VIEWER_DATABASE_DRAFTS_FUSION_OP_DUMPER_H

#include "analysis/csrc/viewer/database/drafts/base_dumper.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Drafts {
using FusionOpsDumpData = std::tuple<std::string, std::string, uint32_t, uint64_t,
                                     std::string, uint32_t, std::string, std::string,
                                     std::string, std::string, std::string, std::string>;

class FusionOpDumper : public BaseDumper<FusionOpDumper> {
    using FusionOps = std::vector<std::shared_ptr<MsprofAdditionalInfo>>;
public:
    explicit FusionOpDumper(const std::string &hostFilePath);

    std::vector<FusionOpsDumpData> GenerateData(const FusionOps &fusionOps);
};
} // Drafts
} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_CSRC_VIEWER_DATABASE_DRAFTS_FUSION_OP_DUMPER_H