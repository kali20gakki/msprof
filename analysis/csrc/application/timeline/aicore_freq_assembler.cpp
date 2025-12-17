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
