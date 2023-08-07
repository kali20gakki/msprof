#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
import logging

from analyzer.scene_base.profiling_scene import ProfilingScene
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

    @classmethod
    def get_task_time_sql(cls):
        select_sql = "(select {0}.model_id, {0}.index_id, {0}.stream_id, {0}.task_id, " \
                     "{0}.batch_id, {0}.start_time as running, " \
                     "{0}.start_time + {0}.duration as complete from {0} )".format(DBNameConstant.TABLE_ASCEND_TASK)
        return select_sql

    def rebuild_hccl_table(self):
        self.create_table_by_name(DBNameConstant.TABLE_HCCL_ALL_REDUCE)

    def rebuild_hccl_op_report_table(self):
        self.create_table_by_name(DBNameConstant.TABLE_HCCL_OP_REPORT)

    def get_hccl_communication_data(self):
        """
        generate the table for communication op and task executed in device.
        """

        if not self.attach_to_db(DBNameConstant.DB_ASCEND_TASK):
            logging.error("Attach to db %s failed, task data not found.", DBNameConstant.DB_ASCEND_TASK)
            return [], []
        task_time_sql = self.get_task_time_sql()
        where_condition = ''
        if not ProfilingScene().is_operator():
            where_condition = 'and t1.model_id=t2.model_id and (t1.index_id=t2.index_id or t1.is_dynamic=0)'
        sql1 = "SELECT t1.model_id as model_id, t1.index_id as index_id, t1.op_name as op_name, " \
              "t1.name as hccl_name, " \
              "t1.plane_id as plane_id, t1.args as args, t2.running as timestamp, " \
              "t2.complete-t2.running as duration, t1.is_dynamic as is_dynamic, t1.task_type as task_type, " \
              "t1.op_type as op_type, t1.begin as first_timestamp " \
              "from (select {0}.op_name, {0}.task_type, {0}.op_type, {0}.model_id, " \
              "{0}.index_id, {1}.name, {1}.plane_id, {1}.args, " \
              "{1}.stream_id, {1}.task_id, {1}.batch_id, {0}.is_dynamic, {0}.begin from {0} " \
              "inner join {1} where {1}.timestamp >={0}.begin and {1}.timestamp <= {0}.end ) t1 " \
              "inner join {task_time_sql} t2 " \
              "on  t1.stream_id = t2.stream_id " \
              "and t1.task_id = t2.task_id " \
              "and t1.batch_id = t2.batch_id {where_condition} " \
              "order by t2.running".format(DBNameConstant.TABLE_HCCL_OP, DBNameConstant.TABLE_HCCL_TASK,
                                           task_time_sql=task_time_sql, where_condition=where_condition)

        sql2 = "SELECT t4.op_type as op_type, count(t4.op_type), sum(t4.duration) as total_time, " \
               "min(t4.duration) as min, sum(t4.duration)/count(t4.op_type) as avg, max(t4.duration) as max FROM " \
               "(SELECT t3.op_name as op_name, t3.op_type as op_type, " \
               "max(t3.timestamp+duration)-min(t3.timestamp) as duration " \
               "FROM ({hccl_all_reduce_sql}) t3 group by t3.op_name, t3.op_type) t4 group by t4.op_type"\
               .format(hccl_all_reduce_sql=sql1)

        return DBManager.fetch_all_data(self.cur, sql1, dto_class=HcclDto), \
            DBManager.fetch_all_data(self.cur, sql2)

    def get_hccl_op_data(self):
        """
        get the real execution of the communication op
        """
        sql = f"select model_id, index_id, op_name, min(timestamp) as timestamp, " \
              f"max(timestamp + duration) - min(timestamp) as duration, task_type, op_type " \
              f"from {DBNameConstant.TABLE_HCCL_ALL_REDUCE} " \
              f"group by op_name, first_timestamp"
        return DBManager.fetch_all_data(self.cur, sql, dto_class=HcclDto)

    def get_hccl_op_time_section(self):
        sql = f'select min(timestamp) as start_time, max(timestamp + duration) as end_time ' \
              f'from {DBNameConstant.TABLE_HCCL_ALL_REDUCE} ' \
              f'group by op_name, first_timestamp'
        return DBManager.fetch_all_data(self.cur, sql, dto_class=CommunicationTimeSection)

    def create_table_by_name(self, table_name):
        if DBManager.judge_table_exist(self.cur, table_name):
            self.drop_table(table_name)
        table_map = "{0}Map".format(table_name)
        sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
        DBManager.execute_sql(self.conn, sql)
