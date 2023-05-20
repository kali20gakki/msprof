#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.

import logging
import os
import re

from functools import partial
from common_func.common import init_log
from common_func.common import check_free_memory
from common_func.common import error
from common_func.common import warn
from common_func.common import print_info
from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_exception import ProfException
from common_func.msvp_common import files_chmod
from common_func.ms_constant.number_constant import NumberConstant
from common_func.os_manager import check_dir_readable
from common_func.os_manager import check_dir_writable
from common_func.os_manager import check_file_writable
from common_func.path_manager import PathManager
from common_func.file_manager import FileManager
from common_func.file_name_manager import FileNameManagerConstant
from framework.collection_engine import AI
from framework.file_dispatch import FileDispatch


class MsProfCommonConstant:
    """
    msprof common constant
    """
    DEFAULT_IP = '127.0.0.1'
    SUMMARY = "summary"
    TIMELINE = "timeline"
    DEFAULT_JOB = 'job_default'
    COMMON_FILE_NAME = os.path.basename(__file__)

    # key for the query data
    JOB_INFO = "job_info"
    DEVICE_ID = "device_id"
    JOB_NAME = "job_name"
    COLLECTION_TIME = "collection_time"
    PARSED = "parsed"
    MODEL_ID = "model_id"
    ITERATION_ID = "iteration_id"
    TOP_TIME_ITERATION = "top_time_iteration"
    RANK_ID = "rank_id"
    INDEX_ID = "index_id"
    TAG = "tag"

    def get_msprof_common_class_name(self: any) -> any:
        """
        get msprof common class name
        """
        return self.__class__.__name__

    def get_msprof_common_class_member(self: any) -> any:
        """
        get msprof common class member num
        """
        return self.__dict__


def update_sample_json(sample_config: dict, collect_path: str) -> None:
    """
    update sample config
    :param sample_config: raw sample json
    :param collect_path:
    :return:
    """
    sample_config["result_dir"] = collect_path
    sample_config["tag_id"] = os.path.basename(collect_path)
    sample_config["host_id"] = MsProfCommonConstant.DEFAULT_IP
    device_list = InfoConfReader().get_device_list()
    if not device_list or not device_list[0].isdigit():
        if InfoConfReader().is_host_profiling():
            logging.info("No device info, may be no device task has run.")
        else:
            logging.error("Get device id failed, maybe data is incomplete, "
                          "please check the info.json under the directory: %s",
                          sample_config["tag_id"])
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)
    else:
        sample_config["device_id"] = device_list[0]


def check_path_valid(path: str, is_output: bool) -> None:
    """
    check path valid
    :param path: the path to check
    :param is_output: the path is output
    :return: None
    """
    if path == "":
        raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR,
                            "The path is empty. Please enter a valid path.")
    try:
        if is_output and not os.path.exists(path):
            os.makedirs(path, mode=NumberConstant.DIR_AUTHORITY)
            os.chmod(path, NumberConstant.DIR_AUTHORITY)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as ex:
        message = f"Failed to create \"{path}\". " \
                  f"Please check that the path is accessible or the disk space is enough. {str(ex)} "
        raise ProfException(ProfException.PROF_INVALID_PATH_ERROR, message) from ex
    finally:
        pass
    check_dir_writable(path)


def prepare_for_parse(output_path: str) -> None:
    """
    create data and corresponding directories
    """
    check_path_valid(PathManager.get_sql_dir(output_path), True)
    prepare_log(output_path)


def prepare_log(output_path: str) -> None:
    """
    create data and corresponding directories
    """
    check_path_valid(PathManager.get_log_dir(output_path), True)
    log_path = PathManager.get_collection_log_path(output_path)
    check_file_writable(log_path)
    init_log(output_path)


def analyze_collect_data(collect_path: str, sample_config: dict) -> None:
    """
    analyze collection data
    :param collect_path: the collect path
    :param sample_config: the sample config
    """
    if not check_collection_dir(collect_path):
        return
    prepare_for_parse(collect_path)
    print_info(MsProfCommonConstant.COMMON_FILE_NAME,
               'Start analyzing data in "%s" ...' % collect_path)
    print_info(MsProfCommonConstant.COMMON_FILE_NAME,
               "It may take few minutes, please be patient ...")
    update_sample_json(sample_config, collect_path)
    parser = AI(sample_config)
    parser.project_preparation(collect_path)
    parser.import_control_flow()
    file_dispatch = FileDispatch(sample_config)
    file_dispatch.dispatch_parser()
    files_chmod(collect_path)
    add_all_file_complete(collect_path)
    print_info(MsProfCommonConstant.COMMON_FILE_NAME,
               'Analysis data in "%s" finished.' % collect_path)


def add_all_file_complete(collect_path: str) -> None:
    """
    add all file complete when parse finished
    :param collect_path: the collect path
    """
    file_path = PathManager.get_data_dir(collect_path)
    if not os.path.exists(file_path):
        logging.error("No data dir found, add all complete file error")
        return
    try:
        with os.fdopen(os.open(os.path.join(file_path, FileNameManagerConstant.ALL_FILE_TAG), Constant.WRITE_FLAGS,
                               Constant.WRITE_MODES), "w"):
            os.chmod(os.path.join(file_path, FileNameManagerConstant.ALL_FILE_TAG), FileManager.FILE_AUTHORITY)
    except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
        error(os.path.basename(__file__), err)
    finally:
        pass


def check_collection_dir(collect_path: str) -> bool:
    """
    check whether the file is valid.
    :param collect_path: the collect path
    """
    if not os.path.exists(PathManager.get_data_dir(collect_path)):
        message = f"There is no \"data\" directory in \"{collect_path}\". Collect data failed. " \
                  f"More info could be found in the path of slog on your core."
        raise ProfException(ProfException.PROF_INVALID_EXECUTE_CMD_ERROR, message, warn)
    check_dir_writable(collect_path)
    check_free_memory(collect_path)
    file_all = os.listdir(PathManager.get_data_dir(collect_path))
    if not file_all:
        message = f"There is no file in {PathManager.get_data_dir(collect_path)}. " \
                  f"Collect data failed. More info could be found in the path of slog on your core."
        warn(MsProfCommonConstant.COMMON_FILE_NAME, message)
        return False
    if not InfoConfReader().is_version_matched():
        warn(MsProfCommonConstant.COMMON_FILE_NAME, f'The version package of data collection '
                                                    f'does not match the package of data analyzing, '
                                                    f'and some data may not be analyzed.')
        return True
    return True


def get_info_by_key(path: str, key: any) -> str:
    """
    get the value of key in info.json.dev_id
    :param path: the info.json.dev_id dir
    :param key: the key
    :return: the value of key
    """
    check_dir_readable(path)
    for file_name in os.listdir(path):
        if not re.match(InfoConfReader.INFO_PATTERN, file_name):
            continue
        return InfoConfReader().get_root_data(key)


def get_path_dir(path: str) -> list:
    """
    check result path exist JOB dir
    path : result path
    """
    path_dir_filter = filter(partial(_path_dir_filter_func, root_dir=path), os.listdir(path))
    sub_dirs = list(path_dir_filter)
    if not sub_dirs:
        message = f"The path \"{path}\" does not have PROF dir. Please check the path."
        raise ProfException(ProfException.PROF_INVALID_PATH_ERROR, message)
    return sub_dirs


def _path_dir_filter_func(sub_path, root_dir):
    return sub_path not in Constant.FILTER_DIRS and os.path.isdir(os.path.realpath(os.path.join(root_dir, sub_path)))
