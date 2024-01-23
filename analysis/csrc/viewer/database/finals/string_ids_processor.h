/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : string_ids_processor.h
 * Description        : stringids_processor，处理StringIds数据
 * Author             : msprof team
 * Creation Date      : 2023/12/16
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_FINALS_STRINGIDS_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_FINALS_STRINGIDS_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {

// 该类用于生成StringIds表
class StringIdsProcessor : public TableProcessor {
    using OriDataFormat = std::unordered_map<std::string, uint64_t>;
    using ProcessedDataFormat = std::vector<std::tuple<uint64_t, std::string>>;
public:
    StringIdsProcessor() = default;
    explicit StringIdsProcessor(const std::string &reportDBPath);
    virtual ~StringIdsProcessor() = default;
    bool Run() override;
protected:
    bool Process(const std::string &fileDir = "") override;
private:
    static ProcessedDataFormat FormatData(const OriDataFormat &oriData);
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_FINALS_STRINGIDS_PROCESSOR_H
