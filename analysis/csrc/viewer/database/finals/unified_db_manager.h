/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : unified_db_manager.h
 * Description        : db类型交付件生成
 * Author             : msprof team
 * Creation Date      : 2023/12/14
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_UNIFIED_DB_MANAGER_H
#define ANALYSIS_VIEWER_DATABASE_UNIFIED_DB_MANAGER_H

#include <string>
#include <vector>

namespace Analysis {
namespace Viewer {
namespace Database {

class UnifiedDBManager {
public:
    UnifiedDBManager(const std::string &output, const std::vector<std::string> &profPaths)
        : output_(output), profPaths_(profPaths) {};
    bool Run();
private:
    void Init();
    std::string reportDBPath_;
    std::string output_;
    std::vector<std::string> profPaths_;
};

}  // Database
}  // Viewer
}  // Analysis

#endif // #define ANALYSIS_VIEWER_DATABASE_UNIFIED_DB_MANAGER_H

