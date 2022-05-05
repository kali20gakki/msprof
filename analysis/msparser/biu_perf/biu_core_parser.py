# coding=utf-8
"""
function: script used to parse biu cube and vector data and save it to db
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import os
import logging
from abc import abstractmethod

from framework.offset_calculator import OffsetCalculator
from common_func.ms_constant.str_constant import StrConstant
from common_func.path_manager import PathManager
from common_func.utils import Utils
from profiling_bean.biu_perf.monitor0_bean import Monitor0Bean
from profiling_bean.biu_perf.monitor1_bean import Monitor1Bean
from profiling_bean.biu_perf.core_info_bean import CoreInfo


class BiuCoreParser:
    """
    The base parser of BiuCubeParser and BiuVectorParser
    """

    # byte
    CUBE_SIZE = 96
    VECTOR_SIZE = 64

    # the location of monitor0 and monitor1
    CUBE_MONITOR1_INDEX_RANGE = [[32, 40], [48, 56], [64, 72], [80, 88]]
    CUBE_MONITOR0_INDEX_RANGE = [[40, 48], [56, 64], [72, 80], [88, 96]]

    VECTOR_MONITOR1_INDEX_RANGE = [32, 64]

    def __init__(self: any, sample_config: dict, core_info: any) -> None:
        self._file_list = core_info.file_list
        self._sample_config = sample_config
        self._project_path = self._sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self.core_info = core_info
        self.monitor1_data = []
        self.monitor0_data = []

    def process_file(self: any) -> None:
        """
        process biu perf file
        return: None
        """
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))
        for file_name in self._file_list:
            file_path = PathManager.get_data_file_path(self._project_path, file_name)
            self.parse_binary_file(file_path)

    @abstractmethod
    def parse_binary_file(self: any, file_path: str) -> None:
        """
        parse binary file
        """

    def add_monitor1_data(self: any, monitor1_bean: any) -> None:
        """
        get list from monitor1 bean
        return: None
        """
        if self.core_info.core_type == CoreInfo.AI_CUBE:
            vector_cycles = 0
            cube_cycles = monitor1_bean.cube_cycles
        else:
            vector_cycles = monitor1_bean.vector_cycles
            cube_cycles = 0

        self.monitor1_data.append([vector_cycles, monitor1_bean.scalar_cycles,
                                   cube_cycles, monitor1_bean.lsu0_cycles,
                                   monitor1_bean.lsu1_cycles, monitor1_bean.lsu2_cycles,
                                   monitor1_bean.timestamp, self.core_info.core_id,
                                   self.core_info.group_id, self.core_info.core_type])

    def add_monitor0_data(self: any, monitor0_bean: any) -> None:
        """
        get list from monitor0 bean
        return: None
        """
        self.monitor0_data.append([monitor0_bean.stat_rcmd_num, monitor0_bean.stat_wcmd_num,
                                   monitor0_bean.stat_rlat_raw, monitor0_bean.stat_wlat_raw,
                                   monitor0_bean.stat_flux_rd, monitor0_bean.stat_flux_wr,
                                   monitor0_bean.stat_flux_rd_l2, monitor0_bean.stat_flux_wr_l2,
                                   monitor0_bean.timestamp, monitor0_bean.l2_cache_hit, self.core_info.core_id,
                                   self.core_info.group_id, self.core_info.core_type])


class BiuCubeParser(BiuCoreParser):
    def __init__(self: any, sample_config: dict, core_info: any) -> None:
        super().__init__(sample_config, core_info)
        self._offset_calculator = OffsetCalculator(self._file_list, self.CUBE_SIZE, self._project_path)

    def parse_binary_file(self: any, file_path: str) -> None:
        """
        parse binary file
        """
        with open(file_path, 'rb') as file_reader:
            all_bytes = self._offset_calculator.pre_process(file_reader, os.path.getsize(file_path))

        for chunk in Utils.chunks(all_bytes, self.CUBE_SIZE):
            monitor1_chunk, monitor0_chunk = self.split_monitor_data(chunk)
            monitor1_bean = Monitor1Bean.decode(monitor1_chunk)
            monitor0_bean = Monitor0Bean.decode(monitor0_chunk)

            # timestamp of ai cube monitor 1 is invalid, so it is repalced by monitor 0
            monitor1_bean.timestamp = monitor0_bean.timestamp
            self.add_monitor1_data(monitor1_bean)
            self.add_monitor0_data(monitor0_bean)

    def split_monitor_data(self: any, chunk: any) -> tuple:
        """
        split monitor data
        params: chunk
        return: tuple of monitor1_chunk and monitor0_chunk
        """
        if len(chunk) != self.CUBE_SIZE:
            logging.error("The length of AI cube core chunk is not equal to %d", self.CUBE_SIZE)
            return [], []

        monitor1_chunk = bytes()
        for start_index, end_index in self.CUBE_MONITOR1_INDEX_RANGE:
            monitor1_chunk += chunk[start_index:end_index]

        monitor0_chunk = bytes()
        for start_index, end_index in self.CUBE_MONITOR0_INDEX_RANGE:
            monitor0_chunk += chunk[start_index:end_index]

        return monitor1_chunk, monitor0_chunk

    def get_monitor_data(self: any) -> tuple:
        """
        get monitor data
        return: tuple of monitor1_data and monitor0_data
        """
        self.process_file()
        return self.monitor1_data, self.monitor0_data


class BiuVectorParser(BiuCoreParser):
    def __init__(self: any, sample_config: dict, core_info: any) -> None:
        super().__init__(sample_config, core_info)
        self._offset_calculator = OffsetCalculator(self._file_list, self.VECTOR_SIZE, self._project_path)

    def parse_binary_file(self: any, file_path: str) -> None:
        """
        parse binary file
        """
        with open(file_path, 'rb') as file_reader:
            all_bytes = self._offset_calculator.pre_process(file_reader, os.path.getsize(file_path))

        for chunk in Utils.chunks(all_bytes, self.VECTOR_SIZE):
            monitor1_chunk = chunk[self.VECTOR_MONITOR1_INDEX_RANGE[0]:self.VECTOR_MONITOR1_INDEX_RANGE[1]]
            monitor1_bean = Monitor1Bean.decode(monitor1_chunk)
            self.add_monitor1_data(monitor1_bean)

    def get_monitor_data(self: any) -> list:
        """
        get monitor data
        """
        self.process_file()
        return self.monitor1_data
