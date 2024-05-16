/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ts_track_parser.cpp
 * Description        : tsTrack解析器
 * Author             : msprof team
 * Creation Date      : 2024/4/28
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/parser/track/include/ts_track_parser.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/utils/utils.h"

namespace Analysis {
namespace Domain {
using namespace Infra;
using namespace Utils;

#pragma pack(1)
struct TsTrackHeader {
    uint8_t resv;
    uint8_t funcType;
};
#pragma pack()

std::vector<std::string> TsTrackParser::GetFilePattern()
{
    return filePrefix_;
}

uint32_t TsTrackParser::GetTrunkSize()
{
    return TS_TRACK_STRUCT_SIZE;
}

uint32_t TsTrackParser::GetNoFileCode()
{
    return PARSER_GET_TS_TRACK_FILE_ERROR;
}

uint32_t TsTrackParser::ParseDataItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *data)
{
    if (binaryDataSize < sizeof(TsTrackHeader)) {
        ERROR("The binaryDataSize is small than ParseDataItem");
        return ANALYSIS_ERROR;
    }
    auto *header = ReinterpretConvert<TsTrackHeader *>(binaryData);

    std::function<int(uint8_t *, uint32_t, uint8_t *)> parser = GetParseItem(TRACK_PARSER, header->funcType);
    if (parser != nullptr) {
        parser(binaryData, binaryDataSize, data);
        return ANALYSIS_OK;
    }
    ERROR("There is no Parser function to handle data! functype is %", header->funcType);
    return ANALYSIS_ERROR;
}

uint32_t TsTrackParser::ParseData(DataInventory& dataInventory)
{
    auto trunkSize = this->GetTrunkSize();
    auto structCount = this->binaryDataSize / trunkSize;
    int stat{ANALYSIS_OK};
    INFO("TsTrack structCount: %", structCount);

    halUniData_.resize(structCount);

    for (uint64_t i = 0; i < structCount; i++) {
        auto *dataPoint = ReinterpretConvert<uint8_t *>(&this->halUniData_[i]);
        if (this->ParseDataItem(&this->binaryData[i * trunkSize], trunkSize, dataPoint) == ANALYSIS_ERROR) {
            ERROR("parse ts track data failed, total of % pieces of data are parsed", i);
            stat = ANALYSIS_ERROR;
        }
    }
    std::shared_ptr<std::vector<HalTrackData>> data;
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HalTrackData>, ANALYSIS_ERROR, std::move(halUniData_));
    dataInventory.Inject(data);

    return stat;
}

REGISTER_PROCESS_SEQUENCE(TsTrackParser, true);
REGISTER_PROCESS_SUPPORT_CHIP(TsTrackParser, CHIP_ID_ALL);
}
}