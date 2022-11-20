#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.

import logging
import os
import sqlite3
from collections import defaultdict

from config.config_manager import ConfigManager
from analyzer.get_op_table_task_time import GetOpTableTsTime
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.common import CommonConstant
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_exception import ProfException
from common_func.msprof_iteration import MsprofIteration
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from viewer.ge_info_report import get_ge_model_name_dict
from profiling_bean.db_dto.ge_task_dto import GeTaskDto


class MergeOPCounter:
    """
    class to merge GE DB and runtime tasktime data
    """
    TABLE_PATH = ConfigManager.TABLES
    MODEL_NAME_INDEX = 8
    MODEL_ID_INDEX = 7

    def __init__(self: any, sample_config: dict) -> None:
        self.sample_config = sample_config
        self.iter_id = self.sample_config.get("iter_id")
        self.model_id = self.sample_config.get("model_id")
        self.device_id = self.sample_config.get("device_id")
        self.project_path = self.sample_config.get("result_dir")
        self.sql_path = None
        self.conn = None
        self.curs = None

    @staticmethod
    def _get_op_report_sql() -> str:
        # ge or subtask need modify the context_id or subtask_id so that it should be same.
        sql = "select op_type, {0}.task_type, count(op_type), sum(duration) as total_time, " \
              "min(duration) as min, sum(duration)/count(op_type) as avg, " \
              "max(duration) as max, {0}.model_id from {0}, {1} " \
              "where {0}.task_id={1}.task_id and {0}.stream_id={1}.stream_id " \
              "and {0}.batch_id={1}.batch_id " \
              "and ({0}.context_id={1}.subtask_id or ({0}.context_id={context_id} and subtask_id={subtask_id})) " \
              "group by op_type,{0}.task_type" \
            .format(CommonConstant.GE_TASK_MEGED_TABLE, CommonConstant.RTS_TASK_TABLE,
                    context_id=NumberConstant.DEFAULT_GE_CONTEXT_ID,
                    subtask_id=NumberConstant.DEFAULT_FFTS_SUBTASK_ID)
        return sql

    @staticmethod
    def _cal_total(type_time: dict) -> dict:
        """
        calculate total time for each device
        :param type_time: {"model":{"op":{'task_type': 0,'count': 0,'duration': 0,
        'min': 0,'avg': 0,max': 0}}}
        :return: total time
        """
        total_time = defaultdict(int)
        for model in type_time:
            for ops in type_time[model].values():
                total_time[model] += ops.get("duration", 0)
        return total_time

    def create_db(self: any, map_path: str) -> None:
        """
        analysis and create db for op statics
        :param map_path: table config file path
        :return: connection of op statics
        """
        self.conn, self.curs = DBManager.create_connect_db(
            os.path.join(self.sql_path, DBNameConstant.DB_OP_COUNTER))
        if not self.conn or not self.curs:
            logging.error("unable to create op counter db connection")
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        ge_create_sql = DBManager.sql_create_general_table("GeMergeMap", DBNameConstant.TABLE_OP_COUNTER_GE_MERGE,
                                                           map_path)
        DBManager.execute_sql(self.conn, ge_create_sql)

        rts_task_create_sql = DBManager.sql_create_general_table("RtsTaskMap",
                                                                 DBNameConstant.TABLE_OP_COUNTER_RTS_TASK,
                                                                 map_path)
        DBManager.execute_sql(self.conn, rts_task_create_sql)

        op_report_create_sql = DBManager.sql_create_general_table("OpReportMap",
                                                                  DBNameConstant.TABLE_OP_COUNTER_OP_REPORT,
                                                                  map_path)
        DBManager.execute_sql(self.conn, op_report_create_sql)

    def create_and_insert_db(self: any) -> None:
        if not self._is_db_need_to_create():
            logging.warning("No need to create db for op counter, "
                            "maybe the data of framework or task is not collected.")
            return
        map_path = \
            self.TABLE_PATH if ProfilingScene().is_step_trace() else ConfigManager.TABLES_TRAINING
        self.create_db(map_path)
        self._create_ge_merge()
        self._create_task()
        self._create_report()
        DBManager.destroy_db_connect(self.conn, self.curs)

    def run(self: any) -> None:
        """
        merge process
        :return: None
        """
        try:
            self._init_params()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self.create_and_insert_db()

    def _is_db_need_to_create(self: any) -> bool:
        ge_db_path = PathManager.get_db_path(self.project_path, DBNameConstant.DB_GE_INFO)
        if DBManager.check_tables_in_db(ge_db_path, DBNameConstant.TABLE_GE_TASK):
            return True
        if ChipManager().is_chip_v1():
            return DBManager.check_tables_in_db(PathManager.get_db_path(self.project_path, DBNameConstant.DB_RUNTIME),
                                                DBNameConstant.TABLE_RUNTIME_TASK_TIME)
        return DBManager.check_tables_in_db(PathManager.get_db_path(self.project_path, DBNameConstant.DB_HWTS),
                                            DBNameConstant.TABLE_HWTS_TASK_TIME)

    def _init_params(self: any) -> None:
        """
        initial and check params
        :return: None
        """
        if self.iter_id and str(self.iter_id).isdigit():
            self.iter_id = int(self.iter_id)
        self.sql_path = self.sample_config.get('sql_path', self._get_sql_dir())
        if not os.path.exists(self.sql_path):
            logging.error("Failed to get sqlite path.")
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        if os.path.exists(os.path.join(self.sql_path, DBNameConstant.DB_OP_COUNTER)):
            os.remove(os.path.join(self.sql_path, DBNameConstant.DB_OP_COUNTER))

    def _get_ge_data(self: any, ge_curs: any) -> list:
        ge_data = []
        iter_list = MsprofIteration(self.project_path).get_iter_list_with_index_and_model(self.iter_id, self.model_id)
        ge_sql = 'select model_id, op_name, op_type, task_type, task_id, stream_id, batch_id, context_id ' \
                 'from {0} where (index_id=? or index_id=0) ' \
                 'and model_id=?'.format(DBNameConstant.TABLE_GE_TASK)
        for index_and_model in iter_list:
            ge_data.extend(DBManager.fetch_all_data(ge_curs, ge_sql, index_and_model))

        return ge_data

    def _get_ge_data_from_merge_task(self) -> list:
        if not DBManager.judge_table_exist(self.curs, DBNameConstant.TABLE_OP_COUNTER_GE_MERGE):
            return []
        ge_sql = "SELECT task_type, stream_id, task_id, batch_id, context_id from {0}".format(
            DBNameConstant.TABLE_OP_COUNTER_GE_MERGE)
        return DBManager.fetch_all_data(self.curs, ge_sql, dto_class=GeTaskDto)

    def _create_ge_merge(self: any) -> None:
        """
        merge GE ge_task_data、ge_graph_data into merged db
        :return: None
        """
        ge_conn, ge_curs = DBManager.check_connect_db_path(
            os.path.join(self.sql_path, DBNameConstant.DB_GE_INFO))
        if ge_conn and ge_curs and DBManager.judge_table_exist(ge_curs, DBNameConstant.TABLE_GE_TASK):
            ge_data = self._get_ge_data(ge_curs)
            if ge_data:
                insert_sql = "insert into {} values({value})".format(CommonConstant.GE_TASK_MEGED_TABLE,
                                                                     value='?,' * (len(ge_data[0]) - 1) + '?')
                DBManager.executemany_sql(self.conn, insert_sql, ge_data)
        DBManager.destroy_db_connect(ge_conn, ge_curs)

    def _create_task(self: any) -> None:
        """
        insert data to task time table
        :return:
        """
        ge_data = self._get_ge_data_from_merge_task()
        rts_data = GetOpTableTsTime(self.sample_config).get_task_time_data(ge_data)
        try:
            DBManager.insert_data_into_table(self.conn, CommonConstant.RTS_TASK_TABLE, rts_data)
        except sqlite3.Error as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass

    def _get_sql_dir(self: any) -> str:
        return PathManager.get_sql_dir(self.project_path)

    def _update_model_name(self: any, task_data: list) -> list:
        model_name_dict = get_ge_model_name_dict(self.project_path)
        result_data = [0] * len(task_data)
        for index, _task_data in enumerate(task_data):
            _task_data = list(_task_data)
            model_name = model_name_dict.get(_task_data[self.MODEL_ID_INDEX], Constant.NA)
            _task_data.insert(self.MODEL_NAME_INDEX, model_name)
            result_data[index] = _task_data
        return result_data

    def _create_report(self: any) -> None:
        """
        create report table
        :return: None
        """
        sql = self._get_op_report_sql()
        task_data = DBManager.fetch_all_data(self.curs, sql)
        if not task_data:
            return
        task_data = self._update_model_name(task_data)
        type_time = defaultdict(dict)
        for task in task_data:
            type_time[task[self.MODEL_NAME_INDEX]]["{}_{}".format(task[0], task[1])] = {
                    'op_type': task[0], 'task_type': task[1], 'count': task[2], 'duration': task[3],
                    'min': task[4], 'avg': task[5], 'max': task[6]
            }
        total_time = self._cal_total(type_time)
        total_data = []
        for model in type_time:
            for op_type in type_time[model]:
                task_data = type_time[model][op_type]
                try:
                    task_duration = round(float(task_data["duration"] / total_time[model] * 100),
                                          NumberConstant.DECIMAL_ACCURACY)
                except ZeroDivisionError:
                    task_duration = 0
                total_data.append(
                    (model,
                     task_data['op_type'],
                     str(task_data['task_type']),
                     task_data["count"],
                     round(float(task_data["duration"]), NumberConstant.DECIMAL_ACCURACY),
                     round(float(task_data["min"]), NumberConstant.DECIMAL_ACCURACY),
                     round(float(task_data["avg"]), NumberConstant.DECIMAL_ACCURACY),
                     round(float(task_data["max"]), NumberConstant.DECIMAL_ACCURACY),
                     task_duration))
        if total_data:
            sorted_total_data = sorted(total_data, key=lambda x: x[7], reverse=True)
            sql = 'insert into {} values({})'.format(CommonConstant.OP_REPORT_TABLE,
                                                     '?,' * (len(sorted_total_data[0]) - 1) + '?')
            DBManager.executemany_sql(self.conn, sql, sorted_total_data)
