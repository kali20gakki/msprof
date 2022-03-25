#!/usr/bin/python3
# coding=utf-8
"""
Copyright Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.
"""
import json
import logging
import os

from common_func.ai_stack_data_check_manager import AiStackDataCheckManager
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import check_path_valid
from common_func.path_manager import PathManager


class Utils:
    """
    utils class
    """

    def __init__(self: any) -> any:
        pass

    @staticmethod
    def is_training_trace_scene(result_dir: str) -> bool:
        """
        check the scene of training trace or not
        :param result_dir: project path
        :return: True or False
        """
        return AiStackDataCheckManager.contain_training_trace_data(result_dir)

    @staticmethod
    def is_step_scene(result_dir: str) -> bool:
        """
        check the scene of step info or not
        :param result_dir: project path
        :return: True or False
        """
        db_path = PathManager.get_db_path(result_dir, DBNameConstant.DB_STEP_TRACE)
        return DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_STEP_TRACE_DATA)

    @staticmethod
    def is_single_op_scene(result_dir: str) -> bool:
        """
        check the scene of single_op or not
        :param result_dir: project path
        :return: True or False
        """
        return not Utils.is_step_scene(result_dir) and not Utils.is_training_trace_scene(result_dir)

    @staticmethod
    def get_scene(result_dir: str) -> str:
        """
        get the current scene
        :param result_dir:  project path
        :return: scene
        """
        if Utils.is_step_scene(result_dir):
            return Constant.STEP_INFO
        if Utils.is_training_trace_scene(result_dir):
            return Constant.TRAIN
        return Constant.SINGLE_OP

    @classmethod
    def get_func_type(cls: any, header: int) -> str:
        """
        func type is the lower 6 bits of the first byte, used to distinguish different data types.
        63 is 0b00111111 keep 6 lower bits which represent func type
        :param header:
        :return: func_type.For example: 000011
        """
        return bin(header & 63)[2:].zfill(6)

    @classmethod
    def get_cnt(cls: any, header: int) -> str:
        """
        cnt is the 7 - 10 bits of the first byte.
        960 is 0b001111000000 keep 7 - 10 bits which represent cnt
        :param header:
        :return: func_type.For example: 1111
        """
        return bin(header & 960)[2:].zfill(6)

    @staticmethod
    def generator_to_list(gen: any) -> list:
        """
        convert generator to list
        :param gen : generator
        :return:
        """
        result = []
        try:
            for data in gen:
                result.append(data)
            return result
        except(TypeError,) as err:
            logging.error("Failed to convert generator to list. %s", err, exc_info=Constant.TRACE_BACK_SWITCH)
            return result
        finally:
            pass

    @staticmethod
    def chunks(all_log_bytes: bytes, struct_size: int) -> any:
        """
        Yield successive n-sized chunks from all_log_bytes.
        :param all_log_bytes:
        :param struct_size:
        :return: generator
        """
        for i in range(0, len(all_log_bytes), struct_size):
            yield all_log_bytes[i:i + struct_size]

    @staticmethod
    def obj_list_to_list(data_list: any) -> list:
        """
        :return:
        """
        res = []
        for data in data_list:
            temp = []
            for value in data.__dict__.values():
                temp.append(value)
            res.append(temp)
        return res

    @staticmethod
    def dict_copy(ori_data: dict) -> dict:
        """
        ori_data copy
        """
        copy_dict = {}
        for key, val in ori_data.items():
            copy_dict[key] = val
        return copy_dict

    @staticmethod
    def write_json_files(json_data: list, file_path: str) -> None:
        """
        write json data  to file
        :param json_data:
        :param file_path:
        :return:
        """
        try:
            with os.fdopen(os.open(file_path, Constant.WRITE_FLAGS,
                                   Constant.WRITE_MODES), 'w') as trace_file:
                trace_file.write(json.dumps(json_data))
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(os.path.basename(__file__), err)
        finally:
            pass

    @staticmethod
    def get_json_data(info_json_path: str) -> list:
        """
        read json data from file
        :param info_json_path:info json path
        :return:
        """
        if not info_json_path or not os.path.exists(info_json_path) or not os.path.isfile(
                info_json_path):
            return []
        check_path_valid(info_json_path, is_file=True)
        if os.path.getsize(info_json_path) > 1024 * 1024 * 1024:
            return []
        with open(info_json_path, "r") as json_reader:
            json_data = json_reader.read()
            json_data = json.loads(json_data)
            return json_data
