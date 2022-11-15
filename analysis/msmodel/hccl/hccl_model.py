#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from common_func.db_manager import DBManager
from common_func.db_manager import ClassRowType
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel
from profiling_bean.db_dto.hccl_dto import HcclDto


class HCCLModel(ParserModel):
    """
    acsq task model class
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_HCCL, table_list)

    def flush(self: any, data_list: list) -> None:
        """
        insert data to table
        :param data_list: hccl data
        :return:
        """
        self.insert_data_to_db(DBNameConstant.TABLE_HCCL_ALL_REDUCE, data_list)

    def get_hccl_data(self: any) -> list:
        """
        get hccl data
        :return:
        """
        sql = "select * from {}".format(DBNameConstant.TABLE_HCCL_ALL_REDUCE)
        data = DBManager.fetch_all_data(self.cur, sql, dto_class=HcclDto)
        return data
