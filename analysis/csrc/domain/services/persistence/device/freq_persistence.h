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

#ifndef ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_FREQ_PERSISTENCE_H
#define ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_FREQ_PERSISTENCE_H

#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/domain/services/parser/freq/include/freq_parser.h"

namespace Analysis {
namespace Domain {
using namespace Infra;
using ProcessedDataFormat = std::vector<std::tuple<uint64_t, uint32_t>>;
class FreqPersistence : public Process {
private:
    ProcessedDataFormat GenerateFreqData(std::vector<HalFreqLpmData>& freqData);
    uint32_t ProcessEntry(DataInventory& dataInventory, const Context& context) override;
};
}
}
#endif // ANALYSIS_DOMAIN_SERVICES_PERSISTENCE_FREQ_PERSISTENCE_H
