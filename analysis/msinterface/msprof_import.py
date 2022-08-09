#!/usr/bin/python3
# coding:utf-8
"""
This scripts is used to parse some project path profiling data
Copyright Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
"""
import datetime
import logging
import os
import shutil

from common_func.common import warn, print_info
from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant
from common_func.data_check_manager import DataCheckManager
from common_func.info_conf_reader import InfoConfReader
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
        self.is_cluster_scence = args.cluster_flag
        self.cluster_device_paths = []

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
        if not self.is_cluster_scence:
            self._process_parse()
        else:
            _unparsed_dirs = self._find_unparsered_dirs()
            self._prepare_for_cluster_parse()
            if _unparsed_dirs:
                self._parse_unparsed_dirs(_unparsed_dirs)
            print_info(MsProfCommonConstant.COMMON_FILE_NAME,
                       'Start parse cluster data in "%s" ...' % self.collection_path)
            cluster_info_parser = ClusterInfoParser(self.collection_path, self.cluster_device_paths)
            cluster_info_parser.ms_run()
            cluster_step_trace_parser = ClusterStepTraceParser(self.collection_path)
            cluster_step_trace_parser.ms_run()
            print_info(MsProfCommonConstant.COMMON_FILE_NAME,
                       'Cluster data parse finished!')

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
                warn(self.FILE_NAME, 'Invalid parsing dir(%s), -dir must be profiling data dir, '
                                     'such as PROF_XXX_XXX_XXX' % collect_path)
            else:
                self._process_sub_dirs(sub_dir, is_cluster=True)

    def _find_unparsered_dirs(self: any) -> list:
        prof_dirs = self._get_prof_dirs()
        _unparsed_dirs = {}
        for prof_dir in prof_dirs:
            _unparsed_prof_sub_dirs = []
            prof_path = os.path.realpath(
                        os.path.join(self.collection_path, prof_dir))
            if not os.listdir(prof_path):
                warn(MsProfCommonConstant.COMMON_FILE_NAME, 'The path(%s) is not a PROF dir,'
                                                            ' Please check the path.' % prof_path)
                continue
            prof_sub_dirs = get_path_dir(prof_path)
            for prof_sub_dir in prof_sub_dirs:
                prof_sub_path = os.path.realpath(
                                os.path.join(prof_path, prof_sub_dir))
                is_cluster_device_or_host_path = self._judge_cluster_device_or_host_path(prof_sub_path)
                if not is_cluster_device_or_host_path:
                    continue
                sqlite_path = os.path.join(prof_sub_path, 'sqlite')
                if not os.path.exists(sqlite_path) or not os.listdir(sqlite_path):
                    _unparsed_prof_sub_dirs.append(prof_sub_path)
            if _unparsed_prof_sub_dirs:
                _unparsed_dirs.setdefault(prof_dir, _unparsed_prof_sub_dirs)
        if not self.cluster_device_paths:
            error(MsProfCommonConstant.COMMON_FILE_NAME, 'None of PROF dir find in the dir(%s), '
                                                         'please check the -dir argument' % self.collection_path)
            raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR)
        return _unparsed_dirs

    def _get_prof_dirs(self: any) -> list:
        self._dir_exception_check(self.collection_path)
        prof_dirs = get_path_dir(self.collection_path)
        for prof_dir in prof_dirs:
            self._dir_exception_check(os.path.join(self.collection_path, prof_dir))
        return prof_dirs

    def _dir_exception_check(self: any, path: str) -> None:
        if DataCheckManager.contain_info_json_data(path):
            error(MsProfCommonConstant.COMMON_FILE_NAME, 'Incorrect parse dir(%s),'
                                             '-dir argument must be cluster data root dir.' % self.collection_path)
            raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR)

    def _judge_cluster_device_or_host_path(self: any, path: str) -> bool:
        if not DataCheckManager.contain_info_json_data(path):
            warn(MsProfCommonConstant.COMMON_FILE_NAME, 'The path(%s) is not a PROF data dir,'
                                                        ' Please check the path.' % path)
            return False
        InfoConfReader().load_info(path)
        if not InfoConfReader().is_host_profiling():
            if InfoConfReader().get_rank_id() == Constant.NA:
                error(MsProfCommonConstant.COMMON_FILE_NAME, "The data is not collected in a clustered environment, "
                                                             "please check the directory: %s" % path)
                raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR)
            self.cluster_device_paths.append(path)
        return True

    def _parse_unparsed_dirs(self: any, unparsed_dirs: list) -> None:
        for pro_dir, result_dir_list in unparsed_dirs.items():
            prof_path = os.path.join(self.collection_path, pro_dir)
            for result_dir in result_dir_list:
                result_path = os.path.realpath(
                    os.path.join(prof_path, result_dir))
                check_path_valid(result_path, False)
                LoadInfoManager.load_info(result_path)
                self.do_import(result_path)

    def _prepare_for_cluster_parse(self:any) -> None:
        cluster_sqlite_path = os.path.join(self.collection_path, 'sqlite')
        if os.path.exists(cluster_sqlite_path):
            shutil.rmtree(cluster_sqlite_path)
        prepare_for_parse(self.collection_path)
