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
        {"id", SQL_INTEGER_TYPE, true},
        {"value", SQL_TEXT_TYPE}
    };

    const TableColumns SESSION_TIME_INFO = {
        {"startTimeNs", SQL_INTEGER_TYPE},
        {"endTimeNs", SQL_INTEGER_TYPE},
        {"baseTimeNs", SQL_INTEGER_TYPE},
    };

    const TableColumns NPU_INFO = {
        {"id", SQL_INTEGER_TYPE, true},
        {"name", SQL_TEXT_TYPE}
    };

    const TableColumns TASK = {
        {"start", SQL_INTEGER_TYPE},
        {"end", SQL_INTEGER_TYPE},
        {"deviceId", SQL_INTEGER_TYPE},
        {"connectionId", SQL_INTEGER_TYPE},
        {"correlationId", SQL_INTEGER_TYPE, true},
        {"globalPid", SQL_INTEGER_TYPE},
        {"taskType", SQL_INTEGER_TYPE},
        {"contextId", SQL_INTEGER_TYPE},
        {"streamId", SQL_INTEGER_TYPE},
        {"taskId", SQL_INTEGER_TYPE},
        {"modelId", SQL_INTEGER_TYPE}
    };

    const TableColumns COMPUTE_TASK_INFO = {
        {"name", SQL_INTEGER_TYPE},
        {"correlationId", SQL_INTEGER_TYPE, true},
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
        {"correlationId", SQL_INTEGER_TYPE, true},
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
        {"linkType", SQL_INTEGER_TYPE},
        {"opId", SQL_INTEGER_TYPE}
    };

    const TableColumns COMMUNICATION_OP = {
        {"opName", SQL_INTEGER_TYPE},
        {"start", SQL_INTEGER_TYPE},
        {"end", SQL_INTEGER_TYPE},
        {"connectionId", SQL_INTEGER_TYPE},
        {"groupName", SQL_INTEGER_TYPE},
        {"opId", SQL_INTEGER_TYPE, true}
    };

    const TableColumns API = {
        {"start", SQL_INTEGER_TYPE},
        {"end", SQL_INTEGER_TYPE},
        {"level", SQL_INTEGER_TYPE},
        {"globalTid", SQL_INTEGER_TYPE},
        {"connectionId", SQL_INTEGER_TYPE, true},
        {"name", SQL_INTEGER_TYPE}
    };

    const TableColumns NPU_MEM = {
        {"type", SQL_INTEGER_TYPE},
        {"ddrUsage", SQL_NUMERIC_TYPE},
        {"hbmUsage", SQL_NUMERIC_TYPE},
        {"timestamp", SQL_INTEGER_TYPE},
        {"deviceId", SQL_INTEGER_TYPE}
    };

    const TableColumns NPU_MODULE_MEM = {
        {"moduleId", SQL_INTEGER_TYPE},
        {"timestamp", SQL_INTEGER_TYPE},
        {"totalReserved", SQL_NUMERIC_TYPE},
        {"deviceId", SQL_INTEGER_TYPE}
    };

    const TableColumns NPU_OP_MEM = {
        {"operatorName", SQL_INTEGER_TYPE},
        {"addr", SQL_INTEGER_TYPE},
        {"type", SQL_INTEGER_TYPE},
        {"size", SQL_INTEGER_TYPE},
        {"timestamp", SQL_INTEGER_TYPE},
        {"globalTid", SQL_INTEGER_TYPE},
        {"totalAllocate", SQL_NUMERIC_TYPE},
        {"totalReserve", SQL_NUMERIC_TYPE},
        {"component", SQL_INTEGER_TYPE},
        {"deviceId", SQL_INTEGER_TYPE}
    };

    const TableColumns NIC = {
        {"deviceId", SQL_INTEGER_TYPE},
        {"timestamp", SQL_INTEGER_TYPE},
        {"bandwidth", SQL_INTEGER_TYPE},
        {"rxPacketRate", SQL_NUMERIC_TYPE},
        {"rxByteRate", SQL_NUMERIC_TYPE},
        {"rxPackets", SQL_INTEGER_TYPE},
        {"rxBytes", SQL_INTEGER_TYPE},
        {"rxErrors", SQL_INTEGER_TYPE},
        {"rxDropped", SQL_INTEGER_TYPE},
        {"txPacketRate", SQL_NUMERIC_TYPE},
        {"txByteRate", SQL_NUMERIC_TYPE},
        {"txPackets", SQL_INTEGER_TYPE},
        {"txBytes", SQL_INTEGER_TYPE},
        {"txErrors", SQL_INTEGER_TYPE},
        {"txDropped", SQL_INTEGER_TYPE},
        {"funcId", SQL_INTEGER_TYPE}
    };

    const TableColumns RoCE = {
        {"deviceId", SQL_INTEGER_TYPE},
        {"timestamp", SQL_INTEGER_TYPE},
        {"bandwidth", SQL_INTEGER_TYPE},
        {"rxPacketRate", SQL_NUMERIC_TYPE},
        {"rxByteRate", SQL_NUMERIC_TYPE},
        {"rxPackets", SQL_INTEGER_TYPE},
        {"rxBytes", SQL_INTEGER_TYPE},
        {"rxErrors", SQL_INTEGER_TYPE},
        {"rxDropped", SQL_INTEGER_TYPE},
        {"txPacketRate", SQL_NUMERIC_TYPE},
        {"txByteRate", SQL_NUMERIC_TYPE},
        {"txPackets", SQL_INTEGER_TYPE},
        {"txBytes", SQL_INTEGER_TYPE},
        {"txErrors", SQL_INTEGER_TYPE},
        {"txDropped", SQL_INTEGER_TYPE},
        {"funcId", SQL_INTEGER_TYPE}
    };

    const TableColumns HBM = {
        {"deviceId", SQL_INTEGER_TYPE},
        {"timestamp", SQL_INTEGER_TYPE},
        {"bandwidth", SQL_NUMERIC_TYPE},
        {"hbmId", SQL_INTEGER_TYPE},
        {"type", SQL_INTEGER_TYPE}
    };

    const TableColumns DDR = {
        {"deviceId", SQL_INTEGER_TYPE},
        {"timestamp", SQL_INTEGER_TYPE},
        {"read", SQL_NUMERIC_TYPE},
        {"write", SQL_NUMERIC_TYPE}
    };

    const TableColumns LLC = {
        {"deviceId", SQL_INTEGER_TYPE},
        {"llcId", SQL_INTEGER_TYPE},
        {"timestamp", SQL_INTEGER_TYPE},
        {"hitRate", SQL_REAL_TYPE},
        {"throughput", SQL_REAL_TYPE},
        {"mode", SQL_INTEGER_TYPE},
    };

    const TableColumns ENUM_API_LEVEL = {
        {"id", SQL_INTEGER_TYPE, true},
        {"name", SQL_TEXT_TYPE}
    };

    const TableColumns ENUM_MEMORY = {
        {"id", SQL_INTEGER_TYPE, true},
        {"name", SQL_TEXT_TYPE}
    };

    const TableColumns ENUM_NPU_MODULE = {
        {"id", SQL_INTEGER_TYPE, true},
        {"name", SQL_TEXT_TYPE}
    };
}

ReportDB::ReportDB()
{
    dbName_ = "report.db";
    tableColNames_ = {
        {TABLE_NAME_STRING_IDS, STRING_IDS},
        {TABLE_NAME_SESSION_TIME_INFO, SESSION_TIME_INFO},
        {TABLE_NAME_NPU_INFO, NPU_INFO},
        {TABLE_NAME_TASK, TASK},
        {TABLE_NAME_COMPUTE_TASK_INFO, COMPUTE_TASK_INFO},
        {TABLE_NAME_COMMUNICATION_TASK_INFO, COMMUNICATION_TASK_INFO},
        {TABLE_NAME_COMMUNICATION_OP, COMMUNICATION_OP},
        {TABLE_NAME_API, API},
        {TABLE_NAME_NPU_MEM, NPU_MEM},
        {TABLE_NAME_NPU_MODULE_MEM, NPU_MODULE_MEM},
        {TABLE_NAME_NPU_OP_MEM, NPU_OP_MEM},
        {TABLE_NAME_NIC, NIC},
        {TABLE_NAME_ROCE, RoCE},
        {TABLE_NAME_HBM, HBM},
        {TABLE_NAME_DDR, DDR},
        {TABLE_NAME_LLC, LLC},
        // ENUM
        {TABLE_NAME_ENUM_API_LEVEL, ENUM_API_LEVEL},
        {TABLE_NAME_ENUM_MEMORY, ENUM_MEMORY},
        {TABLE_NAME_ENUM_NPU_MODULE, ENUM_NPU_MODULE},
    };
}


} // Database
} // Viewer
} // Analysis