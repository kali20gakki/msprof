#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to operate AI_CPU
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

from common_func.db_name_constant import DBNameConstant
from msmodel.ai_cpu.ai_cpu_from_ts.ai_cpu_from_ts_model import AiCpuFromTsModel


class AiCpuFromTimelineModel(AiCpuFromTsModel):
    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir)
        self.start_tag = 2
        self.end_tag = 3
        self._origin_table = DBNameConstant.TABLE_TASK_TYPE

    def get_task_data(self: any) -> list:
        """
        insert data to table
        :param data_list: ai_cpu data
        :param ai_cpu_table_name: ai cpu table name
        :return:
        """
        sql = "select stream_id, task_id, timestamp, task_state from {0} " \
              "order by timestamp".format(self._origin_table)

        ai_cpu_from_ts = DBManager.fetch_all_data(self.cur, sql)
