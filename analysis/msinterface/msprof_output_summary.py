#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

"""
The MsprofOutputSummary's function:
1 search all file in summary/timeline dir,and save thier name in set
2 cyclically reads the file by the name in the set, with multiprocess
3 dump by FileSliceHelper

for timeline:
find the file by name in timeline dir, and classify them by the slice number
                       ----------   ----------   ----------         ----------
process_0---slice_0----|device_0|   |device_1|   |device_2|   ...   |device_*|---> gather all to slice_0.json
                       ----------   ----------   ----------         ----------
                       ----------   ----------   ----------         ----------
process_1---slice_1----|device_0|   |device_1|   |device_2|   ...   |device_*|---> gather all to slice_1.json
                       ----------   ----------   ----------         ----------
   ...        ...         ....         ....         ....      ...      ....
                       ----------   ----------   ----------         ----------
process_*---slice_*----|device_0|   |device_1|   |device_2|   ...   |device_*|---> gather all to slice_*.json
                       ----------   ----------   ----------         ----------
In slice_*, will be 8 or more device's data, all off them will be gathered,
and dump to a file named slice_*.json.
Enable multiple processes based on the number of slice.
"""

import csv
import logging
import multiprocessing
import os
import re
import shutil

from common_func.common import warn
from common_func.common import print_info
from common_func.constant import Constant
from common_func.data_check_manager import DataCheckManager
from common_func.file_manager import FdOpen
from common_func.file_manager import FileOpen
from common_func.file_manager import check_path_valid
from common_func.file_slice_helper import FileSliceHelper
from common_func.file_slice_helper import make_export_file_name
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import MsProfCommonConstant
from common_func.msprof_common import get_path_dir
from common_func.msprof_exception import ProfException
from common_func.msvp_common import is_number
from common_func.msvp_common import check_dir_writable
from common_func.path_manager import PathManager
from common_func.utils import Utils
from msconfig.config_manager import ConfigManager


class MsprofOutputSummary:
    """
    class used to export all job data.
    """
    DEVICE_PREFIX = "device_"
    INVALID_SUFFIX = "invalid"
    MSPROF_HOST_DIR = "host"
    DEVICE_ID = "Device_id"
    DEVICE_ID_PREFIX_LEN = 7
    SLICE_LEN = 7
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
    def _get_file_name(file_name: str) -> str:
        """
        get filemane like "op_summary"
        """
        match = re.search(r'(_\d)?(_slice_\d+)?_\d+', file_name)
        if match and match.start() > 0:
            return file_name[:match.start()]
        logging.warning("The file name  %s is invalid!", file_name)
        return "invalid"

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

    @classmethod
    def get_newest_file_list(cls, file_list, data_type: str) -> list:
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

    def export(self: any, command_type: str) -> None:
        """
        export all data
        :return:
        """
        print_info(MsProfCommonConstant.COMMON_FILE_NAME, f"Start exporting {command_type} output_file.")
        if not self._is_in_prof_file():
            return
        if not self._clear_output_folder(self._get_file_suffix(command_type)):
            logging.error("Clear file in mindstudio_profiler_output.")
            return
        if command_type == MsProfCommonConstant.SUMMARY:
            self._export_msprof_summary()
        elif command_type == MsProfCommonConstant.TIMELINE:
            self._export_msprof_timeline()
        self._export_readme_file()
        print_info(MsProfCommonConstant.COMMON_FILE_NAME, f"End exporting {command_type} output_file.")

    def _is_in_prof_file(self):
        """
        If the current directory contains host or device_* file
        Then consider this directory as PROF_XXX
        """
        file_list = os.listdir(self._output)
        for file_name in file_list:
            if file_name == self.MSPROF_HOST_DIR or \
                    file_name.startswith(self.DEVICE_PREFIX):
                return True
        return False

    def _get_file_suffix(self, command_type: str) -> str:
        """
        get summary or timeline's suffix, ".csv" or ".json"
        """
        if command_type == MsProfCommonConstant.SUMMARY:
            return StrConstant.FILE_SUFFIX_CSV
        elif command_type == MsProfCommonConstant.TIMELINE:
            return StrConstant.FILE_SUFFIX_JSON
        else:
            logging.error("%s is invalid, can't export this type file.", command_type)
            return self.INVALID_SUFFIX

    def _clear_output_folder(self, suffix: str) -> bool:
        if suffix == self.INVALID_SUFFIX:
            return False
        self._output_dir = os.path.join(self._output, PathManager.MINDSTUDIO_PROFILER_OUTPUT)
        if not os.path.exists(self._output_dir):
            os.makedirs(self._output_dir, Constant.FOLDER_MASK)
            return True
        file_list = os.listdir(self._output_dir)
        check_dir_writable(self._output_dir)
        try:
            for output_file in file_list:
                dir_name = os.path.join(self._output_dir, output_file)
                if not output_file.endswith(suffix) and output_file != self.README:
                    continue
                check_path_valid(dir_name, is_file=True, max_size=self.FILE_MAX_SIZE)
                os.remove(dir_name)
            return True
        except PermissionError as err:
            logging.warning("PermissionError: Can not delete file in mindstudio_profiler_output!")
            warn(MsProfCommonConstant.COMMON_FILE_NAME, err)
            return False

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
        file_list = self.get_newest_file_list(os.listdir(summary_path), StrConstant.FILE_SUFFIX_CSV)
        for file_name in file_list:
            params = {
                StrConstant.PARAM_RESULT_DIR: self._output_dir,
                StrConstant.PARAM_DATA_TYPE: self._get_file_name(file_name),
                StrConstant.PARAM_EXPORT_TYPE: MsProfCommonConstant.SUMMARY
            }
            shutil.copy(os.path.join(summary_path, file_name),
                        make_export_file_name(params))

    def _merge_device_summary(self):
        """
        merge csv file if they have the same name
        op_summary_0_0_1 and op_summary_0_0_2 both op_summary
        """
        sub_dirs = get_path_dir(self._output)
        summary_file_set = set()
        for sub_dir in sub_dirs:
            if sub_dir == self.MSPROF_HOST_DIR:
                continue
            sub_path = os.path.realpath(os.path.join(self._output, sub_dir))
            check_path_valid(sub_path, is_file=False)
            if not DataCheckManager.contain_info_json_data(sub_path):
                continue
            summary_file_set.update(self._get_summary_file_name(sub_path))

        pool = multiprocessing.Pool(processes=4)
        for summary_file in summary_file_set:
            try:
                pool.apply_async(func=self._save_summary_data,
                                 args=(summary_file, sub_dirs))
            except ProfException as err:
                logging.error("Output: An exception occurs when multiple processes process the summary file. "
                              "The error is %s", err)
                return
        pool.close()
        pool.join()

    def _get_summary_file_name(self, device_path: str) -> set:
        """
        get target summary file in summary dir
        """
        device_summary_set = set()
        summary_path = os.path.realpath(
            os.path.join(device_path, MsProfCommonConstant.SUMMARY))
        if not os.path.exists(summary_path):
            return device_summary_set
        file_list = os.listdir(summary_path)
        for file_name in self.get_newest_file_list(file_list, StrConstant.FILE_SUFFIX_CSV):
            device_summary_set.add(self._get_file_name(file_name))
        return device_summary_set

    def _save_summary_data(self, targe_name: str, sub_dirs: list):
        """
        get summary_data then create or open csv file in target dir
        """
        helper = FileSliceHelper(self._output_dir, targe_name, MsProfCommonConstant.SUMMARY)
        for sub_dir in sub_dirs:
            if sub_dir == self.MSPROF_HOST_DIR:
                continue
            sub_path = os.path.realpath(os.path.join(self._output, sub_dir))
            check_path_valid(sub_path, is_file=False)
            if not DataCheckManager.contain_info_json_data(sub_path):
                continue
            summary_path = os.path.realpath(
                os.path.join(sub_path, MsProfCommonConstant.SUMMARY))
            if not os.path.exists(summary_path):
                return
            file_list = os.listdir(summary_path)
            device_id = os.path.basename(sub_path)[self.DEVICE_ID_PREFIX_LEN:]
            for file_name in self.get_newest_file_list(file_list, StrConstant.FILE_SUFFIX_CSV):
                if not file_name.startswith(targe_name):
                    continue
                file_name_path = os.path.join(summary_path, file_name)
                self._insert_summary_data(file_name_path, device_id, helper)
        helper.dump_csv_data(force=True)

    def _insert_summary_data(self, file_name_path: str, device_id: str,
                             helper: FileSliceHelper):
        with FileOpen(file_name_path, mode='r', max_size=self.FILE_MAX_SIZE) as _csv_file:
            reader = csv.DictReader(_csv_file.file_reader)
            if reader.fieldnames and helper.check_header_is_empty():
                csv_header = [self.DEVICE_ID, *reader.fieldnames]
                helper.set_header(csv_header)
            all_data = [[]] * FileSliceHelper.CSV_LIMIT
            for i, row in enumerate(reader):
                remainder = i % FileSliceHelper.CSV_LIMIT
                if i and not remainder:
                    helper.insert_data(all_data)
                row[self.DEVICE_ID] = device_id
                all_data[remainder] = row
            helper.insert_data(all_data[:(reader.line_num - 1) % FileSliceHelper.CSV_LIMIT])

    def _export_msprof_timeline(self: any) -> None:
        self._export_all_timeline_data()

    def _export_all_timeline_data(self: any) -> None:
        """
        get json file from different dir,and then to load
        """
        sub_dirs = get_path_dir(self._output)
        processes = []
        for json_file in self.JSON_LIST:
            try:
                process = multiprocessing.Process(target=self._save_timeline_data,
                                                  args=(json_file, sub_dirs))
                process.start()
                processes.append(process)
            except ProfException as err:
                logging.error("Output: An exception occurs when multiple processes process the timeline file. "
                              "The error is %s", err)
                return
        for process in processes:
            process.join()

    def _save_timeline_data(self, targe_name: str, sub_dirs: list):
        timeline_file_dict = {}
        slice_max_count = 0
        for sub_dir in sub_dirs:
            sub_path = os.path.realpath(os.path.join(self._output, sub_dir))
            check_path_valid(sub_path, is_file=False)
            if not DataCheckManager.contain_info_json_data(sub_path):
                continue
            timeline_file_dict, slice_count = \
                self._get_timeline_file_with_slice(targe_name, sub_path, timeline_file_dict)
            slice_max_count = max(slice_count, slice_max_count)

        pool = multiprocessing.Pool(processes=4)
        is_need_slice = True if slice_max_count else False
        for index in range(slice_max_count + 1):
            helper = FileSliceHelper(self._output_dir, targe_name, MsProfCommonConstant.TIMELINE)
            try:
                pool.apply_async(func=self._insert_json_data,
                                 args=(timeline_file_dict.get(index, []), helper,
                                       is_need_slice, index))
            except ProfException as err:
                logging.error("Output: An exception occurs when multiple processes process the timeline file. "
                              "The error is %s", err)
                return
        pool.close()
        pool.join()

    def _insert_json_data(self, file_list: list, helper: FileSliceHelper,
                          is_need_slice: bool, slice_index: int):
        """
        1 one device only have one "slice_0"
        2 if only one device, then no need to merge file, copy will be better.
        """
        if len(file_list) == 1:
            params = {
                StrConstant.PARAM_RESULT_DIR: self._output_dir,
                StrConstant.PARAM_DATA_TYPE: self._get_file_name(os.path.basename(file_list[0])),
                StrConstant.PARAM_EXPORT_TYPE: MsProfCommonConstant.TIMELINE
            }
            shutil.copy(file_list[0], make_export_file_name(params, slice_index, slice_switch=is_need_slice))
            return
        for _file_name in file_list:
            helper.insert_data(Utils.get_json_data(_file_name))
        helper.dump_json_data(slice_index, is_need_slice=is_need_slice)

    def _get_timeline_file_with_slice(self, target_name: str, dir_path: str, timeline_file_dict: dict) -> tuple:
        """
        Differentiate files with the same name by slice_count.
        in timeline_file_dict:
        0:xxx_slice_0.json, xxx.json
        1:xxx_slice_1.json
        """
        slice_max_count = 0
        timeline_path = os.path.realpath(
            os.path.join(dir_path, MsProfCommonConstant.TIMELINE))
        if not os.path.exists(timeline_path):
            return timeline_file_dict, slice_max_count
        file_list = os.listdir(timeline_path)
        for _file_name in self.get_newest_file_list(file_list, StrConstant.FILE_SUFFIX_JSON):
            if not _file_name.startswith(target_name):
                continue
            match = re.search(r'_slice_\d+', _file_name)
            file_name = os.path.join(timeline_path, _file_name)
            slice_count = 0
            if match and match.start() > 0:
                slice_count = _file_name[match.start() + self.SLICE_LEN: match.end()]
                if not is_number(slice_count):
                    logging.warning("This file name is invalid: %s", file_name)
                    continue
                slice_max_count = max(int(slice_count), slice_max_count)
            slice_file_list = timeline_file_dict.get(int(slice_count), [])
            slice_file_list.append(file_name)
            timeline_file_dict.update({int(slice_count): slice_file_list})
        return timeline_file_dict, slice_max_count

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

        file_path = os.path.join(self._output_dir, self.README)
        with FdOpen(file_path) as readme:
            context = self._get_readme_info(timeline_set, timeline_dict,
                                            StrConstant.FILE_SUFFIX_JSON)
            context += self._get_readme_info(summary_set, summary_dict,
                                             StrConstant.FILE_SUFFIX_CSV)
            readme.write(context)
