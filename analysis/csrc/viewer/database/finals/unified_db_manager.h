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
#include <set>

namespace Analysis {
namespace Viewer {
namespace Database {

class UnifiedDBManager {
public:
    UnifiedDBManager(const std::string &output, const std::set<std::string> &profPaths)
        : outputPath_(output), ProfFolderPaths_(profPaths) {};
    int Run();
    int Init();
    static bool CheckProfDirsValid(const std::string outputDir,
                                   const std::set<std::string> &profFolderPaths, std::string &errInfo);
private:
    std::string reportDBPath_;
    std::string outputPath_;
    std::set<std::string> ProfFolderPaths_;
};

}  // Database
}  // Viewer
}  // Analysis

#endif // #define ANALYSIS_VIEWER_DATABASE_UNIFIED_DB_MANAGER_H
