/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : meta_data_processor.h
 * Description        : 元信息表
 * Author             : msprof team
 * Creation Date      : 2024/5/14
 * *****************************************************************************
 */

#ifndef MSPROF_ANALYSIS_META_DATA_PROCESSOR_H
#define MSPROF_ANALYSIS_META_DATA_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于元数据相关，特指msprof工具的信息，同一工具对不同数据，导出内容相同或相近
class MetaDataProcessor : public TableProcessor {
using DataFormat = std::vector<std::tuple<std::string, std::string>>;
public:
    MetaDataProcessor() = default;
    MetaDataProcessor(const std::string &msprofDBPath);
    bool Run() override;
protected:
    bool Process(const std::string &fileDir = "") override;
private:
    bool SaveMetaData();
};


} // Database
} // Viewer
} // Analysis

#endif // MSPROF_ANALYSIS_META_DATA_PROCESSOR_H
