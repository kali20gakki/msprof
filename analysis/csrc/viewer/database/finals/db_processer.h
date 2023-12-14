/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : db_processer.h
 * Description        : db工厂类
 * Author             : msprof team
 * Creation Date      : 2023/12/07
 * *****************************************************************************
 */

#ifndef ANALYSIS_VIEWER_DATABASE_FINALS_DB_PROSSER_H
#define ANALYSIS_VIEWER_DATABASE_FINALS_DB_PROSSER_H

#include <memory>

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Finals {

// 选择器父类
class DBProcesser {
public:
    DBProcesser() = default;
    explicit DBProcesser(const std::string &outputDir, const std::vector<std::string> &profPath);
    virtual int Run() = 0;
    virtual ~DBProcesser() = default;
protected:
    std::string outputDir_;
    std::vector<std::string> profPath_;
}; // class DBProcesser


class AscendTaskProcesser : public DBProcesser {
public:
    AscendTaskProcesser() = default;
    explicit AscendTaskProcesser(const std::string &outputDir, const std::vector<std::string> &profPath);
    virtual int Run() override;
    virtual ~AscendTaskProcesser() = default;
}; // class DBProcesser


class DBProcesserFactory {
public:
    static std::shared_ptr<DBProcesser> CreateDBProcessor(const std::string &dbName,
                                                          const std::string &outputDir,
                                                          const std::vector<std::string> &profPath);
    ~DBProcesserFactory() {};
};  // class DBProcesserFactory


}  // Finals
}  // Database
}  // Viewer
}  // Analysis
#endif // ANALYSIS_VIEWER_DATABASE_FINALS_DB_PROSSER_H
