/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_info_processor.h
 * Description        : 落盘不同device对应芯片型号数据
 * Author             : msprof team
 * Creation Date      : 2023/12/18
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_NPU_INFO_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_NPU_INFO_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于落盘不同device对应卡的芯片型号相关数据
class NpuInfoProcessor : public TableProcessor {
// gpu_chip_num, gpu_name
using NpuInfoDataFormat = std::vector<std::tuple<uint16_t, std::string>>;
public:
    NpuInfoProcessor() = default;
    NpuInfoProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths);
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    static void UpdateNpuData(const std::string &fileDir, const std::string &deviceDir,
                              NpuInfoDataFormat &npuInfoData);
};


} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_NPU_INFO_PROCESSOR_H
