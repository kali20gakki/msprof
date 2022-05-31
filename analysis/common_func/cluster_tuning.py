#!/usr/bin/python3
"""
This script is used to provide functions to read data from info.json
Copyright Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
"""

from common_func.singleton import singleton


@singleton
class ClusterTuning:
    """
    class used to read data from info.json
    """

    INFO_PATTERN = r"^info\.json\.?(\d?)$"
    FREQ = "38.4"
    NPU_PROFILING_TYPE = "npu_profiling"
    HOST_PROFILING_TYPE = "host_profiling"

    def __init__(self: any) -> None:
        self._is_cluster_scene = None
        self.cluster_list = []
        self.avg_time_dict = {}

    def init_cluster_scene(self: any, collect_path: str) -> None:
        self.set_cluster_scene()
        self.cluster_list.append(collect_path)

    def set_cluster_scene(self):
        self._is_cluster_scene = True

    @property
    def scene(self: any) -> bool:
        return self._is_cluster_scene

    def set_node_avg_time(self: any, data_path: str, avg_time: float):
        self.avg_time_dict[data_path] = [avg_time]
