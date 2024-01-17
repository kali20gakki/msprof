/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : database.cpp
 * Description        : DB相关常量映射
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#include "analysis/csrc/viewer/database/database.h"

#include "analysis/csrc/dfx/log.h"

namespace Analysis {
namespace Viewer {
namespace Database {

namespace {
    const TableColumns ApiEventData = {
        {"struct_type", SQL_TEXT_TYPE},
        {"id", SQL_TEXT_TYPE},
        {"level", SQL_TEXT_TYPE},
        {"thread_id", SQL_INTEGER_TYPE},
        {"item_id", SQL_TEXT_TYPE},
        {"start", SQL_INTEGER_TYPE},
        {"end", SQL_INTEGER_TYPE},
        {"connection_id", SQL_INTEGER_TYPE}
    };

    const TableColumns HostTask = {
        {"model_id", SQL_INTEGER_TYPE},
        {"request_id", SQL_INTEGER_TYPE},
        {"stream_id", SQL_INTEGER_TYPE},
        {"task_id", SQL_INTEGER_TYPE},
        {"context_ids", SQL_TEXT_TYPE},
        {"batch_id", SQL_INTEGER_TYPE},
        {"task_type", SQL_TEXT_TYPE},
        {"device_id", SQL_INTEGER_TYPE},
        {"timestamp", SQL_NUMERIC_TYPE},
        {"connection_id", SQL_INTEGER_TYPE}
    };

    const TableColumns TaskInfo = {
        {"model_id", SQL_INTEGER_TYPE},
        {"op_name", SQL_TEXT_TYPE},
        {"stream_id", SQL_INTEGER_TYPE},
        {"task_id", SQL_INTEGER_TYPE},
        {"block_dim", SQL_INTEGER_TYPE},
        {"mix_block_dim", SQL_INTEGER_TYPE},
        {"op_state", SQL_TEXT_TYPE},
        {"task_type", SQL_TEXT_TYPE},
        {"op_type", SQL_TEXT_TYPE},
        {"index_id", SQL_INTEGER_TYPE},
        {"thread_id", SQL_INTEGER_TYPE},
        {"timestamp", SQL_NUMERIC_TYPE},
        {"batch_id", SQL_INTEGER_TYPE},
        {"tensor_num", SQL_INTEGER_TYPE},
        {"input_formats", SQL_TEXT_TYPE},
        {"input_data_types", SQL_TEXT_TYPE},
        {"input_shapes", SQL_TEXT_TYPE},
        {"output_formats", SQL_TEXT_TYPE},
        {"output_data_types", SQL_TEXT_TYPE},
        {"output_shapes", SQL_TEXT_TYPE},
        {"device_id", SQL_INTEGER_TYPE},
        {"context_id", SQL_INTEGER_TYPE},
        {"op_flag", SQL_TEXT_TYPE}
    };

    const TableColumns StepInfo = {
        {"model_id", SQL_INTEGER_TYPE},
        {"thread_id", SQL_INTEGER_TYPE},
        {"timestamp", SQL_NUMERIC_TYPE},
        {"cur_iter_num", SQL_INTEGER_TYPE},
        {"tag", SQL_TEXT_TYPE}
    };

    const TableColumns GeHashInfo = {
        {"hash_key", SQL_TEXT_TYPE},
        {"hash_value", SQL_TEXT_TYPE}
    };

    const TableColumns TypeHashInfo = {
        {"hash_key", SQL_TEXT_TYPE},
        {"hash_value", SQL_TEXT_TYPE},
        {"level", SQL_TEXT_TYPE}
    };

    const TableColumns HCCLTask = {
        {"model_id", SQL_INTEGER_TYPE},
        {"index_id", SQL_INTEGER_TYPE},
        {"name", SQL_TEXT_TYPE},
        {"group_name", SQL_TEXT_TYPE},
        {"plane_id", SQL_INTEGER_TYPE},
        {"timestamp", SQL_NUMERIC_TYPE},
        {"duration", SQL_REAL_TYPE},
        {"stream_id", SQL_INTEGER_TYPE},
        {"task_id", SQL_INTEGER_TYPE},
        {"context_id", SQL_INTEGER_TYPE},
        {"batch_id", SQL_INTEGER_TYPE},
        {"device_id", SQL_INTEGER_TYPE},
        {"is_master", SQL_INTEGER_TYPE},
        {"struct_type", SQL_TEXT_TYPE},
        {"local_rank", SQL_INTEGER_TYPE},
        {"remote_rank", SQL_INTEGER_TYPE},
        {"transport_type", SQL_TEXT_TYPE},
        {"size", SQL_REAL_TYPE},
        {"data_type", SQL_TEXT_TYPE},
        {"link_type", SQL_TEXT_TYPE},
        {"notify_id", SQL_INTEGER_TYPE},
        {"rdma_type", SQL_TEXT_TYPE}
    };

    const TableColumns HCCLOP = {
        {"device_id", SQL_INTEGER_TYPE},
        {"model_id", SQL_INTEGER_TYPE},
        {"index_id", SQL_INTEGER_TYPE},
        {"thread_id", SQL_INTEGER_TYPE},
        {"op_name", SQL_TEXT_TYPE},
        {"task_type", SQL_TEXT_TYPE},
        {"op_type", SQL_TEXT_TYPE},
        {"begin", SQL_TEXT_TYPE},
        {"end", SQL_REAL_TYPE},
        {"is_dynamic", SQL_INTEGER_TYPE},
        {"connection_id", SQL_INTEGER_TYPE}
    };

    const TableColumns HostTaskFlip = {
        {"device_id", SQL_INTEGER_TYPE},
        {"stream_id", SQL_INTEGER_TYPE},
        {"timestamp", SQL_NUMERIC_TYPE},
        {"task_id", SQL_INTEGER_TYPE},
        {"flip_num", SQL_INTEGER_TYPE}
    };

    const TableColumns AscendTask = {
        {"model_id", SQL_INTEGER_TYPE},
        {"index_id", SQL_INTEGER_TYPE},
        {"stream_id", SQL_INTEGER_TYPE},
        {"task_id", SQL_INTEGER_TYPE},
        {"context_id", SQL_INTEGER_TYPE},
        {"batch_id", SQL_INTEGER_TYPE},
        {"start_time", SQL_NUMERIC_TYPE},
        {"duration", SQL_NUMERIC_TYPE},
        {"host_task_type", SQL_TEXT_TYPE},
        {"device_task_type", SQL_TEXT_TYPE},
        {"connection_id", SQL_INTEGER_TYPE}
    };

    const TableColumns HCCLSingleDevice = {
        {"model_id", SQL_INTEGER_TYPE},
        {"index_id", SQL_INTEGER_TYPE},
        {"op_name", SQL_TEXT_TYPE},
        {"iteration", SQL_INTEGER_TYPE},
        {"hccl_name", SQL_TEXT_TYPE},
        {"group_name", SQL_TEXT_TYPE},
        {"first_timestamp", SQL_NUMERIC_TYPE},
        {"plane_id", SQL_INTEGER_TYPE},
        {"timestamp", SQL_NUMERIC_TYPE},
        {"duration", SQL_REAL_TYPE},
        {"is_dynamic", SQL_REAL_TYPE},
        {"task_type", SQL_TEXT_TYPE},
        {"op_type", SQL_TEXT_TYPE},
        {"connection_id", SQL_INTEGER_TYPE},
        {"is_master", SQL_INTEGER_TYPE},
        {"stream_id", SQL_INTEGER_TYPE},
        {"task_id", SQL_INTEGER_TYPE},
        {"struct_type", SQL_TEXT_TYPE},
        {"duration_estimated", SQL_INTEGER_TYPE},
        {"local_rank", SQL_INTEGER_TYPE},
        {"remote_rank", SQL_INTEGER_TYPE},
        {"transport_type", SQL_TEXT_TYPE},
        {"size", SQL_INTEGER_TYPE},
        {"data_type", SQL_TEXT_TYPE},
        {"link_type", SQL_TEXT_TYPE},
        {"bandwidth", SQL_REAL_TYPE},
        {"context_id", SQL_INTEGER_TYPE},
        {"notify_id", SQL_INTEGER_TYPE},
        {"batch_id", SQL_INTEGER_TYPE},
        {"rdma_type", SQL_TEXT_TYPE}
    };
}

std::string Database::GetDBName() const
{
    return dbName_;
}

TableColumns Database::GetTableCols(const std::string &tableName)
{
    auto iter = tableColNames_.find(tableName);
    if (iter == tableColNames_.end()) {
        ERROR("There is no table name % in tableColNames.", tableName);
        return {};
    }
    return iter->second;
}

ApiEventDB::ApiEventDB()
{
    dbName_ = "api_event.db";
    tableColNames_["ApiEventData"] = ApiEventData;
}

RuntimeDB::RuntimeDB()
{
    dbName_ = "runtime.db";
    tableColNames_["HostTask"] = HostTask;
}

GEInfoDB::GEInfoDB()
{
    dbName_ = "ge_info.db";
    tableColNames_["TaskInfo"] = TaskInfo;
    tableColNames_["StepInfo"] = StepInfo;
}

HashDB::HashDB()
{
    dbName_ = "ge_hash.db";
    tableColNames_["GeHashInfo"] = GeHashInfo;
    tableColNames_["TypeHashInfo"] = TypeHashInfo;
}

HCCLDB::HCCLDB()
{
    dbName_ = "hccl.db";
    tableColNames_["HCCLTask"] = HCCLTask;
    tableColNames_["HCCLOP"] = HCCLOP;
}

RtsTrackDB::RtsTrackDB()
{
    dbName_ = "rts_track.db";
    tableColNames_["HostTaskFlip"] = HostTaskFlip;
}

AscendTaskDB::AscendTaskDB()
{
    dbName_ = "ascend_task.db";
    tableColNames_["AscendTask"] = AscendTask;
}

HCCLSingleDeviceDB::HCCLSingleDeviceDB()
{
    dbName_ = "hccl_single_device.db";
    tableColNames_["HCCLSingleDevice"] = HCCLSingleDevice;
}
} // namespace Database
} // namespace Viewer
} // namespace Analysis