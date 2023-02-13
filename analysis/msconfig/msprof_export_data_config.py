#!/usr/bin/python3
# -*- coding: utf-8 -*-
#  Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from msconfig.meta_config import MetaConfig


class MsProfExportDataConfig(MetaConfig):
    DATA = {
        'runtime_api': [
            ('handler', '_get_runtime_api_data'),
            ('headers', 'Name,Stream ID,Time(%),Time(ns),Calls,Avg(ns),Min(ns),Max(ns),Process ID,Thread ID'),
            ('table', 'ApiCall'),
            ('db', 'runtime.db')
        ]
        , 'task_time': [
            ('handler', '_get_task_time_data'),
            ('headers',
             'Time(%),Time(us),Count,Avg(us),Min(us),Max(us),Waiting(us),Running(us),'
             'Pending(us),Type,API,Task ID,Op Name,Stream ID'),
            ('table', 'ReportTask'),
            ('db', 'runtime.db')
        ]
        , 'ddr': [
            ('handler', '_get_ddr_data'),
            ('headers', 'Metric,Read(MB/s),Write(MB/s)'),
            ('db', 'ddr.db')
        ]
        , 'cpu_usage': [
            ('handler', '_get_cpu_usage_data'),
            ('headers', 'Cpu Type,User(%),Sys(%),IoWait(%),Irq(%),Soft(%),Idle(%)'),
            ('db', 'cpu_usage_{}.db'),
            ('table', 'SysCpuUsage')
        ]
        , 'process_cpu_usage': [
            ('handler', '_get_cpu_usage_data'),
            ('headers', 'PID,Name,CPU(%)'),
            ('db', 'cpu_usage_{}.db'),
            ('table', 'ProCpuUsage')
        ]
        , 'sys_mem': [
            ('handler', '_get_memory_data'),
            ('headers',
             'Memory Total,Memory Free,Buffers,Cached,Share Memory,Commit Limit,'
             'Committed AS,Huge Pages Total(pages),Huge Pages Free(pages)'),
            ('db', 'memory_{}.db'),
            ('table', 'sysmem')
        ]
        , 'process_mem': [
            ('handler', '_get_memory_data'),
            ('headers', 'PID,Name,Size(pages),Resident(pages),Shared(pages)'),
            ('db', 'memory_{}.db'),
            ('table', 'pidmem')
        ]
        , 'acl': [
            ('handler', '_get_acl_data'),
            ('headers', 'Name,Type,Start Time,Duration(us),Process ID,Thread ID'),
            ('db', 'acl_module.db'),
            ('table', 'acl_data')
        ]
        , 'acl_statistic': [
            ('handler', '_get_acl_statistic_data'),
            ('headers', 'Name,Type,Time(%),Time(us),Count,Avg(us),Min(us),Max(us),Process ID,Thread ID'),
            ('db', 'acl_module.db'),
            ('table', 'acl_data')
        ]
        , 'op_summary': [
            ('handler', '_get_op_summary_data'),
            ('headers',
             'Model Name,Model ID,Task ID,Stream ID,Infer ID,Op Name,OP Type,Task Type,'
             'Task Start Time,Task Duration(us),Task Wait Time(us),Block Dim,Mix Block Dim'),
            ('db', 'ai_core_op_summary.db')
        ]
        , 'ai_stack_time': [
            ('handler', '_get_ai_stack_time_data'),
            ('headers', 'Infer ID,Module,API,Start Time,Duration(ns)')
        ]
        , 'l2_cache': [
            ('handler', '_get_l2_cache_data'),
            ('db', 'l2cache.db'),
            ('table', 'L2CacheSummary'),
            ('unused_cols', 'device_id,task_type'),
            ('headers', 'Read,Write,Victim,Allocate,Timestamp(ns)')
        ]
        , 'step_trace': [
            ('handler', '_get_step_trace_data'),
            ('headers',
             'Iteration ID,FP Start,BP End,Iteration End,Iteration Time(us),FP to BP Time(us),'
             'Iteration Refresh(us),Data Aug Bound(us)')
        ]
        , 'aicpu': [
            ('handler', '_get_aicpu_data')
        ]
        , 'dp': [
            ('handler', '_get_dp_data'),
            ('headers', 'Timestamp,Action,Source,Cached Buffer Size')
        ]
        , 'op_statistic': [
            ('handler', '_get_op_statistic_data'),
            ('headers',
             'Model Name,OP Type,Core Type,Count,Total Time(us),Min Time(us),Avg Time(us),Max Time(us),Ratio(%)'),
            ('db', 'op_counter.db')
        ]
        , 'hbm': [
            ('handler', '_get_hbm_data'),
            ('headers', 'Metric,Read(MB/s),Write(MB/s)')
        ]
        , 'pcie': [
            ('handler', '_get_pcie_data')
        ]
        , 'fusion_op': [
            ('handler', '_get_fusion_op_data'),
            ('headers',
             'Model Name,Model ID,Fusion Op,Original Ops,Memory Input(Bytes),Memory Output(Bytes),'
             'Memory Weight(Bytes),Memory Workspace(Bytes),Memory Total(Bytes)'),
            ('db', 'ge_model_info.db'),
            ('table', 'GeModelLoad')
        ]
        , 'ai_core_utilization': [
            ('handler', '_get_ai_core_sample_based_data'),
            ('db', 'aicore_{}.db')
        ]
        , 'ai_vector_core_utilization': [
            ('handler', '_get_aiv_sample_based_data'),
            ('db', 'ai_vector_core_{}.db')
        ]
        , 'hccs': [
            ('handler', '_get_hccs_data')
        ]
        , 'llc_bandwidth': [
            ('handler', '_get_llc_data')
        ]
        , 'llc_aicpu': [
            ('handler', '_get_llc_data')
        ]
        , 'llc_ctrlcpu': [
            ('handler', '_get_llc_data')
        ]
        , 'llc_read_write': [
            ('handler', '_get_llc_data')
        ]
        , 'dvpp': [
            ('handler', '_get_dvpp_data'),
            ('db', 'peripheral.db'),
            ('table', 'DvppReportData'),
            ('headers', 'Dvpp Id,Engine Type,Engine ID,All Time(us),All Frame,All Utilization(%)')
        ]
        , 'nic': [
            ('handler', '_get_nic_data'),
            ('db', 'nic.db'),
            ('table', 'NicReportData'),
            ('headers',
             'Timestamp,Bandwidth(MB/s),Rx Bandwidth efficiency(%),rxPacket/s,rxError rate(%),'
             'rxDropped rate(%),Tx Bandwidth efficiency(%),txPacket/s,txError rate(%),txDropped rate(%),funcId'),
            ('columns',
             'duration,bandwidth,rxBandwidth,rxPacket,rxErrorRate,rxDroppedRate,txBandwidth,'
             'txPacket,txErrorRate,txDroppedRate,funcId')
        ]
        , 'roce': [
            ('handler', '_get_roce_data'),
            ('db', 'roce.db'),
            ('table', 'RoceReportData'),
            ('headers',
             'Timestamp,Bandwidth(MB/s),Rx Bandwidth efficiency(%),rxPacket/s,rxError rate(%),'
             'rxDropped rate(%),Tx Bandwidth efficiency(%),txPacket/s,txError rate(%),txDropped rate(%),funcId'),
            ('columns',
             'duration,bandwidth,rxBandwidth,rxPacket,rxErrorRate,rxDroppedRate,txBandwidth,'
             'txPacket,txErrorRate,txDroppedRate,funcId')
        ]
        , 'ctrl_cpu_pmu_events': [
            ('handler', '_get_cpu_pmu_events'),
            ('table', 'OriginalData'),
            ('headers', 'Event,Name,Count'),
            ('db', 'ctrlcpu_{}.db')
        ]
        , 'ctrl_cpu_top_function': [
            ('handler', '_get_cpu_top_funcs'),
            ('table', 'EventCount'),
            ('headers', 'Function,Module,Cycles,Cycles(%)'),
            ('db', 'ctrlcpu_{}.db')
        ]
        , 'ai_cpu_pmu_events': [
            ('handler', '_get_cpu_pmu_events'),
            ('table', 'OriginalData'),
            ('headers', 'Event,Name,Count'),
            ('db', 'aicpu_{}.db')
        ]
        , 'ai_cpu_top_function': [
            ('handler', '_get_cpu_top_funcs'),
            ('table', 'EventCount'),
            ('headers', 'Function,Module,Cycles,Cycles(%)'),
            ('db', 'aicpu_{}.db')
        ]
        , 'ts_cpu_pmu_events': [
            ('handler', '_get_cpu_pmu_events'),
            ('table', 'TsOriginalData'),
            ('headers', 'Event,Name,Count'),
            ('db', 'tscpu_{}.db')
        ]
        , 'ts_cpu_top_function': [
            ('handler', '_get_ts_cpu_top_funcs'),
            ('db', 'tscpu_{}.db')
        ]
        , 'ge': [
            ('handler', '_get_ge_data')
        ]
        , 'ge_op_execute': [
            ('handler', '_get_ge_op_execute_data'),
            ('headers', 'Thread ID,OP Name,OP Type,Event Type,Start Time,Duration(us)')
        ]
        , 'os_runtime_api': [
            ('handler', '_get_host_runtime_api'),
            ('db', 'runtime_api_analysis{}.db')
        ]
        , 'os_runtime_statistic': [
            ('handler', '_get_host_runtime_api'),
            ('headers', 'Process ID,Thread ID,Name,Time(%),Time(us),Count,Avg(us),Max(us),Min(us)')
        ]
        , 'host_cpu_usage': [
            ('handler', '_get_host_cpu_usage_data'),
            ('headers', 'Total Cpu Numbers, Occupied Cpu Numbers, Recommend Cpu Numbers')
        ]
        , 'host_mem_usage': [
            ('handler', '_get_host_mem_usage_data'),
            ('headers', 'Total Memory(KB), Peak Used Memory(KB), Recommend Memory(KB)')
        ]
        , 'host_network_usage': [
            ('handler', '_get_host_network_usage_data'),
            ('headers', 'Netcard Speed(KB/s), Peak Used Speed(KB/s), Recommend Speed(KB/s)')
        ]
        , 'host_disk_usage': [
            ('handler', '_get_host_disk_usage_data'),
            ('headers', 'Peak Disk Read(KB/s), Recommend Disk Read(KB/s), '
                        'Peak Disk Write(KB/s), Recommend Disk Write(KB/s)')
        ]
        , 'msprof': [
            ('handler', '_get_bulk_data')
        ]
        , 'ffts_sub_task_time': [
            ('handler', '_get_sub_task_time'),
            ('db', 'soc_log.db'),
            ('table', 'FftsLog')
        ]
        , 'hccl': [
            ('handler', '_get_hccl_timeline'),
            ('db', 'hccl.db'),
            ('table', 'HCCLAllReduce')
        ]
        , 'msprof_tx': [
            ('handler', '_get_msproftx_data'),
            ('db', 'msproftx.db'),
            ('table', 'MsprofTx'),
            ('headers',
             'pid, tid, category, event_type, payload_type, payload_value, Start_time(ns), '
             'End_time(ns), message_type, message, call_trace')
        ]
        , 'inter_soc_transmission': [
            ('handler', '_get_inter_soc_summary'),
            ('db', 'soc_profiler.db'),
            ('table', 'InterSoc'),
            ('headers', 'Metric,L2Buffer_bandwidth_level,MATA_bandwidth_level')
        ]
        , 'acc_pmu': [
            ('handler', '_get_acc_pmu'),
            ('db', 'acc_pmu.db'),
            ('table', 'AccPmu'),
            ('headers',
             'task_id,stream_id,acc_id,block_id,read_bandwidth,write_'
             'bandwidth,read_ost,write_ost,time_stamp,start_time,dur_time')
        ]
        , 'stars_soc': [
            ('handler', '_get_stars_soc_data'),
            ('db', 'soc_profiler.db'),
            ('table', 'InterSoc')
        ]
        , 'stars_chip_trans': [
            ('handler', '_get_stars_chip_trans_data'),
            ('db', 'chip_trans.db')
        ]
        , 'thread_group': [
            ('handler', '_get_thread_group_data')
        ]
        , 'low_power': [
            ('handler', '_get_low_power_data'),
            ('db', 'lowpower.db'),
            ('table', 'LowPower')
        ]
        , 'biu_perf': [
            ('handler', '_get_biu_perf_timeline')
        ]
    }
