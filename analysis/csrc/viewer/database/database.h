/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : db_config.h
 * Description        : DB相关常量映射
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_DATABASE_H
#define ANALYSIS_VIEWER_DATABASE_DATABASE_H

#include <vector>
#include <unordered_map>
#include <string>

namespace Analysis {
namespace Viewer {
namespace Database {

using TABLE_COLS = std::vector<std::pair<std::string, std::string>>;

// DB中Table映射基类
class Database {
public:
    Database();
    // 获取该DB实际落盘的文件名
    std::string GetDBName() const;
    // 获取该DB指定表中字段名
    TABLE_COLS &GetTableCols(const std::string tableName);
private:
    std::string dbName_;
    std::unordered_map<std::string, TABLE_COLS> tableColNames_;
};

} // namespace Database
} // namespace Viewer
} // namespace Analysis
#endif // ANALYSIS_VIEWER_DATABASE_DATABASE_H
