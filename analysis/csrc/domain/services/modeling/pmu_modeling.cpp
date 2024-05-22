/* ******************************************************************************
						版权所有 (c) 华为技术有限公司 2024-2024
						Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pmu_modeling.h
 * Description        : pmu modeling 处理流程类
 * Author             : msprof team
 * Creation Date      : 2024/5/11
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/modeling/include/pmu_modeling.h"
#include <algorithm>
#include "analysis/csrc/domain/services/modeling/batch_id/batch_id.h"
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/domain/services/parser/pmu/include/ffts_profile_parser.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/domain/services/parser/track/include/ts_track_parser.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;
using namespace Analysis::Infra;
void PmuModeling::SplitDataByType(std::vector<HalPmuData>& originPmuData, std::vector<HalTrackData>& flipTrack)
{
    auto pmuData = GroupByStream<HalPmuData, HalPmuType>(originPmuData);
    auto flipData = GroupByStream<HalTrackData, HalTrackType>(flipTrack);

    auto fftsIt = pmuData.find(HalPmuType::PMU);
    if (fftsIt != pmuData.end()) {
        pmu_.swap(fftsIt->second);
    }

    auto flipIt = flipData.find(HalTrackType::TS_TASK_FLIP);
    if (flipIt != flipData.end()) {
        flipGroup_.swap(flipIt->second);
    }
}

void PmuModeling::GenerateBatchId()
{
    for (auto& pmuVec : pmu_) {
        auto it = flipGroup_.find(pmuVec.first);
        if (it != flipGroup_.end()) {
            std::sort(pmuVec.second.begin(), pmuVec.second.end(), cmp<HalPmuData>);
            std::sort(it->second.begin(), it->second.end(), cmp<HalTrackData>);
            ModelingComputeBatchIdBinary(ReinterpretConvert<HalUniData**>(pmuVec.second.data()), pmuVec.second.size(),
                                         ReinterpretConvert<HalUniData**>(it->second.data()), it->second.size());
        }
    }
    INFO("PMU modeling has done!");
}

uint32_t PmuModeling::ProcessEntry(Infra::DataInventory& dataInventory, const Infra::Context&)
{
    auto originPmuData = dataInventory.GetPtr<std::vector<HalPmuData>>();
    auto flipTrack = dataInventory.GetPtr<std::vector<HalTrackData>>();
    if (!originPmuData) {
        INFO("There is no PMU data, can't supplement batchId");
        return Analysis::ANALYSIS_OK;
    }
    SplitDataByType(*originPmuData, *flipTrack);
    GenerateBatchId();
    return Analysis::ANALYSIS_OK;
}

REGISTER_PROCESS_SEQUENCE(PmuModeling, true, FftsProfileParser, TsTrackParser);
REGISTER_PROCESS_DEPENDENT_DATA(PmuModeling, std::vector<HalPmuData>, std::vector<HalTrackData>);
REGISTER_PROCESS_SUPPORT_CHIP(PmuModeling, CHIP_V4_1_0);
}
}