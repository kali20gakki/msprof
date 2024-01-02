/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : type_info_dumper.h
 * Description        : typeInfo数据入库接口
 * Author             : msprof team
 * Creation Date      : 2023/11/16
 * *****************************************************************************
 */


#ifndef ANALYSIS_VIEWER_DATABASE_DRAFTS_TYPE_INFO_DUMPER_H
#define ANALYSIS_VIEWER_DATABASE_DRAFTS_TYPE_INFO_DUMPER_H

#include <thread>
#include <unordered_map>
#include "database.h"
#include "db_runner.h"
#include "base_dumper.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Drafts {
using TypeInfoData = std::unordered_map<uint16_t, std::unordered_map<uint64_t, std::string>>;
using DBRunner = Analysis::Viewer::Database::DBRunner;

class TypeInfoDBDumper final : public BaseDumper<TypeInfoDBDumper> {
public:
    explicit TypeInfoDBDumper(const std::string &hostFilePath);

    std::vector<std::tuple<std::string, std::string, std::string>>
    GenerateData(const TypeInfoData &typeInfoData);
};
} // Drafts
} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_DRAFTS_TYPE_INFO_DUMPER_H