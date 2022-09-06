#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

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
from common_func.path_manager import PathManager
from framework.load_info_manager import LoadInfoManager
from msparser.cluster.cluster_info_parser import ClusterInfoParser, ClusterBasicInfo
from msparser.cluster.cluster_step_trace_parser import ClusterStepTraceParser


class ImportCommand:
    """
    The class for handle import command.
    """

    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any, args: any) -> None:
        self.collection_path = os.path.realpath(args.collection_path)
        self.is_cluster_scence = args.cluster_flag
        self.device_cluster_basic_info = {}
        self.device_or_rank_ids = set()
        self.have_rank_id = {}

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
            _unresolved_dirs = self._find_unresolved_dirs()
            if _unresolved_dirs:
                self._parse_unresolved_dirs(_unresolved_dirs)
            self._prepare_for_cluster_parse()
            print_info(MsProfCommonConstant.COMMON_FILE_NAME,
                       'Start parse cluster data in "%s" ...' % self.collection_path)
            cluster_info_parser = ClusterInfoParser(self.collection_path, self.device_cluster_basic_info)
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

    def _find_unresolved_dirs(self: any) -> list:
        prof_dirs = self._get_prof_dirs()
        print_info(MsProfCommonConstant.COMMON_FILE_NAME,
                   'Start verify that all data is parsed in "%s" .' % self.collection_path)
        _unresolved_dirs = {}
        for prof_dir in prof_dirs:
            _unresolved_prof_sub_dirs = []
            _invalid_prof_sub_dirs = []
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
                prof_sub_path_check = self._check_prof_sub_path(prof_sub_path)
                if prof_sub_path_check == Constant.DEFAULT_TURE_VALUE:
                    _unresolved_prof_sub_dirs.append(prof_sub_path)
                if prof_sub_path_check == Constant.DEFAULT_INVALID_VALUE:
                    _invalid_prof_sub_dirs.append(prof_sub_dir)
            if _unresolved_prof_sub_dirs:
                _unresolved_dirs.setdefault(prof_dir, _unresolved_prof_sub_dirs)
            if _invalid_prof_sub_dirs:
                warn(MsProfCommonConstant.COMMON_FILE_NAME, 'There are some invalid PROF dirs in the path(%s)! '
                                                            'please check the dirs: %s' % (
                         prof_path, _invalid_prof_sub_dirs))
        if not self.device_cluster_basic_info:
            error(MsProfCommonConstant.COMMON_FILE_NAME, 'None of device PROF dir find in the dir(%s), '
                                                         'please check the -dir argument' % self.collection_path)
            raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR)
        return _unresolved_dirs

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

    def _check_prof_sub_path(self: any, prof_sub_path: str) -> int:
        if not DataCheckManager.contain_info_json_data(prof_sub_path):
            return Constant.DEFAULT_INVALID_VALUE
        cluster_basic_info = ClusterBasicInfo(prof_sub_path)
        cluster_basic_info.init()
        if not cluster_basic_info.is_host_profiling:
            _have_rank_id = cluster_basic_info.rank_id != Constant.NA
            if self.have_rank_id.setdefault("have_rank_id", _have_rank_id) != _have_rank_id:
                error(MsProfCommonConstant.COMMON_FILE_NAME, 'The dir(%s) contains unqualified '
                                                             'data!' % self.collection_path)
                raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR)
            _device_or_rank_id = int(cluster_basic_info.rank_id) if _have_rank_id else int(cluster_basic_info.device_id)
            if _device_or_rank_id in self.device_or_rank_ids:
                error(MsProfCommonConstant.COMMON_FILE_NAME, 'There are same rank_id or device_id ( %s ) in '
                                                             'the dir(%s). Please check the PROF dirs!' % (
                          _device_or_rank_id, self.collection_path))
                raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR)
            self.device_or_rank_ids.add(_device_or_rank_id)
            dir_name = os.sep.join(prof_sub_path.split(os.sep)[-2:])
            self.device_cluster_basic_info[dir_name] = cluster_basic_info
        sqlite_path = PathManager.get_sql_dir(prof_sub_path)
        if not os.path.exists(sqlite_path) or not os.listdir(sqlite_path):
            return Constant.DEFAULT_TURE_VALUE
        return Constant.DEFAULT_FALSE_VALUE

    def _parse_unresolved_dirs(self: any, unresolved_dirs: list) -> None:
        print_info(MsProfCommonConstant.COMMON_FILE_NAME,
                   'Start parse unresolved dirs in "%s" ...' % self.collection_path)
        for pro_dir, result_dir_list in unresolved_dirs.items():
            prof_path = os.path.join(self.collection_path, pro_dir)
            for result_dir in result_dir_list:
                result_path = os.path.realpath(
                    os.path.join(prof_path, result_dir))
                check_path_valid(result_path, False)
                LoadInfoManager.load_info(result_path)
                self.do_import(result_path)
        print_info(MsProfCommonConstant.COMMON_FILE_NAME,
                   'Unresolved dirs parse finished!')

    def _prepare_for_cluster_parse(self: any) -> None:
        cluster_sqlite_path = PathManager.get_sql_dir(self.collection_path)
        if os.path.exists(cluster_sqlite_path):
            shutil.rmtree(cluster_sqlite_path)
        prepare_for_parse(self.collection_path)
