/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : model_name_db_dumper.h
 * Description        : modelname落盘
 * Author             : msprof team
 * Creation Date      : 2024/3/7
 * *****************************************************************************
 */
#ifndef ANALYSIS_CSRC_VIEWER_DATABASE_MODEL_NAME_DB_DUMPER_H
#define ANALYSIS_CSRC_VIEWER_DATABASE_MODEL_NAME_DB_DUMPER_H

#include "collector/inc/toolchain/prof_common.h"
#include "analysis/csrc/domain/services/persistence/host/base_dumper.h"

namespace Analysis {
namespace Domain {
using ModelNameData = std::vector<std::tuple<uint32_t, std::string>>;
class ModelNameDBDumper : public BaseDumper<ModelNameDBDumper> {
    using GraphIdMaps = std::vector<std::shared_ptr<MsprofAdditionalInfo>>;
public:
    explicit ModelNameDBDumper(const std::string &hostFilePath);
    ModelNameData GenerateData(const GraphIdMaps &graphIdMaps);
};
} // Domain
} // Analysis

#endif // ANALYSIS_CSRC_VIEWER_DATABASE_MODEL_NAME_DB_DUMPER_H
