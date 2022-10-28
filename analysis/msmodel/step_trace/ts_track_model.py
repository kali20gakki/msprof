#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

from abc import ABC
from functools import partial
from itertools import chain

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.msprof_iteration import MsprofIteration
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from msmodel.interface.base_model import BaseModel


class TsTrackModel(BaseModel, ABC):
    """
    acsq task model class
    """
    TS_AI_CPU_TYPE = 1

    @staticmethod
    def __aicpu_in_time_range(data, min_timestamp, max_timestamp):
        return min_timestamp <= InfoConfReader().time_from_syscnt(data[2],
                                                                  NumberConstant.MICRO_SECOND) <= max_timestamp

    def flush(self: any, table_name: str, data_list: list) -> None:
        """
        flush acsq task data to db
        :param data_list:acsq task data list
        :return: None
        """
        self.insert_data_to_db(table_name, data_list)

    def create_table(self: any, table_name: str) -> None:
        """
        create table
        """
        if DBManager.judge_table_exist(self.cur, table_name):
            DBManager.drop_table(self.conn, table_name)
        table_map = "{0}Map".format(table_name)
        sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
        DBManager.execute_sql(self.conn, sql)

    def get_ai_cpu_data(self: any, model_id: int, index_id: int) -> list:
        """
        get ai cpu data
        :param model_id: model id
        :param index_id: index id
        :return: ai cpu with state
        """
        if not DBManager.check_tables_in_db(PathManager.get_db_path(self.result_dir, DBNameConstant.DB_STEP_TRACE),
                                            DBNameConstant.TABLE_TASK_TYPE):
            return []

        iter_time_range = list(chain.from_iterable(
            MsprofIteration(self.result_dir).get_iteration_time(index_id, model_id)))
        sql = "select stream_id, task_id, timestamp, " \
              "task_state from {0} where task_type={1} order by timestamp ".format(
            DBNameConstant.TABLE_TASK_TYPE,
            self.TS_AI_CPU_TYPE)
        ai_cpu_with_state = DBManager.fetch_all_data(self.cur, sql)

        for index, datum in enumerate(ai_cpu_with_state):
            ai_cpu_with_state[index] = list(datum)
            # index 2 is timestamp
            ai_cpu_with_state[index][2] = int(datum[2])

        if iter_time_range:
            min_timestamp = min(iter_time_range)
            max_timestamp = max(iter_time_range)

            # data index 2 is timestamp
            ai_cpu_with_state = list(filter(partial(self.__aicpu_in_time_range, min_timestamp=min_timestamp,
                                                    max_timestamp=max_timestamp), ai_cpu_with_state))
        return ai_cpu_with_state
