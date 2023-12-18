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

#include "database.h"

#include "log.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Drafts {

std::string Database::GetDBName() const
{
    return dbName_;
}

std::vector<TableColumn> Database::GetTableCols(const std::string &tableName)
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
    tableColNames_["ApiEventData"] = {
        {"struct_type", sqlTextType},
        {"id", sqlTextType},
        {"level", sqlTextType},
        {"thread_id", sqlIntegerType},
        {"item_id", sqlTextType},
        {"start", sqlIntegerType},
        {"end", sqlIntegerType},
        {"connection_id", sqlIntegerType}
    };
}

RuntimeDB::RuntimeDB()
{
    dbName_ = "runtime.db";
    tableColNames_["HostTask"] = {
        {"model_id", sqlIntegerType},
        {"request_id", sqlIntegerType},
        {"stream_id", sqlIntegerType},
        {"task_id", sqlIntegerType},
        {"context_ids", sqlTextType},
        {"batch_id", sqlIntegerType},
        {"task_type", sqlTextType},
        {"device_id", sqlIntegerType},
        {"timestamp", sqlNumericType},
        {"connection_id", sqlIntegerType}
    };
}

GEInfoDB::GEInfoDB()
{
    dbName_ = "ge_info.db";
    tableColNames_["TaskInfo"] = {
        {"model_id", sqlIntegerType},
        {"op_name", sqlTextType},
        {"stream_id", sqlIntegerType},
        {"task_id", sqlIntegerType},
        {"block_dim", sqlIntegerType},
        {"mix_block_dim", sqlIntegerType},
        {"op_state", sqlTextType},
        {"task_type", sqlTextType},
        {"op_type", sqlTextType},
        {"index_id", sqlIntegerType},
        {"thread_id", sqlIntegerType},
        {"timestamp", sqlNumericType},
        {"batch_id", sqlIntegerType},
        {"tensor_num", sqlIntegerType},
        {"input_formats", sqlTextType},
        {"input_data_types", sqlTextType},
        {"input_shapes", sqlTextType},
        {"output_formats", sqlTextType},
        {"output_data_types", sqlTextType},
        {"output_shapes", sqlTextType},
        {"device_id", sqlIntegerType},
        {"context_id", sqlIntegerType},
        {"op_flag", sqlTextType}
    };
    tableColNames_["StepInfo"] = {
        {"model_id", sqlIntegerType},
        {"thread_id", sqlIntegerType},
        {"timestamp", sqlNumericType},
        {"cur_iter_num", sqlIntegerType},
        {"tag", sqlTextType}
    };
}

HashDB::HashDB()
{
    dbName_ = "ge_hash.db";
    tableColNames_["GeHashInfo"] = {
        {"hash_key", sqlTextType},
        {"hash_value", sqlTextType}
    };
    tableColNames_["TypeHashInfo"] = {
        {"hash_key", sqlTextType},
        {"hash_value", sqlTextType},
        {"level", sqlTextType}
    };
}

HCCLDB::HCCLDB()
{
    dbName_ = "hccl.db";
    tableColNames_["HCCLTask"] = {
        {"model_id", sqlIntegerType},
        {"index_id", sqlIntegerType},
        {"name", sqlTextType},
        {"group_name", sqlTextType},
        {"plane_id", sqlIntegerType},
        {"timestamp", sqlNumericType},
        {"duration", sqlRealType},
        {"stream_id", sqlIntegerType},
        {"task_id", sqlIntegerType},
        {"context_id", sqlIntegerType},
        {"batch_id", sqlIntegerType},
        {"device_id", sqlIntegerType},
        {"is_master", sqlIntegerType},
        {"struct_type", sqlTextType},
        {"local_rank", sqlIntegerType},
        {"remote_rank", sqlIntegerType},
        {"transport_type", sqlTextType},
        {"size", sqlRealType},
        {"data_type", sqlTextType},
        {"link_type", sqlTextType},
        {"notify_id", sqlIntegerType}
    };
    tableColNames_["HCCLOP"] = {
        {"device_id", sqlIntegerType},
        {"model_id", sqlIntegerType},
        {"index_id", sqlIntegerType},
        {"thread_id", sqlIntegerType},
        {"op_name", sqlTextType},
        {"task_type", sqlTextType},
        {"op_type", sqlTextType},
        {"begin", sqlRealType},
        {"end", sqlRealType},
        {"is_dynamic", sqlIntegerType},
        {"connection_id", sqlIntegerType}
    };
}

RtsTrackDB::RtsTrackDB()
{
    dbName_ = "rts_track.db";
    tableColNames_["HostTaskFlip"] = {
        {"device_id", sqlIntegerType},
        {"stream_id", sqlIntegerType},
        {"timestamp", sqlNumericType},
        {"task_id", sqlIntegerType},
        {"flip_num", sqlIntegerType}
    };
}
} // namespace Drafts
} // namespace Database
} // namespace Viewer
} // namespace Analysis