#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to operate AI_CPU
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel


class AiCpuModel(ParserModel):
    """
    ai_cpu model class
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_AI_CPU, table_list)

    def flush(self: any, data_list: list, ai_cpu_table_name: str = DBNameConstant.TABLE_AI_CPU) -> None:
        """
        insert data to table
        :param data_list: ai_cpu data
        :param ai_cpu_table_name: ai cpu table name
        :return:
        """
        self.insert_data_to_db(ai_cpu_table_name, data_list)
