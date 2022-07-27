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

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_step import MsprofStep
from common_func.path_manager import PathManager
from common_func.utils import Utils


class MsprofIteration:
    """
    mainly process iteration
    """

    def __init__(self: any, result_dir: str) -> None:
        self._result_dir = result_dir

    @staticmethod
    def _generate_trace_result(trace_datas: list) -> list:
        if not trace_datas:
            return []
        result = [(trace_datas[0][0], trace_datas[1][0])] if len(trace_datas) == 2 else [(0, trace_datas[0][0])]
        result = [(InfoConfReader().time_from_syscnt(result[0][0], NumberConstant.MICRO_SECOND),
                   InfoConfReader().time_from_syscnt(result[0][1], NumberConstant.MICRO_SECOND))]
        return result

    @staticmethod
    def _generate_trace_iter_end_result(trace_datas: list) -> dict:
        iter_end_dict = OrderedDict()
        for trace_data in trace_datas:
            iter_end_dict.setdefault(trace_data[0], trace_data[1])
        return iter_end_dict

    def get_iteration_time(self: any, index_id: int, model_id: int) -> list:
        """
        get iteration start and end timestamp
        :param index_id: index id
        :param model_id: model id
        :return: iteration list
        """
        if Utils.is_step_scene(self._result_dir):
            return self._generate_trace_result(self.get_step_iteration_time(index_id, model_id))
        return []

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

    def get_iteration_id_by_index_id(self: any, index_id: int, model_id: int) -> list:
        """
        get step iteration time
        :param index_id: index id
        :param model_id: model id
        :return: [iter_id - 1, iter_id] or [min_iter_id - 1, max_iter_id] in pytorch graph
        """
        iter_id_list = [index_id - 1, index_id]
        if not Utils.is_step_scene(self._result_dir):
            return iter_id_list
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.check_connect_db_path(db_path)
        if not trace_conn or not trace_curs \
                or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_STEP_TRACE_DATA):
            return iter_id_list
        sql = "select iter_id from {0} " \
              "where index_id=? and model_id=?".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        iter_data = DBManager.fetch_all_data(trace_curs, sql, (index_id, model_id))
        iter_id_list = [iter_data[0][0] - 1, iter_data[0][0]] if iter_data else iter_id_list
        if Utils.need_all_model_in_one_iter(self._result_dir, model_id):
            sql = "select iter_id from {0} " \
                  "where index_id>=? and index_id<=? and model_id=?".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
            iter_data = DBManager.fetch_all_data(trace_curs, sql, (index_id, index_id + 1, model_id))
        DBManager.destroy_db_connect(trace_conn, trace_curs)
        if iter_data and iter_data[0]:
            iter_id_list = [iter_data[0][0] - 1, iter_data[0][0]]
            if len(iter_data) != 1:
                iter_id_list = [iter_data[0][0] - 1, iter_data[1][0] - 1]

        return iter_id_list

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

    def get_iter_dict_with_index_and_model(self: any, index_id: int, model_id: int) -> dict:
        """
        get step iter dict with index and model
        :param index_id: index id
        :param model_id: model id
        :return: {iter_id: [index_id, model_id]} in mix single op and graph
        """
        iter_dict = {index_id: [index_id, model_id]}
        if not Utils.is_step_scene(self._result_dir):
            return iter_dict
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.check_connect_db_path(db_path)
        if not trace_conn or not trace_curs \
                or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_STEP_TRACE_DATA):
            return iter_dict
        sql = "select iter_id, index_id, model_id from {0} " \
              "where index_id=? and model_id=?".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        iter_datas = DBManager.fetch_all_data(trace_curs, sql, (index_id, model_id))
        if Utils.need_all_model_in_one_iter(self._result_dir, model_id):
            iter_id_list = self.get_iteration_id_by_index_id(index_id, model_id)
            sql = "select iter_id, index_id, model_id from {0} " \
                  "where iter_id>? and iter_id<=?".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
            iter_datas = DBManager.fetch_all_data(trace_curs, sql, iter_id_list)
        DBManager.destroy_db_connect(trace_conn, trace_curs)
        if iter_datas and iter_datas[0]:
            iter_dict = {}
            for data in iter_datas:
                iter_dict.update({data[0]: [data[1], data[2]]})
        return iter_dict

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

    def get_iteration_dict(self: any) -> dict:
        """
        get iteration start and end timestamp
        """
        if ProfilingScene().is_mix_operator_and_graph():
            return self.get_graph_iteration_dict()
        if Utils.is_step_scene(self._result_dir):
            return self.__get_iteration_dict()
        return {}

    def get_graph_iteration_dict(self: any) -> dict:
        iter_dict = OrderedDict()
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.check_connect_db(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        if not trace_conn or not trace_curs \
                or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_STEP_TRACE_DATA):
            return {}
        sql = "select iter_id, step_start, step_end from {0}  where model_id!=?" \
              "order by step_start".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        trace_datas = DBManager.fetch_all_data(trace_curs, sql, (Constant.GE_OP_MODEL_ID,))
        DBManager.destroy_db_connect(trace_conn, trace_curs)
        if not trace_datas:
            return iter_dict
        for trace_data in trace_datas:
            iter_dict.setdefault(trace_data[0], [trace_data[1], trace_data[2]])
        return iter_dict

    def get_iteration_info_by_index_id(self: any, index_id: int, model_id: int) -> list:
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.check_connect_db(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        if not trace_conn or not trace_curs \
                or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_STEP_TRACE_DATA):
            return []
        sql = "select iter_id, step_start, step_end from {0} " \
              "where model_id={1} and index_id={2}".format(DBNameConstant.TABLE_STEP_TRACE_DATA, model_id, index_id)
        iter_start_end_time = DBManager.fetch_all_data(trace_curs, sql)[0]
        DBManager.destroy_db_connect(trace_conn, trace_curs)
        if not iter_start_end_time:
            return []
        return iter_start_end_time

    def _get_iteration_time(self: any, trace_curs: any, index_id: int, model_id: int) -> list:
        iter_id = self.get_iteration_id_by_index_id(index_id, model_id)
        if not iter_id:
            return []
        sql = "select step_end from {0} " \
              "where iter_id>=? and iter_id<=? order by " \
              "step_end".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        trace_data = DBManager.fetch_all_data(trace_curs, sql, iter_id)
        if not trace_data:
            return []
        return trace_data

    def __get_iteration_dict(self: any) -> dict:
        iter_dict = OrderedDict()
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.check_connect_db(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        if not trace_conn or not trace_curs \
                or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_STEP_TRACE_DATA):
            return {}
        sql = "select iter_id, step_start, step_end from {0} " \
              "order by step_start".format(DBNameConstant.TABLE_STEP_TRACE_DATA)
        trace_datas = DBManager.fetch_all_data(trace_curs, sql)
        DBManager.destroy_db_connect(trace_conn, trace_curs)
        if not trace_datas:
            return iter_dict
        for trace_data in trace_datas:
            iter_dict.setdefault(trace_data[0], [trace_data[1], trace_data[2]])
        return iter_dict

    def __get_trace_iteration_end(self: any) -> dict:
        iter_end_dict = OrderedDict()
        db_path = PathManager.get_db_path(self._result_dir, DBNameConstant.DB_STEP_TRACE)
        trace_conn, trace_curs = DBManager.check_connect_db(self._result_dir, DBNameConstant.DB_STEP_TRACE)
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