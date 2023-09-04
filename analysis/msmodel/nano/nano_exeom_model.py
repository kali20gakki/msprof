#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
from collections import deque

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.ge.ge_hash_model import GeHashViewModel
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel


class NanoExeomModel(ParserModel):
    """
    nano host data for table:TaskInfo in db:ge_info
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_GE_INFO, [DBNameConstant.TABLE_GE_TASK])

    def flush(self: any, data_list: list, table_name: str = DBNameConstant.TABLE_GE_TASK) -> None:
        # create TaskInfo data
        self.insert_data_to_db(table_name, data_list)


class NanoGraphAddInfoViewModel(ViewModel):
    """
    nano host exeom data view model class
    """

    def __init__(self: any, path: str) -> None:
        super().__init__(path, DBNameConstant.DB_GRAPH_ADD_INFO, [DBNameConstant.TABLE_GRAPH_ADD_INFO])

    def get_nano_model_filename_with_model_id_data(self) -> dict:
        """
        get dbg filename from host data
        """
        sql = "SELECT model_name, graph_id FROM {}".format(DBNameConstant.TABLE_GRAPH_ADD_INFO)
        return dict(DBManager.fetch_all_data(self.cur, sql))

    def get_nano_thread_id_with_model_id_data(self) -> dict:
        """
        get model's thread_id from host data
        """
        sql = "SELECT graph_id, thread_id FROM {}".format(DBNameConstant.TABLE_GRAPH_ADD_INFO)
        return dict(DBManager.fetch_all_data(self.cur, sql))


class NanoExeomViewModel(ViewModel):
    """
    nano host exeom data view model class
    """

    def __init__(self: any, path: str) -> None:
        super().__init__(path, DBNameConstant.DB_GE_INFO, [])

    def get_nano_host_data(self) -> dict:
        """
        get stream_id and block_dim from host data for pmu calculate
        """
        sql = "SELECT model_id, task_id, stream_id, block_dim FROM {}".format(DBNameConstant.TABLE_GE_TASK)
        stream_id_dict = {}
        for data in DBManager.fetch_all_data(self.cur, sql):
            stream_id_dict.setdefault(data[:2], []).append(data[-2:])
        return stream_id_dict
