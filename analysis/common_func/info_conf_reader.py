#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
import configparser
import json
import logging
import os

from common_func.constant import Constant
from common_func.file_manager import check_path_valid
from common_func.file_name_manager import get_dev_start_compiles
from common_func.file_name_manager import get_end_info_compiles
from common_func.file_name_manager import get_host_start_compiles
from common_func.file_name_manager import get_info_json_compiles
from common_func.file_name_manager import get_sample_json_compiles
from common_func.file_name_manager import get_start_info_compiles
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_exception import ProfException
from common_func.msvp_common import is_number
from common_func.msvp_common import is_valid_original_data
from common_func.singleton import singleton
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from profiling_bean.basic_info.host_start import TimerBean


@singleton
class InfoConfReader:
    """
    class used to read data from info.json
    """

    INFO_PATTERN = r"^info\.json\.?(\d?)$"
    FREQ = "38.4"
    NPU_PROFILING_TYPE = "npu_profiling"
    HOST_PROFILING_TYPE = "host_profiling"
    HOST_DEFAULT_FREQ = 0
    ANALYSIS_VERSION = "1.0"

    def __init__(self: any) -> None:
        self._info_json = None
        self._start_info = None
        self._end_info = None
        self._sample_json = None
        self._start_log_time = 0
        self._host_mon = 0
        self._dev_cnt = 0

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

    def load_info(self: any, result_path: str) -> None:
        """
        load all info
        """
        self._load_json(result_path)
        if not self.is_host_profiling():
            self._load_dev_start_time(result_path)
            self._load_host_start_time(result_path)

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
        device_reader = self._sample_json.get("devices")
        devices = ""
        if device_reader:
            devices += str(device_reader) + ","
        return list(filter(None, devices.split(",")))

    def get_rank_id(self: any) -> int:
        """
        get rank_id
        :return: rank_id
        """
        rank_id = self._info_json.get("rank_id", Constant.DEFAULT_INVALID_RANK_ID_VALUE)
        if rank_id == Constant.DEFAULT_INVALID_VALUE or len(str(rank_id)) == 0:
            rank_id = Constant.DEFAULT_INVALID_RANK_ID_VALUE
        return rank_id

    def is_version_matched(self):
        """
        check the version between data-collection and the data-analysis
        """
        return self._info_json.get("version", Constant.NA) == self.ANALYSIS_VERSION

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
            if not freq or float(freq) <= 0:
                logging.error("unable to get %s frequency.", search_type)
                raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        except (ValueError, TypeError) as err:
            logging.error(err)
            raise ProfException(ProfException.PROF_SYSTEM_EXIT) from err
        return float(freq) * 1000000.0

    def get_collect_time(self: any) -> tuple:
        """
        Compatibility for getting collection time
        """
        return self._start_info.get(StrConstant.COLLECT_TIME_BEGIN), self._end_info.get(StrConstant.COLLECT_TIME_END)

    def get_collect_start_time(self: any) -> str:
        """
        Compatibility for getting collection start time
        """
        collect_time = self._start_info.get(StrConstant.COLLECT_DATE_BEGIN, Constant.NA)
        if not collect_time:
            return Constant.NA
        return collect_time

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

    def get_json_pid_name(self: any) -> str:
        """
        get profiling pid name
        """
        return self._info_json.get("pid_name")

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

    def get_instr_profiling_freq(self: any) -> int:
        """
        get instr_profiling_freq from info json
        """
        instr_profiling_freq0 = self._sample_json.get("instr_profiling_freq")
        instr_profiling_freq1 = self._sample_json.get("instrProfilingFreq")
        if instr_profiling_freq0 is None and instr_profiling_freq1 is None:
            logging.error(
                "instr profiling frequency not found in sample.json",
                exc_info=Constant.TRACE_BACK_SWITCH
            )
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)

        instr_profiling_freq_val = instr_profiling_freq0 if instr_profiling_freq1 is None else instr_profiling_freq1
        instr_profiling_freq = int(instr_profiling_freq_val)

        if instr_profiling_freq <= 0:
            logging.error("Instr Profiling Frequency is invalid.")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)

        return instr_profiling_freq

    def get_job_basic_info(self: any) -> list:
        job_info = self.get_job_info()
        device_id = self.get_device_id()
        rank_id = self.get_rank_id()
        collection_time, _ = InfoConfReader().get_collect_date()
        if not collection_time:
            collection_time = Constant.NA
        return [job_info, device_id, collection_time, rank_id]

    def get_host_freq(self: any) -> float:
        host_cpu_info = self._info_json.get('CPU', [])
        if host_cpu_info:
            freq = host_cpu_info[0].get('Frequency', self.HOST_DEFAULT_FREQ)
            if is_number(freq):
                return freq
        return self.HOST_DEFAULT_FREQ

    def get_host_time_by_sampling_timestamp(self: any, timestamp: any) -> int:
        """
        Obtain the actual time based on the sampling timestamp (us).
        :return: int
        """
        return int(timestamp * NumberConstant.USTONS + self.get_start_timestamp() +
                   self.get_delta_time() * NumberConstant.NANO_SECOND) / NumberConstant.USTONS

    def get_ai_core_profiling_mode(self):
        return self._sample_json.get("ai_core_profiling_mode")

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
        self._dev_cnt = 0
        self._start_log_time = 0
        while True:
            line = log_file.readline(Constant.MAX_READ_LINE_BYTES)
            if not line:
                break
            split_str = line.strip().split(":")
            if len(split_str) != 2 or not is_number(split_str[1]):
                continue
            if split_str[0] == StrConstant.MONOTONIC_TIME:
                self._start_log_time = int(split_str[1])
            elif split_str[0] == StrConstant.DEVICE_SYSCNT:
                self._dev_cnt = float(split_str[1]) / NumberConstant.NANO_SECOND
            elif self._start_log_time and self._dev_cnt:
                break

    def _load_dev_start_time(self: any, result_path: str) -> None:
        """
        load start log
        """
        dev_start_path = self.get_conf_file_path(result_path, get_dev_start_compiles())
        if not os.path.exists(dev_start_path):
            return
        try:
            with open(dev_start_path, "r") as log_file:
                self._load_dev_start_path_line_by_line(log_file)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)

    def _load_host_start_time(self: any, project_path: str) -> None:
        """
        load host start time
        :return: None
        """
        host_start_file = self.get_conf_file_path(project_path, get_host_start_compiles())
        try:
            if os.path.exists(host_start_file):
                check_path_valid(host_start_file, True)
                config = configparser.ConfigParser()
                config.read(host_start_file)
                sections = config.sections()
                if not sections:
                    return
                time = dict(config.items(sections[0]))
                timer = TimerBean(time, self.get_host_freq())
                self._host_mon = float(timer.host_mon) / NumberConstant.NANO_SECOND
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error('Parse time sync data error: %s', str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        if self._host_mon <= 0 or self._dev_cnt <= 0:
            logging.error("The monotonic time %s or cntvct %s is unusual, "
                          "maybe get data from driver failed", self._host_mon, self._dev_cnt)

