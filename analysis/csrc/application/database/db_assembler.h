/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : db_assembler.h
 * Description        : DB数据组合类
 * Author             : msprof team
 * Creation Date      : 2024/8/27
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_DB_ASSEMBLER_H
#define ANALYSIS_APPLICATION_DB_ASSEMBLER_H

#include "analysis/csrc/infrastructure/db/include/db_info.h"
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/viewer/database/finals/msprof_db.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Infra;

class DBAssembler {
public:
    DBAssembler() = default;
    DBAssembler(const std::string &profPath, const std::string &outputPath);
    virtual ~DBAssembler() = default;
    bool Run(DataInventory &dataInventory);
    const static std::set<std::string>& GetProcessList();
private:
    std::string profPath_;
    std::string outputPath_;
    DBInfo msprofDB_;
};

template<typename... Args>
bool SaveData(const std::vector<std::tuple<Args...>> &data, const std::string &tableName, DBInfo& msprofDB)
{
    INFO("DBAssembler Save % Data.", tableName);
    if (data.empty()) {
        ERROR("% is empty.", tableName);
        return false;
    }
    if (msprofDB.database == nullptr) {
        ERROR("Msprof db database is nullptr.");
        return false;
    }
    if (msprofDB.dbRunner == nullptr) {
        ERROR("Msprof db runner is nullptr.");
        return false;
    }
    if (!msprofDB.dbRunner->CreateTable(tableName, msprofDB.database->GetTableCols(tableName))) {
        ERROR("Create table: % failed", tableName);
        return false;
    }
    if (!msprofDB.dbRunner->InsertData(tableName, data)) {
        ERROR("Insert data into % failed", tableName);
        return false;
    }
    return true;
}

}
}

#endif // ANALYSIS_APPLICATION_DB_ASSEMBLER_H
