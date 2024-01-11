/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : string_ids_processer.h
 * Description        : stringids_processer，处理StringIds数据
 * Author             : msprof team
 * Creation Date      : 2023/12/16
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_FINALS_STRINGIDS_PROCESSER_H
#define ANALYSIS_VIEWER_DATABASE_FINALS_STRINGIDS_PROCESSER_H

#include "analysis/csrc/viewer/database/finals/table_processer.h"

namespace Analysis {
namespace Viewer {
namespace Database {

// 该类用于生成StringIds表
class StringIdsProcesser : public TableProcesser {
    using OriDataFormat = std::unordered_map<std::string, uint64_t>;
    using ProcessedDataFormat = std::vector<std::tuple<uint64_t, std::string>>;
public:
    StringIdsProcesser() = default;
    explicit StringIdsProcesser(const std::string &reportDBPath);
    virtual ~StringIdsProcesser() = default;
    bool Run() override;
private:
    ProcessedDataFormat FormatData(const OriDataFormat &oriData);
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_FINALS_STRINGIDS_PROCESSER_H
