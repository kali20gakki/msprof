#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel


class KfcInfoModel(ParserModel):
    """
    kfc information model class
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_KFC_INFO, table_list)

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_KFC_INFO) -> None:
        """
        insert data to table
        :param data_list: hccl information data
        :param table_name: table name
        :return:
        """
        self.insert_data_to_db(table_name, data_list)
