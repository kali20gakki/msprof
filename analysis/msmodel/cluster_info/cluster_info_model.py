#!/usr/bin/python3
# coding=utf-8
"""
function: save cluster rank_id data to db
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel


class ClusterInfoModel(ParserModel):
    """
    class used to operate cluster db
    """
    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CLUSTER, [DBNameConstant.TABLE_CLUSTER_RANK])

    def flush(self: any, data_list: list) -> None:
        """
        flush data to db
        :param data_list:
        :return:
        """
        self.insert_data_to_db(DBNameConstant.TABLE_CLUSTER_RANK, data_list)