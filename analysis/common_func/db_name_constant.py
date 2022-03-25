# coding=utf-8
"""
This script is amid to define db names.
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
from abc import ABCMeta
from abc import abstractmethod


class DBNameConstant(metaclass=ABCMeta):
    """
    DB name and DB table names
    """
    # DB name
    DB_ACL_MODULE = "acl_module.db"
    DB_AICORE_OP_SUMMARY = "ai_core_op_summary.db"
    DB_RUNTIME = "runtime.db"
    DB_GE_MODEL_INFO = "ge_model_info.db"
    DB_GE_MODEL_TIME = "ge_model_time.db"
    DB_GE_HOST_INFO = "ge_host_info.db"
    DB_AI_CPU = "ai_cpu.db"
    DB_GE_INFO = "ge_info.db"
    DB_HASH = "hash_mapping.db"
    DB_HCCS = "hccs.db"
    DB_HOST_CPU_USAGE = "host_cpu_usage.db"
    DB_HOST_DISK_USAGE = "host_disk_usage.db"
    DB_HOST_MEM_USAGE = "host_mem_usage.db"
    DB_HOST_NETWORK_USAGE = "host_network_usage.db"
    DB_HOST_RUNTIME_API = "host_runtime_api.db"
    DB_HWTS = "hwts.db"
    DB_HWTS_AIV = "hwts_aiv.db"
    DB_HWTS_REC = "hwts-rec.db"
    DB_L2CACHE = "l2cache.db"
    DB_LLC = "llc.db"
    DB_OP_COUNTER = "op_counter.db"
    DB_PERIPHERAL = "peripheral.db"
    DB_RTS_TRACK = "rts_track.db"
    DB_TRACE = "trace.db"
    DB_TIME = "time.db"
    DB_STEP_TRACE = "step_trace.db"
    DB_MEMORY_COPY = "memory_copy.db"
    DB_TASK_PMU = "task_pmu.db"
    DB_NIC_ORIGIN = "nic.db"
    DB_ROCE_ORIGIN = "roce.db"
    DB_NIC_RECEIVE = "nicreceivesend_table.db"
    DB_ROCE_RECEIVE = "rocereceivesend_table.db"
    DB_DDR = "ddr.db"
    DB_PCIE = "pcie.db"
    DB_HBM = "hbm.db"
    DB_HCCL = "hccl.db"
    DB_MSPROFTX = 'msproftx.db'
    DB_GE_HASH = "ge_hash.db"
    DB_ACSQ = "acsq.db"
    DB_SOC_LOG = "soc_log.db"
    DB_STARS_SOC = "soc_profiler.db"
    DB_STARS_CHIP_TRANS = "chip_trans.db"

    # DB tables
    TABLE_ACL_DATA = "acl_data"
    TABLE_AI_CORE_METRIC_SUMMARY = "MetricSummary"
    TABLE_AIV_METRIC_SUMMARY = "AivMetricSummary"
    TABLE_AI_CORE_REC = "AiCoreRec"
    TABLE_AI_CPU = "ai_cpu_datas"
    TABLE_ALL_REDUCE = "all_reduce"
    TABLE_API_CALL = "ApiCall"
    TABLE_EVENT_COUNTER = "EventCounter"
    TABLE_EVENT_COUNT = "EventCount"
    TABLE_GE_LOAD_TABLE = "GELoad"
    TABLE_GE_GRAPH = "ge_graph_data"
    TABLE_GE_INFER = "GEInfer"
    TABLE_GE_STEP_INFO = "StepInfo"
    TABLE_GE_STEP_INFO_DATA = "step_info_data"
    TABLE_GE_HOST = "GEHostInfo"
    TABLE_HASH_ACL = "hash_acl_dict"
    TABLE_HWTS_SYS_RANGE = "hwts_sys_cnt_range"
    TABLE_HWTS_ITER_SYS = "HwtsIter"
    TABLE_HWTS_BATCH = "HwtsBatch"
    TABLE_HWTS_TASK = "HwtsTask"
    TABLE_HWTS_TASK_TIME = "HwtsTaskTime"
    TABLE_LLC_METRIC_DATA = "LLCMetricData"
    TABLE_LLC_DSID = "LLCDsidData"
    TABLE_METRICS_SUMMARY = "MetricSummary"
    TABLE_OP_COUNTER_GE_MERGE = "ge_task_merge"
    TABLE_OP_COUNTER_OP_REPORT = "op_report"
    TABLE_OP_COUNTER_RTS_TASK = "rts_task"
    TABLE_RUNTIME_REPORT_TASK = "ReportTask"
    TABLE_RUNTIME_TASK_TIME = "TaskTime"
    TABLE_RUNTIME_TIMELINE = "TimeLine"
    TABLE_RUNTIME_TRACK = "RuntimeTrack"
    TABLE_SUMMARY_GE = "ge_summary"
    TABLE_SUMMARY_METRICS = "ai_core_metrics"
    TABLE_SUMMARY_TASK_TIME = "task_time"
    TABLE_SUMMARY_TENSOR = "ge_tensor"
    TABLE_TS_MEMCPY_CALCULATION = "TsMemcpyCalculation"
    TABLE_TIME = 'time'
    TABLE_TRACE_FILES = 'files'
    TABLE_TRAINING_TRACE = 'training_trace'
    # step trace
    TABLE_STEP_TRACE = "StepTrace"
    TABLE_TS_MEMCPY = "TsMemcpy"
    TABLE_MODEL_WITH_Q = "ModelWithQ"
    TABLE_STEP_TRACE_DATA = "step_trace_data"
    # cpu usage
    TABLE_HOST_CPU_INFO = "CpuInfo"
    TABLE_HOST_CPU_USAGE = "CpuUsage"
    # disk usage
    TABLE_HOST_DISK_USAGE = "DiskUsage"
    # mem usage
    TABLE_HOST_MEM_USAGE = "MemUsage"
    # network usage
    TABLE_HOST_NETWORK_USAGE = "NetworkUsage"
    TABLE_HOST_PROCESS_USAGE = "ProcessUsage"
    # host syscal
    TABLE_HOST_RUNTIME_API = "Syscall"
    # stars
    TABLE_ACSQ_TASK = "AcsqTask"
    TABLE_THREAD_TASK = "ThreadTime"
    TABLE_SUBTASK_TIME = "SubtaskTime"
    TABLE_L2CACHE_PARSE = 'L2CacheParse'
    TABLE_L2CACHE_SUMMARY = 'L2CacheSummary'

    # dvpp
    TABLE_DVPP_ORIGIN = "DvppOriginalData"
    TABLE_DVPP_REPORT = "DvppReportData"
    TABLE_DVPP_TREE = "DvppTreeData"
    # nic
    TABLE_NIC_ORIGIN = 'NicOriginalData'
    TABLE_NIC_REPORT = 'NicReportData'
    TABLE_NIC_RECEIVE = 'NicReceiveSend'
    # roce
    TABLE_ROCE_ORIGIN = 'RoceOriginalData'
    TABLE_ROCE_REPORT = 'RoceReportData'
    TABLE_ROCE_RECEIVE = 'RoceReceiveSend'
    # tscpu
    TABLE_TSCPU_ORIGIN = "TsOriginalData"
    # ddr
    TABLE_DDR_ORIGIN = "DDROriginalData"
    TABLE_DDR_METRIC = "DDRMetricData"
    # sys_mem
    TABLE_SYS_MEM = "sysmem"
    TABLE_PID_MEM = "pidmem"
    # sys_usage
    TABLE_SYS_USAGE = "SysCpuUsage"
    TABLE_PID_USAGE = "ProCpuUsage"
    # pcie
    TABLE_PCIE = "PcieOriginalData"
    # hbm
    TABLE_HBM_ORIGIN = "HBMOriginalData"
    TABLE_HBM_BW = "HBMbwData"
    # hccs
    TABLE_HCCS_ORIGIN = 'HCCSOriginalData'
    TABLE_HCCS_EVENTS = 'HCCSEventsData'

    # llc
    TABLE_LLC_ORIGIN = "LLCOriginalData"
    TABLE_LLC_EVENTS = "LLCEvents"
    TABLE_LLC_METRICS = "LLCMetrics"
    TABLE_MINI_LLC_METRICS = "LLCMetricData"
    TABLE_LLC_CAPACITY = "LLCCapacity"
    TABLE_LLC_BANDWIDTH = "LLCBandwidth"
    # aicpu/ctrl_cpu
    TABLE_CPU_ORIGIN = "OriginalData"

    # tscpu
    TABLE_TS_CPU_EVENT = "EventCount"
    TABLE_TS_CPU_HOT_INS = "HotIns"
    TABLE_HCCL_ALL_REDUCE = "HCCLAllReduce"

    # msproftx
    TABLE_MSPROFTX = "MsprofTx"

    # stars
    TABLE_PCIE_DATA = "PcieData"
    TABLE_ACC_PMU_DATA = "AccPmu"
    TABLE_LPE_DATA = 'Lpe'
    TABLE_LPS_DATA = "Lps"
    TABLE_SOC_DATA = "InterSoc"
    TABLE_STARS_DATA = "StarsMessage"
    TABLE_FFTS_LOG = "FftsLog"
    TABLE_STARS_PA_LINK = "PaLinkInfo"
    TABLE_STARS_PCIE = "PcieInfo"

    # ge
    TABLE_GE_TASK = "TaskInfo"
    TABLE_GE_TENSOR = "TensorInfo"
    TABLE_GE_STEP = "StepInfo"
    TABLE_GE_SESSION = "SessionInfo"
    TABLE_GE_HASH = "HashInfo"

    # ge model
    TABLE_GE_MODEL_LOAD = "GeModelLoad"
    TABLE_GE_FUSION_OP_INFO = "GeFusionOpInfo"

    # ge model time
    TABLE_GE_MODEL_TIME = "GeModelTime"

    @abstractmethod
    def get_db_name(self: any) -> str:
        """
        get db name
        :return: db name
        """

    @abstractmethod
    def get_table_name(self: any) -> str:
        """
        get table name
        :return: table name
        """
