#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import logging
import sqlite3
from collections import OrderedDict
from itertools import chain

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_step import MsprofStep
from common_func.path_manager import PathManager
from common_func.utils import Utils
from common_func.empty_class import EmptyClass
from profiling_bean.db_dto.step_trace_dto import StepTraceDto


class MsprofIteration:
    """
    mainly process iteration
    """

    def __init__(self: any, result_dir: str) -> None:
        self._result_dir = result_dir

    @staticmethod
    def _generate_trace_result(trace_datas: list, time_fmt: int = NumberConstant.MICRO_SECOND) -> list:
        trace_datas = list(chain.from_iterable(trace_datas))
        if not trace_datas:
            return []
        trace_datas = [InfoConfReader().time_from_syscnt(timestamp, time_fmt) for timestamp in trace_datas]
        result = [(0, max(trace_datas))] if len(trace_datas) == 1 else [(min(trace_datas), max(trace_datas))]
        return result

    @staticmethod
    def _generate_trace_iter_end_result(trace_datas: list) -> dict:
        iter_end_dict = OrderedDict()
        for trace_data in trace_datas:
            iter_end_dict.setdefault(trace_data[0], trace_data[1])
        return iter_end_dict

    def get_iteration_time(self: any, index_id: int, model_id: int,
                           time_fmt: int = NumberConstant.MICRO_SECOND) -> list:
        """
        get iteration start and end timestamp
        :param index_id: index id
        :param model_id: model id
        :param time_fmt: timestamp format
        :return: iteration list
        """
        if Utils.is_step_scene(self._result_dir):
            return self._generate_trace_result(self.get_step_iteration_time(index_id, model_id), time_fmt)
        return []

    def get_iter_interval(self: any, iter_id: int, model_id: int) -> list:
        trace_conn, trace_curs = DBManager.check_connect_db(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        if not trace_conn or not trace_curs \
                or not DBManager.judge_table_exist(trace_curs, DBNameConstant.TABLE_STEP_TRACE_DATA):
            return []
        trace_sql = "select step_start, step_end from step_trace_data where model_id=? and index_id=?"
        try:
            trace_sql_data = DBManager().fetch_all_data(trace_curs, trace_sql, (model_id, iter_id))[0]
        except sqlite3.Error as trace_err:
            logging.error("Get step trace data failed, "
                          "%s", str(trace_err), exc_info=Constant.TRACE_BACK_SWITCH)
            return []
        finally:
            DBManager.destroy_db_connect(trace_conn, trace_curs)
        if len(trace_sql_data) != 2:
            return []
        return [InfoConfReader().time_from_syscnt(trace_sql_data[0]),
                InfoConfReader().time_from_syscnt(trace_sql_data[1])]

    def get_step_iteration_time(self: any, index_id: int, model_id: int) -> list:
        """
        get step iteration time
        """
        trace_conn, trace_curs = DBManager.check_connect_db(self._result_dir, DBNameConstant.DB_STEP_TRACE)
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

    def get_parallel_iter_range(self: any, index_id: int, model_id: int) -> list:
        """
        get step iteration time
        :param index_id: index id
        :param model_id: model id
        :return: [iter_id - 1, iter_id] or [min_iter_id - 1, max_iter_id] in pytorch graph
        """
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.check_connect_db_path(db_path)
        if not trace_conn or not trace_curs \
                or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_STEP_TRACE_DATA):
            return []

        current_iter = self.get_iteration_info_by_index_id(index_id, model_id)
        if not current_iter:
            return []

        # find first parallel iter
        sql = "select model_id, index_id, iter_id, step_start, step_end from {0} " \
              "where step_end>? and step_end<=? order by step_end".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        first_parallel_iter = DBManager.fetchone(
            trace_curs, sql, (current_iter.step_start, current_iter.step_end, ), dto_class=StepTraceDto)
        DBManager.destroy_db_connect(trace_conn, trace_curs)
        return [first_parallel_iter.iter_id, current_iter.iter_id]

    def get_iter_id_by_index_id(self: any, index_id: int, model_id: int) -> tuple:
        """
        get step iteration time
        :param index_id: index id
        :param model_id: model id
        :return: [iter_id - 1, iter_id] or [min_iter_id - 1, max_iter_id] in pytorch graph
        """
        with MsprofStep(self._result_dir) as step_trace:
            iter_id = step_trace.get_graph_iter_id(index_id, model_id)
            if ProfilingScene().is_mix_operator_and_graph():
                iter_id = step_trace.get_mix_op_iter_id(index_id, model_id)
        if iter_id:
            return iter_id
        return index_id - 1, index_id

    def get_iter_list_with_index_and_model(self: any, index_id: int, model_id: int) -> dict:
        """
        get step iter dict with index and model
        :param index_id: index id
        :param model_id: model id
        :return: {iter_id: [index_id, model_id]} in mix single op and graph
        """
        iter_list = [[index_id, model_id]]
        if not (ProfilingScene().is_mix_operator_and_graph() and model_id == Constant.GE_OP_MODEL_ID):
            return iter_list

        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.check_connect_db_path(db_path)
        if not trace_conn or not trace_curs \
                or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_STEP_TRACE_DATA):
            return []
        iter_range = self.get_parallel_iter_range(index_id, model_id)
        if not iter_range:
            return iter_list
        sql = "select model_id, index_id, iter_id, step_start, step_end from {0} " \
              "where iter_id>=? and iter_id<=?".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        parallel_iter_info_list = DBManager.fetch_all_data(
            trace_curs, sql, iter_range, dto_class=StepTraceDto)

        return [[parallel_iter_info.index_id, parallel_iter_info.model_id]
                for parallel_iter_info in parallel_iter_info_list]

    def get_op_iteration_dict(self: any) -> dict:
        """
        get iteration end dict where model id is 4294967295 in mix op and graph scene
        :return:
        """
        with MsprofStep(self._result_dir) as step_trace:
            return step_trace.get_op_iteration_dict()

    def get_iteration_end_dict(self: any) -> dict:
        """
        get iteration end timestamp
        """
        if Utils.is_step_scene(self._result_dir):
            return self.__get_trace_iteration_end()
        return {}

    def get_iteration_info_by_index_id(self: any, index_id: int, model_id: int) -> any:
        """
        get iteration info by index_id
        """
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.check_connect_db(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        if not trace_conn or not trace_curs \
                or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_STEP_TRACE_DATA):
            return EmptyClass()
        sql = "select model_id, index_id, iter_id, step_start, step_end from {0} " \
              "where model_id={1} and index_id={2}".format(DBNameConstant.TABLE_STEP_TRACE_DATA, model_id, index_id)
        iter_info = DBManager.fetchone(trace_curs, sql, dto_class=StepTraceDto)
        DBManager.destroy_db_connect(trace_conn, trace_curs)
        return iter_info

    def get_condition_within_iteration(self: any, index_id: int, model_id: int, time_start_key: str, time_end_key: str):
        """
        get the condition for sql that data should be within iteration_id.
        """
        iter_range = self.get_iteration_time(index_id, model_id, time_fmt=NumberConstant.NANO_SECOND)
        if not iter_range:
            return ''
        iter_start, iter_end = iter_range[0]
        return f'where ({time_start_key}>={iter_start} and {time_start_key}<={iter_end}) ' \
               f'or ({time_start_key}<={iter_start} and {iter_start}<={time_end_key})'

    def _get_iteration_time(self: any, trace_curs: any, index_id: int, model_id: int) -> list:
        current_iter = self.get_iteration_info_by_index_id(index_id, model_id)
        if not current_iter:
            return []

        # find last and not parallel iter
        sql = "select step_end from {0} " \
              "where step_end<? order by step_end desc ".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        last_not_parallel_iter = DBManager.fetchone(
            trace_curs, sql, (current_iter.step_start, ), dto_class=StepTraceDto)

        if not last_not_parallel_iter:
            return [NumberConstant.ZERO_ITER_END, (current_iter.step_end, )]
        return [(last_not_parallel_iter.step_end, ), (current_iter.step_end, )]

    def __get_trace_iteration_end(self: any) -> dict:
        iter_end_dict = OrderedDict()
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.check_connect_db(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        if not trace_conn or not trace_curs \
                or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_STEP_TRACE_DATA):
            return iter_end_dict
        sql = "select iter_id, step_end from {0} " \
              "order by step_end".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        trace_datas = DBManager.fetch_all_data(trace_curs, sql)
        DBManager.destroy_db_connect(trace_conn, trace_curs)
        if not trace_datas:
            return iter_end_dict
        return MsprofIteration._generate_trace_iter_end_result(trace_datas)
