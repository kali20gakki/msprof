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

#ifndef ANALYSIS_DOMAIN_MODEL_NAME_PROCESSOR_H
#define ANALYSIS_DOMAIN_MODEL_NAME_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/model_name_data.h"

namespace Analysis {
namespace Domain {
// model_id, model_name
using OriModelNameDataFormat = std::vector<std::tuple<uint64_t, std::string>>;
class ModelNameProcessor : public DataProcessor {
public:
    ModelNameProcessor() = default;
    explicit ModelNameProcessor(const std::string &profPath);

private:
    bool Process(DataInventory& dataInventory) override;
    OriModelNameDataFormat LoadData(const DBInfo &modelNameDB, const std::string &dbPath);
    std::vector<ModelName> FormatData(const OriModelNameDataFormat &oriData);
};
}
}
#endif // ANALYSIS_DOMAIN_MODEL_NAME_PROCESSOR_H