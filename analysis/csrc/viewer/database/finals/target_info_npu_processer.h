/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : target_info_npu_processer.h
 * Description        : 落盘不同device对应芯片型号数据
 * Author             : msprof team
 * Creation Date      : 2023/12/18
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_TARGET_INFO_NPU_PROCESSER_H
#define ANALYSIS_VIEWER_DATABASE_TARGET_INFO_NPU_PROCESSER_H

#include "analysis/csrc/viewer/database/finals/table_processer.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于落盘不同device对应卡的芯片型号相关数据
class TargetInfoNpuProcesser : public TableProcesser {
// gpu_chip_num, gpu_name
using NpuInfoDataFormat = std::vector<std::tuple<uint16_t, std::string>>;
public:
    TargetInfoNpuProcesser() = default;
    TargetInfoNpuProcesser(std::string reportDBPath, const std::set<std::string> &profPaths);
protected:
    bool Process(const std::string &fileDir) override;
private:
    static void UpdateGpuData(const std::string &fileDir, const std::string &deviceDir,
                              NpuInfoDataFormat &npuInfoData);
};


} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_TARGET_INFO_NPU_PROCESSER_H
