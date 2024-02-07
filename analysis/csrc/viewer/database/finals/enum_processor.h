/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : enum_processor.h
 * Description        : 落盘api level层级数据
 * Author             : msprof team
 * Creation Date      : 2023/12/18
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_ENUM_PROCESSOR_H
#define MSPROF_ANALYSIS_ENUM_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于落盘枚举相关数据
// api level 不同的PROF文件中通用，因此只需调用一回
class EnumProcessor : public TableProcessor {
// level_id, level_name
using EnumFormat = std::vector<std::tuple<uint16_t, std::string>>;
public:
    EnumProcessor() = default;
    EnumProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths);
    bool Run() override;
protected:
    bool Process(const std::string &fileDir = "") override;
private:
    bool GetApiLevelData();
    bool GetIOTypeData();
    bool GetNpuModuleData();
};


} // Database
} // Viewer
} // Analysis

#endif // MSPROF_ANALYSIS_ENUM_PROCESSOR_H
