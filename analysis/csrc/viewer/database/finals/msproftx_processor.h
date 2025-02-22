/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : msproftx_processor.h
 * Description        : msproftx_processor，处理msproftx表 msproftx打点相关数据
 * Author             : msprof team
 * Creation Date      : 2024/5/20
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_MSPROFTX_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_MSPROFTX_PROCESSOR_H
 
#include "analysis/csrc/viewer/database/finals/table_processor.h"
 
namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于定义处理msproftx.db中MsprofTx表和MsprofTxEx表
class MsprofTxProcessor : public TableProcessor {
// Msproftx表字段数据类型
// tid, category, event_type, start_time, end_time, message
using MsprofTxDataFormat = std::vector<std::tuple<uint32_t, uint32_t, std::string,
        uint64_t, uint64_t, std::string>>;

// MsprofTxEx表字段数据类型
// tid, event_type, start_time, end_time, message, domain, mark_id
using MsprofTxExDataFormat = std::vector<std::tuple<uint32_t, std::string,
        uint64_t, uint64_t, std::string, std::string, uint64_t>>;

// startNs, endNs, eventType, rangeId, category, message, globalTid, endGlobalTid, domainId, connectionId
using ProcessedDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint16_t,
        uint32_t, uint32_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;

public:
    MsprofTxProcessor() = default;
    MsprofTxProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths);
    virtual ~MsprofTxProcessor() = default;
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
    bool ProcessTxData(const std::string &fileDir, Utils::SyscntConversionParams &params,
                       Utils::ProfTimeRecord &record);
    bool ProcessTxExData(const std::string &fileDir, Utils::SyscntConversionParams &params,
                         Utils::ProfTimeRecord &record);
private:
    MsprofTxDataFormat GetTxData(const std::string &fileDir, DBInfo &msprofTxDb);
    MsprofTxExDataFormat GetTxExData(const std::string &fileDir, DBInfo &msprofTxExDb);
    bool FormatTxData(const MsprofTxDataFormat &msprofTxData, ProcessedDataFormat &processedData,
                      uint32_t pid, Utils::SyscntConversionParams &params, Utils::ProfTimeRecord &record);
    bool FormatTxExData(const MsprofTxExDataFormat &txExData, ProcessedDataFormat &processedData,
                        const std::string &fileDir, Utils::SyscntConversionParams &params,
                        Utils::ProfTimeRecord &record);
};
 
} // Database
} // Viewer
} // Analysis
 
#endif // ANALYSIS_VIEWER_DATABASE_MSPROFTX_PROCESSOR_H