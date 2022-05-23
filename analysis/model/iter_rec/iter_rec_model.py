#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to record hwts iter db
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""
import logging
import sqlite3

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from model.interface.parser_model import ParserModel


class HwtsIterModel(ParserModel):
    """
    class used to operate hwts iter db
    """

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

    def get_task_offset_and_sum(self: any, iter_id: int, data_type: str) -> (int, int):
        """
        Get the number of hwts tasks in all previous iterations and the number of tasks in this round of iteration
        :param data_type:
        :param iter_id:
        :return: offset_count is the number of tasks in all previous iterations
        sum_count is the number of tasks in this round of iteration
        """
        return self._get_task_offset(iter_id, data_type), self._get_task_count(iter_id, data_type)

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
        try:
            sql = "select sum(ai_core_num) from {0}".format(DBNameConstant.TABLE_HWTS_ITER_SYS)
            return self.cur.execute(sql).fetchone()[0]
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return Constant.DEFAULT_COUNT
        finally:
            pass

    def get_batch_list(self: any, iter_id: tuple, table_name) -> list:
        """
        get batch list from hwts batch table
        :return: batch list
        """
        sql = "select batch_id from {0} where iter_id>? and iter_id<=?".format(table_name)
        return DBManager.fetch_all_data(self.cur, sql, iter_id)

    def _get_task_count(self: any, iter_id: int, data_type: str) -> (int, int):
        task_count = 0
        sql = "select sum({1}) from {0} where iter_id=?" \
            .format(DBNameConstant.TABLE_HWTS_ITER_SYS, data_type)
        try:
            curr_num = self.cur.execute(sql, (iter_id,)).fetchone()
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return task_count
        if curr_num and curr_num[0]:
            task_count = curr_num[0]
        return task_count

    def _get_task_offset(self: any, iter_id: int, data_type: str) -> (int, int):
        task_offset = 0
        sql = "select sum({1}) from {0} where iter_id<?".format(DBNameConstant.TABLE_HWTS_ITER_SYS, data_type)
        try:
            cur_offset = self.cur.execute(sql, (iter_id,)).fetchone()
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return task_offset
        if cur_offset and cur_offset[0]:
            task_offset = cur_offset[0]
        return task_offset
