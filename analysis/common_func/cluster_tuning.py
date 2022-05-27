#!/usr/bin/python3
"""
This script is used to provide functions to read data from info.json
Copyright Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
"""

from common_func.singleton import singleton


@singleton
class Cluster_tuning:
    """
    class used to read data from info.json
    """

    INFO_PATTERN = r"^info\.json\.?(\d?)$"
    FREQ = "38.4"
    NPU_PROFILING_TYPE = "npu_profiling"
    HOST_PROFILING_TYPE = "host_profiling"

    def __init__(self: any) -> None:
        self._info_json = None
        self._start_info = None
        self._end_info = None
        self._sample_json = None
        self._start_log_time = 0
        self._host_mon = 0
        self._dev_cnt = 0
