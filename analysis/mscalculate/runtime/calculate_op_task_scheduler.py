#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import logging
import os

from config.config_manager import ConfigManager
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.ms_constant.number_constant import NumberConstant
from viewer.calculate_rts_data import calculate_task_schedule_data
from viewer.calculate_rts_data import multi_calculate_task_cost_time
from mscalculate.ts_task.ai_cpu.aicpu_from_ts_collector import AICpuFromTsCollector


class CalculateOpTaskScheduler:
    """
    analysis task data for scene of operator
    """
    STREAM_TASK_FMT = "{0}-{1}"
    COMPLETE_TIME_INDEX = 9

    def __init__(self: any, sample_config: dict) -> None:
        self.sample_config = sample_config
        self.device_id = sample_config.get("device_id", 0)
        self.index_id = sample_config.get("iter_id")
        self.project_path = sample_config.get("result_dir")
        self.task_data = []
        self.cnt_miss = -1

    @staticmethod
    def _create_task_time_table(runtime_conn: any, runtime_curs: any) -> None:
        if DBManager.judge_table_exist(runtime_curs, DBNameConstant.TABLE_RUNTIME_TASK_TIME):
            DBManager.drop_table(runtime_conn, DBNameConstant.TABLE_RUNTIME_TASK_TIME)
        sql = DBManager.sql_create_general_table('TaskTimeMap', DBNameConstant.TABLE_RUNTIME_TASK_TIME,
                                                 ConfigManager.TABLES_OPERATOR)
        DBManager.execute_sql(runtime_conn, sql)

    @staticmethod
    def _update(api_data: list) -> list:
        """
        api update data for op scene
        :param api_data: runtime api data
        :return: api data
        """
        api_down_task_op = []
        for api in api_data:
            # API data is in the format of api,rowid,stream_id,task_id,batch_id for single op scene
            task_id_op = api[3].split(',')
            batch_id_op = api[4].split(',')
            if len(task_id_op) != len(batch_id_op):
                logging.error("The num of task id is not equal to batch id")
                return []
            for task, batch in zip(task_id_op, batch_id_op):
                api_down_task_op.append(api[0:3] + (task, batch,))
        return api_down_task_op

    @staticmethod
    def _insert_report_task_data(runtime_conn: any, runtime_curs: any, device_id: int) -> None:
        report_data = calculate_task_schedule_data(runtime_curs, device_id)
        if not report_data:
            logging.info('Unable to get report task data')
            return
        sql = 'insert into {0} values({value})'.format(DBNameConstant.TABLE_RUNTIME_REPORT_TASK,
                                                       value='?,' * (len(report_data[0]) - 1) + '?')
        DBManager.executemany_sql(runtime_conn, sql, report_data)

    def run(self: any) -> None:
        """
        parsing task process
        :return: None
        """
        try:
            if ChipManager().is_chip_v1():
                self.op_generate_report_data()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err))
        finally:
            pass

    def op_create_task_time(self: any, runtime_conn: any, device: int) -> None:
        """
        create task time table
        :param runtime_conn: connect for runtime
        :param device: device id
        :return: NA
        """
        logging.info('start create task time table')
        runtime_curs = runtime_conn.cursor()
        if not DBManager.judge_table_exist(runtime_curs, DBNameConstant.TABLE_RUNTIME_TIMELINE):
            logging.info("TimeLine table doesn't exist")
            return
        self._create_task_time_table(runtime_conn, runtime_curs)
        try:
            cal_task_data = self._get_timeline_data(device, runtime_curs)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return
        task_time = self._add_info(cal_task_data)
        self._collect_aicpu(task_time)
        self._insert_task_time_data(task_time, runtime_conn, runtime_curs)
        logging.info('create task time table end')

    def op_pre_mini_task_data(self: any, project_path: str, device_id: int) -> None:
        """
        deal with the ts track data for operator
        :param project_path: project path
        :param device_id: device id
        :return: NA
        """
        runtime_conn, runtime_curs = \
            DBManager.check_connect_db_path(PathManager.get_db_path(project_path, DBNameConstant.DB_RUNTIME))
        if not runtime_conn or not runtime_curs:
            return
        try:
            self.op_create_task_time(runtime_conn, device_id)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
        else:
            self.op_update_timeline_api(runtime_conn)
        finally:
            DBManager.destroy_db_connect(runtime_conn, runtime_curs)

    def op_insert_report_data(self: any, project_path: str, device_id: int) -> None:
        """
        insert data into report
        :param project_path: project path
        :param device_id: device id
        :return: None
        """
        logging.info('start insert op data into report table')
        db_path = os.path.join(PathManager.get_sql_dir(project_path), DBNameConstant.DB_RUNTIME)
        runtime_conn, runtime_curs = DBManager.check_connect_db_path(db_path)
        if not runtime_conn or not runtime_curs \
                or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_RUNTIME_TASK_TIME):
            return
        self._create_report_task_table(runtime_conn)
        self._insert_report_task_data(runtime_conn, runtime_curs, device_id)
        logging.info('Insert op data into report table finished.')
        DBManager.destroy_db_connect(runtime_conn, runtime_curs)

    def op_generate_report_data(self: any) -> None:
        """
        insert report task table
        :return: None
        """
        if DBManager.check_tables_in_db(PathManager.get_db_path(self.project_path, DBNameConstant.DB_RUNTIME),
                                        DBNameConstant.TABLE_RUNTIME_REPORT_TASK):
            return
        self.op_pre_mini_task_data(self.project_path, self.device_id)
        self.op_insert_report_data(self.project_path, self.device_id)

    def op_update_timeline_api(self: any, runtime_conn: any) -> None:
        """
        updates timeline api
        :param runtime_conn: connect of runtime
        :return: NA
        """
        logging.info('Start to update TaskTime API.')
        runtime_curs = runtime_conn.cursor()
        if not (DBManager.judge_table_exist(runtime_curs, DBNameConstant.TABLE_RUNTIME_TASK_TIME)
                and DBManager.judge_table_exist(runtime_curs, DBNameConstant.TABLE_API_CALL)):
            logging.warning("No need to update task time data, runtime "
                            "api data or ts timeline data had not been collected.")
            return
        select_api_sql = 'select api,rowid,stream_id,task_id,batch_id from {0} where tasknum != 0 ' \
                         'order by entry_time;'.format(DBNameConstant.TABLE_API_CALL)
        api_thread = DBManager.fetch_all_data(runtime_curs, select_api_sql)
        try:
            api_down_data = self._update(api_thread)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return
        sql = 'update TaskTime set api=?, apiRowId=? where stream_id=? and task_id=? and batch_id=?'
        DBManager.executemany_sql(runtime_conn, sql, api_down_data)
        logging.info('Update TimeLine API finished.')

    def _add_info(self: any, cal_task_data: list) -> list:
        # 0 is default batch id
        task_time = [task_data + (
            self.index_id, NumberConstant.DEFAULT_BATCH_ID) for task_data in cal_task_data]
        return task_time

    def _collect_aicpu(self: any, task_time: list) -> None:
        aicpu_collector = AICpuFromTsCollector(self.project_path)

        for data in task_time:
            task_id = data[5]
            stream_id = data[6]
            start = data[9]
            end = data[10]
            task_type = data[4]

            aicpu_feature = (stream_id, task_id, start, end, task_type)
            aicpu_collector.filter_aicpu(aicpu_feature)
        aicpu_collector.save_aicpu()

    def _insert_task_time_data(self: any, task_time: list, runtime_conn: any, runtime_curs: any) -> None:
        # sort by complete time
        task_time = sorted(task_time, key=lambda data: data[self.COMPLETE_TIME_INDEX])
        insert_sql = "insert into TaskTime " \
                     "values ({value})".format(value="?," * (len(task_time[0]) - 1) + "?")
        DBManager.executemany_sql(runtime_conn, insert_sql, task_time)

    def _get_timeline_data(self: any, device: int, runtime_curs: any) -> list:
        timeline_sql = "select replayId,device_id,'','',taskType," \
                       "task_id,stream_id,timeStamp,taskState " \
                       "from TimeLine WHERE device_id=?" \
                       "order by task_id, stream_id,timeStamp,taskState,device_id;"
        timeline_data = DBManager.fetch_all_data(runtime_curs, timeline_sql, (device,))
        cal_task_data = multi_calculate_task_cost_time(timeline_data, self.project_path)
        return cal_task_data

    def _create_report_task_table(self: any, runtime_conn: any) -> None:
        if DBManager.check_tables_in_db(PathManager.get_db_path(self.project_path, DBNameConstant.DB_RUNTIME),
                                        DBNameConstant.TABLE_RUNTIME_REPORT_TASK):
            DBManager.drop_table(runtime_conn, DBNameConstant.TABLE_RUNTIME_REPORT_TASK)
        sql = DBManager.sql_create_general_table('ReportTaskMap', DBNameConstant.TABLE_RUNTIME_REPORT_TASK,
                                                 ConfigManager.TABLES_OPERATOR)
        DBManager.execute_sql(runtime_conn, sql)
