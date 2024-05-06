#!/usr/bin/python3
# -*- coding: utf-8 -*-
#  Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from msconfig.meta_config import MetaConfig


class MsProfExportDataConfig(MetaConfig):
    DATA = {
        'task_time': [
            ('handler', '_get_task_time_data'),
            ('headers',
             'Time(%),Time(us),Count,Avg(us),Min(us),Max(us),Waiting(us),Running(us),'
             'Pending(us),Type,API,Task ID,Op Name,Stream ID'),
            ('table', 'ReportTask'),
            ('db', 'runtime.db')
        ],
        'ddr': [
            ('handler', '_get_ddr_data'),
            ('headers', 'Metric,Read(MB/s),Write(MB/s)'),
            ('db', 'ddr.db')
        ],
        'cpu_usage': [
            ('handler', '_get_cpu_usage_data'),
            ('headers', 'Cpu Type,User(%),Sys(%),IoWait(%),Irq(%),Soft(%),Idle(%)'),
            ('db', 'cpu_usage_{}.db'),
            ('table', 'SysCpuUsage')
        ],
        'process_cpu_usage': [
            ('handler', '_get_cpu_usage_data'),
            ('headers', 'PID,Name,CPU(%)'),
            ('db', 'cpu_usage_{}.db'),
            ('table', 'ProCpuUsage')
        ],
        'sys_mem': [
            ('handler', '_get_memory_data'),
            ('headers',
             'Memory Total,Memory Free,Buffers,Cached,Share Memory,Commit Limit,'
             'Committed AS,Huge Pages Total(pages),Huge Pages Free(pages)'),
            ('db', 'memory_{}.db'),
            ('table', 'sysmem')
        ],
        'process_mem': [
            ('handler', '_get_memory_data'),
            ('headers', 'PID,Name,Size(pages),Resident(pages),Shared(pages)'),
            ('db', 'memory_{}.db'),
            ('table', 'pidmem')
        ],
        'op_summary': [
            ('handler', '_get_op_summary_data'),
            ('headers',
             'Model Name,Model ID,Task ID,Stream ID,Infer ID,Op Name,OP Type,OP State,Task Type,'
             'Task Start Time(us),Task Duration(us),Task Wait Time(us),Block Dim,Mix Block Dim,HF32 Eligible'),
            ('db', 'ai_core_op_summary.db')
        ],
        'l2_cache': [
            ('handler', '_get_l2_cache_data'),
            ('db', 'l2cache.db'),
            ('table', 'L2CacheSummary'),
            ('unused_cols', 'device_id,task_type'),
        ],
        'step_trace': [
            ('handler', '_get_step_trace_data'),
            ('headers',
             'Iteration ID,FP Start(us),BP End(us),Iteration End(us),Iteration Time(us),FP to BP Time(us),'
             'Iteration Refresh(us),Data Aug Bound(us)')
        ],
        'aicpu': [
            ('handler', '_get_aicpu_data')
        ],
        'dp': [
            ('handler', '_get_dp_data'),
            ('headers', 'Timestamp(us),Action,Source,Cached Buffer Size')
        ],
        'op_statistic': [
            ('handler', '_get_op_statistic_data'),
            ('headers',
             'Model Name,OP Type,Core Type,Count,Total Time(us),Min Time(us),Avg Time(us),Max Time(us),Ratio(%)'),
            ('db', 'op_counter.db')
        ],
        'hbm': [
            ('handler', '_get_hbm_data'),
            ('headers', 'Metric,Read(MB/s),Write(MB/s)')
        ],
        'pcie': [
            ('handler', '_get_pcie_data')
        ],
        'fusion_op': [
            ('handler', '_get_fusion_op_data'),
            ('headers',
             'Model Name,Model ID,Fusion Op,Original Ops,Memory Input(KB),Memory Output(KB),'
             'Memory Weight(KB),Memory Workspace(KB),Memory Total(KB)'),
            ('db', 'ge_model_info.db'),
            ('table', 'ModelName')
        ],
        'ai_core_utilization': [
            ('handler', '_get_ai_core_sample_based_data'),
            ('db', 'aicore_{}.db')
        ],
        'ai_vector_core_utilization': [
            ('handler', '_get_aiv_sample_based_data'),
            ('db', 'ai_vector_core_{}.db')
        ],
        'hccs': [
            ('handler', '_get_hccs_data')
        ],
        'llc_bandwidth': [
            ('handler', '_get_llc_data')
        ],
        'llc_aicpu': [
            ('handler', '_get_llc_data')
        ],
        'llc_ctrlcpu': [
            ('handler', '_get_llc_data')
        ],
        'llc_read_write': [
            ('handler', '_get_llc_data')
        ],
        'dvpp': [
            ('handler', '_get_dvpp_data'),
            ('db', 'peripheral.db'),
            ('table', 'DvppReportData'),
            ('headers', 'Dvpp Id,Engine Type,Engine ID,All Time(us),All Frame,All Utilization(%)')
        ],
        'nic': [
            ('handler', '_get_nic_data'),
            ('db', 'nic.db'),
            ('table', 'NicReportData'),
            ('headers',
             'Timestamp(us),Bandwidth(MB/s),Rx Bandwidth efficiency(%),rxPacket/s,rxError rate(%),'
             'rxDropped rate(%),Tx Bandwidth efficiency(%),txPacket/s,txError rate(%),txDropped rate(%),funcId')
        ],
        'roce': [
            ('handler', '_get_roce_data'),
            ('db', 'roce.db'),
            ('table', 'RoceReportData'),
            ('headers',
             'Timestamp(us),Bandwidth(MB/s),Rx Bandwidth efficiency(%),rxPacket/s,rxError rate(%),'
             'rxDropped rate(%),Tx Bandwidth efficiency(%),txPacket/s,txError rate(%),txDropped rate(%),funcId')
        ],
        'ctrl_cpu_pmu_events': [
            ('handler', '_get_cpu_pmu_events'),
            ('table', 'OriginalData'),
            ('headers', 'Event,Name,Count'),
            ('db', 'ctrlcpu_{}.db')
        ],
        'ctrl_cpu_top_function': [
            ('handler', '_get_cpu_top_funcs'),
            ('table', 'EventCount'),
            ('headers', 'Function,Module,Cycles,Cycles(%)'),
            ('db', 'ctrlcpu_{}.db')
        ],
        'ai_cpu_pmu_events': [
            ('handler', '_get_cpu_pmu_events'),
            ('table', 'OriginalData'),
            ('headers', 'Event,Name,Count'),
            ('db', 'aicpu_{}.db')
        ],
        'ai_cpu_top_function': [
            ('handler', '_get_cpu_top_funcs'),
            ('table', 'EventCount'),
            ('headers', 'Function,Module,Cycles,Cycles(%)'),
            ('db', 'aicpu_{}.db')
        ],
        'ts_cpu_pmu_events': [
            ('handler', '_get_cpu_pmu_events'),
            ('table', 'TsOriginalData'),
            ('headers', 'Event,Name,Count'),
            ('db', 'tscpu_{}.db')
        ],
        'ts_cpu_top_function': [
            ('handler', '_get_ts_cpu_top_funcs'),
            ('db', 'tscpu_{}.db')
        ],
        'os_runtime_api': [
            ('handler', '_get_host_runtime_api'),
            ('db', 'runtime_api_analysis{}.db')
        ],
        'os_runtime_statistic': [
            ('handler', '_get_host_runtime_api'),
            ('headers', 'Process ID,Thread ID,Name,Time(%),Time(us),Count,Avg(us),Max(us),Min(us)')
        ],
        'host_cpu_usage': [
            ('handler', '_get_host_cpu_usage_data'),
            ('headers', 'Total Cpu Numbers, Occupied Cpu Numbers, Recommend Cpu Numbers')
        ],
        'host_mem_usage': [
            ('handler', '_get_host_mem_usage_data'),
            ('headers', 'Total Memory(KB), Peak Used Memory(KB), Recommend Memory(KB)')
        ],
        'host_network_usage': [
            ('handler', '_get_host_network_usage_data'),
            ('headers', 'Netcard Speed(KB/s), Peak Used Speed(KB/s), Recommend Speed(KB/s)')
        ],
        'host_disk_usage': [
            ('handler', '_get_host_disk_usage_data'),
            ('headers', 'Peak Disk Read(KB/s), Recommend Disk Read(KB/s), '
                        'Peak Disk Write(KB/s), Recommend Disk Write(KB/s)')
        ],
        'msprof': [
            ('handler', '_get_bulk_data')
        ],
        'ffts_sub_task_time': [
            ('handler', '_get_task_timeline'),
            ('db', 'soc_log.db'),
            ('table', 'FftsLog')
        ],
        'hccl': [
            ('handler', '_get_hccl_timeline'),
            ('db', 'hccl_single_device.db'),
            ('table', 'HCCLTaskSingleDevice')
        ],
        'msprof_tx': [
            ('handler', '_get_msproftx_data'),
            ('db', 'msproftx.db'),
            ('table', 'MsprofTx'),
            ('headers',
             'pid, tid, category, event_type, payload_type, payload_value, Start_time(us), '
             'End_time(us), message_type, message')
        ],
        'sio': [
            ('handler', '_get_sio_data'),
            ('db', 'sio.db'),
            ('table', 'Sio')
        ],
        'inter_soc_transmission': [
            ('handler', '_get_inter_soc_summary'),
            ('db', 'soc_profiler.db'),
            ('table', 'InterSoc'),
            ('headers', 'Metric,L2Buffer_bandwidth_level,MATA_bandwidth_level')
        ],
        'acc_pmu': [
            ('handler', '_get_acc_pmu'),
            ('db', 'acc_pmu.db'),
            ('table', 'AccPmu'),
            ('headers',
             'task_id,stream_id,acc_id,block_id,read_bandwidth,write_'
             'bandwidth,read_ost,write_ost,time_stamp(us),start_time,dur_time')
        ],
        'stars_soc': [
            ('handler', '_get_stars_soc_data'),
            ('db', 'soc_profiler.db'),
            ('table', 'InterSoc')
        ],
        'stars_chip_trans': [
            ('handler', '_get_stars_chip_trans_data'),
            ('db', 'chip_trans.db')
        ],
        'low_power': [
            ('handler', '_get_low_power_data'),
            ('db', 'lowpower.db'),
            ('table', 'LowPower')
        ],
        'instr': [
            ('handler', '_get_biu_perf_timeline')
        ],
        'npu_mem': [
            ('handler', '_get_npu_mem_data'),
            ('headers', 'event,ddr(KB),hbm(KB),memory(KB),timestamp(us)'),
            ('db', 'npu_mem.db'),
            ('table', 'NpuMem')
        ],
        'memory_record': [
            ('handler', '_get_mem_record_data'),
            ('headers', 'Component,Timestamp(us),Total Allocated(KB),Total Reserved(KB),Device'),
        ],
        'npu_module_mem': [
            ('handler', '_get_npu_module_mem_data'),
            ('headers', 'Component,Timestamp(us),Total Reserved(KB),Device'),
        ],
        'operator_memory': [
            ('handler', '_get_operator_memory_data'),
            ('headers', 'Name,Size(KB),Allocation Time(us),Duration(us),'
                        'Allocation Total Allocated(KB),Allocation Total Reserved(KB),'
                        'Release Total Allocated(KB),Release Total Reserved(KB),Device'),
            ('db', 'memory_application.db'),
            ('table', 'NpuOpMem')
        ],
        'event': [
            ('handler', '_get_event_data'),
            ('db', 'api_event.db'),
            ('table', 'EventData')
        ],
        'api': [
            ('handler', '_get_api_data'),
            ('db', 'api_event.db'),
            ('table', 'ApiData')
        ],
        'hccl_statistic': [
            ('handler', '_get_hccl_statistic_data'),
            ('headers',
             'OP Type,Count,Total Time(us),Min Time(us),Avg Time(us),Max Time(us),Ratio(%)'),
            ('db', 'hccl_single_device.db')
        ],
        'api_statistic': [
            ('handler', '_get_api_statistic_data'),
            ('headers', 'Level,API Name,Time(us),Count,Avg(us),Min(us),Max(us),Variance'),
        ],
        'aicpu_mi': [
            ('handler', '_get_aicpu_mi_data')
        ],
    }
