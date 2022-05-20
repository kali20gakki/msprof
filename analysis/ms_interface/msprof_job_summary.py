#!/usr/bin/python3
# coding=utf-8
"""
This script is used to export all job summary data
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""
import logging
import os

from common_func.common import print_info
from common_func.constant import Constant
from common_func.data_check_manager import DataCheckManager
from common_func.file_manager import check_path_valid
from common_func.file_name_manager import get_file_name_pattern_match
from common_func.file_name_manager import get_msprof_json_compiles
from common_func.msprof_common import MsProfCommonConstant
from common_func.msprof_common import get_path_dir
from common_func.utils import Utils
from ms_interface.msprof_data_storage import MsprofDataStorage


class MsprofJobSummary:
    """
    class used to export all job data.
    """
    MSPROF_HOST_DIR = "host"
    MSPROF_TIMELINE_DIR = "timeline"

    def __init__(self: any, output: str) -> None:
        self._output = output
        self._host_data = []
        self._file_name = "msprof.json"
        self.param = {'project': self._output, 'data_type': 'msprof'}

    def export(self: any) -> None:
        """
        export tx summary data
        :return:
        """
        self._export_msprof_timeline()

    def get_msprof_json_file(self: any, sub_dir: str) -> list:
        """
        get msprof json file from sub dir
        :param sub_dir:
        :return:
        """
        msprof_json_data = []
        timeline_path = os.path.realpath(
            os.path.join(self._output, sub_dir, self.MSPROF_TIMELINE_DIR))
        if not os.path.exists(timeline_path):
            return msprof_json_data
        file_list = os.listdir(timeline_path)
        for file_name in file_list:
            json_result = get_file_name_pattern_match(file_name, *(get_msprof_json_compiles()))
            if json_result:
                msprof_json_data.extend(Utils.get_json_data(os.path.join(timeline_path, file_name)))
        return msprof_json_data

    def _export_msprof_timeline(self: any) -> None:
        self._host_data = self._get_host_timeline_data()
        if not self._host_data:
            logging.info("Host data not exists, won't export summary timeline data")
            return
        self._export_all_device_data()

    def _export_all_device_data(self: any) -> None:
        sub_dirs = get_path_dir(self._output)
        for sub_dir in sub_dirs:  # result_dir
            if sub_dir != self.MSPROF_HOST_DIR:
                sub_path = os.path.realpath(os.path.join(self._output, sub_dir))
                check_path_valid(sub_path, False)
                if DataCheckManager.contain_info_json_data(sub_path):
                    self._generate_json_file(sub_path)

    def _get_host_timeline_data(self: any) -> list:
        host_dir = os.path.join(self._output, self.MSPROF_HOST_DIR)
        return self.get_msprof_json_file(host_dir)

    def _generate_json_file(self: any, sub_path: str) -> None:
        json_data = self.get_msprof_json_file(sub_path)
        if json_data:
            json_data.extend(self._host_data)
            timeline_dir = os.path.join(self._output, self.MSPROF_TIMELINE_DIR)
            if not os.path.exists(timeline_dir):
                os.makedirs(timeline_dir, Constant.FOLDER_MASK)
            print_info(MsProfCommonConstant.COMMON_FILE_NAME,
                       "Start to export msprof timeline data ...")
            slice_result = MsprofDataStorage().slice_data_list(json_data)
            error_code, result_message = MsprofDataStorage.write_json_files(slice_result, self.param)
            if error_code:
                print_info(MsProfCommonConstant.COMMON_FILE_NAME, "message error: %s" % result_message)
            else:
                print_info(MsProfCommonConstant.COMMON_FILE_NAME,
                           'Export timeline json file success, "%s" ...' % result_message)
