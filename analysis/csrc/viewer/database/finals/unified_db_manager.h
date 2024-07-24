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
    UnifiedDBManager(const std::string &output) : outputPath_(output) {};
    bool Run();
    bool Init();
private:
    bool CheckProfDirsValid(const std::string& outputDir);
    static std::string GetDBPath(const std::string& outputDir);
    bool RunOneProf(const std::string& profPath);
private:
    std::string msprofDBPath_;
    std::string outputPath_;
    std::set<std::string> profFolderPaths_;
};

}  // Database
}  // Viewer
}  // Analysis

#endif // #define ANALYSIS_VIEWER_DATABASE_UNIFIED_DB_MANAGER_H
