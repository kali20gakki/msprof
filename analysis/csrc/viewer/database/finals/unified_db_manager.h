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

#ifndef ANALYSIS_VIEWER_DATABASE_FINALS_UNIFIED_DB_MANAGER_H
#define ANALYSIS_VIEWER_DATABASE_FINALS_UNIFIED_DB_MANAGER_H

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Finals {

class UnifiedDBManager {
public:
    explict UnifiedDBManager(const std::string &outputDir,
                             const std::vector<std::string> &profPath): outputDir_(outputDir)
                                                                        profPath_(profPath) {};

    bool Run();
private:
    bool Init();
    std::string outputDir_;
    std::vector<std::string> profPath_;
}

}  // Finals
}  // Database
}  // Viewer
}  // Analysis
#endif // #define ANALYSIS_VIEWER_DATABASE_FINALS_UNIFIED_DB_MANAGER_H

