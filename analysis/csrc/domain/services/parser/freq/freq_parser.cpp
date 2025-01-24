/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : freq_parser.cpp
 * Description        : freq_lpm类型二进制数据解析
 * Author             : msprof team
 * Creation Date      : 2024/5/15
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/parser/freq//include/freq_parser.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/domain/services/parser/parser_item/freq_lpm_parser_item.h"

namespace Analysis {
namespace Domain {

using namespace Infra;
using namespace Analysis::Utils;

std::vector<std::string> FreqParser::GetFilePattern()
{
    return this->filePrefix_;
}

uint32_t FreqParser::GetTrunkSize()
{
    return Analysis::FREQ_STRUCT_SIZE;
}

uint32_t FreqParser::ParseDataItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* data)
{
    std::function<int(uint8_t *, uint32_t, uint8_t *)> parser =
            ParserItemFactory::GetParseItem(FREQ_PARSER, DEFAULT_FREQ_LPM);
    if (parser == nullptr) {
        ERROR("There is no Parser function to handle data! funcType is %", DEFAULT_FREQ_LPM);
        return ANALYSIS_ERROR;
    }
    parser(binaryData, binaryDataSize, data);
    return ANALYSIS_OK;
}

uint32_t FreqParser::ParseData(DataInventory &dataInventory, const Infra::Context &context)
{
    HalFreqLpmData initData;
    DeviceInfo deviceInfo;
    DeviceStartLog deviceStart;
    std::vector<HalFreqLpmData> lpmDataS;
    try {
        const DeviceContext &deviceContext = dynamic_cast<const DeviceContext&>(context);
        deviceContext.Getter(deviceStart);
        deviceContext.Getter(deviceInfo);
        initData.sysCnt = deviceStart.cntVct;
        initData.freq = deviceInfo.aicFrequency;
    } catch (const std::bad_cast& ex) {
        ERROR("cast context to deviceContext fail in %, .what(): %", std::string(__FUNCTION__), std::string(ex.what()));
    }
    lpmDataS.emplace_back(std::move(initData));
    auto trunkSize = this->GetTrunkSize();
    auto structCount = this->binaryDataSize / trunkSize;
    INFO("FreqProfileData structCount is : %", structCount);
    if (!Utils::Resize(halUniData_, structCount)) {
        ERROR("Resize for FreqProfile data failed!");
        return ANALYSIS_ERROR;
    }
    if (structCount == 0) {
        lpmDataS.clear();  // structCount为0说明没变频数据，不需要落盘，直接使用配置文件频率即可
    }
    int stat{ANALYSIS_OK};
    for (uint64_t i = 0; i < structCount; i++) {
        auto res = this->ParseDataItem(&this->binaryData[i * trunkSize], trunkSize,
                                       ReinterpretConvert<uint8_t *>(&this->halUniData_[i]));
        if (res != ANALYSIS_OK) {
            stat = ANALYSIS_ERROR;
            ERROR("FreqProfileData parse error in %th", i);
        }
    }
    // 根据device启动时间过滤有效频率数据
    for (const auto& freqData : halUniData_) {
        for (uint64_t i = 0; i < freqData.count; ++i) {
            auto freqLpmData = freqData.freqLpmDataS[i];
            if (freqLpmData.sysCnt < deviceStart.cntVct) {
                lpmDataS[0].freq = freqLpmData.freq;
            } else {
                lpmDataS.emplace_back(std::move(freqLpmData));
            }
        }
    }
    INFO("Freq.data parser is completed, begin to save data into dataInventory!");
    std::shared_ptr<std::vector<HalFreqLpmData>> data;
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HalFreqLpmData>, ANALYSIS_ERROR, std::move(lpmDataS));
    dataInventory.Inject(data);
    return stat;
}

REGISTER_PROCESS_SEQUENCE(FreqParser, true);
REGISTER_PROCESS_SUPPORT_CHIP(FreqParser, CHIP_V4_1_0, CHIP_V1_1_1);
}
}
