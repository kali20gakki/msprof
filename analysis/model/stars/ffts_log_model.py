# coding=utf-8
"""
ffts_log_log model
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.stars_constant import StarsConstant
from common_func.utils import Utils
from model.interface.parser_model import ParserModel


class FftsLogModel(ParserModel):
    """
    ffts thread log and subtask log model
    """

    @staticmethod
    def _get_thread_task_time_sql() -> str:
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
                          "group by end_log.subtask_id,end_log.thread_id, end_log.task_id " \
                          "order by end_log.subtask_id".format(DBNameConstant.TABLE_FFTS_LOG,
                                                               StarsConstant.FFTS_LOG_END_TAG,
                                                               StarsConstant.FFTS_LOG_START_TAG)

        return thread_base_sql

    @staticmethod
    def _get_subtask_time_sql() -> str:
        """
        table ffts thread_log have both thread strat logs and end logs.
        A subtask has multiple thread.
        Subtract the start time with the minimum time from the end time with the maximum time.
        """
        subtask_base_sql = "Select end_log.subtask_id, end_log.task_id,end_log.stream_id,end_log.subtask_type," \
                           "end_log.ffts_type,min(start_log.task_time) as start_time, " \
                           "(max(end_log.task_time)-min(start_log.task_time)) as dur_time " \
                           "from {0} end_log join {0} start_log " \
                           "on end_log.subtask_id=start_log.subtask_id " \
                           "and end_log.stream_id=start_log.stream_id " \
                           "and end_log.task_id=start_log.task_id " \
                           "where end_log.task_type='{1}' and start_log.task_type='{2}' group by end_log.subtask_id, " \
                           "end_log.task_id order by start_time".format(DBNameConstant.TABLE_FFTS_LOG,
                                                                        StarsConstant.FFTS_LOG_END_TAG,
                                                                        StarsConstant.FFTS_LOG_START_TAG)

        return subtask_base_sql

    def insert_log_data(self: any, data_list: list) -> None:
        """
        insert soc log data to db
        :param data_list: data list
        :return: None
        """
        result = Utils.generator_to_list((data.stars_common.stream_id, data.stars_common.task_id,
                                          data.subtask_id, data.thread_id,
                                          data.subtask_type,
                                          StarsConstant.FFTS_TYPE.get(data.ffts_type, data.ffts_type), data.func_type,
                                          data.stars_common.timestamp) for data in data_list)

        self.insert_data_to_db(DBNameConstant.TABLE_FFTS_LOG, result)
        self.__create_log_table(DBNameConstant.TABLE_THREAD_TASK, self._get_thread_task_time_sql())
        self.__create_log_table(DBNameConstant.TABLE_SUBTASK_TIME, self._get_subtask_time_sql())

    def flush(self: any, data_list: list) -> None:
        """
        flush data list to db
        :param data_list:
        :return: None
        """
        self.insert_log_data(data_list)

    def get_timeline_data(self: any) -> dict:
        """
        to get timeline data from database
        :return: result list
        """
        if not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_FFTS_LOG):
            return {}
        thread_data_list = self._get_thread_time_data()
        subtask_data_list = self._get_subtask_time_data()
        return {'thread_data_list': thread_data_list, 'subtask_data_list': subtask_data_list}

    def get_summary_data(self: any) -> list:
        """
        to get timeline data from database
        :return: result list
        """
        return self.get_all_data(DBNameConstant.TABLE_FFTS_LOG)

    def get_ffts_task_data(self):
        """
        get ffts task data
        :return:
        """
        sql = "select subtask_id, task_id, stream_id, start_time, dur_time " \
              "from {0} " \
              "order by start_time ".format(DBNameConstant.TABLE_SUBTASK_TIME)
        return DBManager.fetch_all_data(self.cur, sql)

    def _get_thread_time_data(self: any) -> list:
        return self.get_all_data(DBNameConstant.TABLE_THREAD_TASK)

    def _get_subtask_time_data(self: any) -> list:
        return self.get_all_data(DBNameConstant.TABLE_SUBTASK_TIME)

    def __create_log_table(self: any, table_name: str, sql: str) -> None:
        if DBManager.judge_table_exist(self.cur, table_name):
            DBManager.drop_table(self.conn, table_name)
        create_sql = "create table {0} as {1}".format(table_name, sql)
        self.cur.execute(create_sql)
