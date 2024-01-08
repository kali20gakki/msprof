/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : table_process_factory.cpp
 * Description        : processer的父类实现类，规定统一流程
 * Author             : msprof team
 * Creation Date      : 2023/12/14
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/table_processer_factory.h"

#include "analysis/csrc/utils/thread_pool.h"
#include "analysis/csrc/viewer/database/finals/api_processer.h"

namespace Analysis {
namespace Viewer {
namespace Database {

const uint32_t POOLNUM = 2;

std::shared_ptr<TableProcesser> TableProcesserFactory::CreateTableProcessor(
    const std::string &tableName,
    const std::string &reportDBPath,
    const std::vector<std::string> &profPaths)
{
    std::shared_ptr<TableProcesser> processer = nullptr;
    if (tableName == "api_processer") {
        processer = std::make_shared<ApiProcesser>(reportDBPath, profPaths);
    }
    return processer;
}


} // Database
} // Viewer
} // Analysis