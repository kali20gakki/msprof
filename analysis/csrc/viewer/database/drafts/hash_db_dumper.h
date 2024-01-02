/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hash_db_dumper.h
 * Description        : hashData入库接口
 * Author             : msprof team
 * Creation Date      : 2023/11/16
 * *****************************************************************************
 */


#ifndef ANALYSIS_VIEWER_DATABASE_DRAFTS_HASH_DB_DUMPER_H
#define ANALYSIS_VIEWER_DATABASE_DRAFTS_HASH_DB_DUMPER_H

#include <thread>
#include <unordered_map>
#include "db_runner.h"
#include "database.h"
#include "base_dumper.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Drafts {
using HashDataMap = std::unordered_map<uint64_t, std::string>;

class HashDBDumper final : public BaseDumper<HashDBDumper> {
public:
    explicit HashDBDumper(const std::string& hostFilePath);

    std::vector<std::tuple<std::string, std::string>> GenerateData(const HashDataMap &hashData);
};
} // Drafts
} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_DRAFTS_HASH_DB_DUMPER_H
