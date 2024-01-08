/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : base_dumper.h
 * Description        : dumper基类 封装数据落盘DumpData，子类仅需实现GenerateData即可
 * Author             : msprof team
 * Creation Date      : 2023/11/16
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_DRAFTS_BASE_DUMPER_H
#define ANALYSIS_VIEWER_DATABASE_DRAFTS_BASE_DUMPER_H

#include <vector>

#include "analysis/csrc/utils/file.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/viewer/database/database.h"
#include "analysis/csrc/viewer/database/db_runner.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Drafts {
const std::string SQLITE = "sqlite";
template<typename DumperType>
class BaseDumper {
public:
    template<typename DataType>
    bool DumpData(const DataType &input)
    {
        if (!database_) {
            ERROR("Empty pointer: database_");
            return false;
        }
        if (input.empty()) {
            WARN("Input data is empty, pass dump");
            return true;
        }
        dbRunner_ = Utils::MakeShared<DBRunner>(Utils::File::PathJoin({dbPath_, database_->GetDBName()}));
        if (!dbRunner_ || !dbRunner_->CreateTable(tableName_, database_->GetTableCols(tableName_))) {
            ERROR("Create table: % failed", tableName_);
            return false;
        }
        auto data = static_cast<DumperType *>(this)->GenerateData(input);
        if (data.empty()) {
            ERROR("Get Empty data");
            return false;
        }
        if (dbRunner_->template InsertData(tableName_, data)) {
            INFO("Dump % data success", tableName_);
            return true;
        }
        ERROR("Dump table: % failed", tableName_);
        return false;
    };

    BaseDumper(const std::string &hostPath, std::string tableName) : tableName_(std::move(tableName))
    {
        dbPath_ = Utils::File::PathJoin({hostPath, SQLITE});
    };

protected:
    std::shared_ptr<Database> database_;
    std::string dbPath_;
    std::shared_ptr<DBRunner> dbRunner_;
    std::string tableName_;
};
} // Drafts
} // Database
} // Viewer
} // Analysis
#endif // ANALYSIS_VIEWER_DATABASE_DRAFTS_BASE_DUMPER_H
