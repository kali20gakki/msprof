#!/usr/bin/python3
# coding:utf-8
"""
This script is used to provide function to manage files, including check validity of files
Copyright Huawei Technologies Co., Ltd. 2020. All rights reserved.
"""
import json
import os
import re

from common_func.common import CommonConstant
from common_func.common import error
from common_func.constant import Constant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from common_func.file_name_manager import FileNameManagerConstant
from common_func.empty_class import EmptyClass
from common_func.return_code_checker import ReturnCodeCheck


class FileManager:
    """
    class to manage files
    """
    FILE_AUTHORITY = 0o640

    @staticmethod
    def macth_process(pattern: str, file_name: str) -> any:
        """
        match files
        :param pattern: check pattern
        :param file_name: check files
        :return:
        """
        matched_name = re.match(pattern, file_name)
        if matched_name:
            return matched_name
        return EmptyClass("empty match name")

    @classmethod
    def is_info_json_file(cls: any, file_name: str) -> any:
        """
        check whether file match info.json pattern
        :param file_name: check files
        :return: matched groups
        """
        return cls.macth_process(CommonConstant.INFO_JSON_PATTERN, file_name)

    @classmethod
    def is_valid_jobid(cls: any, check_string: str) -> any:
        """
        check whether job id matches JOB_ID_PATTERN
        :param check_string: check strings
        :return: matched groups
        """
        return cls.macth_process(CommonConstant.JOB_ID_PATTERN, check_string)

    @classmethod
    def is_analyzed_data(cls: any, project_path: str) -> bool:
        """
        return if data dir contains all parse complete file
        """
        data_dir = PathManager.get_data_dir(project_path)
        is_analyse = False
        if os.path.exists(data_dir):
            all_file_name = os.path.join(data_dir, FileNameManagerConstant.ALL_FILE_TAG)
            if os.path.exists(all_file_name):
                is_analyse = True
        return is_analyse

    @classmethod
    def add_complete_file(cls: any, project_path: str, file_name: str) -> None:
        """
        add complete file
        """
        file_path = PathManager.get_data_dir(project_path)
        try:
            if os.path.exists(os.path.join(file_path, file_name)) and \
                    not file_name.endswith(Constant.COMPLETE_TAG):
                with os.fdopen(os.open(os.path.join(file_path, file_name + Constant.COMPLETE_TAG), Constant.WRITE_FLAGS,
                                       Constant.WRITE_MODES), "w"):
                    os.chmod(os.path.join(file_path, file_name), cls.FILE_AUTHORITY)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            error(os.path.basename(__file__), err)

    @staticmethod
    def remove_file(file_path: str) -> bool:
        """
        remove file
        """
        try:
            if os.path.isfile(file_path):
                os.remove(file_path)
                return True
            return False
        except OSError as err:
            error(os.path.basename(__file__), str(err))
            return False
        finally:
            pass


class FileOpen:
    """
    open and read file
    """

    def __init__(self: any, file_path: str, mode: str = "r") -> None:
        self.file_path = file_path
        self.file_reader = None
        self.mode = mode

    def __enter__(self: any) -> any:
        check_path_valid(self.file_path, True)
        self.file_reader = open(self.file_path, self.mode)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.file_reader.close()


def check_path_valid(path: str, is_file: bool) -> None:
    """
    check the path is valid or not
    :param path: file path
    :param is_file: file or not
    :return: None
    """
    if path == "":
        ReturnCodeCheck.print_and_return_status(
            json.dumps({'status': NumberConstant.ERROR, 'info': 'The path is empty. '
                                                                'Please enter a valid path.'}))
    if not os.path.exists(path):
        ReturnCodeCheck.print_and_return_status(
            json.dumps({'status': NumberConstant.ERROR,
                        'info': "The path '%s' does not exist. Please check "
                                'that the path exists.' % path}))
    if is_file:
        if not os.path.isfile(path):
            ReturnCodeCheck.print_and_return_status(json.dumps(
                {'status': NumberConstant.ERROR,
                 'info': "The path '%s' is not a file. Please check the "
                         'path.' % path}))
    else:
        if not os.path.isdir(path):
            ReturnCodeCheck.print_and_return_status(json.dumps(
                {'status': NumberConstant.ERROR,
                 'info': "The path '%s' is not a directory. Please check the "
                         'path.' % path}))
    if not os.access(path, os.R_OK):
        ReturnCodeCheck.print_and_return_status(json.dumps(
            {'status': NumberConstant.ERROR,
             'info': "The path '%s' does not have permission to read. Please "
                     'check that the path is readable.' % path}))
