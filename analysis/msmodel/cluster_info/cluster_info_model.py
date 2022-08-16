#!/usr/bin/python3
# coding=utf-8
"""
function: save cluster rank_id data to db
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
from itertools import chain

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel


class ClusterInfoModel(ParserModel):
    """
    class used to operate cluster db
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_CLUSTER_RANK, [DBNameConstant.TABLE_CLUSTER_RANK])

    def flush(self: any, data_list: list) -> None:
        """
        flush data to db
        :param data_list:
        :return:
        """
        self.insert_data_to_db(DBNameConstant.TABLE_CLUSTER_RANK, data_list)


class ClusterInfoViewModel(ViewModel):
    def __init__(self: any, params: dict) -> any:
        self._collection_path = params["collection_path"]
        super().__init__(self._collection_path, DBNameConstant.DB_CLUSTER_RANK, [DBNameConstant.TABLE_CLUSTER_RANK])

    def get_all_rank_id(self: any) -> set:
        sql = f"select distinct rank_id from {DBNameConstant.TABLE_CLUSTER_RANK}"
        return set(chain.from_iterable(DBManager.fetch_all_data(self.cur, sql)))
