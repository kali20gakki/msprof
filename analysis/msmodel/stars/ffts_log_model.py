#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.stars_constant import StarsConstant
from common_func.utils import Utils
from msmodel.interface.parser_model import ParserModel


class FftsLogModel(ParserModel):
    """
    ffts thread log and subtask log model
    """

    def insert_log_data(self: any, data_list: list) -> None:
        """
        insert soc log data to db
        :param data_list: data list
        :return: None
        """
        result = Utils.generator_to_list((data.stars_common.stream_id, data.stars_common.task_id,
                                          data.subtask_id, data.thread_id,
                                          StarsConstant.SUBTASK_TYPE.get(data.subtask_type, data.subtask_type),
                                          StarsConstant.FFTS_TYPE.get(data.ffts_type, data.ffts_type), data.func_type,
                                          data.stars_common.timestamp) for data in data_list)

        self.insert_data_to_db(DBNameConstant.TABLE_FFTS_LOG, result)

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
