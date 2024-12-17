/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : memcpy_info_processor.h
 * Description        : memcpy_info_processor.h，处理MemcpyInfo表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/12/12
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_MEMCPY_INFO_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_MEMCPY_INFO_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于定义处理MEMCPY_INFO表
class MemcpyInfoProcessor : public TableProcessor {
// connection_id, data_size, memcpy_direction, max_size
using MemcpyInfoFormat = std::vector<std::tuple<uint64_t, uint64_t, uint16_t, uint64_t>>;
// connection_id, data_size, memcpy_direction, max_size
using ProcessedDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t>>;
public:
    MemcpyInfoProcessor() = default;
    MemcpyInfoProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths);
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    static MemcpyInfoFormat GetData(const std::string &dbPath, DBInfo &runtimeDB);
    static bool FormatData(const std::string &fileDir, const MemcpyInfoFormat &oriData,
                    ProcessedDataFormat &processedData) ;
};


} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_MEMCPY_INFO_PROCESSOR_H
