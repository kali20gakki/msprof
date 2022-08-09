#!/usr/bin/python3
# coding=utf-8
"""
Function:
QueryCommand class. This class mainly involves the process function.
Copyright Information:
Huawei Technologies Co., Ltd. All Rights Reserved © 2020
"""
import logging
import os
from operator import itemgetter

from common_func.common import print_msg
from common_func.common import warn
from common_func.data_check_manager import DataCheckManager
from common_func.common import error
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.msprof_common import check_path_valid
from common_func.msprof_common import get_path_dir
from common_func.msprof_exception import ProfException
from common_func.msprof_query_data import MsprofQueryData
from common_func.utils import Utils
from framework.load_info_manager import LoadInfoManager
from msmodel.cluster_info.cluster_info_model import ClusterInfoModel


class QueryCommand:
    """
    The class for handle query command.
    """
    FILE_NAME = os.path.basename(__file__)
    SHOW_HEADERS = ['Job Info', 'Device ID', 'Dir Name', 'Collection Time',
                    'Model ID', 'Iteration Number', 'Top Time Iteration', 'Rank ID']

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
                                _result.top_time_iteration, _result.rank_id])
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
        is_cluster_query = self._check_cluster_query()
        if not is_cluster_query:
            self.check_argument_valid()
            table_data = self._get_query_data()
        else:
            table_data = self._get_cluster_query_data()
        sorted_table_data = sorted(table_data, key=itemgetter(0, 3))
        self._format_print(sorted_table_data)

    def _get_query_data(self: any) -> list:
        result_data = []
        if DataCheckManager.contain_info_json_data(self.collection_path):  # find profiling data dir
            result = self._do_get_query_data(os.path.realpath(self.collection_path))
            result_data.extend(result)
        else:
            result_data = self._get_query_data_from_sub_dir(self.collection_path)
        return result_data

    def _get_query_data_from_sub_dir(self: any, path: str) -> list:
        result_data = []
        sub_dirs = get_path_dir(path)
        for sub_dir in sub_dirs:  # result_dir
            if sub_dir != StrConstant.TIMELINE_PATH:
                sub_path = os.path.realpath(
                    os.path.join(path, sub_dir))
                check_path_valid(sub_path, False)
                if DataCheckManager.contain_info_json_data(sub_path):  # find profiling data dir
                    result = self._do_get_query_data(sub_path)
                    result_data.extend(result)
                else:
                    warn(self.FILE_NAME, 'Invalid query dir("%s"), if you want to query cluster data, please import '
                                         '--cluster first! or -dir must be profiling data, '
                                         'such as PROF_XXX_XXX_XXX' % path)
        return result_data

    def _check_cluster_query(self: any) -> bool:
        if DataCheckManager.contain_info_json_data(self.collection_path):
            return False
        if self._check_cluster_sqlite_db(os.path.join(self.collection_path, 'sqlite')):
            return True
        return False

    def _get_cluster_query_data(self: any) -> list:
        with ClusterInfoModel(self.collection_path) as cluster_info_model:
            cluster_info_list = cluster_info_model.get_all_data(DBNameConstant.TABLE_CLUSTER_RANK)
        if not cluster_info_list:
            error(self.FILE_NAME, 'Table ClusterRank does not exist or table ClusterRank is empty!'
                          ' please check the db(%s)' % os.path.join(self.collection_path, 'sqlite\\cluster_rank.db'))
            return []
        return MsprofQueryData.query_cluster_data(self.collection_path, cluster_info_list)

    def _check_cluster_sqlite_db(self: any, cluster_sqlite_path: str) -> bool:
        if not os.path.exists(cluster_sqlite_path):
            return False
        rank_db_path = os.path.join(cluster_sqlite_path, DBNameConstant.DB_CLUSTER_RANK)
        step_db_path = os.path.join(cluster_sqlite_path, DBNameConstant.DB_CLUSTER_STEP_TRACE)
        if not os.path.exists(rank_db_path):
            warn(self.FILE_NAME, "cluster_rank.db not created in the dir(%s), "
                                 "please import --cluster first!" % cluster_sqlite_path)
            raise ProfException(ProfException.PROF_CLUSTER_INVALID_DB)
        if not os.path.exists(step_db_path):
            warn(self.FILE_NAME, "cluster_step_trace.db not created in the dir(%s), "
                            "please import --cluster first!" % cluster_sqlite_path)
            raise ProfException(ProfException.PROF_CLUSTER_INVALID_DB)
        return True