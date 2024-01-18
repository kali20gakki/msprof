/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : api_proceser.h
 * Description        : api_processer，处理api表相关数据
 * Author             : msprof team
 * Creation Date      : 2023/12/13
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_API_PROCESSER_H
#define ANALYSIS_VIEWER_DATABASE_API_PROCESSER_H

#include "analysis/csrc/utils/time_utils.h"
#include "analysis/csrc/viewer/database/finals/table_processer.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于定义处理ApiEvent DB
class ApiProcesser : public TableProcesser {
// struct_type, id, level, thread_id, item_id, start, end, connection_id
using ApiDataFormat = std::vector<std::tuple<std::string, std::string, std::string, uint32_t,
        std::string, uint64_t, uint64_t, uint64_t>>;
// start, end, level, globalTid, connectionId, name
using ProcessedDataFormat = std::vector<std::tuple<double, double, uint16_t,
        uint64_t, uint64_t, uint64_t>>;
public:
    ApiProcesser() = default;
    ApiProcesser(const std::string &reportDBPath, const std::set<std::string> &profPaths);
protected:
    bool Process(const std::string &fileDir) override;
private:
    ApiDataFormat GetData(const std::string &fileDir);
    bool FormatData(const std::string &fileDir, const ApiDataFormat &oriData,
                    ProcessedDataFormat &processedData) const;
    static uint16_t GetLevelValue(const std::string &key);
};


} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_API_PROCESSER_H
