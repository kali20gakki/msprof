/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : aicore_assembler.cpp
 * Description        : 组合aicore freq层数据
 * Author             : msprof team
 * Creation Date      : 2024/9/3
 * *****************************************************************************
 */

#include "analysis/csrc/application/timeline/aicore_freq_assembler.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Viewer::Database;
using namespace Analysis::Infra;
using namespace Analysis::Utils;

AicoreFreqAssembler::AicoreFreqAssembler()
    : JsonAssembler(PROCESS_AI_CORE_FREQ, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

std::unordered_map<uint16_t, uint32_t> AicoreFreqAssembler::GenerateFreqTrace(
    std::vector<AicoreFreqData> &freqData, uint32_t sortIndex, const std::string &profPath)
{
    std::unordered_map<uint16_t, uint32_t> pidMap;
    std::shared_ptr<CounterEvent> event;
    for (const auto &data : freqData) {
        auto pid =  GetDevicePid(pidMap, data.deviceId, profPath, sortIndex);
        MAKE_SHARED_RETURN_VALUE(event, CounterEvent, pidMap, pid, DEFAULT_TID,
                                 DivideByPowersOfTenWithPrecision(data.timestamp), "AI Core Freq");
        event->SetSeriesIValue("MHz", static_cast<uint64_t>(data.freq));
        res_.push_back(event);
    }
    return pidMap;
}

uint8_t AicoreFreqAssembler::AssembleData(DataInventory &dataInventory,
                                          JsonWriter &ostream, const std::string &profPath)
{
    INFO("Begin to assemble % data.", PROCESS_AI_CORE_FREQ);
    auto freqData = dataInventory.GetPtr<std::vector<AicoreFreqData>>();
    if (freqData == nullptr) {
        WARN("Can't get aicore freq Data from dataInventory");
        return DATA_NOT_EXIST;
    }
    auto layerInfo = GetLayerInfo(PROCESS_AI_CORE_FREQ);
    auto pidMap = GenerateFreqTrace(*freqData, layerInfo.sortIndex, profPath);
    if (res_.empty()) {
        ERROR("Can't Generate any aicore freq process data");
        return ASSEMBLE_FAILED;
    }
    GenerateHWMetaData(pidMap, layerInfo, res_);
    for (const auto &node : res_) {
        node->DumpJson(ostream);
    }
    // 为了让下一个写入的内容形成正确的JSON格式，需要补一个","
    ostream << ",";
    INFO("Assemble % data success.", PROCESS_AI_CORE_FREQ);
    return ASSEMBLE_SUCCESS;
}
}
}
