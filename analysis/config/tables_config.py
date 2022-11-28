#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from config.meta_config import MetaConfig


class TablesConfig(MetaConfig):
    DATA = {
        'TaskInfoMap': [
            ('model_id', 'INTEGER,null'),
            ('op_name', 'TEXT,null'),
            ('stream_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('block_dim', 'INTEGER,null'),
            ('mix_block_dim', 'INTEGER,null'),
            ('op_state', 'TEXT,null'),
            ('task_type', 'TEXT,null'),
            ('op_type', 'TEXT,null'),
            ('index_id', 'INTEGER,null'),
            ('thread_id', 'INTEGER,null'),
            ('timestamp', 'INTEGER,null'),
            ('batch_id', 'INTEGER,null'),
            ('context_id', 'INTEGER,null')
        ]
        , 'HashInfoMap': [
            ('hash_key', 'TEXT,null'),
            ('hash_value', 'TEXT,null')
        ]
        , 'TensorInfoMap': [
            ('model_id', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('tensor_num', 'INTEGER,null'),
            ('input_formats', 'TEXT,null'),
            ('input_data_types', 'TEXT,null'),
            ('input_shapes', 'TEXT,null'),
            ('output_formats', 'TEXT,null'),
            ('output_data_types', 'TEXT,null'),
            ('output_shapes', 'TEXT,null'),
            ('index_id', 'INTEGER,null'),
            ('timestamp', 'TEXT,null'),
            ('batch_id', 'INTEGER,null')
        ]
        , 'StepInfoMap': [
            ('model_id', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('thread_id', 'INTEGER,null'),
            ('timestamp', 'REAL,null'),
            ('cur_iter_num', 'INTEGER,null'),
            ('tag', 'TEXT,null'),
            ('batch_id', 'INTEGER,null')
        ]
        , 'SessionInfoMap': [
            ('model_id', 'INTEGER,null'),
            ('graph_id', 'INTEGER,null'),
            ('session_id', 'INTEGER,null'),
            ('mod', 'INTEGER,null'),
            ('timestamp', 'REAL,null')
        ]
        , 'GeModelLoadMap': [
            ('model_id', 'INTEGER,null'),
            ('model_name', 'TEXT,null'),
            ('start_time', 'REAL,null'),
            ('end_time', 'REAL,null')
        ]
        , 'GeFusionOpInfoMap': [
            ('model_id', 'INTEGER,null'),
            ('fusion_name', 'TEXT,null'),
            ('fusion_op_nums', 'INTEGER,null'),
            ('op_names', 'TEXT,null'),
            ('memory_input', 'TEXT,null'),
            ('memory_output', 'TEXT,null'),
            ('memory_weight', 'TEXT,null'),
            ('memory_workspace', 'TEXT,null'),
            ('memory_total', 'TEXT,null')
        ]
        , 'GeModelTimeMap': [
            ('model_name', 'TEXT,null'),
            ('model_id', 'INTEGER,null'),
            ('request_id', 'INTEGER,null'),
            ('thread_id', 'INTEGER,null'),
            ('input_start', 'REAL,null`'),
            ('input_end', 'REAL,null'),
            ('infer_start', 'REAL,null'),
            ('infer_end', 'REAL,null'),
            ('output_start', 'REAL,null'),
            ('output_end', 'REAL,null')
        ]
        , 'GEHostInfoMap': [
            ('thread_id', 'INTEGER,null'),
            ('op_type', 'TEXT,null'),
            ('event_type', 'TEXT,null'),
            ('start_time', 'INTEGER,null'),
            ('end_time', 'INTEGER,null')
        ]
        , 'OriginalDataMap': [
            ('common', 'text,none'),
            ('pid', 'int,none'),
            ('tid', 'int,none'),
            ('core', 'int,none'),
            ('timestamp', 'numeric,none'),
            ('pmucount', 'int,none'),
            ('pmuevent', 'char(10),none'),
            ('ip', 'text,none'),
            ('function', 'text,none'),
            ('offset', 'text,none'),
            ('module', 'text,none'),
            ('callstack', 'text,none'),
            ('replayid', 'int,none')
        ]
        , 'ApiCallMap': [
            ('entry_time', 'INTEGER,null'),
            ('exit_time', 'INTEGER,null'),
            ('api', 'TEXT,null'),
            ('retcode', 'INTEGER,null'),
            ('thread', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('tasknum', 'INTEGER,null'),
            ('task_id', 'TEXT,null'),
            ('batch_id', 'TEXT,null'),
            ('data_size', 'INTEGER,null'),
            ('memcpy_direction', 'TEXT,null')
        ]
        , 'TimeLineMap': [
            ('replayid', 'INTEGER,null'),
            ('tasktype', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('taskstate', 'INTEGER,null'),
            ('timestamp', 'INTEGER,null'),
            ('thread', 'INTEGER,null'),
            ('device_id', 'INTEGER,null'),
            ('mode', 'INTEGER,null')
        ]
        , 'EventCounterMap': [
            ('replayid', 'INTEGER,null'),
            ('tasktype', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('overflow', 'INTEGER,null'),
            ('overflowcycle', 'INTEGER,null'),
            ('timestamp', 'INTEGER,null'),
            ('event1', 'TEXT,null'),
            ('event2', 'TEXT,null'),
            ('event3', 'TEXT,null'),
            ('event4', 'TEXT,null'),
            ('event5', 'TEXT,null'),
            ('event6', 'TEXT,null'),
            ('event7', 'TEXT,null'),
            ('event8', 'TEXT,null'),
            ('task_cyc', 'TEXT,null'),
            ('block', 'INTEGER,null'),
            ('thread', 'INTEGER,null'),
            ('device_id', 'INTEGER,null'),
            ('mode', 'INTEGER,null')
        ]
        , 'StepTraceMap': [
            ('index_id', 'INTEGER,null'),
            ('model_id', 'INTEGER,null'),
            ('timestamp', 'REAL, null'),
            ('stream_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('tag_id', 'INTEGER,null')
        ]
        , 'TsMemcpyMap': [
            ('timestamp', 'REAL, null'),
            ('stream_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('task_state', 'INTEGER,null')
        ]
        , 'TsMemcpyCalculationMap': [
            ('stream_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('receive_time', 'INTEGER,null'),
            ('start_time', 'INTEGER,null'),
            ('end_time', 'INTEGER,null'),
            ('duration', 'INTEGER,null'),
            ('name', 'TEXT,null'),
            ('type', 'TEXT,null')
        ]
        , 'ReportTaskMap': [
            ('timeratio', 'REAL,null'),
            ('time', 'REAL,null'),
            ('count', 'INTEGER,null'),
            ('avg', 'REAL,null'),
            ('min', 'REAL,null'),
            ('max', 'REAL,null'),
            ('waiting', 'REAL,null'),
            ('running', 'REAL,null'),
            ('pending', 'REAL,null'),
            ('type', 'TEXT,null'),
            ('api', 'TEXT,null'),
            ('task_id', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('device_id', 'INTEGER,null'),
            ('batch_id', 'INTEGER,null')
        ]
        , 'TaskTimeMap': [
            ('replayid', 'INTEGER,null'),
            ('device_id', 'INTEGER,null'),
            ('api', 'INTEGER,null'),
            ('apirowid', 'INTEGER,null'),
            ('tasktype', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('waittime', 'TEXT,null'),
            ('pendingtime', 'Text,null'),
            ('running', 'TEXT,null'),
            ('complete', 'TEXT,null'),
            ('index_id', 'INTEGER,null'),
            ('model_id', 'INTEGER,null'),
            ('batch_id', 'INTEGER,null')
        ]
        , 'TsOriginalDataMap': [
            ('replayid', 'INTEGER,null'),
            ('timestamp', 'numeric,null'),
            ('pc', 'TEXT,null'),
            ('callstack', 'TEXT,null'),
            ('event', 'TEXT,null'),
            ('count', 'INTEGER,null'),
            ('function', 'TEXT,null')
        ]
        , 'AICoreOriginalDataMap': [
            ('mode', 'INTEGER,null'),
            ('replayid', 'INTEGER,null'),
            ('timestamp', 'numeric,null'),
            ('coreid', 'INTEGER,null'),
            ('task_cyc', 'Text,null'),
            ('event1', 'Text,null'),
            ('event2', 'Text,null'),
            ('event3', 'Text,null'),
            ('event4', 'Text,null'),
            ('event5', 'Text,null'),
            ('event6', 'Text,null'),
            ('event7', 'Text,null'),
            ('event8', 'Text,null')
        ]
        , 'NicOriginalDataMap': [
            ('device_id', 'INTEGER,null'),
            ('replayid', 'INTEGER,null'),
            ('timestamp', 'REAL,null'),
            ('bandwidth', 'INTEGER,null'),
            ('rxpacket', 'REAL,null'),
            ('rxbyte', 'REAL,null'),
            ('rxpackets', 'REAL,null'),
            ('rxbytes', 'REAL,null'),
            ('rxerrors', 'REAL,null'),
            ('rxdropped', 'REAL,null'),
            ('txpacket', 'REAL,null'),
            ('txbyte', 'REAL,null'),
            ('txpackets', 'REAL,null'),
            ('txbytes', 'REAL,null'),
            ('txerrors', 'REAL,null'),
            ('txdropped', 'REAL,null'),
            ('funcid', 'INTEGER,null')
        ]
        , 'NicReportDataMap': [
            ('device_id', 'INTEGER,null'),
            ('duration', 'TEXT,null'),
            ('bandwidth', 'TEXT,null'),
            ('rxbandwidth', 'TEXT,null'),
            ('txbandwidth', 'TEXT,null'),
            ('rxpacket', 'TEXT,null'),
            ('rxerrorrate', 'TEXT,null'),
            ('rxdroppedrate', 'TEXT,null'),
            ('txpacket', 'TEXT,null'),
            ('txerrorrate', 'TEXT,null'),
            ('txdroppedrate', 'TEXT,null'),
            ('funcid', 'INTEGER,null')
        ]
        , 'DvppOriginalDataMap': [
            ('device_id', 'TEXT,null'),
            ('replayid', 'TEXT,null'),
            ('timestamp', 'REAL,null'),
            ('dvppid', 'TEXT,null'),
            ('enginetype', 'TEXT,null'),
            ('engineid', 'TEXT,null'),
            ('alltime', 'REAL,null'),
            ('allframe', 'REAL,null'),
            ('allutilization', 'TEXT,null'),
            ('proctime', 'REAL,null'),
            ('procframe', 'REAL,null'),
            ('procutilization', 'TEXT,null'),
            ('lasttime', 'REAL,null'),
            ('lastframe', 'REAL,null')
        ]
        , 'DvppReportDataMap': [
            ('dvppid', 'TEXT,null'),
            ('device_id', 'TEXT,null'),
            ('enginetype', 'TEXT,null'),
            ('engineid', 'TEXT,null'),
            ('alltime', 'TEXT,null'),
            ('allframe', 'TEXT,null'),
            ('allutilization', 'TEXT,null')
        ]
        , 'StreamMap': [
            ('replayid', 'INTEGER, null'),
            ('device_id', 'INTEGER, null'),
            ('stream_id', 'INTEGER, null'),
            ('task_id', 'INTEGER, null'),
            ('tasktype', 'INTEGER, null'),
            ('waittime', 'INTEGER, null'),
            ('pendingtime', 'INTEGER,null'),
            ('runtime', 'INTEGER,null'),
            ('completetime', 'INTEGER, null'),
            ('api', 'INTEGER, null'),
            ('apirowid', 'INTEGER, null'),
            ('eventid', 'INTEGER, null'),
            ('streamname', 'TEXT, null')
        ]
        , 'LLCOriginalDataMap': [
            ('device_id', 'INT,null'),
            ('replayid', 'INT,null'),
            ('timestamp', 'REAL,null'),
            ('counts', 'INT,null'),
            ('unit', 'VARCHAR,null'),
            ('event', 'VARCHAR,null')
        ]
        , 'LLCMetricDataMap': [
            ('device_id', 'INT,null'),
            ('replayid', 'INT,null'),
            ('timestamp', 'REAL,null'),
            ('read_allocate', 'REAL,null'),
            ('read_noallocate', 'REAL,null'),
            ('read_hit', 'REAL,null'),
            ('write_allocate', 'REAL,null'),
            ('write_noallocate', 'REAL,null'),
            ('write_hit', 'REAL,null')
        ]
        , 'LLCDsidDataMap': [
            ('device_id', 'INT,null'),
            ('replayid', 'INT,null'),
            ('timestamp', 'REAL,null'),
            ('dsid0', 'REAL,null'),
            ('dsid1', 'REAL,null'),
            ('dsid2', 'REAL,null'),
            ('dsid3', 'REAL,null'),
            ('dsid4', 'REAL,null'),
            ('dsid5', 'REAL,null'),
            ('dsid6', 'REAL,null'),
            ('dsid7', 'REAL,null')
        ]
        , 'ai_core_statusMap': [
            ('mode', 'INTEGER,null'),
            ('replayid', 'INTEGER,null'),
            ('device_id', 'INTEGER,null'),
            ('timestamp', 'REAL,null'),
            ('aicorenumber', 'INTEGER,null'),
            ('aicorestate', 'TEXT,null')
        ]
        , 'ai_vector_statusMap': [
            ('mode', 'INTEGER,null'),
            ('replayid', 'INTEGER,null'),
            ('device_id', 'INTEGER,null'),
            ('timestamp', 'REAL,null'),
            ('aivnumber', 'INTEGER,null'),
            ('aivstate', 'TEXT,null')
        ]
        , 'RuntimeTrackMap': [
            ('device_id', 'INTEGER,null'),
            ('timestamp', 'REAL,null'),
            ('task_type', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('thread', 'INTEGER,null'),
            ('batch_id', 'INTEGER,null')
        ]
        , 'TsTrackMap': [
            ('mode', 'INTEGER,null'),
            ('replayid', 'INTEGER,null'),
            ('device_id', 'INTEGER,null'),
            ('timestamp', 'REAL,null'),
            ('eventname', 'TEXT,null'),
            ('tasktype', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null')
        ]
        , 'DDROriginalDataMap': [
            ('device_id', 'INT,null'),
            ('replayid', 'INT,null'),
            ('timestamp', 'REAL,null'),
            ('counts', 'INT,null'),
            ('unit', 'VARCHAR,null'),
            ('event', 'VARCHAR,null')
        ]
        , 'DDRMetricDataMap': [
            ('device_id', 'INT,null'),
            ('replayid', 'INT,null'),
            ('timestamp', 'REAL,null'),
            ('flux_read', 'REAL,null'),
            ('flux_write', 'REAL,null'),
            ('fluxid_read', 'REAL,null'),
            ('fluxid_write', 'REAL,null')
        ]
        , 'LLCBandwidthMap': [
            ('device_id', 'INT,null'),
            ('timestamp', 'REAL,null'),
            ('read_hit_rate', 'REAL,null'),
            ('read_total', 'REAL,null'),
            ('read_hit', 'REAL,null'),
            ('write_hit_rate', 'REAL,null'),
            ('write_total', 'REAL,null'),
            ('write_hit', 'REAL,null')
        ]
        , 'LLCCapacityMap': [
            ('device_id', 'INT,null'),
            ('timestamp', 'REAL,null'),
            ('ctrlcpu', 'REAL,null'),
            ('aicpu', 'REAL,null')
        ]
        , 'sysmemMap': [
            ('timestamp', 'INT,null'),
            ('memtotal', 'INTEGER,null'),
            ('memfree', 'INTEGER,null'),
            ('buffers', 'INTEGER,null'),
            ('cached', 'INTEGER,null'),
            ('shmem', 'INTEGER,null'),
            ('commitlimit', 'INTEGER,null'),
            ('committed_as', 'INTEGER,null'),
            ('hugepages_total', 'INTEGER,null'),
            ('hugepages_free', 'INTEGER,null'),
            ('unit', 'text,null')
        ]
        , 'pidmemMap': [
            ('timestamp', 'INT,null'),
            ('name', 'text,null'),
            ('size', 'INTEGER,null'),
            ('resident', 'INTEGER,null'),
            ('shared', 'INTEGER,null'),
            ('pid', 'INTEGER,null')
        ]
        , 'SysCpuUsageDataMap': [
            ('timestamp', 'REAL,null'),
            ('cpun', 'TEXT,null'),
            ('user', 'REAL,null'),
            ('nice', 'REAL,null'),
            ('sys', 'REAL,null'),
            ('idle', 'REAL,null'),
            ('iowait', 'REAL,null'),
            ('irq', 'REAL,null'),
            ('soft', 'REAL,null'),
            ('steal', 'REAL,null'),
            ('guest', 'REAL,null'),
            ('gnice', 'REAL,null'),
            ('cputype', 'TEXT,null')
        ]
        , 'ProCpuUsageDataMap': [
            ('pid', 'int,null'),
            ('process_name', 'VARCHAR,null'),
            ('utime', 'REAL,null'),
            ('stime', 'REAL,null'),
            ('cutime', 'REAL,null'),
            ('cstime', 'REAL,null'),
            ('timestamp', 'REAL,null'),
            ('sys_usage', 'REAL,null')
        ]
        , 'AclDataMap': [
            ('api_name', 'text,null'),
            ('api_type', 'text,null'),
            ('start_time', 'INTEGER,null'),
            ('end_time', 'INTEGER,null'),
            ('process_id', 'INTEGER,null'),
            ('thread_id', 'INTEGER,null'),
            ('device_id', 'INTEGER,null')
        ]
        , 'AclHashMap': [
            ('acl_hash', 'text,null'),
            ('api_name', 'text,null')
        ]
        , 'ModifiedTaskTimeMap': [
            ('task_id', 'INTEGER, null'),
            ('stream_id', 'INTEGER, null'),
            ('start_time', 'INTEGER,null'),
            ('duration_time', 'INTEGER, null'),
            ('wait_time', 'INTEGER, null'),
            ('task_type', 'INTEGER,null'),
            ('index_id', 'INTEGER, null'),
            ('model_id', 'INTEGER, null'),
            ('batch_id', 'INTEGER,null'),
            ('subtask_id', 'INTEGER,null')
        ]
        , 'AiCpuDataMap': [
            ('stream_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('sys_start', 'INTEGER, null'),
            ('sys_end', 'INTEGER, null'),
            ('node_name', 'TEXT, null'),
            ('compute_time', 'REAL, null'),
            ('memcpy_time', 'REAL, null'),
            ('task_time', 'REAL, null'),
            ('dispatch_time', 'REAL, null'),
            ('total_time', 'REAL, null'),
            ('batch_id', 'INTEGER,null')
        ]
        , 'AiCpuFromTsMap': [
            ('stream_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('sys_start', 'INTEGER, null'),
            ('sys_end', 'INTEGER, null'),
            ('batch_id', 'INTEGER,null')
        ]
        , 'CpuInfoMap': [
            ('cpu_num', 'INTEGER, null'),
            ('clk_jiffies', 'INTEGER, null')
        ]
        , 'ProcessUsageMap': [
            ('timestamp', 'text,null'),
            ('uptime', 'text,null'),
            ('pid', 'INTEGER, null'),
            ('tid', 'INTEGER, null'),
            ('jiffies', 'INTEGER, null'),
            ('cpu_no', 'text,null'),
            ('usage', 'REAL,null')
        ]
        , 'CpuUsageMap': [
            ('start_time', 'numeric,null'),
            ('end_time', 'text,null'),
            ('cpu_no', 'text,null'),
            ('usage', 'REAL,null')
        ]
        , 'MemUsageMap': [
            ('start_time', 'numeric,null'),
            ('end_time', 'text,null'),
            ('usage', 'REAL,null')
        ]
        , 'DiskUsageMap': [
            ('start_time', 'numeric,null'),
            ('end_time', 'text,null'),
            ('disk_read', 'text,null'),
            ('disk_write', 'text,null'),
            ('swap_in', 'text,null'),
            ('usage', 'REAL,null')
        ]
        , 'NetworkUsageMap': [
            ('start_time', 'numeric,null'),
            ('end_time', 'text,null'),
            ('usage', 'REAL,null')
        ]
        , 'SyscallMap': [
            ('runtime_comm', 'text,null'),
            ('runtime_pid', 'INTEGER,null'),
            ('runtime_tid', 'INTEGER,null'),
            ('runtime_api_name', 'text,null'),
            ('runtime_start_time', 'REAL,null'),
            ('runtime_duration', 'REAL,null'),
            ('runtime_end_time', 'REAL,null'),
            ('runtime_trans_start', 'REAL,null'),
            ('runtime_trans_end', 'REAL,null')
        ]
        , 'HwtsTaskMap': [
            ('task_type', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('sys_counter', 'INTEGER,null'),
            ('log_type', 'INTEGER,null')
        ]
        , 'HwtsTaskTimeMap': [
            ('stream_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('running', 'INTEGER,null'),
            ('complete', 'INTEGER,null'),
            ('index_id', 'INTEGER,null'),
            ('model_id', 'INTEGER,null'),
            ('batch_id', 'INTEGER,null')
        ]
        , 'HwtsIterMap': [
            ('iter_id', 'INTEGER,null'),
            ('model_id', 'INTEGER,null'),
            ('index_id', 'INTEGER,null'),
            ('task_count', 'INTEGER,null'),
            ('task_offset', 'INTEGER,null'),
            ('ai_core_num', 'INTEGER,null'),
            ('ai_core_offset', 'INTEGER,null'),
            ('sys_cnt', 'INTEGER,null')
        ]
        , 'HwtsBatchMap': [
            ('stream_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('batch_id', 'INTEGER,null'),
            ('iter_id', 'INTEGER,null'),
            ('start_time', 'REAL,null'),
            ('end_time', 'REAL,null'),
            ('is_ai_core', 'INTEGER,null')
        ]
        , 'GeMergeMap': [
            ('model_id', 'INTEGER,null'),
            ('op_name', 'text,null'),
            ('op_type', 'text,null'),
            ('task_type', 'text,null'),
            ('task_id', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('batch_id', 'INTEGER,null'),
            ('context_id', 'INTEGER,null')
        ]
        , 'RtsTaskMap': [
            ('task_id', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('start_time', 'INTEGER,null'),
            ('duration', 'INTEGER,null'),
            ('task_type', 'text,null'),
            ('index_id', 'INTEGER, null'),
            ('model_id', 'INTEGER,null'),
            ('batch_id', 'INTEGER,null'),
            ('subtask_id', 'INTEGER,null')
        ]
        , 'OpReportMap': [
            ('model_name', 'text,null'),
            ('op_type', 'text,null'),
            ('core_type', 'text,null'),
            ('occurrences', 'text,null'),
            ('total_time', 'REAL,null'),
            ('min', 'REAL,null'),
            ('avg', 'REAL,null'),
            ('max', 'REAL,null'),
            ('ratio', 'text,null')
        ]
        , 'AcsqTaskMap': [
            ('stream_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('acc_id', 'INTEGER,null'),
            ('task_type', 'TEXT,null'),
            ('start_time', 'INTEGER,null'),
            ('end_time', 'INTEGER,null'),
            ('task_time', 'INTEGER,null')
        ]
        , 'AcsqTaskTimeMap': [
            ('task_id', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('start_time', 'INTEGER,null'),
            ('task_time', 'INTEGER,null'),
            ('task_type', 'TEXT,null'),
            ('index_id', 'INTEGER,null'),
            ('model_id', 'INTEGER,null'),
            ('batch_id', 'INTEGER,null')
        ]
        , 'FftsLogMap': [
            ('stream_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('subtask_id', 'INTEGER,null'),
            ('thread_id', 'TEXT,null'),
            ('subtask_type', 'INTEGER,null'),
            ('ffts_type', 'INTEGER,null'),
            ('task_type', 'TEXT,null'),
            ('task_time', 'INTEGER,null')
        ]
        , 'StarsTaskTimeMap': [
            ('context_id', 'INTEGER, null'),
            ('task_id', 'INTEGER, null'),
            ('stream_id', 'INTEGER, null'),
            ('start_time', 'INTEGER,null'),
            ('duration_time', 'INTEGER, null'),
            ('wait_time', 'INTEGER, null'),
            ('task_type', 'INTEGER,null')
        ]
        , 'L2CacheParseMap': [
            ('device_id', 'INTEGER,null'),
            ('task_type', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('event_lists', 'TEXT,null')
        ]
        , 'L2CacheSummaryMap': [
            ('device_id', 'INTEGER,null'),
            ('task_type', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('hit_rate', 'REAL'),
            ('victim_rate', 'REAL')
        ]
        , 'L2CacheSampleMap': [
            ('read_count', 'INTEGER, null'),
            ('write_count', 'INTEGER, null'),
            ('victim', 'INTEGER, null'),
            ('allocate', 'INTEGER, null')
        ]
        , 'HCCLAllReduceMap': [
            ('op_name', 'TEXT, null'),
            ('iteration', 'INTEGER, null'),
            ('name', 'TEXT, null'),
            ('first_timestamp', 'REAL, null'),
            ('plane_id', 'INTEGER, null'),
            ('timestamp', 'REAL, null'),
            ('duration', 'REAL, null'),
            ('notify_id', 'INTEGER, null'),
            ('stage', 'INTEGER, null'),
            ('step', 'INTEGER, null'),
            ('bandwidth', 'REAL, null'),
            ('stream_id', 'INTEGER, null'),
            ('task_id', 'INTEGER, null'),
            ('task_type', 'TEXT, null'),
            ('src_rank', 'INTEGER, null'),
            ('dst_rank', 'INTEGER, null'),
            ('transport_type', 'TEXT, null'),
            ('size', 'REAL, null')
        ]
        , 'MsprofTxMap': [
            ('pid', 'INTEGER, null'),
            ('tid', 'INTEGER, null'),
            ('category', 'INTEGER, null'),
            ('event_type', 'TEXT, null'),
            ('payload_type', 'INTEGER, null'),
            ('payload_value', 'INTEGER, null'),
            ('start_time', 'INTEGER, null'),
            ('end_time', 'INTEGER, null'),
            ('message_type', 'INTEGER, null'),
            ('message', 'TEXT, null')
        ]
        , 'InterSocMap': [
            ('l2_buffer_bw_level', 'INTEGER, null'),
            ('mata_bw_level', 'INTEGER, null'),
            ('sys_time', 'REAL,null')
        ]
        , 'AccPmuMap': [
            ('task_id', 'INTEGER, null'),
            ('stream_id', 'INTEGER, null'),
            ('acc_id', 'INTEGER,null'),
            ('block_id', 'INTEGER, null'),
            ('read_bandwidth', 'INTEGER, null'),
            ('write_bandwidth', 'INTEGER, null'),
            ('read_ost', 'INTEGER, null'),
            ('write_ost', 'INTEGER, null'),
            ('timestamp', 'REAL, null'),
            ('start_time', 'REAL, null'),
            ('dur_time', 'INTEGER, null')
        ]
        , 'AccPmuOriginMap': [
            ('task_id', 'INTEGER, null'),
            ('stream_id', 'INTEGER, null'),
            ('acc_id', 'INTEGER,null'),
            ('block_id', 'INTEGER, null'),
            ('read_bandwidth', 'INTEGER, null'),
            ('write_bandwidth', 'INTEGER, null'),
            ('read_ost', 'INTEGER, null'),
            ('write_ost', 'INTEGER, null'),
            ('timestamp', 'REAL, null')
        ]
        , 'ModelWithQMap': [
            ('index_id', 'INTEGER, null'),
            ('model_id', 'INTEGER, null'),
            ('timestamp', 'REAL, null'),
            ('tag_id', 'INTEGER, null'),
            ('event_id', 'INTEGER, null')
        ]
        , 'PaLinkInfoMap': [
            ('pa_link_id', 'INTEGER, null'),
            ('pa_link_traffic_monit_rx', 'INTEGER, null'),
            ('pa_link_traffic_monit_tx', 'INTEGER,null'),
            ('sys_time', 'INTEGER,null')
        ]
        , 'PcieInfoMap': [
            ('pcie_id', 'INTEGER, null'),
            ('pcie_write_bandwidth', 'INTEGER, null'),
            ('pcie_read_bandwidth', 'INTEGER, null'),
            ('sys_time', 'INTEGER,null')
        ]
        , 'LowPowerMap': [
            ('sys_time', 'INTEGER,null'),
            ('vf_samp_cnt', 'INTEGER,null'),
            ('pwr_samp_cnt', 'INTEGER,null'),
            ('temp_samp_cnt', 'INTEGER,null'),
            ('tem_of_ai_core', 'INTEGER,null'),
            ('tem_of_hbm', 'INTEGER,null'),
            ('tem_of_hbm_granularity', 'INTEGER,null'),
            ('tem_of_3ds_ram', 'INTEGER,null'),
            ('tem_of_cpu', 'INTEGER,null'),
            ('tem_of_peripherals', 'INTEGER,null'),
            ('tem_of_l2_buff', 'INTEGER,null'),
            ('aic_current_dpm', 'INTEGER,null'),
            ('power_consumption_dpm', 'INTEGER,null'),
            ('aic_current_sd5003', 'INTEGER,null'),
            ('power_consumption_sd5003', 'INTEGER,null'),
            ('pwr2prof_dat_imon', 'INTEGER,null'),
            ('pmc2prof_freq_prof', 'INTEGER,null'),
            ('tem_warn_cnt0', 'INTEGER,null'),
            ('tem_warn_cnt1', 'INTEGER,null'),
            ('tem_warn_cnt2', 'INTEGER,null'),
            ('tem_warn_cnt3', 'INTEGER,null'),
            ('cos_warn_cnt0', 'INTEGER,null'),
            ('cos_warn_cnt1', 'INTEGER,null'),
            ('cos_warn_cnt2', 'INTEGER,null'),
            ('cos_warn_cnt3', 'INTEGER,null'),
            ('io_warn_cnt0', 'INTEGER,null'),
            ('io_warn_cnt1', 'INTEGER,null'),
            ('io_warn_cnt2', 'INTEGER,null'),
            ('io_warn_cnt3', 'INTEGER,null'),
            ('epd_warn', 'INTEGER,null'),
            ('sft_samp_cfg', 'INTEGER,null'),
            ('voltage', 'INTEGER,null')
        ]
        , 'MonitorFlowMap': [
            ('stat_rcmd_num', 'INTEGER, null'),
            ('stat_wcmd_num', 'INTEGER, null'),
            ('stat_rlat_raw', 'INTEGER, null'),
            ('stat_wlat_raw', 'INTEGER, null'),
            ('stat_flux_rd', 'INTEGER, null'),
            ('stat_flux_wr', 'INTEGER, null'),
            ('stat_flux_rd_l2', 'INTEGER, null'),
            ('stat_flux_wr_l2', 'INTEGER, null'),
            ('timestamp', 'TEXT, null'),
            ('l2_cache_hit', 'INTEGER, null'),
            ('core_id', 'INTEGER, null'),
            ('group_id', 'INTEGER, null'),
            ('core_type', 'TEXT, null')
        ]
        , 'MonitorCyclesMap': [
            ('vector_cycles', 'INTEGER, null'),
            ('scalar_cycles', 'INTEGER, null'),
            ('cube_cycles', 'INTEGER, null'),
            ('lsu0_cycles', 'INTEGER, null'),
            ('lsu1_cycles', 'INTEGER, null'),
            ('lsu2_cycles', 'INTEGER, null'),
            ('timestamp', 'TEXT, null'),
            ('core_id', 'INTEGER, null'),
            ('group_id', 'INTEGER, null'),
            ('core_type', 'TEXT, null')
        ]
        , 'BiuFlowMap': [
            ('timestamp', 'TEXT, null'),
            ('flow', 'INTEGER, null'),
            ('flow_type', 'TEXT, null'),
            ('unit_name', 'TEXT, null'),
            ('pid', 'INTEGER, null'),
            ('tid', 'INTEGER, null'),
            ('interval_start', 'INTEGER, null')
        ]
        , 'BiuCyclesMap': [
            ('timestamp', 'TEXT, null'),
            ('duration', 'INTEGER, null'),
            ('cycle_type', 'TEXT, null'),
            ('unit_name', 'TEXT, null'),
            ('cycle_num', 'INTEGER, null'),
            ('ratio', 'REAL, null'),
            ('pid', 'INTEGER, null'),
            ('tid', 'INTEGER, null'),
            ('interval_start', 'INTEGER, null')
        ]
        , 'TaskTypeMap': [
            ('timestamp', 'TEXT, null'),
            ('stream_id', 'INTEGER, null'),
            ('task_id', 'INTEGER, null'),
            ('task_type', 'TEXT, null'),
            ('task_state', 'INTEGER, null')
        ]
        , 'GeSummaryMap': [
            ('model_id', 'INTEGER,null'),
            ('batch_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('op_name', 'TEXT,null'),
            ('op_type', 'TEXT,null'),
            ('block_dim', 'INTEGER,null'),
            ('mix_block_dim', 'INTEGER,null'),
            ('task_type', 'TEXT,null'),
            ('timestamp', 'INTEGER,null'),
            ('index_id', 'INTEGER,null'),
            ('context_id', 'INTEGER,null')
        ]
        , 'GeTensorMap': [
            ('model_id', 'INTEGER,null'),
            ('stream_id', 'INTEGER,null'),
            ('task_id', 'INTEGER,null'),
            ('tensor_num', 'INTEGER,null'),
            ('input_formats', 'TEXT,null'),
            ('input_data_types', 'TEXT,null'),
            ('input_shapes', 'TEXT,null'),
            ('output_formats', 'TEXT,null'),
            ('output_data_types', 'TEXT,null'),
            ('output_shapes', 'TEXT,null'),
            ('index_id', 'INTEGER,null'),
            ('timestamp', 'TEXT,null'),
            ('batch_id', 'INTEGER,null')
        ]
        , 'TimeMap': [
            ('device_id', 'INTEGER,null'),
            ('dev_mon', 'INTEGER,null'),
            ('dev_wall', 'INTEGER,null'),
            ('dev_cntvct', 'INTEGER,null'),
            ('host_mon', 'INTEGER,null'),
            ('host_wall', 'INTEGER,null')
        ]
        , 'ClusterRankMap': [
            ('job_info', 'TEXT,null'),
            ('device_id', 'INTEGER,null'),
            ('collection_time', 'TEXT,null'),
            ('rank_id', 'INTEGER,null'),
            ('dir_name', 'TEXT,null')
        ]
        , 'ClusterStepTraceMap': [
            ('device_id', 'INTEGER,null'),
            ('model_id', 'INTEGER,null'),
            ('iteration_id', 'INTEGER,null'),
            ('fp_start', 'REAL,null'),
            ('bp_end', 'REAL,null'),
            ('iteration_end', 'REAL,null'),
            ('iteration_time', 'REAL,null'),
            ('fp_bp_time', 'REAL,null'),
            ('grad_refresh_bound', 'REAL,null'),
            ('data_aug_bound', 'REAL,null'),
            ('ge_tag', 'INTEGER,null')
        ]
        , 'DataQueueMap': [
            ('node_name', 'TEXT,null'),
            ('queue_size', 'INTEGER,null'),
            ('start_time', 'REAL,null'),
            ('end_time', 'REAL,null'),
            ('duration', 'REAL,null')
        ]
        , 'AllReduceMap': [
            ('device_id', 'INTEGER,null'),
            ('model_id', 'INTEGER,null'),
            ('index_id', 'INTEGER,null'),
            ('iteration_end', 'INTEGER,null'),
            ('all_reduce_start', 'INTEGER,null'),
            ('all_reduce_end', 'INTEGER,null')
        ]
        , 'HostQueueMap': [
            ('type', 'INTEGER,null'),
            ('extra_info', 'INTEGER,null'),
            ('index_id', 'INTEGER,null'),
            ('value', 'INTEGER,null'),
            ('mode', 'INTEGER,null')
        ]
        , 'HcclOperatorExeMap': [
            ('model_id', 'INTEGER,null'),
            ('index_id', 'INTEGER,null'),
            ('op_name', 'TEXT,null'),
            ('op_type', 'TEXT,null'),
            ('start_time', 'REAL,null'),
            ('end_time', 'REAL,null')
        ]
        , 'ParallelStrategyMap': [
            ('ai_frame_type', 'TEXT,null'),
            ('stage_num', 'INTEGER,null'),
            ('rankId', 'INTEGER,null'),
            ('stageId', 'INTEGER,null'),
            ('parallelType', 'TEXT,null'),
            ('stageDevices', 'TEXT,null'),
            ('parallel_mode', 'TEXT,null')
        ]
        , 'HcclOperatorOverlapMap': [
            ('model_id', 'INTEGER,null'),
            ('index_id', 'INTEGER,null'),
            ('op_name', 'TEXT,null'),
            ('op_type', 'TEXT,null'),
            ('start_time', 'REAL,null'),
            ('end_time', 'REAL,null'),
            ('overlap_time', 'REAL,null')
        ]
        , 'ComputationTimeMap': [
            ('model_id', 'INTEGER,null'),
            ('index_id', 'INTEGER,null'),
            ('step_time', 'REAL,null'),
            ('computation_time', 'REAL,null')
        ]
        , 'ClusterDataParallelMap': [
            ('rank_id', 'INTEGER,null'),
            ('device_id', 'INTEGER,null'),
            ('model_id', 'INTEGER,null'),
            ('iteration_id', 'INTEGER,null'),
            ('step_time', 'REAL,null'),
            ('computation_time', 'REAL,null'),
            ('pure_communication_time', 'REAL,null'),
            ('communication_time', 'REAL,null'),
            ('interval_of_communication_time', 'REAL,null'),
            ('hccl_op_num', 'INTEGER,null')
        ]
        , 'ClusterModelParallelMap': [
            ('rank_id', 'INTEGER,null'),
            ('device_id', 'INTEGER,null'),
            ('model_id', 'INTEGER,null'),
            ('iteration_id', 'INTEGER,null'),
            ('step_time', 'REAL,null'),
            ('computation_time', 'REAL,null'),
            ('pure_communication_time', 'REAL,null'),
            ('communication_time', 'REAL,null')
        ]
        , 'ClusterPipelineParallelMap': [
            ('rank_id', 'INTEGER,null'),
            ('device_id', 'INTEGER,null'),
            ('model_id', 'INTEGER,null'),
            ('iteration_id', 'INTEGER,null'),
            ('step_time', 'REAL,null'),
            ('computation_time', 'REAL,null'),
            ('pure_communication_time', 'REAL,null'),
            ('communication_time', 'REAL,null'),
            ('pure_communication_time_only_revice', 'REAL,null'),
            ('pure_communication_time_except_revice', 'REAL,null')
        ]
        , 'ClusterParallelStrategyMap': [
            ('ai_frame_type', 'TEXT,null'),
            ('stage_num', 'INTEGER,null'),
            ('rankId', 'INTEGER,null'),
            ('stageId', 'INTEGER,null'),
            ('parallelType', 'TEXT,null'),
            ('stageDevices', 'TEXT,null'),
            ('parallel_mode', 'TEXT,null')
        ]
    }
