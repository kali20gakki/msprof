#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.sqe_type_map import SqeType
from profiling_bean.db_dto.ge_task_dto import GeTaskDto
from profiling_bean.db_dto.task_time_dto import TaskTimeDto


class AcsqTaskModel(ParserModel):
    """
    acsq task model class
    """

    def flush(self: any, data_list: list) -> None:
        """
        flush acsq task data to db
        :param data_list:acsq task data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_ACSQ_TASK, data_list)

    def flush_task_time(self: any, data_list: list) -> None:
        """
        flush acsq task data to db
        :param data_list:acsq task data list
        :return: None
        """
        self.insert_data_to_db(DBNameConstant.TABLE_ACSQ_TASK_TIME, data_list)

    def get_summary_data(self: any) -> list:
        """
        get op_summary data from table
        :return: op_summary data list
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_ACSQ_TASK):
            return []
        sql = "select 'N/A' as op_name, task_type, stream_id, task_id, task_time/{NS_TO_US} as task_time, " \
              "start_time, end_time from {}".format(
                DBNameConstant.TABLE_ACSQ_TASK, NS_TO_US=NumberConstant.NS_TO_US)
        task_time_data = DBManager.fetch_all_data(self.cur, sql, dto_class=TaskTimeDto)
        return task_time_data

    def get_timeline_data(self: any) -> list:
        """
        get timeline data from table
        :return: timeline data list
        """
        return self.get_all_data(DBNameConstant.TABLE_ACSQ_TASK)

    def get_ffts_type_data(self: any) -> list:
        """
        get all ffts type data
        :param step_start:
        :param step_end:
        :return: list
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_ACSQ_TASK):
            return []
        sql = "select 0, task_id, stream_id, start_time, task_time " \
              "from {0} " \
              "where task_type={task_type}".format(DBNameConstant.TABLE_ACSQ_TASK,
                                                   task_type=SqeType.AI_CORE.name)
        return DBManager.fetch_all_data(self.cur, sql)
