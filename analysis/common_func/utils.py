#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.

import json
import logging
import os

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import check_path_valid
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import is_number
from common_func.path_manager import PathManager
from msmodel.interface.base_model import BaseModel


class Utils:
    """
    utils class
    """

    def __init__(self: any) -> any:
        pass

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
        return not Utils.is_step_scene(result_dir)

    @staticmethod
    def is_single_op_graph_mix(result_dir: str) -> bool:
        """
        check the scene of single op graph mix or not
        :param result_dir: project path
        :return: True or False
        """

        _model = BaseModel(result_dir, DBNameConstant.DB_STEP_TRACE, [DBNameConstant.TABLE_STEP_TRACE_DATA])
        if not _model.check_table():
            return False
        sql = "select model_id from {}".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        model_ids = DBManager.fetch_all_data(_model.cur, sql)
        _model.finalize()
        model_id_set = set([model_id[0] for model_id in model_ids])
        if len(model_id_set) == 1:
            return False
        if Constant.GE_OP_MODEL_ID in model_id_set:
            return True
        return False

    @staticmethod
    def get_scene(result_dir: str) -> str:
        """
        get the current scene
        :param result_dir:  project path
        :return: scene
        """
        if Utils.is_single_op_graph_mix(result_dir):
            return Constant.MIX_OP_AND_GRAPH
        if Utils.is_step_scene(result_dir):
            return Constant.STEP_INFO
        return Constant.SINGLE_OP

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

    @staticmethod
    def data_processing_with_decimals(origin_data: list, accuracy=NumberConstant.ROUND_FOUR_DECIMAL):
        """
        read json data from file
        :param origin_data: two-dimensional array
        :param accuracy: preserve decimal
        """
        if not origin_data:
            return origin_data

        processed_data = [0] * len(origin_data)
        for index, data in enumerate(origin_data):
            if not isinstance(data, list) and not isinstance(data, tuple):
                logging.warning("The format of the origin data is invalid, two-dimensional array needed.")
                return origin_data
            processed_data[index] = [Utils.__handle_invalid_zero(_data, accuracy) for _data in data]
        return processed_data

    @staticmethod
    def __handle_invalid_zero(data, accuracy):
        return str(round(float(data), accuracy)).rstrip('0').rstrip('.') if is_number(data) else data

    @classmethod
    def need_all_model_in_one_iter(cls: any, result_dir: str, model_id: int) -> bool:
        """
        check the scene of need export all model in one iter or not
        :param result_dir:
        :param model_id:
        :return:
        """
        if cls.is_single_op_scene(result_dir):
            return True
        if cls.is_single_op_graph_mix(result_dir) and model_id == Constant.GE_OP_MODEL_ID:
            return True
        return False

    @classmethod
    def get_func_type(cls: any, header: int) -> str:
        """
        func type is the lower 6 bits of the first byte, used to distinguish different data types.
        keep 6 lower bits which represent func type
        :param header:
        :return: func_type.For example: 000011
        """
        return bin(header & 63)[2:].zfill(6)

    @classmethod
    def get_cnt(cls: any, header: int) -> str:
        """
        cnt is the 7 - 10 bits of the first byte.
        keep 7 - 10 bits which represent cnt
        :param header:
        :return: func_type.For example: 1111
        """
        return bin(header & 960)[2:].zfill(6)
