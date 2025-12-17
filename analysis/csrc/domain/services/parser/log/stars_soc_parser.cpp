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

#include "analysis/csrc/domain/services/parser/log/include/stars_soc_parser.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/services/parser/parser_item_factory.h"
#include "analysis/csrc/infrastructure/resource/binary_struct_info.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"

namespace Analysis {
namespace Domain {
using namespace Infra;
using namespace Utils;

#pragma pack(1)
struct StarsSocHeader {
    uint16_t funcType : 6;
    uint16_t resv : 4;
    uint16_t taskType : 6;
};
#pragma pack()

std::vector<std::string> StarsSocParser::GetFilePattern()
{
    return filePrefix_;
}

uint32_t StarsSocParser::GetTrunkSize()
{
    return STARS_SOC_STRUCT_SIZE;
}

uint32_t StarsSocParser::ParseDataItem(uint8_t* binaryData, uint32_t binaryDataSize, uint8_t* data)
{
    if (binaryDataSize < sizeof(StarsSocHeader)) {
        ERROR("The binaryDataSize is small than StarsSocHeader");
        return ANALYSIS_ERROR;
    }
    auto *header = ReinterpretConvert<StarsSocHeader *>(binaryData);

    std::function<int(uint8_t *, uint32_t, uint8_t *)> parser =
            ParserItemFactory::GetParseItem(LOG_PARSER, header->funcType);
    if (parser == nullptr) {
        WARN("There is no Parser function to handle data! functype is %", header->funcType);
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

uint32_t StarsSocParser::ParseData(DataInventory &dataInventory, const Infra::Context &context)
{
    auto trunkSize = this->GetTrunkSize();
    auto structCount = this->binaryDataSize / trunkSize;
    INFO("StarsSoc structCount: %", structCount);
    int stat{ANALYSIS_OK};
    if (!Utils::Resize(halUniData_, structCount)) {
        ERROR("Resize for StarsSoc data failed!");
        return ANALYSIS_ERROR;
    }
    if (structCount < MIN_COUNT && structCount != 0) {
        ERROR("The stars_soc data volume is less than 2");
        stat = ANALYSIS_ERROR;
    }
    for (uint64_t i = 0; i < structCount; i++) {
        if (this->ParseDataItem(&this->binaryData[i * trunkSize], trunkSize,
                                ReinterpretConvert<uint8_t *>(&this->halUniData_[i])) == ANALYSIS_ERROR) {
            ERROR("parse data failed, total of % pieces of data are parsed", i);
            stat = ANALYSIS_ERROR;
        }
    }
    std::shared_ptr<std::vector<HalLogData>> data;
    MAKE_SHARED_RETURN_VALUE(data, std::vector<HalLogData>, ANALYSIS_ERROR, std::move(halUniData_));
    dataInventory.Inject(data);
    return stat;
}

REGISTER_PROCESS_SEQUENCE(StarsSocParser, true);
REGISTER_PROCESS_SUPPORT_CHIP(StarsSocParser, CHIP_V4_1_0);

}
}