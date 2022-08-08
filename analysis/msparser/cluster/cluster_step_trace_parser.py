# !/usr/bin/python
# coding=utf-8
"""
This script is used to parse step trace data for cluster.
Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
"""
import logging
import os
from itertools import chain

from common_func.common import error
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
    """
    Step trace data parser for cluster scene.
    """
    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any, collection_path: str) -> None:
        self.collection_path = collection_path
        self.id_with_project_path_map = {}
        self.id_with_table_map = {}
        self.cluster_model = None

    def ms_run(self: any) -> None:
        if not self._check_collection_path_valid():
            logging.error("The input dir doesn't have cluster database, please check.")
            error(ClusterStepTraceParser.FILE_NAME,
                  "The input dir doesn't have cluster database, please check.")
            return
        self.parse()
        self.save()

    def parse(self: any) -> None:
        if not self._collect_project_paths():
            error(ClusterStepTraceParser.FILE_NAME,
                  "The cluster step trace parsing failed.")

    def save(self: any) -> None:
        if not self.id_with_project_path_map:
            return
        if not self._init_cluster_step_trace_model():
            return
        self._collect_and_save_step_trace_data()
        self.cluster_model.finalize()

    def _check_collection_path_valid(self: any) -> bool:
        db_path = PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_RANK)
        return os.path.exists(db_path)

    def _init_cluster_step_trace_model(self: any) -> bool:
        rank_ids = list(self.id_with_project_path_map.keys())
        for rank_id in rank_ids:
            self.id_with_table_map.setdefault(rank_id, "step_trace_{}".format(rank_id))
        self.cluster_model = ClusterStepTraceModel(self.collection_path, DBNameConstant.DB_CLUSTER_STEP_TRACE,
                                                   self.id_with_table_map.values())
        if not self.cluster_model.init():
            logging.error("The cluster step trace datebase initialization is failed.")
            return False
        self.cluster_model.create_table()
        return True

    def _collect_project_paths(self: any) -> bool:
        rank_db_path = PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_RANK)
        conn, curs = DBManager.check_connect_db_path(rank_db_path)
        if not conn or not curs or not DBManager.check_tables_in_db(rank_db_path, DBNameConstant.TABLE_CLUSTER_RANK):
            DBManager.destroy_db_connect(conn, curs)
            logging.error("The cluster rank table doesn't exist.")
            return False
        sql = "select rank_id, dir_name from {}".format(DBNameConstant.TABLE_CLUSTER_RANK)
        data = DBManager.fetch_all_data(curs, sql)
        DBManager.destroy_db_connect(conn, curs)
        if not data:
            logging.error("The query cluster rank info is invalid.")
            return False
        # sort by rank_id
        data.sort(key=lambda x: x[0])
        self.id_with_project_path_map = dict(data)
        return True

    def _collect_and_save_step_trace_data(self: any) -> None:
        for rank_id, path in self.id_with_project_path_map.items():
            project_path = os.path.join(self.collection_path, path)
            sub_dirs = get_path_dir(project_path)
            for sub_dir in sub_dirs:
                project_sub_path = os.path.join(project_path, sub_dir)
                if not DataCheckManager.contain_info_json_data(project_sub_path):
                    continue
                InfoConfReader().load_info(project_sub_path)
                if InfoConfReader().is_host_profiling():
                    continue
                step_trace_data = self._collect_step_trace_data(project_sub_path)
                if not step_trace_data:
                    continue
                self.cluster_model.insert_data_to_db(self.id_with_table_map.get(rank_id), step_trace_data)

    def _collect_step_trace_data(self: any, project_path: str) -> list:
        step_trace_data = []
        db_path_ge = PathManager.get_db_path(project_path, DBNameConstant.DB_GE_INFO)
        conn_ge, curs_ge = DBManager.check_connect_db_path(db_path_ge)
        if not conn_ge or not curs_ge or not DBManager.check_tables_in_db(db_path_ge, DBNameConstant.TABLE_GE_TASK):
            DBManager.destroy_db_connect(conn_ge, curs_ge)
            logging.error("Can't find ge task data table.")
            return step_trace_data
        db_path = PathManager.get_db_path(project_path, DBNameConstant.DB_TRACE)
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not conn or not curs or not DBManager.check_tables_in_db(db_path, DBNameConstant.TABLE_TRAINING_TRACE):
            DBManager.destroy_db_connect(conn_ge, curs_ge)
            DBManager.destroy_db_connect(conn, curs)
            logging.error("Can't find step trace data table")
            return step_trace_data
        model_ids = self._search_ge_model_id(curs_ge)
        step_trace_data = self._search_step_trace(curs)
        DBManager.destroy_db_connect(conn_ge, curs_ge)
        DBManager.destroy_db_connect(conn, curs)
        if not step_trace_data:
            logging.error("Can't get step trace data.")
            return step_trace_data
        step_trace_data = self._append_ge_model_tag(step_trace_data, model_ids)
        return step_trace_data

    def _search_ge_model_id(self: any, curs: any) -> list:
        sql = "select distinct model_id from {}".format(DBNameConstant.TABLE_GE_TASK)
        model_ids = DBManager.fetch_all_data(curs, sql)
        model_ids = list(chain.from_iterable(model_ids))
        return model_ids

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

    def _append_ge_model_tag(self: any, step_trace_data: list, model_ids: list) -> list:
        result = []
        for item in step_trace_data:
            item = list(item)
            if item[1] in model_ids:
                item.append(1)
            else:
                item.append(0)
            result.append(item)
        return result