#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import csv
import json
import logging
import os
import re
import shutil

from common_func.common import print_info
from common_func.constant import Constant
from common_func.data_check_manager import DataCheckManager
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


class MsprofMindStudioProfiler:
    """
    class used to export all job data.
    """
    MSPROF_HOST_DIR = "host"
    DEVICE_ID = "Device_id"
    JSON_LIST = [
        "msprof", "step_trace", "msprof_tx"
    ]

    def __init__(self: any, output: str) -> None:
        self._output = output
        self._output_dir = ""
        self._log_dir = ""

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
            shutil.rmtree(self._output_dir)
        os.makedirs(self._output_dir, Constant.FOLDER_MASK)

    def _export_msprof_summary(self):
        self._move_host_summary()
        self._merge_device_summary()

    def _move_host_summary(self):
        """
        all csv file in host/summary will be moved
        """
        host_dir = os.path.join(self._output, self.MSPROF_HOST_DIR)
        summary_path = os.path.realpath(
            os.path.join(self._output, host_dir, MsProfCommonConstant.SUMMARY))
        if not os.path.exists(summary_path):
            return
        file_list = self._update_file_list(os.listdir(summary_path), StrConstant.FILE_SUFFIX_CSV)
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
            if sub_dir != self.MSPROF_HOST_DIR:
                sub_path = os.path.realpath(os.path.join(self._output, sub_dir))
                check_path_valid(sub_path, False)
                if DataCheckManager.contain_info_json_data(sub_path):
                    self._get_summary_file_name(summary_file_set, sub_path)

        for file in summary_file_set:
            helper = FileSliceHelper(self._output_dir, file, MsProfCommonConstant.SUMMARY)
            for sub_dir in sub_dirs:
                if sub_dir != self.MSPROF_HOST_DIR:
                    sub_path = os.path.realpath(os.path.join(self._output, sub_dir))
                    check_path_valid(sub_path, False)
                    if DataCheckManager.contain_info_json_data(sub_path):
                        self._get_summary_data(file, sub_path, helper)
            helper.dump_csv_data(True)

    def _get_summary_file_name(self, file_set: set, device_path: str):
        """
        get target summary file in summary dir
        """
        summary_path = os.path.realpath(
            os.path.join(self._output, device_path, MsProfCommonConstant.SUMMARY))
        if not os.path.exists(summary_path):
            return
        file_list = os.listdir(summary_path)
        for file_name in self._update_file_list(file_list, StrConstant.FILE_SUFFIX_CSV):
            file_set.add(self._get_file_name(file_name))

    def _get_summary_data(self, targe_name: str, device_path: str, helper: FileSliceHelper):
        """
        get summary_data then create or open csv file in target dir
        """
        summary_path = os.path.realpath(
            os.path.join(self._output, device_path, MsProfCommonConstant.SUMMARY))
        if not os.path.exists(summary_path):
            return
        file_list = os.listdir(summary_path)
        device_id = os.path.basename(device_path)[7:]
        for file_name in self._update_file_list(file_list, StrConstant.FILE_SUFFIX_CSV):
            if file_name.startswith(targe_name):
                all_data = []
                file_name_path = os.path.join(summary_path, file_name)
                with open(file_name_path, 'r') as _csv_file:
                    reader = csv.DictReader(_csv_file)
                    if reader.fieldnames:
                        csv_header = [self.DEVICE_ID, *reader.fieldnames]
                        helper.set_header(csv_header)
                    for row in reader:
                        row[self.DEVICE_ID] = device_id
                        all_data.append(row)
                helper.insert_data(all_data)

    @staticmethod
    def _update_file_list(file_list, data_type: str) -> list:
        """
        filename is key,timestamp is value,update time to get the newest file list
        """
        file_dict = {}
        for file in file_list:
            if file.endswith(data_type):
                underscore_pos = file.rfind("_")
                point_pos = file.rfind(".")
                if underscore_pos < 1 or underscore_pos > len(file) - 1 or \
                        point_pos < 0 or point_pos > len(file) or \
                        point_pos < underscore_pos:
                    logging.warning("The file name  %s is invalid!", file)
                    continue
                time_str = file[underscore_pos + 1: point_pos]
                if not is_number(time_str):
                    logging.warning("The file name  %s is invalid!", file)
                    continue
                time = int(time_str)
                key = file[:underscore_pos + 1]
                if (key not in file_dict) or (key in file_dict and file_dict[key] < time):
                    file_dict[key] = time
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
        for file_name in self._update_file_list(file_list, StrConstant.FILE_SUFFIX_JSON):
            if self._get_file_name(file_name) == params.get(StrConstant.PARAM_DATA_TYPE):
                json_data.extend(Utils.get_json_data(os.path.join(timeline_path, file_name)))
        return json_data

    def _export_msprof_timeline(self: any) -> None:
        self._export_all_timeline_data()

    def _export_all_timeline_data(self: any) -> None:
        sub_dirs = get_path_dir(self._output)
        for json_name in self.JSON_LIST:
            timeline_data = []
            params = {StrConstant.PARAM_RESULT_DIR: self._output_dir,
                      StrConstant.PARAM_DATA_TYPE: json_name,
                      StrConstant.PARAM_EXPORT_TYPE: MsProfCommonConstant.TIMELINE}
            for sub_dir in sub_dirs:
                sub_path = os.path.realpath(os.path.join(self._output, sub_dir))
                check_path_valid(sub_path, False)
                if DataCheckManager.contain_info_json_data(sub_path):
                    json_data = self._get_json_data(sub_path, params)
                    self._data_deduplication(json_data, timeline_data)
            self._generate_json_file(params, timeline_data)

    def _generate_json_file(self: any, params: dict, timeline_data: list) -> None:
        if timeline_data:
            slice_result = MsprofDataStorage().slice_data_list(timeline_data)
            error_code, result_message = self._write_json_files(slice_result, params)
            if error_code:
                print_info(MsProfCommonConstant.COMMON_FILE_NAME, "message error: %s" % result_message)
            else:
                print_info(MsProfCommonConstant.COMMON_FILE_NAME,
                           'Export timeline json file success, "%s" ...' % result_message)

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

    def _export_readme_file(self):
        cfg_data = ConfigManager.get("FilenameIntroductionConfig").DATA
        timeline_dict = dict(cfg_data.get(MsProfCommonConstant.TIMELINE))
        timeline_set = set()
        summary_dict = dict(cfg_data.get(MsProfCommonConstant.SUMMARY))
        summary_set = set()
        file_list = os.listdir(self._output_dir)
        for file in file_list:
            if file.endswith(StrConstant.FILE_SUFFIX_CSV):
                summary_set.add(self._get_file_name(file))
            elif file.endswith(StrConstant.FILE_SUFFIX_JSON):
                timeline_set.add(self._get_file_name(file))

        with open(os.path.join(self._output_dir, "README.txt"), "w") as readme:
            desc = timeline_dict.get("begin", "")
            readme.write(desc + "\n")
            for index, file in enumerate(timeline_set):
                desc = timeline_dict.get(file)
                if not desc:
                    desc = "Here is no description about this file: " + file + \
                           ", please check in 'Profiling Instructions'!"
                readme.write(str(index + 1) + "." + file + StrConstant.FILE_SUFFIX_JSON + ": " + desc + "\n")
            readme.write("\n")

            desc = summary_dict.get("begin", "")
            readme.write(desc + "\n")
            for index, file in enumerate(summary_set):
                desc = summary_dict.get(file)
                if not desc:
                    desc = "Here is no description about this file: " + file + \
                           ", please check in 'Profiling Instructions'!"
                readme.write(str(index + 1) + "." + file + StrConstant.FILE_SUFFIX_CSV + ": " + desc + "\n")




