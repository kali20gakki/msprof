#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.time_section_dto import TimeSectionDto


class TaskTime:
    """
    class used to present task time
    """

    def __init__(self: any) -> None:
        self._task_id = None
        self._stream_id = None
        self._start_time = 0
        self._duration_time = 0
        self._wait_time = 0
        self._index_id = 0
        self._model_id = 0

    @property
    def task_id(self):
        """
        get task id
        :return: task_id
        """
        return self._task_id

    @property
    def stream_id(self):
        """
        get stream id
        :return: stream id
        """
        return self._stream_id

    @property
    def wait_time(self):
        """
        get wait time
        :return: wait time
        """
        return self._wait_time

    @property
    def model_id(self):
        """
        get model id
        :return: model id
        """
        return self._model_id

    @staticmethod
    def _pre_check(*args: list) -> None:
        if len(args) != 7:
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR, "Invalid task time data")

    @model_id.setter
    def model_id(self, model_id: int) -> None:
        """
        set model id
        :param model_id:
        :return:
        """
        self._model_id = model_id

    @wait_time.setter
    def wait_time(self, last_complete_time: int) -> None:
        """
        set wait time
        :param last_complete_time:
        :param is_first_task:
        :return: None
        """
        if last_complete_time == 0 or self._start_time - last_complete_time < 0:
            self._wait_time = 0
        else:
            self._wait_time = self._start_time - last_complete_time

    def construct(self, *args: list) -> object:
        """
        construct task time instance
        :param args:
        :return: Task time instance
        """
        self._pre_check(args)
        self._task_id = args[0]
        self._stream_id = args[1]
        self._start_time = int(float(args[2]))
        self._duration_time = int(float(args[3])) - int(float(args[2]))
        self._index_id = args[5]


class OpSummaryViewModel(ViewModel):
    """
                delete the file in the future
    """

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_AICORE_OP_SUMMARY, [])

    def get_operator_data_by_task_type(self: any, task_type: str) -> list:
        db_path = PathManager.get_db_path(self.result_dir, DBNameConstant.DB_AICORE_OP_SUMMARY)
        if not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_SUMMARY_TASK_TIME,
                                            DBNameConstant.TABLE_SUMMARY_GE):
            return []
        ge_summary_headers = DBManager.get_table_headers(self.cur, DBNameConstant.TABLE_SUMMARY_GE)
        task_time_headers = DBManager.get_table_headers(self.cur, DBNameConstant.TABLE_SUMMARY_TASK_TIME)
        inner_join_condition = ""
        if "model_id" in ge_summary_headers and "model_id" in task_time_headers:
            inner_join_condition += " and a.model_id=b.model_id"
        if "index_id" in ge_summary_headers and "index_id" in task_time_headers:
            inner_join_condition += " and (a.index_id=b.index_id or b.index_id=0)"
        sql = "SELECT start_time, start_time+duration_time as end_time FROM {0} a INNER JOIN {1} b " \
              "on a.stream_id=b.stream_id and a.task_id=b.task_id and a.batch_id=b.batch_id {2} " \
              "and b.task_type=?".format(DBNameConstant.TABLE_SUMMARY_TASK_TIME, DBNameConstant.TABLE_SUMMARY_GE,
                                         inner_join_condition)
        return DBManager.fetch_all_data(self.cur, sql, (task_type,), dto_class=TimeSectionDto)
