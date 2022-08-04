#!/usr/bin/python3
# coding:utf-8
"""
This scripts is used to parse some project path profiling data
Copyright Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
"""
import logging
import os

from common_func.common import warn, print_info
from common_func.config_mgr import ConfigMgr
from common_func.data_check_manager import DataCheckManager
from common_func.msprof_common import analyze_collect_data, prepare_for_parse
from common_func.common import error
from common_func.msprof_common import MsProfCommonConstant
from common_func.msprof_common import check_path_valid
from common_func.msprof_common import get_path_dir
from common_func.msprof_exception import ProfException
from framework.load_info_manager import LoadInfoManager
from msparser.cluster.cluster_info_parser import ClusterInfoParser
from msparser.cluster.cluster_step_trace_parser import ClusterStepTraceParser


class ImportCommand:
    """
    The class for handle import command.
    """

    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any, args: any) -> None:
        self.collection_path = os.path.realpath(args.collection_path)
        self.cluster_flag = args.cluster_flag

    @staticmethod
    def do_import(result_dir: str) -> None:
        """
        parse data from result dir
        :param result_dir: result dir
        :return: None
        """
        try:
            analyze_collect_data(result_dir, ConfigMgr.read_sample_config(result_dir))
        except ProfException:
            warn(MsProfCommonConstant.COMMON_FILE_NAME,
                 'Analysis data in "%s" failed. Maybe the data is incomplete.' % result_dir)

    def process(self: any) -> None:
        """
        command import command entry
        :return: None
        """
        check_path_valid(self.collection_path, False)
        if self.cluster_flag is False:
            self._process_parse()
        else:
            _unparsed_dirs = self._check_cluster_path()
            prepare_for_parse(self.collection_path)
            if _unparsed_dirs:
                self._parse_unparsed_dirs(_unparsed_dirs)
            sqlite_path = os.path.realpath(os.path.join(self.collection_path, 'sqlite'))
            if not os.path.exists(sqlite_path):
                os.makedirs(sqlite_path)
            print_info(MsProfCommonConstant.COMMON_FILE_NAME,
                       'Start parse cluster data in "%s" ...' % self.collection_path)
            cluster_info_parser = ClusterInfoParser(self.collection_path)
            cluster_info_parser.ms_run()
            cluster_step_trace_parser = ClusterStepTraceParser(self.collection_path)
            cluster_step_trace_parser.ms_run()
            print_info(MsProfCommonConstant.COMMON_FILE_NAME,
                       'cluster data parse finished!')

    def _process_parse(self: any) -> None:
        if DataCheckManager.contain_info_json_data(self.collection_path):  # find prof data dir
            LoadInfoManager.load_info(self.collection_path)
            self.do_import(os.path.realpath(self.collection_path))
        else:
            self._process_sub_dirs()

    def _process_sub_dirs(self: any, subdir: str = '', is_cluster: bool = False) -> None:
        collect_path = self.collection_path
        if subdir:
            collect_path = os.path.join(self.collection_path, subdir)
        sub_dirs = get_path_dir(collect_path)
        for sub_dir in sub_dirs:  # result_dir
            sub_path = os.path.realpath(
                os.path.join(collect_path, sub_dir))
            check_path_valid(sub_path, False)
            if DataCheckManager.contain_info_json_data(sub_path):
                LoadInfoManager.load_info(sub_path)
                self.do_import(sub_path)
            elif collect_path and is_cluster:
                warn(self.FILE_NAME, 'Invalid parsing dir("%s"), -dir must be profiling data dir. '
                                     'such as PROF_XXX_XXX_XXX' % collect_path)
            else:
                self._process_sub_dirs(sub_dir, is_cluster=True)

    def _check_cluster_path(self: any) -> list:
        check_path_valid(self.collection_path, False)
        if DataCheckManager.contain_info_json_data(self.collection_path):
            error(MsProfCommonConstant.COMMON_FILE_NAME, 'Incorrect parse dir(%s),'
                                     '-dir argument must be cluster data root dir.' % self.collection_path)
            raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR)
        _unparsed_dirs = {}
        first_sub_dirs = get_path_dir(self.collection_path)
        for first_sub_dir in first_sub_dirs:
            first_sub_path = os.path.realpath(
                    os.path.join(self.collection_path, first_sub_dir))
            if DataCheckManager.contain_info_json_data(first_sub_path):
                error(MsProfCommonConstant.COMMON_FILE_NAME, 'Incorrect parse dir(%s),'
                                         '-dir argument must be cluster data root dir.' % self.collection_path)
                raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR)
            _unparsed_second_dirs = []
            second_sub_dirs = get_path_dir(first_sub_path)
            for second_sub_dir in second_sub_dirs:
                second_sub_path = os.path.realpath(
                        os.path.join(first_sub_path, second_sub_dir))
                if not DataCheckManager.contain_info_json_data(second_sub_path):
                    error(MsProfCommonConstant.COMMON_FILE_NAME, 'The path (%s) is not a correct PROF dir.'
                                                                 ' Please check the path.' % first_sub_path)
                    _unparsed_second_dirs = []
                    break
                sqlite_path = os.path.join(second_sub_path, 'sqlite')
                if not os.path.exists(sqlite_path) or not os.listdir(sqlite_path):
                    _unparsed_second_dirs.append(second_sub_dir)
            if _unparsed_second_dirs:
                _unparsed_dirs.setdefault(first_sub_dir, _unparsed_second_dirs)
        return _unparsed_dirs

    def _parse_unparsed_dirs(self: any, unparsed_dirs: list) -> None:
        for pro_dir, result_dir_list in unparsed_dirs.items():
            prof_path = os.path.join(self.collection_path, pro_dir)
            for result_dir in result_dir_list:
                result_path = os.path.realpath(
                    os.path.join(prof_path, result_dir))
                check_path_valid(result_path, False)
                LoadInfoManager.load_info(result_path)
                self.do_import(result_path)