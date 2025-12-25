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

#ifndef ANALYSIS_DOMAIN_SERVICES_PARSER_FREQ_FREQ_PARSER_H
#define ANALYSIS_DOMAIN_SERVICES_PARSER_FREQ_FREQ_PARSER_H

#include <vector>
#include "analysis/csrc/domain/entities/hal/include/hal_freq.h"
#include "analysis/csrc/domain/services/parser/parser.h"

namespace Analysis {
namespace Domain {
class FreqParser : public Parser {
private:
    uint32_t ParseDataItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* data);
    std::vector<std::string> GetFilePattern() override;
    uint32_t GetTrunkSize() override;
    uint32_t ParseData(Infra::DataInventory &dataInventory, const Infra::Context &context) override;
private:
    std::vector<HalFreqData> halUniData_;
    std::vector<std::string> filePrefix_{"lpmFreqConv."};
};
}
}

#endif // ANALYSIS_DOMAIN_SERVICES_PARSER_FREQ_FREQ_PARSER_H
