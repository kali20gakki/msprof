#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from msconfig.meta_config import MetaConfig


class FilenameIntroductionConfig(MetaConfig):
    DATA = {
        'timeline': [
            ('begin', "Timeline file description:"),
            ('msprof', "The timeline report generated in the single-operator scenario."),
            ('step_trace', "Iterative trajectory data, determine which iteration took the longest"
                           "based on the length of the iteration."),
            ('msprof_tx', "Host msproftx data."),
        ],
        'summary': [
            ('begin', "Summary file description:"),
            ('host_cpu_usage', "Include host cpu usage."),
            ('host_mem_usage', "Include host memory usage."),
            ('host_disk_usage', "Include host disk usage."),
            ('host_network_usage', "Include host network(I/O) usage."),
            ('os_runtime_statistic', "Include host syscall and pthreadcall data."),
            ('process_cpu_usage', "Include host process cpu usage data."),
            ('api_statistic', "Statistical execution time information for the interface of the CANN layer."),
            ('op_summary', "Used to collect statistics on operator details and time consumption."),
            ('op_statistic', "Analyzes the total invoking time and times of various operators,"
                             "checks whether a certain operator takes a long time,"
                             "and then analyzes whether the operator can be optimized."),
            ('step_trace', "Iterative track data is the software information of the training task"
                           "and AI software stack, which realizes the performance analysis of the training task."
                           "The default two-segment gradient segmentation is used as an example."
                           "The time of the key node fp_start/bp_end/Reduce Start/Reduce Duration(us)"
                           "in the training task is printed to clearly describe "
                           "the execution status of an iteration."),
            ('hccl_statistic', "Based on the statistics of set communication operators,"
                               "you can learn about the time consumption of this type of operators"
                               "and the time consumption proportion of each HCCL operator in set communication."
                               "In this way, you can determine whether an operator has optimization space."),
            ('aicpu', "This file collects AI CPU data reported by data preprocessing."
                      "Other files that involve AI CPU data collect full AI CPU data."),
            ('fusion_op', "Information before and after operator fusion in the model."),
            ('task_time', "Task scheduling data."),
            ('ai_core_utilization', "ai_core_utilization data."),
            ('memory_record', "Records the memory applied for by the GE component at the CANN level"
                              "and the memory usage time."),
            ('operator_memory', "Records the memory required and occupied time when"
                                "CANN level operators are executed on the NPU. The memory is applied by the GE."),
            ('ddr', "DDR memory read/write speed data."),
            ('hbm', "HBM memory read/write speed data."),
            ('npu_mem', "NPU memory usage data."),
            ('hccs', "Aggregate communication bandwidth data."),
            ('roce', "RoCE communication interface bandwidth data."),
            ('pcie', "PCIe bandwidth data."),
            ('cpu_usage', "AI CPU and Ctrl CPU usage data."),
            ('sys_mem', "System memory data."),
            ('nic', "Network information data of each time node."),
            ('dvpp', "DVPP data."),
            ('llc_read_write', "L3 Cache read/write speed data."),
            ('llc_aicpu', "AI CPU L3 cache usage data."),
            ('llc_ctrlcpu', "Ctrl CPU L3 Cache Usage data."),
            ('llc_bandwidth', "L3 Cache Bandwidth data."),
            ('ai_cpu_top_function', "AI CPU hotspot function data."),
            ('ai_cpu_pmu_events', "AI CPU PMU event data."),
            ('ctrl_cpu_pmu_events', "Ctrl CPU event data."),
            ('ts_cpu_top_function', "TS CPU hotspot function data."),
            ('ts_cpu_pmu_events', "TS CPU PMU Event Data."),
        ]
    }

    def __init__(self):
        super().__init__()
