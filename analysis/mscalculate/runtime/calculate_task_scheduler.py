#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to operate task scheduler
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

import logging
import os
import sqlite3

from common_func.msvp_common import MsvpCommonConst
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.ms_constant.number_constant import NumberConstant
from viewer.calculate_rts_data import calculate_task_schedule_data
from viewer.calculate_rts_data import multi_calculate_task_cost_time


class CalculateTaskScheduler:
    """
    calculate origin data
    """
    TABLE_PATH = os.path.join(MsvpCommonConst.CONFIG_PATH, 'Tables.ini')
    COMPLETE_TIME_INDEX = 10

    def __init__(self: any, sample_config: dict) -> None:
        self.sample_config = sample_config
        self.project_path = sample_config.get("result_dir")
        self.index_id = sample_config.get("iter_id")
        self.model_id = sample_config.get("model_id")

    @staticmethod
    def update(api_data: list) -> list:
        """
        api update data
        :param api_data: api data
        :return:
        """
        api_down_task = []
        for api in api_data:
            # API data is in the format of api,rowid,stream_id,task_id,batch_id for step scene
            task_id = api[3].split(',')
            batch_id = api[4].split(',')
            if len(task_id) != len(batch_id):
                logging.error("The num of task id is not equal to batch id")
                return []
            for task, batch in zip(task_id, batch_id):
                api_down_task.append(api[0:3] + (task, batch,))
        return api_down_task

    @staticmethod
    def _insert_report_task_data(runtime_conn: any, runtime_curs: any, device_id: str) -> None:
        report_data = calculate_task_schedule_data(runtime_curs, device_id)
        if not report_data:
            logging.info('Unable to get report task data')
            return
        sql = 'insert into ReportTask values({value})'.format(
            value='?,' * (len(report_data[0]) - 1) + '?')
        DBManager.executemany_sql(runtime_conn, sql, report_data)

    def create_task_time(self: any, runtime_conn: any, device: int, iter_time_range: list) -> None:
        """
        create task time table
        :param runtime_conn: connect for runtime
        :param device: device id
        :param iter_time_range: iteration range
        :return: None
        """
        runtime_curs = runtime_conn.cursor()
        logging.info('start create task time table')
        if not DBManager.judge_table_exist(runtime_curs, DBNameConstant.TABLE_RUNTIME_TIMELINE):
            logging.warning("TimeLine data not found, and no need to create task time data, "
                            "please check the ts track data.")
            return
        self._create_task_time_table(runtime_conn, runtime_curs)
        try:
            cal_task_data = self._get_timeline_data(device, iter_time_range, runtime_curs)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self._insert_task_time_data(cal_task_data, runtime_conn, runtime_curs)
        logging.info('create task time table end')

    def update_timeline_api(self: any, runtime_conn: any) -> None:
        """
        updates timeline api
        :param runtime_conn: connection for runtime
        :return: None
        """
        logging.info('Start to update TaskTime API.')
        runtime_curs = runtime_conn.cursor()
        if not (DBManager.judge_table_exist(runtime_curs, DBNameConstant.TABLE_RUNTIME_TASK_TIME)
                and DBManager.judge_table_exist(runtime_curs, DBNameConstant.TABLE_API_CALL)):
            logging.warning("No need to update task time data, runtime "
                            "api data or ts timeline data had not been collected.")
            return
        select_api_sql = 'select api,rowid,stream_id,task_id,batch_id from ApiCall ' \
                         'where tasknum != 0 order by entry_time;'
        api_thread = DBManager.fetch_all_data(runtime_curs, select_api_sql)
        api_down_data = []
        try:
            api_down_data = self.update(api_thread)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return
        sql = 'update TaskTime set api=?, apiRowId=? where stream_id=? and task_id=? and batch_id=?'
        DBManager.executemany_sql(runtime_conn, sql, api_down_data)
        logging.info('Update TimeLine API finished.')

    def insert_report_data(self: any, project_path: str, device_id: str) -> None:
        """
        insert data into report
        :param device_id:
        :param project_path: project path
        :return:
        """
        logging.info('start insert data into report table')
        db_path = os.path.join(PathManager.get_sql_dir(project_path), DBNameConstant.DB_RUNTIME)
        runtime_conn, runtime_curs = DBManager.check_connect_db_path(db_path)
        if not runtime_conn or not runtime_curs \
                or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_RUNTIME_TASK_TIME):
            return
        self._create_report_task_table(runtime_conn, runtime_curs)
        self._insert_report_task_data(runtime_conn, runtime_curs, device_id)

        logging.info('Insert data into report table finished.')
        DBManager.destroy_db_connect(runtime_conn, runtime_curs)

    def generate_report_data(self: any) -> None:
        """
        insert report task table
        :return: None
        """
        devices = InfoConfReader().get_device_list()
        if not devices:
            logging.error("No device list found in info.json")
            return
        iter_time_range = self.__get_iter_time_range(self.project_path)
        if not iter_time_range:
            return
        self.__pre_mini_task_data(self.project_path, devices[0], iter_time_range)
        self.insert_report_data(self.project_path, devices[0])

    def run(self: any) -> None:
        """
        calculate for task scheduler
        :return:
        """
        try:
            if ChipManager().is_chip_v1():
                self.generate_report_data()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err))
        finally:
            pass

    def __get_iter_time_range(self: any, project_path: str) -> list:
        time_range_result = []
        step_conn, step_curs = DBManager.check_connect_db(project_path, DBNameConstant.DB_STEP_TRACE)
        if not step_conn or not step_curs \
                or not DBManager.judge_table_exist(step_curs, DBNameConstant.TABLE_STEP_TRACE_DATA):
            return time_range_result
        sql = "select step_start, step_end from {0} " \
              "where index_id=? and model_id=?".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        try:
            time_range = step_curs.execute(sql, (self.index_id, self.model_id)).fetchone()
        except sqlite3.Error as step_err:
            logging.error(step_err, exc_info=Constant.TRACE_BACK_SWITCH)
            return time_range_result
        finally:
            DBManager.destroy_db_connect(step_conn, step_curs)
        if time_range:
            time_range_result = list(time_range)
        return time_range_result

    def _insert_task_time_data(self: any, cal_task_data: list, runtime_conn: any, runtime_curs: any) -> None:
        # 0 is default batch id
        task_time = (task_data + (
            self.index_id, self.model_id, NumberConstant.DEFAULT_BATCH_ID) for task_data in cal_task_data)
        # sort by complete time
        task_time = sorted(task_time, key=lambda data: data[self.COMPLETE_TIME_INDEX])
        insert_sql = "insert into TaskTime " \
                     "values ({value})".format(value="?," * (len(task_time[0]) - 1) + "?")
        DBManager.executemany_sql(runtime_conn, insert_sql, task_time)

    def _create_task_time_table(self: any, runtime_conn: any, runtime_curs: any) -> None:
        if DBManager.judge_table_exist(runtime_curs, DBNameConstant.TABLE_RUNTIME_TASK_TIME):
            DBManager.drop_table(runtime_conn, DBNameConstant.TABLE_RUNTIME_TASK_TIME)
        sql = DBManager.sql_create_general_table('TaskTimeMap', DBNameConstant.TABLE_RUNTIME_TASK_TIME,
                                                 self.TABLE_PATH)
        DBManager.execute_sql(runtime_conn, sql)

    def _get_timeline_data(self: any, device: int, iter_time_range: list, runtime_curs: any) -> list:
        timeline_sql = "select replayId,device_id,'','',taskType," \
                       "task_id,stream_id,timeStamp,taskState " \
                       "from TimeLine WHERE device_id=? and timestamp>? and timestamp<?" \
                       "order by task_id, stream_id,timeStamp,taskState,device_id;"
        timeline_data = DBManager.fetch_all_data(runtime_curs, timeline_sql, (device,
                                                                              iter_time_range[0], iter_time_range[1]))
        cal_task_data = multi_calculate_task_cost_time(timeline_data, self.project_path)
        return cal_task_data

    def __pre_mini_task_data(self: any, project_path: str, device_id: int, iter_time_range: list) -> None:
        runtime_conn, runtime_curs = \
            DBManager.check_connect_db_path(PathManager.get_db_path(project_path, DBNameConstant.DB_RUNTIME))
        if not runtime_conn or not runtime_curs:
            return
        try:
            self.create_task_time(runtime_conn, device_id, iter_time_range)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
        else:
            self.update_timeline_api(runtime_conn)
        finally:
            DBManager.destroy_db_connect(runtime_conn, runtime_curs)

    def _create_report_task_table(self: any, runtime_conn: any, runtime_curs: any) -> None:
        if DBManager.check_tables_in_db(PathManager.get_db_path(self.project_path, DBNameConstant.DB_RUNTIME),
                                        DBNameConstant.TABLE_RUNTIME_REPORT_TASK):
            DBManager.drop_table(runtime_conn, DBNameConstant.TABLE_RUNTIME_REPORT_TASK)
        sql = DBManager.sql_create_general_table('ReportTaskMap', 'ReportTask', self.TABLE_PATH)
        DBManager.execute_sql(runtime_conn, sql)
