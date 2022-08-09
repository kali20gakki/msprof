#!/usr/bin/python3
"""
This script is used to provide functions to read data from info.json
Copyright Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
"""

import json
import logging
import os

from common_func.common import print_msg
from common_func.constant import Constant
from common_func.file_name_manager import get_dev_start_compiles
from common_func.file_name_manager import get_sample_json_compiles
from common_func.file_name_manager import get_end_info_compiles
from common_func.file_name_manager import get_host_start_compiles
from common_func.file_name_manager import get_info_json_compiles
from common_func.file_name_manager import get_start_info_compiles
from common_func.file_manager import check_path_valid
from common_func.msprof_exception import ProfException
from common_func.msvp_common import get_host_device_time
from common_func.msvp_common import is_number
from common_func.msvp_common import is_valid_original_data
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.singleton import singleton
from common_func.trace_view_header_constant import TraceViewHeaderConstant


@singleton
class InfoConfReader:
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

    @classmethod
    def get_json_tid_data(cls: any) -> int:
        """
        get timeline json tid data
        """
        return TraceViewHeaderConstant.DEFAULT_TID_VALUE

    @classmethod
    def get_conf_file_path(cls: any, project_path: str, conf_patterns: tuple) -> str:
        """
        get the config file path with pattern
        """
        for _file in os.listdir(project_path):
            for conf_pattern in conf_patterns:
                if conf_pattern.match(_file) \
                        and is_valid_original_data(_file, project_path, is_conf=True):
                    return os.path.join(project_path, _file)
        return ""

    @staticmethod
    def __get_json_data(info_json_path: str) -> dict:
        """
        read json data from file
        :param info_json_path:info json path
        :return:
        """
        if not info_json_path or not os.path.exists(info_json_path) or not os.path.isfile(
                info_json_path):
            return {}
        check_path_valid(info_json_path, is_file=True)
        try:
            with open(info_json_path, "r") as json_reader:
                json_data = json_reader.readline(Constant.MAX_READ_LINE_BYTES)
                json_data = json.loads(json_data)
                return json_data
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            logging.error("json data decode fail")
            return {}

    def load_info(self: any, result_path: str) -> None:
        """
        load all info
        """
        self._load_json(result_path)
        if not self.is_host_profiling():
            self._load_dev_start_time(result_path)
            self._load_dev_cnt(result_path)

    def get_start_timestamp(self: any) -> int:
        """
        get start time
        """
        return self._start_log_time

    def get_root_data(self: any, datatype: any) -> str:
        """
        write data into info.json
        :param datatype: desired data type
        :return:
        """
        return self._info_json.get(datatype)

    def get_device_list(self: any) -> list:
        """
        get device list from project path
        :return: devices list in the format: "0,1,2,3"
        """
        device_reader = self._info_json.get("devices")
        devices = ""
        if device_reader:
            devices += str(device_reader) + ","
        return list(filter(None, devices.split(",")))

    def get_rank_id(self: any) -> str:
        """
        get rank_id
        :return: rank_id
        """
        return self._end_info.get("rankId", Constant.NA)

    def get_device_id(self: any) -> str:
        """
        get device_id
        :return: device id
        """
        return self._info_json.get("devices", Constant.NA)

    def get_job_info(self: any) -> str:
        """
        get job info message
        """
        return self._info_json.get("jobInfo", Constant.NA)

    def get_freq(self: any, search_type: any) -> float:
        """
        get HWTS frequency from info.json
        :param search_type: search type aic|hwts
        :return:desired frequency
        """
        freq = str(self.get_data_under_device("{}_frequency".format(search_type)))
        try:
            if not freq or (not freq.isdigit and freq != self.FREQ) or float(freq) <= 0:
                print_msg("unable to get %s frequency.", search_type)
                logging.error("unable to get %s frequency.", search_type)
                raise ProfException(ProfException.PROF_SYSTEM_EXIT)
            return float(freq) * 1000000.0
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err)
            raise ProfException(ProfException.PROF_SYSTEM_EXIT) from err

    def get_collect_time(self: any) -> tuple:
        """
        Compatibility for getting collection time
        """
        return self._start_info.get(StrConstant.COLLECT_TIME_BEGIN), self._end_info.get(StrConstant.COLLECT_TIME_END)

    def get_collect_raw_time(self: any) -> tuple:
        """
        Compatibility for getting collection raw time
        """
        return self._start_info.get(StrConstant.COLLECT_RAW_TIME_BEGIN), self._end_info.get(
            StrConstant.COLLECT_RAW_TIME_END)

    def get_collect_date(self: any) -> tuple:
        """
        Compatibility for getting collection date
        """
        return self._start_info.get(StrConstant.COLLECT_DATE_BEGIN), self._end_info.get(StrConstant.COLLECT_DATE_END)

    def time_from_syscnt(self: any, sys_cnt: int, time_fmt: int = NumberConstant.NANO_SECOND) -> float:
        """
        transfer sys cnt to time unit.
        :param sys_cnt: sys cnt
        :param time_fmt: time format
        :return: sys time
        """
        hwts_freq = self.get_freq(StrConstant.HWTS)
        return float(
            sys_cnt - self._dev_cnt * NumberConstant.NANO_SECOND) / hwts_freq * time_fmt + self._host_mon * time_fmt

    def get_json_pid_data(self: any) -> int:
        """
        get pid message
        """
        process_id = self._info_json.get("pid")
        return int(process_id) if is_number(process_id) else TraceViewHeaderConstant.DEFAULT_PID_VALUE

    def get_cpu_info(self: any) -> list:
        """
        get cpu info
        """
        return [self._info_json.get(StrConstant.CPU_NUMS), self._info_json.get(StrConstant.SYS_CLOCK_FREQ)]

    def get_net_info(self: any) -> tuple:
        """
        get net info
        """
        return self._info_json.get(StrConstant.NET_CARD_NUMS), self._info_json.get(StrConstant.NET_CARD)

    def get_mem_total(self: any) -> str:
        """
        get net info
        """
        return self._info_json.get(StrConstant.MEMORY_TOTAL)

    def get_info_json(self: any) -> dict:
        """
        get info json
        """
        return self._info_json

    def is_host_profiling(self: any) -> bool:
        """
        get profiling type by device info if exist
        """
        device_info = self._info_json.get("DeviceInfo")
        return not device_info

    def get_data_under_device(self: any, data_type: any) -> str:
        """
        get ai core number
        :param data_type: desired data type
        :return:
        """
        device_items = self._info_json.get("DeviceInfo")
        if device_items is None:
            logging.error("unable to get DeviceInfo from info.json")
            return ""

        if device_items and device_items[0]:
            return device_items[0].get(data_type, "")
        return ""

    def get_delta_time(self: any) -> float:
        """
        calculate time difference between host and device
        """
        return self._host_mon - self._start_log_time / NumberConstant.NANO_SECOND

    def get_biu_sample_cycle(self: any) -> int:
        """
        calculate biu sample cycle
        """
        try:
            biu_sample_cycle = int(self._sample_json.get("biu_freq"))
        except TypeError as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR) from err

        if biu_sample_cycle <= 0:
            logging.error("Biu freq is invalid.")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)

        return biu_sample_cycle

    def get_job_basic_info(self: any) -> list:
        job_info = self.get_job_info()
        device_id = self.get_device_id()
        rank_id = self.get_rank_id()
        collection_time, _ = InfoConfReader().get_collect_date()
        if not collection_time:
            collection_time = Constant.NA
        return [job_info, device_id, collection_time, rank_id]

    def _load_json(self: any, result_path: str) -> None:
        """
        load info.json once
        """
        self._info_json = self.__get_json_data(
            self.get_conf_file_path(result_path, get_info_json_compiles()))
        self._start_info = self.__get_json_data(
            self.get_conf_file_path(result_path, get_start_info_compiles()))
        self._end_info = self.__get_json_data(
            self.get_conf_file_path(result_path, get_end_info_compiles()))
        self._sample_json = self.__get_json_data(
            self.get_conf_file_path(result_path, get_sample_json_compiles()))

    def _load_dev_start_path_line_by_line(self: any, log_file: any) -> None:
        while True:
            line = log_file.readline(Constant.MAX_READ_LINE_BYTES)
            if line:
                line = line.strip()
                if line.startswith(StrConstant.MONOTONIC_TIME):
                    _, value = line.split(":")
                    self._start_log_time = int(value.strip())
                    break

    def _load_dev_start_time(self: any, result_path: str) -> None:
        """
        load start log
        """
        dev_start_path = self.get_conf_file_path(result_path, get_dev_start_compiles())
        try:
            with open(dev_start_path, "r") as log_file:
                self._load_dev_start_path_line_by_line(log_file)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass

    def _load_dev_cnt(self: any, project_path: str) -> None:
        """
        load dev cnt
        :return: None
        """
        dev_start_file = self.get_conf_file_path(project_path, get_dev_start_compiles())
        host_start_file = self.get_conf_file_path(project_path, get_host_start_compiles())
        _, self._host_mon, _, _, self._dev_cnt = \
            get_host_device_time(host_start_file, dev_start_file, self.get_device_list()[0])
        if self._host_mon <= 0 or self._dev_cnt <= 0:
            logging.error("The monotonic time %s or cntvct %s is unusual, "
                          "maybe get data from driver failed", self._host_mon, self._dev_cnt)
