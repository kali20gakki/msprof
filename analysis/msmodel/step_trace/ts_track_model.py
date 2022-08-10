#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to operate ts track
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""
from abc import ABC

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from msmodel.interface.base_model import BaseModel


class TaskTimeline:
    def __init__(self: any) -> None:
        self.init()

    def init(self: any) -> None:
        self.start = None
        self.end = None

    def get_timeline(self: any) -> list:
        timeline = [self.start, self.end]
        self.init()
        return timeline


class AiCpuTagHandler:
    RECEIVE_TAG = 0
    START_TAG = 1
    END_TAG = 2

    def __init__(self: any, stream_id: int, task_id: int) -> None:
        self.stream_id = stream_id
        self.task_id = task_id
        self.new_task = TaskTimeline()
        self.ai_cpu_list = []

    def process_record(self: any, timestamp: int, task_state: int) -> None:
        if task_state == self.RECEIVE_TAG or task_state == self.START_TAG:
            self.new_task.start = timestamp

        if task_state == self.END_TAG:
            self.new_task.end = timestamp
            if self.new_task.start is None:
                print("ERROR, task not start")
            self.ai_cpu_list.append(self.new_task.get_timeline())

    def get_aicpu_list(self: any) -> list:
        aicpu_with_stream_task = []
        for aicpu in self.ai_cpu_list:
            datum = [self.stream_id, self.task_id]
            datum.extend(aicpu)
            aicpu_with_stream_task.append(datum)
        return aicpu_with_stream_task


class TsTrackModel(BaseModel, ABC):
    """
    acsq task model class
    """
    TS_AI_CPU_TYPE = 1

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

    def get_ai_cpu_data(self: any) -> list:
        """
        get ai cpu data
        """
        sql = "select stream_id, task_id, timestamp, task_state from {0} where task_type={1} " \
              "order by timestamp".format(DBNameConstant.TABLE_TASK_TYPE, self.TS_AI_CPU_TYPE)

        ai_cpu_from_ts = DBManager.fetch_all_data(self.cur, sql)

        ai_cpu_data = []
        stream_task_group = {}
        for stream_id, task_id, timestamp, task_state in ai_cpu_from_ts:
            tag_handler = stream_task_group.setdefault((stream_id, task_id), AiCpuTagHandler(stream_id, task_id))
            tag_handler.process_record(timestamp, task_state)

        for stream_id, task_id in stream_task_group:
            tag_handler = stream_task_group.setdefault((stream_id, task_id), AiCpuTagHandler(stream_id, task_id))
            ai_cpu_data.extend(tag_handler.get_aicpu_list())

        # 3 sys end
        ai_cpu_data = sorted(ai_cpu_data, key=lambda datum: datum[3])
        return ai_cpu_data
