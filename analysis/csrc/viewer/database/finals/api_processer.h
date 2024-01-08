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

#include "analysis/csrc/viewer/database/finals/table_processer.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于定义处理ApiEvent DB
class ApiProcesser : public TableProcesser {
using ORI_DATA_FORMAT = std::vector<std::tuple<std::string, std::string, std::string, uint32_t,
        std::string, uint64_t, uint64_t, uint32_t>>;
using PROCESSED_DATA_FORMAT = std::vector<std::tuple<uint64_t, uint64_t, uint16_t,
        uint64_t, uint32_t, uint64_t>>;
public:
    ApiProcesser() = default;
    ApiProcesser(const std::string &reportDBPath, const std::vector<std::string> &profPaths)
        : TableProcesser(reportDBPath, profPaths)
{
        apieventDB_.dbName = "api_event.db";
        apieventDB_.tableName = "ApiData";
        reportDB_.tableName = "API";
};
    virtual ~ApiProcesser() = default;
protected:
    void Process(const std::string &fileDir) override;
private:
    bool GetData(const std::string &dbPath, ORI_DATA_FORMAT &oriData);
    bool FormatData(const ORI_DATA_FORMAT &oriData, PROCESSED_DATA_FORMAT &processedData);
    bool SaveData(const PROCESSED_DATA_FORMAT &processedData);
    DBInfo apieventDB_;
};


} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_API_PROCESSER_H
