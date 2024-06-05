#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
from collections import namedtuple
from dataclasses import dataclass

from common_func.db_name_constant import DBNameConstant
from common_func.msprof_object import CustomizedNamedtupleFactory
from common_func.db_manager import DBManager
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel


class KfcInfoModel(ParserModel):
    """
    kfc info model class
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


class KfcInfoViewModel(ViewModel):
    KFC_HCCL_INFO_TYPE = CustomizedNamedtupleFactory.enhance_namedtuple(
        namedtuple("KfcInfo",
                   ["timestamp", "op_name", "ccl_tag", "group_name", "local_rank", "remote_rank",
                    "rank_size", "work_flow_mode", "plane_id", "context_id", "notify_id", "stage", "role",
                    "duration_estimated", "src_addr", "dst_addr", "size", "op_type", "data_type",
                    "link_type", "transport_type", "rdma_type", "stream_id", "task_id", "batch_id",
                    "start_time", "duration", "bandwidth"]),
        {})
    KFC_COMM_TURN_TYPE = CustomizedNamedtupleFactory.enhance_namedtuple(
        namedtuple("KfcCommTurn",
                   ["device_id", "stream_id", "task_id", "comm_turn", "current_turn", "wait_notify_start_time",
                    "kfc_alg_exe_start_time", "send_task_start_time", "wait_active_start_time",
                    "active_start_time", "wait_exe_end_start_time", "rtsq_exe_end_time"]),
        {})
    KFC_COMPUTE_TURN_TYPE = CustomizedNamedtupleFactory.enhance_namedtuple(
        namedtuple("KfcComputeTurn",
                   ["device_id", "stream_id", "task_id", "compute_turn", "current_turn", "wait_compute_start_time",
                    "compute_start_time", "compute_exe_end_time"]),
        {})

    def __init__(self, result_dir: str, table_list: list):
        super().__init__(result_dir, DBNameConstant.DB_KFC_INFO, table_list)

    def get_kfc_info_data(self: any) -> list:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_KFC_INFO):
            return []
        sql = "select *, 0 as batch_id, 0 as start_time, 0 as duration, 0 as bandwidth " \
              "from {}".format(DBNameConstant.TABLE_KFC_INFO)
        kfc_info_data = self.get_sql_data(sql)
        return [self.KFC_HCCL_INFO_TYPE(*data) for data in kfc_info_data]

    def get_kfc_comm_turn_data(self: any) -> list:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_KFC_COMM_TURN):
            return []
        sql = "select * " \
              "from {}".format(DBNameConstant.TABLE_KFC_COMM_TURN)
        kfc_info_data = self.get_sql_data(sql)
        return [self.KFC_COMM_TURN_TYPE(*data) for data in kfc_info_data]

    def get_kfc_compute_turn_data(self: any) -> list:
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_KFC_COMPUTE_TURN):
            return []
        sql = "select * " \
              "from {}".format(DBNameConstant.TABLE_KFC_COMPUTE_TURN)
        kfc_info_data = self.get_sql_data(sql)
        return [self.KFC_COMPUTE_TURN_TYPE(*data) for data in kfc_info_data]


@dataclass
class KfcTurnData:
    op_name: str
    stream_id: int
    task_id: int
    start_time: str
    duration: float
