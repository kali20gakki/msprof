#!/usr/bin/python3
# coding=utf-8
"""
Function:
This file mainly involves iteration.
Copyright Information:
Huawei Technologies Co., Ltd. All Rights Reserved © 2021
"""
import logging
import sqlite3
from collections import OrderedDict

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from common_func.utils import Utils


class MsprofIteration:
    """
    mainly process iteration
    """

    def __init__(self: any, result_dir: str) -> None:
        self._result_dir = result_dir

    def get_iteration_time(self: any, index_id: int, model_id: int) -> list:
        """
        get iteration start and end timestamp
        :param index_id: index id
        :param model_id: model id
        :return: iteration list
        """
        if Utils.is_step_scene(self._result_dir):
            return self._generate_trace_result(self.get_step_iteration_time(index_id, model_id))
        if Utils.is_training_trace_scene(self._result_dir):
            return self._generate_trace_result(self.get_train_iteration_time(index_id))
        return []

    @staticmethod
    def _get_iteration_time(trace_curs: any, index_id: int, model_id: int) -> list:
        sql = "select iter_id from {0} " \
              "where index_id=? and model_id=?".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        iter_id = trace_curs.execute(sql, (index_id, model_id)).fetchone()
        if not iter_id:
            return []

        sql = "select step_end from {0} " \
              "where iter_id<=? and iter_id>=? order by " \
              "step_end".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        trace_data = DBManager.fetch_all_data(trace_curs, sql, (iter_id[0], iter_id[0] - 1))
        if not trace_data:
            return []
        return trace_data

    def get_step_iteration_time(self: any, index_id: int, model_id: int) -> list:
        """
        get step iteration time
        """
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.create_connect_db(db_path)
        if not trace_conn or not trace_curs \
                or not DBManager.judge_table_exist(trace_curs, DBNameConstant.TABLE_STEP_TRACE_DATA):
            return []
        try:
            trace_data = self._get_iteration_time(trace_curs, index_id, model_id)
            return trace_data
        except sqlite3.Error as trace_err:
            logging.error("Get step trace data failed, "
                          "%s", str(trace_err), exc_info=Constant.TRACE_BACK_SWITCH)
            return []
        finally:
            DBManager.destroy_db_connect(trace_conn, trace_curs)

    @staticmethod
    def _generate_trace_result(trace_datas: list) -> list:
        if not trace_datas:
            return []
        result = [(trace_datas[0][0], trace_datas[1][0])] if len(trace_datas) == 2 else [(0, trace_datas[0][0])]
        result = [(InfoConfReader().time_from_syscnt(result[0][0], NumberConstant.MICRO_SECOND),
                   InfoConfReader().time_from_syscnt(result[0][1], NumberConstant.MICRO_SECOND))]
        return result

    def get_train_iteration_time(self: any, iteration_id: int) -> list:
        """
        get train iteration time
        """
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_TRACE)
        trace_conn, trace_curs = DBManager.check_connect_db_path(db_path)
        if not trace_conn or not trace_curs:
            return []
        iter_id_before = iteration_id - 1
        sql = "select iteration_end from {0} where (iteration_id=? or iteration_id=?)" \
              "order by iteration_end". \
            format(DBNameConstant.TABLE_TRAINING_TRACE)
        trace_datas = DBManager.fetch_all_data(trace_curs, sql, (iteration_id, iter_id_before))
        DBManager.destroy_db_connect(trace_conn, trace_curs)
        if not trace_datas:
            return []
        return trace_datas

    def get_iteration_id_by_index_id(self: any, index_id: int, model_id: int) -> int:
        """
        get step iteration time
        :param index_id: index id
        :param model_id: model id
        :return: index id
        """
        if not Utils.is_step_scene(self._result_dir):
            return index_id
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.check_connect_db_path(db_path)
        if not trace_conn or not trace_curs \
                or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_STEP_TRACE_DATA):
            return index_id
        sql = "select iter_id from {0} " \
              "where index_id=? and model_id=?".format(DBNameConstant.TABLE_STEP_TRACE_DATA)

        iteration_ids = DBManager.fetch_all_data(trace_curs, sql, (index_id, model_id))
        DBManager.destroy_db_connect(trace_conn, trace_curs)
        if not iteration_ids:
            return index_id
        return iteration_ids[0][0]

    @staticmethod
    def _generate_trace_iter_end_result(trace_datas: list) -> dict:
        iter_end_dict = OrderedDict()
        for trace_data in trace_datas:
            iter_end_dict.setdefault(trace_data[0], trace_data[1])
        return iter_end_dict

    def __get_trace_iteration_end(self: any) -> dict:
        iter_end_dict = OrderedDict()
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.create_connect_db(db_path)
        if not trace_conn or not trace_curs \
                or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_STEP_TRACE_DATA):
            return iter_end_dict
        sql = "select iter_id, step_end from {0} " \
              "order by step_start".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        trace_datas = DBManager.fetch_all_data(trace_curs, sql)
        DBManager.destroy_db_connect(trace_conn, trace_curs)
        if not trace_datas:
            return iter_end_dict
        return MsprofIteration._generate_trace_iter_end_result(trace_datas)

    def __get_iter_by_training_trace(self: any) -> dict:
        iter_end_dict = OrderedDict()
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_TRACE)
        trace_conn, trace_curs = DBManager.create_connect_db(db_path)
        if not trace_conn or not trace_curs \
                or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_TRAINING_TRACE):
            return iter_end_dict
        sql = "select iteration_id, iteration_end from {0} order by iteration_id".format(
            DBNameConstant.TABLE_TRAINING_TRACE)
        trace_datas = DBManager.fetch_all_data(trace_curs, sql)
        DBManager.destroy_db_connect(trace_conn, trace_curs)
        if not trace_datas:
            return iter_end_dict
        return MsprofIteration._generate_trace_iter_end_result(trace_datas)

    def get_iteration_end_dict(self: any) -> dict:
        """
        get iteration start and end timestamp
        """
        if Utils.is_step_scene(self._result_dir):
            return self.__get_trace_iteration_end()
        if Utils.is_training_trace_scene(self._result_dir):
            return self.__get_iter_by_training_trace()
        return {}
