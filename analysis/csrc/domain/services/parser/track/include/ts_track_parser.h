/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** *
/* ******************************************************************************
 * File Name          : ts_track_parser.h
 * Description        : ts_trace_data二进制文件解析
 * Author             : msprof team
 * Creation Date      : 2024/4/26
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_TRACK_TS_TRACK_PARSER_H
#define MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_TRACK_TS_TRACK_PARSER_H

#include <vector>
#include "analysis/csrc/domain/services/parser/parser.h"
#include "analysis/csrc/domain/entities/hal/include/hal_track.h"

namespace Analysis {
namespace Domain {
class TsTrackParser : public Parser {
private:
    uint32_t ParseDataItem(void *binaryData, uint32_t binaryDataSize, Domain::HalTrackData &data);
    std::vector<std::string> GetFilePattern() override;
    uint32_t GetTrunkSize() override;
    uint32_t ParseData(Infra::DataInventory& dataInventory) override;
    uint32_t GetNoFileCode() override;

private:
    std::vector<HalTrackData> halUniData_;
    std::vector<std::string> filePrefix_{"ts_track."};
};
}
}
#endif // MSPROF_ANALYSIS_DOMAIN_SERVICES_PARSER_TRACK_TS_TRACK_PARSER_H