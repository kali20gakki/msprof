/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : table_processor_factory.h
 * Description        : db工厂类
 * Author             : msprof team
 * Creation Date      : 2023/12/07
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_TABLE_PROCESSOR_FACTORY_H
#define ANALYSIS_VIEWER_DATABASE_TABLE_PROCESSOR_FACTORY_H

#include <string>
#include <memory>

#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/viewer/database/finals/report_db.h"
#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {


class TableProcessorFactory {
public:
    static std::shared_ptr<TableProcessor> CreateTableProcessor(const std::string &processorName,
                                                                const std::string &reportDBPath,
                                                                const std::set<std::string> &profPaths);
    ~TableProcessorFactory() = default;
};  // class TableProcessorFactory


}  // Database
}  // Viewer
}  // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_TABLE_PROCESSOR_FACTORY_H
