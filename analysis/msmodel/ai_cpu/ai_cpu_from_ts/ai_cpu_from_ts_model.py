#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to operate AI_CPU
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

from msmodel.interface.base_model import BaseModel
from common_func.db_name_constant import DBNameConstant
from common_func.db_manager import DBManager


class AiCpuFromTsModel(BaseModel):
    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_AI_CPU, DBNameConstant.TABLE_AI_CPU_FROM_TS)
        self._source_model = None
        self._source_table = None

    def flush(self: any, data_list: list) -> None:
        """
        insert data to table
        :param data_list: ai_cpu data
        :param ai_cpu_table_name: ai cpu table name
        :return:
        """
        self.insert_data_to_db(DBNameConstant.TABLE_AI_CPU_FROM_TS, data_list)

    def get_task_data(self: any) -> list:
        """
        insert data to table
        :param data_list: ai_cpu data
        :param ai_cpu_table_name: ai cpu table name
        :return:
        """
        ai_cpu_from_ts = []
        if self._source_table is None or self._source_model is None:
            return ai_cpu_from_ts

        with self._source_model:
            sql = "select stream_id, task_id, timestamp, task_state from {0} " \
                  "order by timestamp".format(self._source_table)

            ai_cpu_from_ts = DBManager.fetch_all_data(self.cur, sql)
