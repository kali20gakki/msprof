#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import csv
import json
import logging
import os
import re
import shutil

from common_func.common import print_info, error
from common_func.constant import Constant
from common_func.data_check_manager import DataCheckManager
from common_func.file_manager import FileOpen
from common_func.file_manager import check_path_valid
from common_func.file_slice_helper import FileSliceHelper
from common_func.file_slice_helper import make_export_file_name
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import MsProfCommonConstant
from common_func.msprof_common import get_path_dir
from common_func.msvp_common import is_number
from common_func.path_manager import PathManager
from common_func.utils import Utils
from msconfig.config_manager import ConfigManager
from msinterface.msprof_data_storage import MsprofDataStorage


class MsprofOutputSummary:
    """
    class used to export all job data.
    """
    MSPROF_HOST_DIR = "host"
    DEVICE_ID = "Device_id"
    DEVICE_ID_PREFIX_LEN = 7
    README = "README.txt"
    FILE_MAX_SIZE = 1024 * 1024 * 1024
    JSON_LIST = [
        "msprof", "step_trace", "msprof_tx"
    ]

    def __init__(self: any, output: str) -> None:
        self._output = output
        self._output_dir = ""
        self._log_dir = ""

    @staticmethod
    def _valid_pos(underscore_pos: int, point_pos: int, filename: str) -> bool:
        if underscore_pos < 1 or point_pos < 0:
            return False
        elif underscore_pos > len(filename) - 1 or point_pos > len(filename):
            return False
        elif underscore_pos > point_pos:
            return False
        return True

    @staticmethod
    def _get_newest_file_list(file_list, data_type: str) -> list:
        """
        filename is key,timestamp is value,update time to get the newest file list
        """
        file_dict = {}
        for filename in file_list:
            if filename.endswith(data_type):
                underscore_pos = filename.rfind("_")
                point_pos = filename.rfind(".")
                if not MsprofOutputSummary._valid_pos(underscore_pos, point_pos, filename):
                    logging.warning("The file name  %s is invalid!", filename)
                    continue
                time_str = filename[underscore_pos + 1: point_pos]
                if not is_number(time_str):
                    logging.warning("The file name  %s is invalid!", filename)
                    continue
                time = int(time_str)
                key = filename[:underscore_pos + 1]
                value = file_dict.get(key, 0)
                if not value or value < time:
                    file_dict.update({key: time})

        return [k + str(v) + data_type for k, v in file_dict.items()]

    @staticmethod
    def _get_file_name(file_name: str) -> str:
        """
        get filemane like "op_summary"
        """
        match = re.search(r'\d', file_name)
        if match and match.start() > 1:
            return file_name[:match.start() - 1]
        logging.warning("The file name  %s is invalid!", file_name)
        return "invalid"

    @staticmethod
    def _data_deduplication(json_data: list, timeline_data: list):
        """
        data deduplication in cann level
        """
        empty_flag = (len(timeline_data) == 0)
        for data in json_data:
            if not empty_flag and "@" in data.get("name"):
                continue
            timeline_data.append(data)

    @staticmethod
    def _write_json_files(json_data: tuple, params: dict) -> tuple:
        """
        write json data  to file
        :param json_data:
        :param params:
        :return:
        """
        data_path = []
        for slice_time in range(len(json_data[1])):
            timeline_file_path = make_export_file_name(params, slice_time, json_data[0])
            try:
                with os.fdopen(os.open(timeline_file_path, Constant.WRITE_FLAGS,
                                       Constant.WRITE_MODES), 'w') as trace_file:
                    trace_file.write(json.dumps(json_data[1][slice_time]))
                    data_path.append(timeline_file_path)
            except (OSError, SystemError, ValueError, TypeError,
                    RuntimeError) as err:
                logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
                return NumberConstant.ERROR, err
        return NumberConstant.SUCCESS, data_path

    @staticmethod
    def _get_readme_info(file_set: set, file_dict: dict, suffix: str) -> str:
        context = file_dict.get("begin", "")
        for index, filename in enumerate(file_set):
            desc = file_dict.get(filename)
            if not desc:
                desc = "Here is no description about this file: " + filename + \
                       ", please check in 'Profiling Instructions'!\n"
            context += f"{str(index + 1)}.{filename}{suffix}:{desc}"
        context += "\n"
        return context

    def export(self: any) -> None:
        """
        export all data
        :return:
        """
        self._make_new_folder()
        self._export_msprof_summary()
        self._export_msprof_timeline()
        self._export_readme_file()

    def _make_new_folder(self):
        self._output_dir = os.path.join(self._output, PathManager.MINDSTUDIO_PROFILER_OUTPUT)
        if os.path.exists(self._output_dir):
            try:
                shutil.rmtree(self._output_dir)
            except PermissionError as err:
                error(self._output_dir, err)
                return
        os.makedirs(self._output_dir, Constant.FOLDER_MASK)

    def _export_msprof_summary(self):
        self._copy_host_summary()
        self._merge_device_summary()

    def _copy_host_summary(self):
        """
        all csv file in host/summary will be moved
        """
        host_dir = os.path.join(self._output, self.MSPROF_HOST_DIR)
        summary_path = os.path.realpath(
            os.path.join(self._output, host_dir, MsProfCommonConstant.SUMMARY))
        if not os.path.exists(summary_path):
            return
        file_list = self._get_newest_file_list(os.listdir(summary_path), StrConstant.FILE_SUFFIX_CSV)
        for file_name in file_list:
            params = {
                StrConstant.PARAM_RESULT_DIR: self._output_dir,
                StrConstant.PARAM_DATA_TYPE: self._get_file_name(file_name),
                StrConstant.PARAM_EXPORT_TYPE: MsProfCommonConstant.SUMMARY
            }
            shutil.copy(os.path.join(summary_path, file_name),
                        make_export_file_name(params))

    def _merge_device_summary(self):
        sub_dirs = get_path_dir(self._output)
        summary_file_set = set()
        for sub_dir in sub_dirs:
            if sub_dir == self.MSPROF_HOST_DIR:
                continue
            sub_path = os.path.realpath(os.path.join(self._output, sub_dir))
            check_path_valid(sub_path, False)
            if DataCheckManager.contain_info_json_data(sub_path):
                summary_file_set.update(self._get_summary_file_name(sub_path))

        for summary_file in summary_file_set:
            helper = FileSliceHelper(self._output_dir, summary_file, MsProfCommonConstant.SUMMARY)
            for sub_dir in sub_dirs:
                if sub_dir == self.MSPROF_HOST_DIR:
                    continue
                sub_path = os.path.realpath(os.path.join(self._output, sub_dir))
                check_path_valid(sub_path, False)
                if DataCheckManager.contain_info_json_data(sub_path):
                    self._save_summary_data(summary_file, sub_path, helper)
            helper.dump_csv_data(force=True)

    def _get_summary_file_name(self, device_path: str) -> set:
        """
        get target summary file in summary dir
        """
        device_summary_set = set()
        summary_path = os.path.realpath(
            os.path.join(self._output, device_path, MsProfCommonConstant.SUMMARY))
        if not os.path.exists(summary_path):
            return device_summary_set
        file_list = os.listdir(summary_path)
        for file_name in self._get_newest_file_list(file_list, StrConstant.FILE_SUFFIX_CSV):
            device_summary_set.add(self._get_file_name(file_name))
        return device_summary_set

    def _save_summary_data(self, targe_name: str, device_path: str, helper: FileSliceHelper):
        """
        get summary_data then create or open csv file in target dir
        """
        summary_path = os.path.realpath(
            os.path.join(self._output, device_path, MsProfCommonConstant.SUMMARY))
        if not os.path.exists(summary_path):
            return
        file_list = os.listdir(summary_path)
        device_id = os.path.basename(device_path)[self.DEVICE_ID_PREFIX_LEN:]
        for file_name in self._get_newest_file_list(file_list, StrConstant.FILE_SUFFIX_CSV):
            if not file_name.startswith(targe_name):
                continue
            all_data = []
            file_name_path = os.path.join(summary_path, file_name)
            with FileOpen(file_name_path, mode='r', max_size=self.FILE_MAX_SIZE) as _csv_file:
                reader = csv.DictReader(_csv_file.file_reader)
                if reader.fieldnames and helper.check_header_is_empty():
                    csv_header = [self.DEVICE_ID, *reader.fieldnames]
                    helper.set_header(csv_header)
                for row in reader:
                    row[self.DEVICE_ID] = device_id
                    all_data.append(row)
            helper.insert_data(all_data)

    def _get_json_data(self: any, sub_dir: str, params: dict) -> list:
        """
        get msprof json file from sub dir
        :param sub_dir:
        :param params:
        :return:
        """
        json_data = []
        timeline_path = os.path.realpath(
            os.path.join(self._output, sub_dir, MsProfCommonConstant.TIMELINE))
        if not os.path.exists(timeline_path):
            return json_data
        file_list = os.listdir(timeline_path)
        for file_name in self._get_newest_file_list(file_list, StrConstant.FILE_SUFFIX_JSON):
            if self._get_file_name(file_name) == params.get(StrConstant.PARAM_DATA_TYPE):
                json_data.extend(Utils.get_json_data(os.path.join(timeline_path, file_name)))
        return json_data

    def _export_msprof_timeline(self: any) -> None:
        self._export_all_timeline_data()

    def _export_all_timeline_data(self: any) -> None:
        sub_dirs = get_path_dir(self._output)
        for json_name in self.JSON_LIST:
            timeline_data = []
            params = {
                StrConstant.PARAM_RESULT_DIR: self._output_dir,
                StrConstant.PARAM_DATA_TYPE: json_name,
                StrConstant.PARAM_EXPORT_TYPE: MsProfCommonConstant.TIMELINE
            }
            for sub_dir in sub_dirs:
                sub_path = os.path.realpath(os.path.join(self._output, sub_dir))
                check_path_valid(sub_path, False)
                if DataCheckManager.contain_info_json_data(sub_path):
                    json_data = self._get_json_data(sub_path, params)
                    self._data_deduplication(json_data, timeline_data)
            self._generate_json_file(params, timeline_data)

    def _generate_json_file(self: any, params: dict, timeline_data: list) -> None:
        if not timeline_data:
            return
        slice_result = MsprofDataStorage().slice_data_list(timeline_data)
        error_code, result_message = self._write_json_files(slice_result, params)
        if error_code:
            print_info(MsProfCommonConstant.COMMON_FILE_NAME, "message error: %s" % result_message)
        else:
            print_info(MsProfCommonConstant.COMMON_FILE_NAME,
                       'Export timeline json file success, "%s" ...' % result_message)

    def _export_readme_file(self):
        cfg_data = ConfigManager.get("FilenameIntroductionConfig").DATA
        timeline_dict = dict(cfg_data.get(MsProfCommonConstant.TIMELINE))
        timeline_set = set()
        summary_dict = dict(cfg_data.get(MsProfCommonConstant.SUMMARY))
        summary_set = set()
        file_list = os.listdir(self._output_dir)
        for ori_filename in file_list:
            if ori_filename.endswith(StrConstant.FILE_SUFFIX_CSV):
                summary_set.add(self._get_file_name(ori_filename))
            elif ori_filename.endswith(StrConstant.FILE_SUFFIX_JSON):
                timeline_set.add(self._get_file_name(ori_filename))

        with os.fdopen(os.open(os.path.join(self._output_dir, self.README), Constant.WRITE_FLAGS,
                               Constant.WRITE_MODES), 'w') as readme:
            context = self._get_readme_info(timeline_set, timeline_dict,
                                            StrConstant.FILE_SUFFIX_JSON)
            context += self._get_readme_info(summary_set, summary_dict,
                                             StrConstant.FILE_SUFFIX_CSV)
            readme.write(context)
