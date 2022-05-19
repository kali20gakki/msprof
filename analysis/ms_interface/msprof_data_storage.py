#!/usr/bin/python3
# coding:utf-8
"""
This scripts is used to export summary timeline
Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
"""

import json
import logging
import math
import os
import re

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.file_manager import FileOpen
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import MsProfCommonConstant
from common_func.msvp_common import create_csv
from common_func.msvp_common import create_json
from common_func.msvp_constant import MsvpConstant
from common_func.os_manager import check_file_writable, check_file_readable
from common_func.path_manager import PathManager
from profiling_bean.prof_enum.timeline_slice_strategy import LoadingTimeLevel, TimeLineSliceStrategy


class MsprofDataStorage:
    """
    This class is used to slicing a timeline json file.
    """

    SLICE_CONFIG_PATH = os.path.join(MsvpConstant.CONFIG_PATH, 'msprof_slice.json')
    DATA_TO_FILE = 1024 * 1024
    DEFAULT_SETTING = ('on', 0, 0)

    def __init__(self: any) -> None:
        self.tid_set = set()
        self.slice_config = None
        self.timeline_head = []
        self.slice_data = []
        self.data_list = None

    @staticmethod
    def export_timeline_data_to_json(result: str or dict, params: dict) -> any:
        """
        export data to json file
        :param result: export result
        :param params: params
        :return: result
        """
        if not result:
            return json.dumps({"status": NumberConstant.WARN,
                               "info": "Unable to get %s data. Maybe the data is not "
                                       "collected, or the data may fail to be analyzed."
                                       % params.get(StrConstant.PARAM_DATA_TYPE)})
        if isinstance(result, dict):
            if 'status' in result:
                return result
            result = json.dumps(result)
        elif isinstance(result, str):
            if 'status' in json.loads(result):
                return result
        try:
            result = MsprofDataStorage().slice_data_list(json.loads(result))
        except (json.JSONDecodeError, TypeError, ValueError, IOError) as err:
            return json.dumps({"status": NumberConstant.ERROR,
                               "info": "message error: %s" % err})
        MsprofDataStorage.clear_timeline_dir(params)
        data_path = []
        for slice_time in range(len(result[1])):
            timeline_file_path = MsprofDataStorage._make_export_file_name(params, slice_time, result[0])
            check_file_writable(timeline_file_path)
            if os.path.exists(timeline_file_path):
                os.remove(timeline_file_path)
            try:
                with os.fdopen(os.open(timeline_file_path, Constant.WRITE_FLAGS,
                                       Constant.WRITE_MODES), 'w') as trace_file:
                    trace_file.write(json.dumps(result[1][slice_time]))
                    data_path.append(timeline_file_path)
            except (OSError, SystemError, ValueError, TypeError,
                    RuntimeError) as err:
                return json.dumps({"status": NumberConstant.ERROR,
                                   "info": "message error: %s" % err})
        return json.dumps({'status': NumberConstant.SUCCESS,
                           'data': data_path})

    @staticmethod
    def export_summary_data(headers: list, data: list, params: dict) -> any:
        """
        export data to csv file
        :param headers: header
        :param data: data
        :param params: params
        :return:
        """
        if headers and data:
            summary_file_path = MsprofDataStorage._make_export_file_name(params)
            check_file_writable(summary_file_path)
            if params.get(StrConstant.PARAM_EXPORT_FORMAT) == StrConstant.EXPORT_CSV:
                return create_csv(summary_file_path, headers, data, save_old_file=False)
            if params.get(StrConstant.PARAM_EXPORT_FORMAT) == StrConstant.EXPORT_JSON:
                return create_json(summary_file_path, headers, data, save_old_file=False)
        if data:
            return data
        return json.dumps({"status": NumberConstant.WARN,
                           "info": "Unable to get %s data. Maybe the data is not "
                                   "collected, or the data may fail to be analyzed."
                                   % params.get(StrConstant.PARAM_DATA_TYPE)})

    @staticmethod
    def clear_timeline_dir(params: dict) -> None:
        timeline_dir = PathManager.get_timeline_dir(params.get(StrConstant.PARAM_RESULT_DIR))
        for file in os.listdir(timeline_dir):
            file_suffix = ''
            if params.get(StrConstant.PARAM_DEVICE_ID) is not None:
                file_suffix += "_" + str(params.get(StrConstant.PARAM_DEVICE_ID))
                if ProfilingScene().is_step_trace():
                    file_suffix += "_" + str(params.get(StrConstant.PARAM_MODEL_ID))
                if params.get(StrConstant.PARAM_ITER_ID) is not None:
                    file_suffix += "_" + str(params.get(StrConstant.PARAM_ITER_ID))
            if re.match(r'{0}{1}.*'.format(params.get(StrConstant.PARAM_DATA_TYPE), file_suffix), file):
                check_file_writable(os.path.join(timeline_dir, file))
                os.remove(os.path.join(timeline_dir, file))

    @staticmethod
    def write_json_files(json_data: list, file_path: str) -> None:
        """
        write json data  to file
        :param json_data:
        :param file_path:
        :return:
        """
        try:
            with os.fdopen(os.open(file_path, Constant.WRITE_FLAGS,
                                   Constant.WRITE_MODES), 'w') as trace_file:
                trace_file.write(json.dumps(json_data))
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(os.path.basename(__file__), err)

    @staticmethod
    def _calculate_loading_time(row_line_level: int, count_line_level: int) -> float:
        """
        Calculate the approximate time
        :return:approximate time
        The basis of this formula:
        The number of rows is proportional to the time,
        And the number of data records is proportional to the slope.
        And the start value is proportional to the total number of records.
        """
        return (7 * count_line_level / 36000000 + 5.1) * row_line_level / 1000 + 9 * count_line_level / 2000000 - 2.5

    @staticmethod
    def _make_export_file_name(params: dict, slice_times: int = 0, slice_switch=False) -> str:
        file_name = params.get(StrConstant.PARAM_DATA_TYPE)
        if params.get(StrConstant.PARAM_DEVICE_ID) is not None:
            file_name += "_" + str(params.get(StrConstant.PARAM_DEVICE_ID))
            if ProfilingScene().is_step_trace():
                file_name += "_" + str(params.get(StrConstant.PARAM_MODEL_ID))
            if params.get(StrConstant.PARAM_ITER_ID) is not None:
                file_name += "_" + str(params.get(StrConstant.PARAM_ITER_ID))
            if slice_switch:
                file_name += "_slice_{}".format(str(slice_times))

        if params.get(StrConstant.PARAM_EXPORT_TYPE) == MsProfCommonConstant.SUMMARY:
            file_suffix = StrConstant.FILE_SUFFIX_CSV
            if params.get(StrConstant.PARAM_EXPORT_FORMAT) == StrConstant.EXPORT_JSON:
                file_suffix = StrConstant.FILE_SUFFIX_JSON
            return os.path.join(
                PathManager.get_summary_dir(params.get(StrConstant.PARAM_RESULT_DIR)),
                file_name + file_suffix)
        return os.path.join(
            PathManager.get_timeline_dir(params.get(StrConstant.PARAM_RESULT_DIR)),
            file_name + StrConstant.FILE_SUFFIX_JSON)

    @staticmethod
    def _get_time_level(line_level: float) -> int:
        """
        judge loading time level
        :return: slice times in range [1,10,30]
        """
        time_level_list = [i.value for i in LoadingTimeLevel]
        for index, level in enumerate(time_level_list):
            if line_level < level:
                return time_level_list[index-1] if index > 0 else time_level_list[index]
        return LoadingTimeLevel.BAD_LEVEL.value

    def init_params(self: any, data_list: list) -> None:
        """
        init data params
        :return: None
        """
        self.data_list = data_list
        self.data_list.sort(key=lambda x: x.get('ts', 0))
        self.set_tid()
        self._update_timeline_head()

    def slice_data_list(self: any, data_list: list) -> tuple:
        """
        split data to slices
        return: tuple (slice count, slice data)
        """
        self.init_params(data_list)
        slice_switch, limit_size, method = self.read_slice_config()
        slice_count = self.get_slice_times(limit_size, method)
        if slice_switch == 'off' or not slice_count:
            return False, [self.timeline_head + data_list]
        slice_point = len(self.data_list) // slice_count
        slice_data = []
        for i in range(slice_count):
            slice_data.append(self.timeline_head + data_list[i * slice_point:(i + 1) * slice_point])
        return True, slice_data

    def set_tid(self: any):
        for data in self.data_list:
            pid = str(data.get(StrConstant.TRACE_HEADER_PID, ''))
            tid = str(data.get(StrConstant.TRACE_HEADER_TID, ''))
            if pid and tid:
                self.tid_set.add('{}-{}'.format(pid, tid))

    def read_slice_config(self: any) -> tuple:
        """
        read the configuration file
        :return: tuple
        :return: params: slice_switch slice switch
        :return: params: limit_size specifies the size of the split file. (MB)
        :return: params: slice_switch priority (0: granularity first,1: loading time first)
        """
        check_file_readable(self.SLICE_CONFIG_PATH)
        try:
            with FileOpen(self.SLICE_CONFIG_PATH, "r") as rule_reader:
                config_json = json.load(rule_reader.file_reader)
        except (FileNotFoundError, TypeError, json.JSONDecodeError):
            logging.warning("Read slice config failed: %s", os.path.basename(self.SLICE_CONFIG_PATH))
            return self.DEFAULT_SETTING
        slice_switch = config_json.get('slice_switch', 'on')
        limit_size = config_json.get('slice_file_size(MB)', 0)
        method = config_json.get('strategy', 0)
        return slice_switch, limit_size, method

    def get_slice_times(self: any, limit_size: int = 0, method: int = 0) -> int:
        """
        read the configuration file
        :return: slice times: int
        """
        list_length = len(self.data_list)
        str_length = len(json.dumps(self.data_list))
        # If an exception occurs, continue the calculation logic.
        try:
            if isinstance(limit_size, int) and limit_size >= 200:
                str_size_of_mb = str_length // self.DATA_TO_FILE
                return 1 + str_size_of_mb // limit_size if str_size_of_mb > limit_size else 0
        except (TypeError, ValueError) as err:
            logging.warning(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
        row_line_level = len(self.tid_set)
        formula = MsprofDataStorage._calculate_loading_time(row_line_level, list_length)
        time_level = self._get_time_level(formula)
        slice_time = 2
        slice_method = LoadingTimeLevel.BAD_LEVEL.value
        if method == TimeLineSliceStrategy.LOADING_TIME_PRIORITY.value:
            slice_method = LoadingTimeLevel.FINE_LEVEL.value
        try:
            while time_level >= slice_method:
                slice_length = math.ceil(list_length / slice_time)
                formula = MsprofDataStorage._calculate_loading_time(row_line_level, slice_length)
                time_level = self._get_time_level(formula)
                slice_time += 1
        except ZeroDivisionError as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return 0
        return slice_time - 1 if slice_time > 2 else 0

    def _update_timeline_head(self: any) -> None:
        while self.data_list:
            if self.data_list[0].get('ph', '') == "M":
                self.timeline_head.append(self.data_list.pop(0))
            else:
                break
