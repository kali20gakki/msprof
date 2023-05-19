#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.hccl_dto import HcclDto
from profiling_bean.db_dto.time_section_dto import CommunicationTimeSection


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


class HcclViewModel(ViewModel):
    def __init__(self, result_dir: str, db_name: str, table_list: list):
        super().__init__(result_dir, db_name, table_list)

    def rebuild_hccl_table(self):
        self.create_table_by_name(DBNameConstant.TABLE_HCCL_ALL_REDUCE)

    def get_hccl_communication_data(self):
        """
        generate the table for communication op and task executed in device.
        """
        if not self.attach_to_db(DBNameConstant.DB_HWTS):
            return []
        sql = "SELECT t1.model_id as model_id, t1.index_id as index_id, t1.op_name as op_name, t1.name as hccl_name, " \
              "t1.plane_id as plane_id, t1.args as args, t2.running as timestamp, " \
              "t2.complete-t2.running as duration, t1.is_dynamic, t1.task_type, t1.op_type " \
              "from (select op_name, task_type, op_type, model_id, index_id, name, plane_id, args, " \
              "stream_id, task_id, is_dynamic from {0} " \
              "inner join {1} where timestamp >=begin and timestamp <= end ) t1 " \
              "inner join (select model_id, index_id, stream_id, task_id, running, complete from {2} ) t2 " \
              "on t1.model_id=t2.model_id " \
              "and (t1.index_id=t2.index_id or t1.is_dynamic=0)" \
              "and t1.stream_id = t2.stream_id " \
              "and t1.task_id = t2.task_id " \
              "order by t2.running".format(DBNameConstant.TABLE_HCCL_OP, DBNameConstant.TABLE_HCCL_TASK,
                                           DBNameConstant.TABLE_HWTS_TASK_TIME)
        return DBManager.fetch_all_data(self.cur, sql, dto_class=HcclDto)

    def get_hccl_op_data(self):
        """
        get the real execution of the communication op
        """
        sql = f"select model_id, index_id, op_name, first_timestamp, " \
              f"max(timestamp + duration) - first_timestamp as duration, task_type, op_type, args " \
              f"from {DBNameConstant.TABLE_HCCL_ALL_REDUCE} " \
              f"group by op_name, first_timestamp"
        return DBManager.fetch_all_data(self.cur, sql, dto_class=HcclDto)

    def get_hccl_op_time_section(self):
        sql = f'select first_timestamp as start_time, max(timestamp + duration) as end_time ' \
              f'from {DBNameConstant.TABLE_HCCL_ALL_REDUCE} ' \
              f'group by op_name, first_timestamp'
        return DBManager.fetch_all_data(self.cur, sql, dto_class=CommunicationTimeSection)

    def create_table_by_name(self, table_name):
        if DBManager.judge_table_exist(self.cur, table_name):
            self.drop_table(DBNameConstant.TABLE_HCCL_ALL_REDUCE)
        table_map = "{0}Map".format(table_name)
        sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
        DBManager.execute_sql(self.conn, sql)
