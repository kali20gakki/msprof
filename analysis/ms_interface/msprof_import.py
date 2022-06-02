#!/usr/bin/python3
# coding:utf-8
"""
This scripts is used to parse some project path profiling data
Copyright Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
"""

import os

from common_func.common import warn
from common_func.config_mgr import ConfigMgr
from common_func.data_check_manager import DataCheckManager
from common_func.msprof_common import analyze_collect_data
from common_func.msprof_common import check_collection_dir
from common_func.msprof_common import MsProfCommonConstant
from common_func.msprof_common import check_path_valid
from common_func.msprof_common import get_path_dir
from common_func.msprof_exception import ProfException
from framework.load_info_manager import LoadInfoManager


class ImportCommand:
    """
    The class for handle import command.
    """

    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any, collection_path: str) -> None:
        self.collection_path = os.path.realpath(collection_path)

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

    def cluster_tuning(self: any, collect_path: str) -> None:
        pass

    def process(self: any) -> None:
        """
        command import command entry
        :return: None
        """
        check_path_valid(self.collection_path, False)
        if DataCheckManager.contain_info_json_data(self.collection_path):  # find profiling data dir
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
                warn(self.FILE_NAME, 'Invalid parsing dir("%s"), -dir must be profiling data dir '
                                     'such as PROF_XXX_XXX_XXX' % collect_path)
            else:
                self._process_sub_dirs(sub_dir, is_cluster=True)
