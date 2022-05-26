# coding=utf-8
"""
data analysis base model
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""
import os
from abc import abstractmethod

from common_func.constant import Constant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_common import is_valid_original_data
from common_func.os_manager import check_file_readable
from common_func.path_manager import PathManager
from common_func.utils import Utils
from framework.offset_calculator import OffsetCalculator
from msparser.interface.iparser import IParser


class DataParser(IParser):
    """
    stars base model class. Used to operate db.
    """

    def __init__(self: any, sample_config: dict) -> None:
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

    @staticmethod
    def read_binary_data(bean_class: any, bean_data: any) -> any:
        """
        read binary data
        :param bean_class:
        :param bean_data:
        :return:
        """
        return bean_class.decode(bean_data)

    @abstractmethod
    def parse(self: any) -> None:
        """
        parse the data under the file path
        :return: NA
        """

    @abstractmethod
    def save(self: any) -> None:
        """
        save the result of data parsing
        :return: NA
        """

    def parse_plaintext_data(self: any, file_list: list, format_func: any) -> list:
        """
        parse plaintext data
        :return:
        """
        result_data = []
        for file_name in file_list:
            file_path = self.get_file_path_and_check(file_name)
            with FileOpen(file_path, "r") as hash_dict:
                data_lines = hash_dict.file_reader.readlines(Constant.MAX_READ_FILE_BYTES)
                result_data.extend(format_func(data_lines))
        return result_data

    def parse_bean_data(self: any, file_list: list, format_size: int, bean_class: any, format_func: any) -> list:
        """
        parse bean file
        :param file_list:
        :param format_size:
        :param bean_class:
        :param format_func:
        :return:
        """
        result_data = []
        _offset_calculator = OffsetCalculator(file_list, format_size, self._project_path)
        for file_name in file_list:
            if not is_valid_original_data(file_name, self._project_path):
                continue
            FileManager.add_complete_file(self._project_path, file_name)
            file_path = self.get_file_path_and_check(file_name)
            with FileOpen(file_path, "rb") as file:
                all_log_bytes = _offset_calculator.pre_process(file.file_reader, os.path.getsize(file_path))
                for bean_data in Utils.chunks(all_log_bytes, format_size):
                    _fusion_bean_data = self.read_binary_data(bean_class, bean_data)
                    result_data.append(format_func(_fusion_bean_data))
        return result_data

    def get_file_path_and_check(self: any, file_name: str) -> str:
        """
        get file path and check it
        """
        file_path = PathManager.get_data_file_path(self._project_path, file_name)
        check_file_readable(file_path)
        return file_path
