#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import logging

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.profiling_scene import ProfilingScene
from mscalculate.hccl.hccl_task import HcclOps
from mscalculate.hccl.hccl_task import HcclTask
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.hccl_dto import HcclDto
from profiling_bean.db_dto.time_section_dto import CommunicationTimeSection


class HCCLModel(ParserModel):
    """
    acsq task model class
    """

    def __init__(self: any, result_dir: str, table_list: list) -> None:
        super().__init__(result_dir, DBNameConstant.DB_HCCL_SINGLE_DEVICE, table_list)

    def flush(self: any, data_list: list) -> None:
        """
        insert data to table
        :param data_list: hccl data
        :return:
        """
        self.insert_data_to_db(DBNameConstant.TABLE_HCCL_SINGLE_DEVICE, data_list)

    def get_hccl_data(self: any) -> list:
        """
        get hccl data
        :return:
        """
        sql = "select * from {}".format(DBNameConstant.TABLE_HCCL_SINGLE_DEVICE)
        data = DBManager.fetch_all_data(self.cur, sql, dto_class=HcclDto)
        return data


class HcclViewModel(ViewModel):
    def __init__(self, result_dir: str, db_name: str, table_list: list):
        super().__init__(result_dir, db_name, table_list)

    @classmethod
    def get_task_time_sql(cls):
        select_sql = "(select {0}.model_id, {0}.index_id, {0}.stream_id, {0}.task_id, " \
                     "{0}.batch_id, {0}.context_id, {0}.start_time as running, " \
                     "{0}.start_time + {0}.duration as complete from {0} )".format(DBNameConstant.TABLE_ASCEND_TASK)
        return select_sql

    def rebuild_hccl_table(self):
        self.create_table_by_name(DBNameConstant.TABLE_HCCL_SINGLE_DEVICE)

    def rebuild_hccl_op_report_table(self):
        self.create_table_by_name(DBNameConstant.TABLE_HCCL_OP_REPORT)

    def get_hccl_op_data(self):
        """
        get the real execution of the communication op
        """
        sql = f"select model_id, index_id, op_name, min(timestamp) as timestamp, " \
              f"max(timestamp + duration) - min(timestamp) as duration, task_type, op_type, connection_id " \
              f"from {DBNameConstant.TABLE_HCCL_SINGLE_DEVICE} " \
              f"group by op_name, first_timestamp"
        return DBManager.fetch_all_data(self.cur, sql, dto_class=HcclDto)

    def get_hccl_task_data(self):
        if not self.attach_to_db(DBNameConstant.DB_ASCEND_TASK):
            logging.error("Attach to db %s failed, task data not found.", DBNameConstant.DB_ASCEND_TASK)
            return []

        if not self.attach_to_db(DBNameConstant.DB_HCCL):
            logging.error("Attach to db %s failed, task data not found.", DBNameConstant.DB_HCCL)
            return []

        device_id = InfoConfReader().get_device_id()
        if device_id == Constant.NA:
            logging.error("No device id found.")
            return []

        sql = "SELECT a.model_id as model_id, a.index_id as index_id, a.name as hccl_name, a.plane_id as plane_id, " \
              "a.args as args, a.timestamp as host_timestamp, " \
              "a.group_name as group_name, b.start_time as timestamp, a.is_master as is_master, " \
              "b.connection_id as connection_id, b.duration as duration from {0} as a inner join " \
              "{1} as b on " \
              "a.stream_id = b.stream_id " \
              "and a.task_id = b.task_id " \
              "and a.batch_id = b.batch_id " \
              "and a.context_id = b.context_id " \
              "and a.device_id = {device_id} " \
              "and b.start_time != {invalid_start} " \
              "order by host_timestamp" \
            .format(DBNameConstant.TABLE_HCCL_TASK, DBNameConstant.TABLE_ASCEND_TASK, device_id=device_id,
                    invalid_start=NumberConstant.INVALID_TASK_TIME)
        return DBManager.fetch_all_data(self.cur, sql, dto_class=HcclTask)

    def get_hccl_ops(self, model_id: int, index_id: int):
        device_id = InfoConfReader().get_device_id()
        if device_id == Constant.NA:
            logging.error("No device id found.")
            return []

        where_condition = ""
        if not ProfilingScene().is_all_export():
            where_condition = f'and model_id={model_id} and (index_id={index_id} or index_id=0)'

        sql = "SELECT model_id, index_id, op_name, task_type, op_type, connection_id, begin as timestamp, " \
              "end - begin as duration, is_dynamic from {0} " \
              "WHERE device_id = {device_id} " \
              "{where_condition} " \
              "order by timestamp" \
            .format(DBNameConstant.TABLE_HCCL_OP, device_id=device_id, where_condition=where_condition)
        return DBManager.fetch_all_data(self.cur, sql, dto_class=HcclOps)

    def get_hccl_op_data_by_group(self):
        """
        get the real execution of the communication op
        """
        sql = f"select model_id, index_id, op_name, group_name, min(timestamp) as timestamp, " \
              f"max(timestamp + duration) - min(timestamp) as duration, task_type, op_type, connection_id " \
              f"from {DBNameConstant.TABLE_HCCL_SINGLE_DEVICE} " \
              f"WHERE is_master = 1 " \
              f"group by op_name, first_timestamp, group_name"
        return DBManager.fetch_all_data(self.cur, sql, dto_class=HcclDto)

    def get_hccl_op_time_section(self):
        sql = f'select min(timestamp) as start_time, max(timestamp + duration) as end_time ' \
              f'from {DBNameConstant.TABLE_HCCL_SINGLE_DEVICE} ' \
              f"WHERE is_master = 1 " \
              f'group by op_name, first_timestamp'
        return DBManager.fetch_all_data(self.cur, sql, dto_class=CommunicationTimeSection)

    def create_table_by_name(self, table_name):
        if DBManager.judge_table_exist(self.cur, table_name):
            self.drop_table(table_name)
        table_map = "{0}Map".format(table_name)
        sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
        DBManager.execute_sql(self.conn, sql)
