#!/usr/bin/python3
# coding=utf-8
"""
Function:
QueryCommand class. This class mainly involves the process function.
Copyright Information:
Huawei Technologies Co., Ltd. All Rights Reserved © 2020
"""

import os
from operator import itemgetter

from common_func.common import print_msg
from common_func.common import warn
from common_func.data_check_manager import DataCheckManager
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import check_path_valid
from common_func.msprof_common import get_path_dir
from common_func.msprof_query_data import MsprofQueryData
from common_func.utils import Utils
from framework.load_info_manager import LoadInfoManager


class QueryCommand:
    """
    The class for handle query command.
    """
    FILE_NAME = os.path.basename(__file__)
    SHOW_HEADERS = ['Job Info', 'Device ID', 'Dir Name', 'Collection Time',
                    'Model ID', 'Iteration Number', 'Top Time Iteration']

    def __init__(self: any, collection_path: str) -> None:
        self.collection_path = os.path.realpath(collection_path)

    @staticmethod
    def _calculate_str_length(headers: list, data: list) -> list:
        max_header_list = Utils.generator_to_list(len(header) for header in headers)
        max_column_list = max_header_list
        for _data in data:
            max_data_list = Utils.generator_to_list(len(str(_element)) for _element in _data)
            max_column_list = Utils.generator_to_list(max(_element) for _element in zip(max_data_list, max_column_list))
        return max_column_list

    @staticmethod
    def _do_get_query_data(result_dir: str) -> list:
        result_data = []
        LoadInfoManager().load_info(result_dir)
        result = MsprofQueryData(result_dir).query_data()
        for _result in result:
            result_data.append([_result.job_info, _result.device_id,
                                _result.job_name, _result.collection_time,
                                _result.model_id, _result.iteration_id,
                                _result.top_time_iteration])
        return result_data

    @classmethod
    def _format_print(cls: any, data: list, headers: list = None) -> None:
        if not data:
            return

        if headers is None:
            headers = cls.SHOW_HEADERS

        max_column_list = cls._calculate_str_length(headers, data)
        cls._show_query_header(headers, max_column_list)
        cls._show_query_data(data, max_column_list)

    @classmethod
    def _show_query_data(cls: any, data: list, max_column_list: list) -> None:
        for _data in data:
            for index, _element in enumerate(_data):
                print_msg(str(_element).ljust(max_column_list[index], ' '), end="\t")
            print_msg("\n")

    @classmethod
    def _show_query_header(cls: any, headers: list, max_column_list: list) -> None:
        for index, header in enumerate(headers):
            print_msg(str(header).ljust(max_column_list[index], ' '), end="\t")
        print_msg("\n")

    def check_argument_valid(self: any) -> None:
        """
        Check the argument valid
        :return: None
        """
        check_path_valid(self.collection_path, False)

    def process(self: any) -> None:
        """
        handle query command
        :return: None
        """
        self.check_argument_valid()
        table_data = self._get_query_data()
        sorted_table_data = sorted(table_data, key=itemgetter(0, 3))
        self._format_print(sorted_table_data)

    def _get_query_data(self: any) -> list:
        result_data = []
        if DataCheckManager.contain_info_json_data(self.collection_path):  # find profiling data dir
            result = self._do_get_query_data(os.path.realpath(self.collection_path))
            result_data.extend(result)
        else:
            self.process_sub_dirs(result_data)
        return result_data

    def process_sub_dirs(self: any, result_data: list):
        sub_dirs = get_path_dir(self.collection_path)
        for sub_dir in sub_dirs:  # result_dir
            if sub_dir == StrConstant.TIMELINE_PATH:
                continue
            sub_path = os.path.realpath(
                os.path.join(self.collection_path, sub_dir))
            if DataCheckManager.process_check(sub_path):
                self._process_sub_dirs(result_data)
                break
            else:
                self._process_sub_dirs(result_data, sub_dir)

    def _process_sub_dirs(self: any, result_data: list, subdir: str = '') -> None:
        collect_path = self.collection_path
        result = []
        if subdir:
            collect_path = os.path.join(self.collection_path, subdir)
        sub_dirs = get_path_dir(collect_path)
        for sub_dir in sub_dirs:  # result_dir
            sub_path = os.path.realpath(
                os.path.join(collect_path, sub_dir))
            if DataCheckManager.process_check(sub_path):
                result = self._do_get_query_data(sub_path)
            else:
                warn(self.FILE_NAME, 'Invalid parsing dir("%s"), -dir must be profiling data dir '
                                     'such as PROF_XXX_XXX_XXX' % collect_path)
        if subdir and result:
            for data in result:
                data[2] = '{}\\{}'.format(subdir, data[2])
        result_data.extend(result)
