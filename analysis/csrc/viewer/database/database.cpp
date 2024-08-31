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
        {"op_flag", SQL_TEXT_TYPE},
        {"hashid", SQL_TEXT_TYPE}
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

    const TableColumns GeFusionOPInfo = {
        {"model_id", SQL_INTEGER_TYPE},
        {"fusion_name", SQL_TEXT_TYPE},
        {"fusion_op_nums", SQL_INTEGER_TYPE},
        {"op_names", SQL_TEXT_TYPE},
        {"memory_input", SQL_TEXT_TYPE},
        {"memory_output", SQL_TEXT_TYPE},
        {"memory_weight", SQL_TEXT_TYPE},
        {"memory_workspace", SQL_TEXT_TYPE},
        {"memory_total", SQL_TEXT_TYPE},
    };

    const TableColumns ModelName = {
        {"model_id", SQL_INTEGER_TYPE},
        {"model_name", SQL_TEXT_TYPE}
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
        {"local_rank", SQL_INTEGER_TYPE},
        {"remote_rank", SQL_INTEGER_TYPE},
        {"transport_type", SQL_TEXT_TYPE},
        {"size", SQL_REAL_TYPE},
        {"data_type", SQL_TEXT_TYPE},
        {"link_type", SQL_TEXT_TYPE},
        {"notify_id", SQL_TEXT_TYPE},
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
        {"begin", SQL_REAL_TYPE},
        {"end", SQL_REAL_TYPE},
        {"is_dynamic", SQL_TEXT_TYPE},
        {"connection_id", SQL_INTEGER_TYPE},
        {"relay", SQL_INTEGER_TYPE},
        {"retry", SQL_INTEGER_TYPE},
        {"data_type", SQL_TEXT_TYPE},
        {"alg_type", SQL_TEXT_TYPE},
        {"count", SQL_NUMERIC_TYPE},
        {"group_name", SQL_TEXT_TYPE}
    };

    const TableColumns HostTaskFlip = {
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

    const TableColumns HcclOpReport = {
        {"op_type", SQL_TEXT_TYPE},
        {"occurrences", SQL_TEXT_TYPE},
        {"total_time", SQL_NUMERIC_TYPE},
        {"min", SQL_NUMERIC_TYPE},
        {"avg", SQL_NUMERIC_TYPE},
        {"max", SQL_NUMERIC_TYPE},
        {"ratio", SQL_TEXT_TYPE},
    };

    const TableColumns StepTraceData = {
        {"index_id", SQL_INTEGER_TYPE},
        {"model_id", SQL_INTEGER_TYPE},
        {"step_start", SQL_INTEGER_TYPE},
        {"step_end", SQL_INTEGER_TYPE},
        {"iter_id", SQL_INTEGER_TYPE}
    };

    const TableColumns StepTime = {
        {"index_id", SQL_INTEGER_TYPE},
        {"model_id", SQL_INTEGER_TYPE},
        {"step_start", SQL_INTEGER_TYPE},
        {"step_end", SQL_INTEGER_TYPE},
        {"iter_id", SQL_INTEGER_TYPE}
    };

    const TableColumns AllReduce = {
        {"device_id", SQL_INTEGER_TYPE},
        {"model_id", SQL_INTEGER_TYPE},
        {"index_id", SQL_INTEGER_TYPE},
        {"iteration_end", SQL_INTEGER_TYPE},
        {"start", SQL_INTEGER_TYPE},
        {"end", SQL_INTEGER_TYPE}
    };

    const TableColumns GetNext = {
        {"model_id", SQL_INTEGER_TYPE},
        {"index_id", SQL_INTEGER_TYPE},
        {"start_time", SQL_INTEGER_TYPE},
        {"end_time", SQL_INTEGER_TYPE}
    };

    const TableColumns TrainingTrace = {
        {"device_id", SQL_INTEGER_TYPE},
        {"model_id", SQL_INTEGER_TYPE},
        {"iteration_id", SQL_INTEGER_TYPE},
        {"FP_start", SQL_INTEGER_TYPE},
        {"BP_end", SQL_INTEGER_TYPE},
        {"iteration_end", SQL_INTEGER_TYPE},
        {"iteration_time", SQL_INTEGER_TYPE},
        {"fp_bp_time", SQL_INTEGER_TYPE},
        {"grad_refresh_bound", SQL_INTEGER_TYPE},
        {"data_aug_bound", SQL_INTEGER_TYPE},
    };

    const TableColumns HCCLTaskSingleDevice = {
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
        {"is_dynamic", SQL_NUMERIC_TYPE},
        {"task_type", SQL_TEXT_TYPE},
        {"op_type", SQL_TEXT_TYPE},
        {"connection_id", SQL_INTEGER_TYPE},
        {"is_master", SQL_INTEGER_TYPE},
        {"stream_id", SQL_INTEGER_TYPE},
        {"task_id", SQL_INTEGER_TYPE},
        {"duration_estimated", SQL_INTEGER_TYPE},
        {"local_rank", SQL_INTEGER_TYPE},
        {"remote_rank", SQL_INTEGER_TYPE},
        {"transport_type", SQL_TEXT_TYPE},
        {"size", SQL_INTEGER_TYPE},
        {"data_type", SQL_TEXT_TYPE},
        {"link_type", SQL_TEXT_TYPE},
        {"bandwidth", SQL_REAL_TYPE},
        {"context_id", SQL_INTEGER_TYPE},
        {"notify_id", SQL_TEXT_TYPE},
        {"batch_id", SQL_INTEGER_TYPE},
        {"rdma_type", SQL_TEXT_TYPE}
    };

    const TableColumns HCCLOpSingleDevice = {
        {"model_id", SQL_INTEGER_TYPE},
        {"op_name", SQL_TEXT_TYPE},
        {"task_type", SQL_TEXT_TYPE},
        {"op_type", SQL_TEXT_TYPE},
        {"timestamp", SQL_NUMERIC_TYPE},
        {"relay", SQL_INTEGER_TYPE},
        {"retry", SQL_INTEGER_TYPE},
        {"data_type", SQL_TEXT_TYPE},
        {"alg_type", SQL_TEXT_TYPE},
        {"count", SQL_NUMERIC_TYPE},
        {"group_name", SQL_TEXT_TYPE},
        {"connection_id", SQL_INTEGER_TYPE}
    };

    const TableColumns NpuMem = {
        {"event", SQL_TEXT_TYPE},
        {"ddr", SQL_INTEGER_TYPE},
        {"hbm", SQL_INTEGER_TYPE},
        {"timestamp", SQL_NUMERIC_TYPE},
        {"memory", SQL_INTEGER_TYPE}
    };

    const TableColumns NpuModuleMem = {
        {"module_id", SQL_INTEGER_TYPE},
        {"syscnt", SQL_NUMERIC_TYPE},
        {"total_size", SQL_INTEGER_TYPE},
        {"device_type", SQL_TEXT_TYPE}
    };

    const TableColumns NpuOpMemRaw = {
        {"operator", SQL_TEXT_TYPE},
        {"addr", SQL_TEXT_TYPE},
        {"size", SQL_INTEGER_TYPE},
        {"timestamp", SQL_NUMERIC_TYPE},
        {"thread_id", SQL_INTEGER_TYPE},
        {"total_allocate_memory", SQL_INTEGER_TYPE},
        {"total_reserve_memory", SQL_INTEGER_TYPE},
        {"level", SQL_INTEGER_TYPE},
        {"type", SQL_INTEGER_TYPE},
        {"device_type", SQL_TEXT_TYPE}
    };

    const TableColumns NicOriginalData = {
        {"device_id", SQL_INTEGER_TYPE},
        {"replayid", SQL_INTEGER_TYPE},
        {"timestamp", SQL_NUMERIC_TYPE},
        {"bandwidth", SQL_INTEGER_TYPE},
        {"rxpacket", SQL_REAL_TYPE},
        {"rxbyte", SQL_REAL_TYPE},
        {"rxpackets", SQL_REAL_TYPE},
        {"rxbytes", SQL_REAL_TYPE},
        {"rxerrors", SQL_REAL_TYPE},
        {"rxdropped", SQL_REAL_TYPE},
        {"txpacket", SQL_REAL_TYPE},
        {"txbyte", SQL_REAL_TYPE},
        {"txpackets", SQL_REAL_TYPE},
        {"txbytes", SQL_REAL_TYPE},
        {"txerrors", SQL_REAL_TYPE},
        {"txdropped", SQL_REAL_TYPE},
        {"funcid", SQL_INTEGER_TYPE}
    };

    const TableColumns NicReportData = {
        {"device_id", SQL_INTEGER_TYPE},
        {"duration", SQL_TEXT_TYPE},
        {"bandwidth", SQL_TEXT_TYPE},
        {"rxbandwidth", SQL_TEXT_TYPE},
        {"txbandwidth", SQL_TEXT_TYPE},
        {"rxpacket", SQL_TEXT_TYPE},
        {"rxerrorrate", SQL_TEXT_TYPE},
        {"rxdroppedrate", SQL_TEXT_TYPE},
        {"txpacket", SQL_TEXT_TYPE},
        {"txerrorrate", SQL_TEXT_TYPE},
        {"txdroppedrate", SQL_TEXT_TYPE},
        {"funcid", SQL_INTEGER_TYPE}
    };

    const TableColumns NicReceiveSend = {
        {"device_id", SQL_INTEGER_TYPE},
        {"timestamp", SQL_REAL_TYPE},
        {"rx_bandwidth_efficiency", SQL_REAL_TYPE},
        {"rx_packets", SQL_REAL_TYPE},
        {"rx_error_rate", SQL_REAL_TYPE},
        {"rx_dropped_rate", SQL_REAL_TYPE},
        {"tx_bandwidth_efficiency", SQL_REAL_TYPE},
        {"tx_packets", SQL_REAL_TYPE},
        {"tx_error_rate", SQL_REAL_TYPE},
        {"tx_dropped_rate", SQL_REAL_TYPE},
        {"func_id", SQL_INTEGER_TYPE}
    };

    const TableColumns RoceOriginalData = {
        {"device_id", SQL_INTEGER_TYPE},
        {"replayid", SQL_INTEGER_TYPE},
        {"timestamp", SQL_REAL_TYPE},
        {"bandwidth", SQL_INTEGER_TYPE},
        {"rxpacket", SQL_REAL_TYPE},
        {"rxbyte", SQL_REAL_TYPE},
        {"rxpackets", SQL_REAL_TYPE},
        {"rxbytes", SQL_REAL_TYPE},
        {"rxerrors", SQL_REAL_TYPE},
        {"rxdropped", SQL_REAL_TYPE},
        {"txpacket", SQL_REAL_TYPE},
        {"txbyte", SQL_REAL_TYPE},
        {"txpackets", SQL_REAL_TYPE},
        {"txbytes", SQL_REAL_TYPE},
        {"txerrors", SQL_REAL_TYPE},
        {"txdropped", SQL_REAL_TYPE},
        {"funcid", SQL_INTEGER_TYPE}
    };

    const TableColumns RoceReportData = {
        {"device_id", SQL_INTEGER_TYPE},
        {"duration", SQL_TEXT_TYPE},
        {"bandwidth", SQL_TEXT_TYPE},
        {"rxbandwidth", SQL_TEXT_TYPE},
        {"txbandwidth", SQL_TEXT_TYPE},
        {"rxpacket", SQL_TEXT_TYPE},
        {"rxerrorrate", SQL_TEXT_TYPE},
        {"rxdroppedrate", SQL_TEXT_TYPE},
        {"txpacket", SQL_TEXT_TYPE},
        {"txerrorrate", SQL_TEXT_TYPE},
        {"txdroppedrate", SQL_TEXT_TYPE},
        {"funcid", SQL_INTEGER_TYPE}
    };

    const TableColumns RoceReceiveSend = {
        {"device_id", SQL_INTEGER_TYPE},
        {"timestamp", SQL_REAL_TYPE},
        {"rx_bandwidth_efficiency", SQL_REAL_TYPE},
        {"rx_packets", SQL_REAL_TYPE},
        {"rx_error_rate", SQL_REAL_TYPE},
        {"rx_dropped_rate", SQL_REAL_TYPE},
        {"tx_bandwidth_efficiency", SQL_REAL_TYPE},
        {"tx_packets", SQL_REAL_TYPE},
        {"tx_error_rate", SQL_REAL_TYPE},
        {"tx_dropped_rate", SQL_REAL_TYPE},
        {"func_id", SQL_INTEGER_TYPE}
    };

    const TableColumns HBMbwData = {
        {"device_id", SQL_INTEGER_TYPE},
        {"timestamp", SQL_REAL_TYPE},
        {"bandwidth", SQL_REAL_TYPE},
        {"hbmid", SQL_INTEGER_TYPE},
        {"event_type", SQL_TEXT_TYPE}
    };

    const TableColumns DDRMetricData = {
        {"device_id", SQL_INTEGER_TYPE},
        {"replayid", SQL_INTEGER_TYPE},
        {"timestamp", SQL_REAL_TYPE},
        {"flux_read", SQL_REAL_TYPE},
        {"flux_write", SQL_REAL_TYPE},
        {"fluxid_read", SQL_REAL_TYPE},
        {"fluxid_write", SQL_REAL_TYPE}
    };

    const TableColumns LLCOriginData = {
        {"device_id", SQL_INTEGER_TYPE},
        {"l3tid", SQL_INTEGER_TYPE},
        {"timestamp", SQL_REAL_TYPE},
        {"hitrate", SQL_REAL_TYPE},
        {"throughput", SQL_REAL_TYPE},
    };

    const TableColumns SampleAICoreOriginalData = {
        {"mode", SQL_INTEGER_TYPE},
        {"replayid", SQL_INTEGER_TYPE},
        {"timestamp", SQL_NUMERIC_TYPE},
        {"coreid", SQL_INTEGER_TYPE},
        {"task_cyc", SQL_TEXT_TYPE},
        {"event1", SQL_TEXT_TYPE},
        {"event2", SQL_TEXT_TYPE},
        {"event3", SQL_TEXT_TYPE},
        {"event4", SQL_TEXT_TYPE},
        {"event5", SQL_TEXT_TYPE},
        {"event6", SQL_TEXT_TYPE},
        {"event7", SQL_TEXT_TYPE},
        {"event8", SQL_TEXT_TYPE},
    };

    const TableColumns SampleMetricSummary = {
        {"metric", SQL_TEXT_TYPE},
        {"value", SQL_NUMERIC_TYPE},
        {"coreid", SQL_INTEGER_TYPE},
    };

    const TableColumns PCIE = {
        {"timestamp", SQL_INTEGER_TYPE},
        {"device_id", SQL_INTEGER_TYPE},
        {"tx_p_bandwidth_min", SQL_REAL_TYPE},
        {"tx_p_bandwidth_max", SQL_REAL_TYPE},
        {"tx_p_bandwidth_avg", SQL_REAL_TYPE},
        {"tx_np_bandwidth_min", SQL_REAL_TYPE},
        {"tx_np_bandwidth_max", SQL_REAL_TYPE},
        {"tx_np_bandwidth_avg", SQL_REAL_TYPE},
        {"tx_cpl_bandwidth_min", SQL_REAL_TYPE},
        {"tx_cpl_bandwidth_max", SQL_REAL_TYPE},
        {"tx_cpl_bandwidth_avg", SQL_REAL_TYPE},
        {"tx_np_lantency_min", SQL_REAL_TYPE},
        {"tx_np_lantency_max", SQL_REAL_TYPE},
        {"tx_np_lantency_avg", SQL_REAL_TYPE},
        {"rx_p_bandwidth_min", SQL_REAL_TYPE},
        {"rx_p_bandwidth_max", SQL_REAL_TYPE},
        {"rx_p_bandwidth_avg", SQL_REAL_TYPE},
        {"rx_np_bandwidth_min", SQL_REAL_TYPE},
        {"rx_np_bandwidth_max", SQL_REAL_TYPE},
        {"rx_np_bandwidth_avg", SQL_REAL_TYPE},
        {"rx_cpl_bandwidth_min", SQL_REAL_TYPE},
        {"rx_cpl_bandwidth_max", SQL_REAL_TYPE},
        {"rx_cpl_bandwidth_avg", SQL_REAL_TYPE}
    };

    const TableColumns HCCS = {
        {"device_id", SQL_INTEGER_TYPE},
        {"timestamp", SQL_REAL_TYPE},
        {"txthroughput", SQL_INTEGER_TYPE},
        {"rxthroughput", SQL_INTEGER_TYPE},
    };

    const TableColumns AccPmu = {
        {"acc_id", SQL_INTEGER_TYPE},
        {"read_bandwidth", SQL_INTEGER_TYPE},
        {"write_bandwidth", SQL_INTEGER_TYPE},
        {"read_ost", SQL_INTEGER_TYPE},
        {"write_ost", SQL_INTEGER_TYPE},
        {"timestamp", SQL_NUMERIC_TYPE}
    };

    const TableColumns InterSoc = {
        {"l2_buffer_bw_level", SQL_INTEGER_TYPE},
        {"mata_bw_level", SQL_INTEGER_TYPE},
        {"sys_time", SQL_REAL_TYPE}
    };

    const TableColumns FreqParse = {
        {"syscnt", SQL_INTEGER_TYPE},
        {"freq", SQL_INTEGER_TYPE}
    };

    const TableColumns MsprofTx = {
        {"pid", SQL_INTEGER_TYPE},
        {"tid", SQL_INTEGER_TYPE},
        {"category", SQL_INTEGER_TYPE},
        {"event_type", SQL_TEXT_TYPE},
        {"payload_type", SQL_INTEGER_TYPE},
        {"payload_value", SQL_INTEGER_TYPE},
        {"start_time", SQL_INTEGER_TYPE},
        {"end_time", SQL_INTEGER_TYPE},
        {"message_type", SQL_INTEGER_TYPE},
        {"message", SQL_TEXT_TYPE},
        {"file_tag", SQL_INTEGER_TYPE}
    };

    const TableColumns MsprofTxEx = {
        {"pid", SQL_INTEGER_TYPE},
        {"tid", SQL_INTEGER_TYPE},
        {"event_type", SQL_TEXT_TYPE},
        {"start_time", SQL_INTEGER_TYPE},
        {"end_time", SQL_INTEGER_TYPE},
        {"mark_id", SQL_INTEGER_TYPE},
        {"message", SQL_TEXT_TYPE},
    };

    const TableColumns StepTrace = {
        {"index_id", SQL_INTEGER_TYPE},
        {"model_id", SQL_INTEGER_TYPE},
        {"timestamp", SQL_NUMERIC_TYPE},
        {"stream_id", SQL_INTEGER_TYPE},
        {"task_id", SQL_INTEGER_TYPE},
        {"tag_id", SQL_INTEGER_TYPE}
    };

    const TableColumns TaskType = {
        {"timestamp", SQL_NUMERIC_TYPE},
        {"stream_id", SQL_INTEGER_TYPE},
        {"task_id", SQL_INTEGER_TYPE},
        {"task_type", SQL_TEXT_TYPE},
        {"task_state", SQL_INTEGER_TYPE}
    };

    const TableColumns TsMemcpy = {
        {"timestamp", SQL_REAL_TYPE},
        {"stream_id", SQL_INTEGER_TYPE},
        {"task_id", SQL_INTEGER_TYPE},
        {"task_state", SQL_INTEGER_TYPE}
    };

    const TableColumns KfcCommTurn = {
        {"device_id", SQL_INTEGER_TYPE},
        {"stream_id", SQL_INTEGER_TYPE},
        {"task_id", SQL_INTEGER_TYPE},
        {"comm_turn", SQL_INTEGER_TYPE},
        {"current_turn", SQL_INTEGER_TYPE},
        {"wait_notify_start_time", SQL_INTEGER_TYPE},
        {"kfc_alg_exe_start_time", SQL_INTEGER_TYPE},
        {"send_task_start_time", SQL_INTEGER_TYPE},
        {"wait_active_start_time", SQL_INTEGER_TYPE},
        {"active_start_time", SQL_INTEGER_TYPE},
        {"wait_exe_end_start_time", SQL_INTEGER_TYPE},
        {"rtsq_exe_end_time", SQL_INTEGER_TYPE},
    };

    const TableColumns KfcComputeTurn = {
        {"device_id", SQL_INTEGER_TYPE},
        {"stream_id", SQL_INTEGER_TYPE},
        {"task_id", SQL_INTEGER_TYPE},
        {"compute_turn", SQL_INTEGER_TYPE},
        {"current_turn", SQL_INTEGER_TYPE},
        {"wait_compute_start_time", SQL_INTEGER_TYPE},
        {"compute_start_time", SQL_INTEGER_TYPE},
        {"compute_exe_end_time", SQL_INTEGER_TYPE},
    };

    const TableColumns CpuUsage = {
        {"start_time", SQL_NUMERIC_TYPE},
        {"end_time", SQL_NUMERIC_TYPE},
        {"cpu_no", SQL_TEXT_TYPE},
        {"usage", SQL_REAL_TYPE}
    };

    const TableColumns MemUsage = {
        {"start_time", SQL_NUMERIC_TYPE},
        {"end_time", SQL_NUMERIC_TYPE},
        {"usage", SQL_REAL_TYPE}
    };

    const TableColumns DiskUsage = {
        {"start_time", SQL_NUMERIC_TYPE},
        {"end_time", SQL_NUMERIC_TYPE},
        {"disk_read", SQL_REAL_TYPE},
        {"disk_write", SQL_REAL_TYPE},
        {"swap_in", SQL_TEXT_TYPE},
        {"usage", SQL_REAL_TYPE}
    };

    const TableColumns NetworkUsage = {
        {"start_time", SQL_NUMERIC_TYPE},
        {"end_time", SQL_NUMERIC_TYPE},
        {"usage", SQL_REAL_TYPE},
        {"speed", SQL_REAL_TYPE}
    };

    const TableColumns PaLinkInfo = {
        {"pa_link_id", SQL_INTEGER_TYPE},
        {"pa_link_traffic_monit_rx", SQL_TEXT_TYPE},
        {"pa_link_traffic_monit_tx", SQL_TEXT_TYPE},
        {"sys_time", SQL_INTEGER_TYPE}
    };

    const TableColumns PcieInfo = {
        {"pcie_id", SQL_INTEGER_TYPE},
        {"pcie_write_bandwidth", SQL_INTEGER_TYPE},
        {"pcie_read_bandwidth", SQL_INTEGER_TYPE},
        {"sys_time", SQL_INTEGER_TYPE}
    };

    const TableColumns GeLogicStreamInfo = {
        {"physic_stream", SQL_INTEGER_TYPE},
        {"logic_stream", SQL_INTEGER_TYPE},
    };

    const TableColumns Sio = {
        {"acc_id", SQL_INTEGER_TYPE},
        {"req_rx", SQL_NUMERIC_TYPE},
        {"rsp_rx", SQL_NUMERIC_TYPE},
        {"snp_rx", SQL_NUMERIC_TYPE},
        {"dat_rx", SQL_NUMERIC_TYPE},
        {"req_tx", SQL_NUMERIC_TYPE},
        {"rsp_tx", SQL_NUMERIC_TYPE},
        {"snp_tx", SQL_NUMERIC_TYPE},
        {"dat_tx", SQL_NUMERIC_TYPE},
        {"timestamp", SQL_NUMERIC_TYPE}
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
    tableColNames_["ApiData"] = ApiEventData;
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

GeModelInfoDB::GeModelInfoDB()
{
    dbName_ = "ge_model_info.db";
    tableColNames_["GeFusionOpInfo"] = GeFusionOPInfo;
    tableColNames_["ModelName"] = ModelName;
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

TraceDB::TraceDB()
{
    dbName_ = "trace.db";
    tableColNames_["all_reduce"] = AllReduce;
    tableColNames_["get_next"] = GetNext;
    tableColNames_["training_trace"] = TrainingTrace;
}

HCCLSingleDeviceDB::HCCLSingleDeviceDB()
{
    dbName_ = "hccl_single_device.db";
    tableColNames_["HcclOpReport"] = HcclOpReport;
    tableColNames_["HCCLOpSingleDevice"] = HCCLOpSingleDevice;
    tableColNames_["HCCLTaskSingleDevice"] = HCCLTaskSingleDevice;
}

NpuMemDB::NpuMemDB()
{
    dbName_ = "npu_mem.db";
    tableColNames_["NpuMem"] = NpuMem;
}

NpuModuleMemDB::NpuModuleMemDB()
{
    dbName_ = "npu_module_mem.db";
    tableColNames_["NpuModuleMem"] = NpuModuleMem;
}

TaskMemoryDB::TaskMemoryDB()
{
    dbName_ = "task_memory.db";
    tableColNames_["NpuOpMemRaw"] = NpuOpMemRaw;
}

NicDB::NicDB()
{
    dbName_ = "nic.db";
    tableColNames_["NicOriginalData"] = NicOriginalData;
    tableColNames_["NicReportData"] = NicReportData;
}

NicReceiveSendDB::NicReceiveSendDB()
{
    dbName_ = "nicreceivesend_table.db";
    tableColNames_["NicReceiveSend"] = NicReceiveSend;
}

RoceDB::RoceDB()
{
    dbName_ = "roce.db";
    tableColNames_["RoceOriginalData"] = RoceOriginalData;
    tableColNames_["RoceReportData"] = RoceReportData;
}

RoceReceiveSendDB::RoceReceiveSendDB()
{
    dbName_ = "rocereceivesend_table.db";
    tableColNames_["RoceReceiveSend"] = RoceReceiveSend;
}

HBMDB::HBMDB()
{
    dbName_ = "hbm.db";
    tableColNames_["HBMbwData"] = HBMbwData;
}

DDRDB::DDRDB()
{
    dbName_ = "ddr.db";
    tableColNames_["DDRMetricData"] = DDRMetricData;
}

LLCDB::LLCDB()
{
    dbName_ = "llc.db";
    tableColNames_["LLCMetrics"] = LLCOriginData;
}
AccPmuDB::AccPmuDB()
{
    dbName_ = "acc_pmu.db";
    tableColNames_["AccPmu"] = AccPmu;
}

SocProfilerDB::SocProfilerDB()
{
    dbName_ = "soc_profiler.db";
    tableColNames_["InterSoc"] = InterSoc;
};

AicoreDB::AicoreDB()
{
    dbName_ = "aicore.db";
    tableColNames_["AICoreOriginalData"] = SampleAICoreOriginalData;
    tableColNames_["MetricSummary"] = SampleMetricSummary;
}

AiVectorCoreDB::AiVectorCoreDB()
{
    dbName_ = "ai_vector_core.db";
    tableColNames_["AICoreOriginalData"] = SampleAICoreOriginalData;
    tableColNames_["MetricSummary"] = SampleMetricSummary;
}

PCIeDB::PCIeDB()
{
    dbName_ = "pcie.db";
    tableColNames_["PcieOriginalData"] = PCIE;
}

HCCSDB::HCCSDB()
{
    dbName_ = "hccs.db";
    tableColNames_["HCCSEventsData"] = HCCS;
}

FreqDB::FreqDB()
{
    dbName_ = "freq.db";
    tableColNames_["FreqParse"] = FreqParse;
}

MsprofTxDB::MsprofTxDB()
{
    dbName_ = "msproftx.db";
    tableColNames_["MsprofTx"] = MsprofTx;
    tableColNames_["MsprofTxEx"] = MsprofTxEx;
}

StepTraceDB::StepTraceDB()
{
    dbName_ = "step_trace.db";
    tableColNames_["StepTrace"] = StepTrace;
    tableColNames_["step_trace_data"] = StepTraceData;
    tableColNames_["StepTime"] = StepTime;
    tableColNames_["TaskType"] = TaskType;
    tableColNames_["TsMemcpy"] = TsMemcpy;
}

KfcInfo::KfcInfo()
{
    dbName_ = "kfc_info.db";
    tableColNames_["KfcCommTurn"] = KfcCommTurn;
    tableColNames_["KfcComputeTurn"] = KfcComputeTurn;
}

HostCpuUsage::HostCpuUsage()
{
    dbName_ = "host_cpu_usage.db";
    tableColNames_["CpuUsage"] = CpuUsage;
}

HostMemUsage::HostMemUsage()
{
    dbName_ = "host_mem_usage.db";
    tableColNames_["MemUsage"] = MemUsage;
}

HostDiskUsage::HostDiskUsage()
{
    dbName_ = "host_disk_usage.db";
    tableColNames_["DiskUsage"] = DiskUsage;
}

HostNetworkUsage::HostNetworkUsage()
{
    dbName_ = "host_network_usage.db";
    tableColNames_["NetworkUsage"] = NetworkUsage;
}

ChipTransDB::ChipTransDB()
{
    dbName_ = "step_trace.db";
    tableColNames_["PaLinkInfo"] = PaLinkInfo;
    tableColNames_["PcieInfo"] = PcieInfo;
}

GeLogicStreamDB::GeLogicStreamDB()
{
    dbName_ = "ge_logic_stream_info.db";
    tableColNames_["GeLogicStreamInfo"] = GeLogicStreamInfo;
}

SioDB::SioDB()
{
    dbName_ = "sio.db";
    tableColNames_["Sio"] = Sio;
}
} // namespace Database
} // namespace Viewer
} // namespace Analysis