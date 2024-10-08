/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : host_info_processor.h
 * Description        : 落盘Host侧数据
 * Author             : msprof team
 * Creation Date      : 2024/04/11
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_HOST_INFO_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_HOST_INFO_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于落盘Host侧信息
class HostInfoProcessor : public TableProcessor {
// hostuid, hostname
using HostInfoDataFormat = std::vector<std::tuple<std::string, std::string>>;
public:
    HostInfoProcessor() = default;
    HostInfoProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths);
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    static void UpdateHostData(const std::string &fileDir, const std::string &deviceDir,
                              HostInfoDataFormat &hostInfoData);
    HostInfoDataFormat hostInfoData_;
};


} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_HOST_INFO_PROCESSOR_H
