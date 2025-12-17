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

#include "analysis/csrc/domain/services/parser/pmu/include/ffts_profile_parser.h"
#include "analysis/csrc/domain/services/parser/parser_error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"

namespace Analysis {

namespace Domain {

using namespace Infra;
using namespace Analysis::Utils;

#pragma pack(1)
struct FftsProfileHeader {
    uint16_t funcType : 6;
    uint16_t resv1 : 10;
    uint16_t resv2;
};
#pragma pack()

std::vector<std::string> FftsProfileParser::GetFilePattern()
{
    return this->filePrefix_;
}

uint32_t FftsProfileParser::GetTrunkSize()
{
    return Analysis::FFTS_PROFILE_STRUCT_SIZE;
}

uint32_t FftsProfileParser::ParseDataItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* data)
{
    if (binaryDataSize < sizeof(FftsProfileHeader)) {
        ERROR("The binaryDataSize is small than FftsProfileHeader");
        return ANALYSIS_ERROR;
    }
    FftsProfileHeader *header = ReinterpretConvert<FftsProfileHeader *>(binaryData);

    std::function<int(uint8_t *, uint32_t, uint8_t *)> parser =
            ParserItemFactory::GetParseItem(PMU_PARSER, header->funcType);
    if (parser == nullptr) {
        WARN("There is no Parser function to handle data! funcType is %", header->funcType);
        return ANALYSIS_OK;
    }
    int currentCnt = parser(binaryData, binaryDataSize, data);
    if (cnt_ == DEFAULT_CNT) {
        cnt_ = currentCnt;
        return ANALYSIS_OK;
    }
    if (currentCnt != cnt_ + 1 && cnt_ - currentCnt != VALID_CNT) {
        WARN("CNT verification failed. prevCnt: %; nowCnt: %", cnt_, currentCnt);
    }
    cnt_ = currentCnt;
    return ANALYSIS_OK;
}

uint32_t FftsProfileParser::ParseData(DataInventory &dataInventory, const Infra::Context &context)
{
    auto trunkSize = this->GetTrunkSize();
    auto structCount = this->binaryDataSize / trunkSize;
    INFO("FftsProfileData structCount is : %", structCount);
    if (!Utils::Resize(halUniData_, structCount)) {
        ERROR("Resize for FftsProfile data failed!");
        return ANALYSIS_ERROR;
    }
    int stat{ANALYSIS_OK};
    for (uint64_t i = 0; i < structCount; i++) {
        auto res = this->ParseDataItem(&this->binaryData[i * trunkSize], trunkSize,
                                       ReinterpretConvert<uint8_t *>(&this->halUniData_[i]));
        if (res != ANALYSIS_OK) {
            stat = ANALYSIS_ERROR;
            ERROR("FftsProfileData parse error in %th", i);
        }
    }
    INFO("Ffts_profile.data parser is completed, begin to save data into dataInventory!");
    std::shared_ptr<std::vector<HalPmuData>> data;
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HalPmuData>, ANALYSIS_ERROR, std::move(halUniData_));
    dataInventory.Inject(data);
    return stat;
}

REGISTER_PROCESS_SEQUENCE(FftsProfileParser, true);
REGISTER_PROCESS_SUPPORT_CHIP(FftsProfileParser, CHIP_V4_1_0);
}
}
