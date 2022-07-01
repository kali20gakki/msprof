#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to operate HCCL
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel


class GeModelTimeModel(ParserModel):
    """
    acsq task model class
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_GE_MODEL_TIME, table_list)

    def flush(self: any, data_list: list) -> None:
        """
        insert data to table
        :param data_list: ge fusion data
        :return:
        """
        self.insert_data_to_db(DBNameConstant.TABLE_GE_MODEL_TIME, data_list)

    def get_ge_model_time_model_name(self: any) -> any:
        """
        get ge model time model name
        """
        return self.__class__.__name__
