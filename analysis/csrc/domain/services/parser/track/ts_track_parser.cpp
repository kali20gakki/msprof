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

#include "analysis/csrc/domain/services/parser/track/include/ts_track_parser.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"

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

uint32_t TsTrackParser::ParseDataItem(uint8_t *binaryData, uint32_t binaryDataSize, uint8_t *data)
{
    if (binaryDataSize < sizeof(TsTrackHeader)) {
        ERROR("The binaryDataSize is small than ParseDataItem");
        return ANALYSIS_ERROR;
    }
    auto *header = ReinterpretConvert<TsTrackHeader *>(binaryData);

    std::function<int(uint8_t *, uint32_t, uint8_t *, uint16_t)> parser =
            ParserItemFactory::GetParseItem(TRACK_PARSER, header->funcType);
    if (parser != nullptr) {
        parser(binaryData, binaryDataSize, data, 0);
        return ANALYSIS_OK;
    }
    ERROR("There is no Parser function to handle data! functype is %", header->funcType);
    return ANALYSIS_ERROR;
}

uint32_t TsTrackParser::ParseData(DataInventory& dataInventory, const Infra::Context &context)
{
    auto trunkSize = this->GetTrunkSize();
    auto structCount = this->binaryDataSize / trunkSize;
    int stat{ANALYSIS_OK};
    INFO("TsTrack structCount: %", structCount);

    if (!Utils::Resize(halUniData_, structCount)) {
        ERROR("Resize for TsTrack data failed!");
        return ANALYSIS_ERROR;
    }

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