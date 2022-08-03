# !/usr/bin/python
# coding=utf-8
"""
Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
"""

import logging
import os

from common_func.data_check_manager import DataCheckManager
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_common import get_path_dir
from common_func.path_manager import PathManager
from common_func.step_trace_constant import StepTraceConstant
from msmodel.step_trace.cluster_step_trace_model import ClusterStepTraceModel
from msparser.interface.iparser import IParser


class ClusterStepTraceParser(IParser):

    def __init__(self: any, collection_path: str) -> None:
        self.collection_path = collection_path
        self.id_with_project_path_map = {}
        self.id_with_table_map = {}
        self.cluster_model = None

    def ms_run(self):
        if DBNameConstant.DB_CLUSTER_RANK not in os.listdir(os.path.join(self.collection_path, PathManager.SQLITE)):
            logging.error("no valid cluster data")
            return
        self.parse()
        self.save()

    def parse(self: any) -> None:
        """
        parse the data under the collection_path
        :return: NA
        """
        self._collect_project_paths()

    def save(self: any) -> None:
        """
        save the result of data parsing
        :return: NA
        """
        if not self._init_database_model():
            return
        self._collect_and_save_step_trace_data()
        self.cluster_model.finalize()

    def _init_database_model(self) -> bool:
        if not self.id_with_project_path_map:
            return False
        rank_ids = list(self.id_with_project_path_map.keys())
        for rank_id in rank_ids:
            self.id_with_table_map.setdefault(rank_id, "step_trace_{}".format(rank_id))
            self.cluster_model = ClusterStepTraceModel(self.collection_path, DBNameConstant.DB_CLUSTER_STEP_TRACE,
                                                       self.id_with_table_map.values())
        if not self.cluster_model.init():
            return False
        self.cluster_model.create_table()
        return True

    def _collect_project_paths(self: any) -> None:
        rank_db_path = PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_RANK)
        conn, curs = DBManager.check_connect_db_path(rank_db_path)
        if not conn or not curs or not DBManager.check_tables_in_db(rank_db_path, DBNameConstant.TABLE_CLUSTER_RANK):
            DBManager.destroy_db_connect(conn, curs)
            logging.error("no cluster table")
            return
        sql = "select rank_id, dir_name from {}".format(DBNameConstant.TABLE_CLUSTER_RANK)
        data = DBManager.fetch_all_data(curs, sql)
        DBManager.destroy_db_connect(conn, curs)
        if not data:
            logging.error("no cluster data")
            return
        data.sort(key=lambda x: x[0])
        self.id_with_project_path_map = dict(data)

    def _collect_and_save_step_trace_data(self: any):
        for rank_id, path in self.id_with_project_path_map.items():
            project_path = os.path.join(self.collection_path, path)
            dirs = get_path_dir(project_path)
            for folder in dirs:
                project_path = os.path.join(project_path, folder)
                if not DataCheckManager.contain_info_json_data(project_path):
                    continue
                InfoConfReader().load_info(project_path)
                if InfoConfReader().is_host_profiling():
                    continue
                step_trace_data = self._search_step_trace_data(project_path)
                if not step_trace_data:
                    continue
                self.cluster_model.insert_data_to_db(self.id_with_table_map.get(rank_id), step_trace_data)

    def _search_step_trace_data(self: any, project_path: str) -> list:
        data = []
        db_path_ge = PathManager.get_db_path(project_path, DBNameConstant.DB_GE_INFO)
        conn_ge, curs_ge = DBManager.check_connect_db_path(db_path_ge)
        if not conn_ge or not curs_ge or not DBManager.check_tables_in_db(
                db_path_ge, DBNameConstant.TABLE_GE_TASK):
            DBManager.destroy_db_connect(conn_ge, curs_ge)
            logging.error("missing ge table")
            return data
        db_path = PathManager.get_db_path(project_path, DBNameConstant.DB_TRACE)
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not conn or not curs or not DBManager.check_tables_in_db(
                db_path, DBNameConstant.TABLE_TRAINING_TRACE):
            DBManager.destroy_db_connect(conn_ge, curs_ge)
            DBManager.destroy_db_connect(conn, curs)
            logging.error("missing step trace table")
            return data
        model_id_set = self._search_ge_model_id(curs_ge)
        step_trace_data = self._search_step_trace(curs)
        DBManager.destroy_db_connect(conn_ge, curs_ge)
        DBManager.destroy_db_connect(conn, curs)
        if not model_id_set or not step_trace_data:
            logging.error("missing data")
            return data
        step_trace_data = self._append_ge_model_tag(step_trace_data, model_id_set)
        return step_trace_data

    def _search_ge_model_id(self: any, curs: any) -> set:
        sql = "select distinct model_id from {}".format(DBNameConstant.TABLE_GE_TASK)
        data = DBManager.fetch_all_data(curs, sql)
        model_id_set = {model_id[0] for model_id in data}
        return model_id_set

    def _search_step_trace(self: any, curs: any) -> list:
        sql = "select device_id, " \
              "model_id, " \
              "iteration_id, " \
              "(case when FP_start={1} then {1} else FP_start*{2} end), " \
              "(case when BP_end={1} then {1} else BP_end*{2} end), " \
              "(case when iteration_end={1} then {1} else iteration_end*{2} end), " \
              "(case when iteration_time={1} then {1} else iteration_time*{2} end), " \
              "(case when fp_bp_time={1} then {1} else fp_bp_time*{2} end), " \
              "(case when grad_refresh_bound={1} then {1} else grad_refresh_bound*{2} end), " \
              "(case when data_aug_bound={1} then {1} else data_aug_bound*{2} end) " \
              "from {3}".format(
            NumberConstant.DEFAULT_MODEL_ID,
            NumberConstant.NULL_NUMBER,
            StepTraceConstant.syscnt_to_micro(),
            DBNameConstant.TABLE_TRAINING_TRACE)
        data = DBManager.fetch_all_data(curs, sql)
        return data

    def _append_ge_model_tag(self: any, data: list, model_id_set: set) -> list:
        result = []
        for item in data:
            item = list(item)
            if item[1] in model_id_set:
                item.append(1)
            else:
                item.append(0)
            result.append(item)
        return result