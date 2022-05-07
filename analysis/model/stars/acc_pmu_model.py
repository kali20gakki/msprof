# coding=utf-8
"""
acc_pmu model
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

import logging
import os
import sqlite3

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.msvp_constant import MsvpConstant
from common_func.utils import Utils
from model.interface.base_model import BaseModel


class AccPmuModel(BaseModel):
    """
    task_based acc_pmu data model
    """

    READ = 1
    WRITE = 0
    TABLES_PATH = os.path.join(MsvpConstant.CONFIG_PATH, 'Tables.ini')

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)
        self.cur = None
        self.conn = None

    def create_table(self: any) -> None:
        """
        create table
        :return: None
        """
        sql = DBManager.sql_create_general_table('AccPmuMap', 'AccPmu', self.TABLES_PATH)
        DBManager.execute_sql(self.conn, sql)

    def insert_pmu_data(self: any, datas: list) -> None:
        """
        insert acc_pmu data to db
        :return: None
        """
        task_time = self.__get_task_time_form_acsq()
        result = Utils.generator_to_list(
            (data.stars_common.task_id, data.stars_common.stream_id, data.acc_id, data.block_id,
             data.bandwidth[self.READ], data.bandwidth[self.WRITE], data.ost[self.READ], data.ost[self.WRITE],
             data.stars_common.timestamp) + task_time.get(data.stars_common.task_id, ('', ''))
            for data in datas)
        if result[0]:
            soc_sql = "INSERT INTO {}  VALUES({})".format(DBNameConstant.TABLE_ACC_PMU_DATA,
                                                          '?,' * (len(result[0]) - 1) + '?')
            DBManager.executemany_sql(self.conn, soc_sql, result)

    def flush(self: any, data: list) -> None:
        """
        insert data to database
        """
        self.insert_pmu_data(data)

    def get_timeline_data(self: any) -> list:
        """
        get soc timeline data
        :return: list
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_ACC_PMU_DATA):
            return []
        sql = "select task_id, stream_id, acc_id, block_id, based_on, " \
              " read_bandwidth, write_bandwidth ,read_ost, write_ost, " \
              "time_stamp, start_time, dur_time from {}".format(DBNameConstant.TABLE_ACC_PMU_DATA)
        data_list = DBManager.fetch_all_data(self.cur, sql)
        return data_list

    def get_summary_data(self: any) -> list:
        """
        get soc timeline data
        :return: list
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_ACC_PMU_DATA):
            return []
        sql = "select task_id, stream_id, acc_id, block_id," \
              " read_bandwidth, write_bandwidth ,read_ost, write_ost, " \
              "time_stamp, start_time, dur_time from {}".format(DBNameConstant.TABLE_ACC_PMU_DATA)
        return DBManager.fetch_all_data(self.cur, sql)

    def __get_task_time_form_acsq(self: any) -> dict:
        """
        get task start_time and dur_time
        :return: dict
        """
        task_time_dict = {}
        conn, curs = DBManager.create_connect_db(os.path.join(self.result_dir, 'sqlite', 'acsq.db'))
        if not (conn and curs):
            return task_time_dict
        sql = 'select task_id, start_time, task_time from {}'.format(DBNameConstant.TABLE_ACSQ_TASK)
        try:
            task_time = DBManager.fetch_all_data(curs, sql)
        except sqlite3.Error as error:
            logging.info(str(error))
            return task_time_dict
        finally:
            DBManager.destroy_db_connect(conn, curs)
        if task_time[0]:
            for task in task_time:
                task_time_dict[task[0]] = (task[1], task[2])
        return task_time_dict
