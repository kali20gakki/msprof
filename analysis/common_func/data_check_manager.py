#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.

import os

from common_func import file_name_manager
from common_func.constant import Constant
from common_func.os_manager import check_dir_readable
from common_func.path_manager import PathManager


class DataCheckManager:
    """
    The data check manager
    """

    DATA_NAME_LENGTH = 4
    HOST_DATA_NAME_LENGTH = 3
    DEVICE_ID_INDEX = 2

    @classmethod
    def check_data_exist(cls: any, result_dir: str, file_patterns: any, device_id: any = None) -> bool:
        """
        check data path readable
        """
        data_path = PathManager.get_data_dir(result_dir)
        check_dir_readable(data_path)
        for file_name in os.listdir(data_path):
            for file_pattern_compile in file_patterns:
                if not file_pattern_compile.match(file_name):
                    continue
                if not cls._check_file_size(file_name, data_path):
                    continue
                if cls._check_device_id_and_file_name(device_id, file_name):
                    return True
        return False

    @classmethod
    def contain_info_json_data(cls: any, result_dir: str, device_info_only: bool = False) -> bool:
        """
        The data path contain job info data or not
        """
        if not os.path.isdir(result_dir):
            return False
        info_json_compiles = file_name_manager.get_info_json_for_device_compiles() if device_info_only else \
            file_name_manager.get_info_json_compiles()
        for file_name in os.listdir(result_dir):
            for info_json_compile in info_json_compiles:
                if info_json_compile.match(file_name):
                    return True
        return False

    @classmethod
    def _check_file_size(cls: any, file_name: str, data_dir: str) -> bool:
        if file_name.endswith(Constant.DONE_TAG) or file_name.endswith(Constant.COMPLETE_TAG):
            return False
        _file_path = os.path.realpath(os.path.join(data_dir, file_name))
        file_size = os.path.getsize(_file_path)
        return file_size > 0

    @classmethod
    def _check_device_id_and_file_name(cls: any, device_id: any, file_name: str) -> bool:
        if device_id is None:
            return True
        split_list = file_name.split('.')
        if len(split_list) == cls.HOST_DATA_NAME_LENGTH:
            return True
        if len(split_list) == cls.DATA_NAME_LENGTH and \
                split_list[cls.DEVICE_ID_INDEX] == device_id:
            return True
        return False
