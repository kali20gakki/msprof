#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import logging
import sqlite3

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from msmodel.interface.parser_model import ParserModel
from msmodel.interface.view_model import ViewModel
from profiling_bean.db_dto.time_section_dto import TimeSectionDto


class HwtsIterModel(ParserModel):
    """
    class used to operate hwts iter db
    """

    AI_CORE_TYPE = 'ai_core'
    TASK_TYPE = "task"

    def __init__(self: any, result_dir: str) -> None:
        super().__init__(result_dir, DBNameConstant.DB_HWTS_REC, [DBNameConstant.TABLE_HWTS_ITER_SYS,
                                                                  DBNameConstant.TABLE_HWTS_BATCH])

    def flush(self: any, data_list: list, table_name: str) -> None:
        """
        flush data to db
        :param data_list:
        :return:
        """
        self.insert_data_to_db(table_name, data_list)

    def get_task_offset_and_sum(self: any, model_id: int, index_id: int, data_type: str) -> (int, int):
        """
        Get the number of hwts tasks in all previous iterations and the number of tasks in this round of iteration
        :param data_type:
        :param model_id:
        :param index_id:
        :return: offset_count is the number of tasks in all previous iterations
        sum_count is the number of tasks in this round of iteration
        """
        offset_count_dict = {
            self.AI_CORE_TYPE: "select ai_core_offset, ai_core_num from {0} "
                               "where model_id=? and index_id=?".format(
                DBNameConstant.TABLE_HWTS_ITER_SYS),
            self.TASK_TYPE: "select task_offset, task_count from {0} "
                            "where model_id=? and index_id=?".format(
                DBNameConstant.TABLE_HWTS_ITER_SYS)
        }

        return self._get_task_num(model_id, index_id, offset_count_dict.get(data_type))

    def check_table(self: any) -> bool:
        """
        check whether the table exists.
        :return: exits or not
        """
        if not self.conn or not self.cur \
                or not DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_HWTS_ITER_SYS):
            return False
        return True

    def get_aic_sum_count(self: any) -> int:
        """
        get all aic count
        :return: sum of aic count
        """
        sql = "select max(ai_core_num+ai_core_offset) from {0}".format(
            DBNameConstant.TABLE_HWTS_ITER_SYS)
        try:
            all_aic_num = self.cur.execute(sql).fetchone()[0]
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return Constant.DEFAULT_COUNT
        if all_aic_num is None:
            return Constant.DEFAULT_COUNT
        return all_aic_num

    def get_batch_list(self: any, table_name, iter_range: list) -> list:
        """
        get batch list from hwts batch table
        :return: batch list
        """
        sql = "select batch_id from {0} where iter_id>=? and iter_id<=?".format(table_name)
        return DBManager.fetch_all_data(self.cur, sql, iter_range)

    def _get_task_num(self: any, model_id: int, index_id: int, sql: str) -> (int, int):
        try:
            task_num = self.cur.execute(sql, (model_id, index_id,)).fetchone()
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return (0, 0)
        if task_num and len(task_num) == 2:
            return task_num
        return (0, 0)


class HwtsIterViewModel(ViewModel):
    def __init__(self: any, path: str) -> None:
        super().__init__(path, DBNameConstant.DB_HWTS_REC, [])

    def get_ai_core_op_data(self: any) -> list:
        sql = "select start_time, end_time from {} where is_ai_core=1 and start_time<>{}".format(
            DBNameConstant.TABLE_HWTS_BATCH, NumberConstant.INVALID_OP_EXE_TIME)
        return DBManager.fetch_all_data(self.cur, sql, dto_class=TimeSectionDto)
