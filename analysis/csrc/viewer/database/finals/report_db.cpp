/* ******************************************************************************
版权所有 (c) 华为技术有限公司 2023-2023
Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : report_db.cpp
 * Description        : report_db 定义reportDB中的表结构
 * Author             : msprof team
 * Creation Date      : 2023/12/13
 * *****************************************************************************
 */


#include "report_db.h"

namespace Analysis {
namespace Viewer {
namespace Database {

namespace {
    const TABLE_COLS STRING_IDS = {
        {"id", SQL_INTEGER_TYPE},
        {"value", SQL_TEXT_TYPE}
    };

    const TABLE_COLS TARGET_INFO_SESSION_START_TIME = {
        {"startTimeNs", SQL_INTEGER_TYPE},
        {"baseTimeNs", SQL_INTEGER_TYPE}
    };

    const TABLE_COLS TARGET_INFO_GPU = {
        {"id", SQL_INTEGER_TYPE},
        {"name", SQL_TEXT_TYPE}
    };

    const TABLE_COLS ENUM_API_LEVEL = {
        {"id", SQL_INTEGER_TYPE},
        {"name", SQL_TEXT_TYPE}
    };

    const TABLE_COLS TASK = {
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

    const TABLE_COLS COMPUTE_TASK_INFO = {
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

    const TABLE_COLS COMMUNICATION_TASK_INFO = {
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

    const TABLE_COLS API = {
        {"start", SQL_INTEGER_TYPE},
        {"end", SQL_INTEGER_TYPE},
        {"level", SQL_INTEGER_TYPE},
        {"globalTid", SQL_INTEGER_TYPE},
        {"connectionId", SQL_INTEGER_TYPE},
        {"name", SQL_INTEGER_TYPE}
    };
}

ReportDB::ReportDB()
{
    dbName_ = "report.db";
    tableColNames_["STRING_IDS"] = STRING_IDS;
    tableColNames_["TARGET_INFO_SESSION_START_TIME"] = TARGET_INFO_SESSION_START_TIME;
    tableColNames_["TARGET_INFO_GPU"] = TARGET_INFO_GPU;
    tableColNames_["ENUM_API_LEVEL"] = ENUM_API_LEVEL;
    tableColNames_["TASK"] = TASK;
    tableColNames_["COMPUTE_TASK_INFO"] = COMPUTE_TASK_INFO;
    tableColNames_["COMMUNICATION_TASK_INFO"] = COMMUNICATION_TASK_INFO;
    tableColNames_["API"] = API;
}


} // Database
} // Viewer
} // Analysis