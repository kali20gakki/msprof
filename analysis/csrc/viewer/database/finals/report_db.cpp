/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : report_db.cpp
 * Description        : 定义reportDB中的表结构
 * Author             : msprof team
 * Creation Date      : 2023/12/13
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/finals/report_db.h"

#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {

namespace {
    const TableColumns STRING_IDS = {
        {"id", SQL_INTEGER_TYPE},
        {"value", SQL_TEXT_TYPE}
    };

    const TableColumns TARGET_INFO_SESSION_TIME = {
        {"startTimeNs", SQL_INTEGER_TYPE},
        {"endTimeNs", SQL_INTEGER_TYPE},
        {"baseTimeNs", SQL_INTEGER_TYPE},
    };

    const TableColumns TARGET_INFO_NPU = {
        {"id", SQL_INTEGER_TYPE},
        {"name", SQL_TEXT_TYPE}
    };

    const TableColumns ENUM_API_LEVEL = {
        {"id", SQL_INTEGER_TYPE},
        {"name", SQL_TEXT_TYPE}
    };

    const TableColumns TASK = {
        {"start", SQL_INTEGER_TYPE},
        {"end", SQL_INTEGER_TYPE},
        {"name", SQL_INTEGER_TYPE},
        {"deviceId", SQL_INTEGER_TYPE},
        {"connectionId", SQL_INTEGER_TYPE},
        {"correlationId", SQL_INTEGER_TYPE},
        {"globalPid", SQL_INTEGER_TYPE},
        {"taskType", SQL_INTEGER_TYPE},
        {"contextId", SQL_INTEGER_TYPE},
        {"streamId", SQL_INTEGER_TYPE},
        {"taskId", SQL_INTEGER_TYPE},
        {"modelId", SQL_INTEGER_TYPE}
    };

    const TableColumns COMPUTE_TASK_INFO = {
        {"name", SQL_INTEGER_TYPE},
        {"correlationId", SQL_INTEGER_TYPE},
        {"block_dim", SQL_INTEGER_TYPE},
        {"mixBlockDim", SQL_INTEGER_TYPE},
        {"taskType", SQL_INTEGER_TYPE},
        {"opType", SQL_INTEGER_TYPE},
        {"inputFormats", SQL_INTEGER_TYPE},
        {"inputDataTypes", SQL_INTEGER_TYPE},
        {"inputShapes", SQL_INTEGER_TYPE},
        {"outputFormats", SQL_INTEGER_TYPE},
        {"outputDataTypes", SQL_INTEGER_TYPE},
        {"outputShapes", SQL_INTEGER_TYPE}
    };

    const TableColumns COMMUNICATION_TASK_INFO = {
        {"name", SQL_INTEGER_TYPE},
        {"correlationId", SQL_INTEGER_TYPE},
        {"taskType", SQL_INTEGER_TYPE},
        {"planeId", SQL_INTEGER_TYPE},
        {"groupName", SQL_INTEGER_TYPE},
        {"notifyId", SQL_INTEGER_TYPE},
        {"rdmaType", SQL_INTEGER_TYPE},
        {"srcRank", SQL_INTEGER_TYPE},
        {"dstRank", SQL_INTEGER_TYPE},
        {"transportType", SQL_INTEGER_TYPE},
        {"size", SQL_INTEGER_TYPE},
        {"dataType", SQL_INTEGER_TYPE},
        {"linkType", SQL_INTEGER_TYPE}
    };

    const TableColumns API = {
        {"start", SQL_NUMERIC_TYPE},
        {"end", SQL_NUMERIC_TYPE},
        {"level", SQL_INTEGER_TYPE},
        {"globalTid", SQL_INTEGER_TYPE},
        {"connectionId", SQL_INTEGER_TYPE},
        {"name", SQL_INTEGER_TYPE}
    };
}

ReportDB::ReportDB()
{
    dbName_ = "report.db";
    tableColNames_ = {
        {TABLE_NAME_STRING_IDS, STRING_IDS},
        {TABLE_NAME_TARGET_INFO_SESSION_TIME, TARGET_INFO_SESSION_TIME},
        {TABLE_NAME_TARGET_INFO_NPU, TARGET_INFO_NPU},
        {TABLE_NAME_ENUM_API_LEVEL, ENUM_API_LEVEL},
        {TABLE_NAME_TASK, TASK},
        {TABLE_NAME_COMPUTE_TASK_INFO, COMPUTE_TASK_INFO},
        {TABLE_NAME_COMMUNICATION_TASK_INFO, COMMUNICATION_TASK_INFO},
        {TABLE_NAME_API, API},
    };
}


} // Database
} // Viewer
} // Analysis