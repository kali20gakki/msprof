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

#ifndef ANALYSIS_APPLICATION_SUMMARY_STEP_TRACE_ASSEMBLER_H
#define ANALYSIS_APPLICATION_SUMMARY_STEP_TRACE_ASSEMBLER_H


#include <utility>
#include "analysis/csrc/application/summary/summary_assembler.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/step_trace_data.h"


namespace Analysis {
namespace Application {
using namespace Analysis::Infra;
using namespace Analysis::Domain;

struct TraceId {
    // trace and all reduce matching needs model_id and iteration_end
    TraceId() = default;

    TraceId(uint32_t _modelId, uint64_t _iterEnd)
        : modelId(_modelId), iterEnd(_iterEnd) {}

    uint32_t modelId = UINT32_MAX;
    uint64_t iterEnd = UINT64_MAX;

bool operator==(const TraceId &other) const
{
    // Compare all member variables; return true if they are equal, otherwise return false
    return modelId == other.modelId && iterEnd == other.iterEnd;
}

bool operator<(const TraceId &other) const
{
    return std::tie(modelId, iterEnd) < std::tie(other.modelId, other.iterEnd);
}
};


class SummaryStepTraceAssembler : public SummaryAssembler {
public:
    SummaryStepTraceAssembler() = default;
    SummaryStepTraceAssembler(const std::string &name, const std::string &profPath);

private:
    uint8_t AssembleData(DataInventory &dataInventory);
    void FormatAllReduceData(const std::vector<AllReduceData> &allReduceData);
    void AssembleStepTraceData(const std::vector<TrainTraceData> &trainTraceData);
    void AddAllReduceHeaders();

private:
    std::map<TraceId, std::vector<std::pair<std::string, std::string>>> formatedAllReduceData_;
    uint64_t allReduceGroupMaxSize_ = 0;
};

}
}

#endif // ANALYSIS_APPLICATION_SUMMARY_STEP_TRACE_ASSEMBLER_H
