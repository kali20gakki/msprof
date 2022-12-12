#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import logging

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.stars_constant import StarsConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_iteration import MsprofIteration
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from profiling_bean.prof_enum.data_tag import DataTag


class SubTaskCalculator(MsMultiProcess):
    """
    calculate subtask data
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.sample_config = sample_config
        self.file_list = file_list
        self.result_dir = self.sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self.iteration_time = MsprofIteration(self.result_dir).get_iter_interval(
            self.sample_config.get(StrConstant.PARAM_ITER_ID))
        self.conn = None
        self.cur = None

    def ms_run(self: any) -> None:
        """
        calculate for subtask
        :return: None
        """
        if not self.file_list.get(DataTag.STARS_LOG) or not ChipManager().is_ffts_plus_type():
            return
        self.calculate()

    def calculate(self: any) -> None:
        try:
            self.init()
        except ValueError:
            logging.warning("calculate subtask failed, maybe the data is not in fftsplus mode")
            return
        try:
            self.save()
        except ValueError as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        finally:
            DBManager().destroy_db_connect(self.conn, self.cur)

    def init(self: any) -> None:
        self.conn, self.cur = DBManager().check_connect_db(self.result_dir, DBNameConstant.DB_SOC_LOG)
        if not self.conn or not self.cur or not DBManager().check_tables_in_db(
                PathManager.get_db_path(self.result_dir, DBNameConstant.DB_SOC_LOG), DBNameConstant.TABLE_FFTS_LOG):
            raise ValueError

    def save(self: any) -> None:
        self.__create_log_table(DBNameConstant.TABLE_THREAD_TASK, self._get_thread_task_time_sql())
        self.__create_log_table(DBNameConstant.TABLE_SUBTASK_TIME, self._get_subtask_time_sql())

    def _get_thread_task_time_sql(self: any) -> str:
        """
        table ffts_log have both thread start logs and end logs
        Timestamps of logs with the same subtask_ID and task_id and stream_id
        are subtracted to obtain the dur_time.
        """
        thread_base_sql = "Select end_log.subtask_id, end_log.task_id,end_log.stream_id," \
                          "end_log.subtask_type, end_log.ffts_type,end_log.thread_id,start_log.task_time," \
                          "(end_log.task_time-start_log.task_time) as dur_time " \
                          "from {0} end_log " \
                          "join {0} start_log on end_log.thread_id=start_log.thread_id " \
                          "and end_log.subtask_id=start_log.subtask_id " \
                          "and end_log.stream_id=start_log.stream_id " \
                          "and end_log.task_id=start_log.task_id " \
                          "where end_log.task_type='{1}' and start_log.task_type='{2}' " \
                          "and start_log.task_time > {3} and end_log.task_time < {4} " \
                          "group by end_log.subtask_id,end_log.thread_id, end_log.task_id " \
                          "order by end_log.subtask_id".format(DBNameConstant.TABLE_FFTS_LOG,
                                                               StarsConstant.FFTS_LOG_END_TAG,
                                                               StarsConstant.FFTS_LOG_START_TAG,
                                                               self.iteration_time[0],
                                                               self.iteration_time[1])

        return thread_base_sql

    def _get_subtask_time_sql(self: any) -> str:
        """
        table ffts thread_log have both thread strat logs and end logs.
        A subtask has multiple thread.
        Subtract the start time with the minimum time from the end time with the maximum time.
        """
        subtask_base_sql = "Select end_log.subtask_id, end_log.task_id,end_log.stream_id,end_log.subtask_type," \
                           "end_log.ffts_type,start_log.task_time as start_time, " \
                           "(end_log.task_time-start_log.task_time) as dur_time " \
                           "from {0} end_log join {0} start_log " \
                           "on end_log.subtask_id=start_log.subtask_id " \
                           "and end_log.stream_id=start_log.stream_id " \
                           "and end_log.task_id=start_log.task_id " \
                           "where end_log.task_type='{1}' and start_log.task_type='{2}' " \
                           "and start_log.task_time > {3} and end_log.task_time < {4} " \
                           "group by end_log.subtask_id, " \
                           "end_log.task_id order by start_time".format(DBNameConstant.TABLE_FFTS_LOG,
                                                                        StarsConstant.FFTS_LOG_END_TAG,
                                                                        StarsConstant.FFTS_LOG_START_TAG,
                                                                        self.iteration_time[0],
                                                                        self.iteration_time[1])

        return subtask_base_sql

    def __create_log_table(self: any, table_name: str, sql: str) -> None:
        if DBManager.judge_table_exist(self.cur, table_name):
            DBManager.drop_table(self.conn, table_name)
        create_sql = "create table {0} as {1}".format(table_name, sql)
        self.cur.execute(create_sql)
