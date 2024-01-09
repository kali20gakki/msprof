/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : table_process_factory.h
 * Description        : db工厂类
 * Author             : msprof team
 * Creation Date      : 2023/12/07
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_TABLE_PROSSER_FACTORY_H
#define ANALYSIS_VIEWER_DATABASE_TABLE_PROSSER_FACTORY_H

#include <string>
#include <memory>

#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/viewer/database/finals/report_db.h"
#include "analysis/csrc/viewer/database/finals/table_processer.h"

namespace Analysis {
namespace Viewer {
namespace Database {


class TableProcesserFactory {
public:
    static std::shared_ptr<TableProcesser> CreateTableProcessor(const std::string &tableName,
                                                                const std::string &reportDBPath,
                                                                const std::set<std::string> &profPaths);
    ~TableProcesserFactory() = default;
};  // class TableProcesserFactory


}  // Database
}  // Viewer
}  // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_TABLE_PROSSER_FACTORY_H
