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
from common_func.path_manager import PathManager
from common_func.step_trace_constant import StepTraceConstant
from msmodel.step_trace.cluster_step_trace_model import ClusterStepTraceModel
from msparser.interface.iparser import IParser


class ClusterStepTraceParser(IParser):
    """
    Step trace data parser for cluster scene.
    """
    FILE_NAME = os.path.basename(__file__)
    MODEL_ID_INDEX = 1
    MODEL_ID_IN_GE = 1
    MODEL_ID_NOT_IN_GE = 0

    def __init__(self: any, collection_path: str) -> None:
        self.collection_path = collection_path
        self.id_with_project_path_map = {}
        self.id_with_table_map = {}
        self.id_with_all_reduce_map = {}
        self.cluster_model = None

    @staticmethod
    def _fetch_data_from_database(db_dir: str, db_name: str, db_table: str, sql: str) -> list:
        data = []
        db_path = PathManager.get_db_path(db_dir, db_name)
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not conn or not curs:
            DBManager.destroy_db_connect(conn, curs)
            logging.error("The connect to %s is failed.", db_name)
            return data
        if not DBManager.judge_table_exist(curs, db_table):
            DBManager.destroy_db_connect(conn, curs)
            logging.error("The %s table doesn't exist.", db_table)
            return data
        data = DBManager.fetch_all_data(curs, sql)
        DBManager.destroy_db_connect(conn, curs)
        return data

    @staticmethod
    def _sql_for_step_trace() -> str:
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
        return sql

    @staticmethod
    def _append_ge_model_tag(step_trace_data: list, model_ids: list) -> list:
        result = []
        for item in step_trace_data:
            item = list(item)
            if item[ClusterStepTraceParser.MODEL_ID_INDEX] in model_ids:
                item.append(ClusterStepTraceParser.MODEL_ID_IN_GE)
            else:
                item.append(ClusterStepTraceParser.MODEL_ID_NOT_IN_GE)
            result.append(item)
        return result

    def ms_run(self: any) -> None:
        logging.info("Start to parse cluster step_trace data!")
        if not self._check_collection_path_valid():
            logging.error("The input dir doesn't have cluster database, please check.")
            error(ClusterStepTraceParser.FILE_NAME,
                  "The input dir doesn't have cluster database, please check.")
            return
        self.parse()
        logging.info("Start to save cluster step_trace data to db!")
        self.save()
        logging.info("cluster_step_trace.db created successful!")

    def parse(self: any) -> None:
        if not self._collect_project_paths():
            error(ClusterStepTraceParser.FILE_NAME,
                  "The cluster step trace parsing is failed.")

    def save(self: any) -> None:
        if not self.id_with_project_path_map:
            return
        self._run_cluster_step_trace_model()

    def _check_collection_path_valid(self: any) -> bool:
        db_path = PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_RANK)
        return os.path.exists(db_path)

    def _run_cluster_step_trace_model(self: any) -> bool:
        rank_ids = list(self.id_with_project_path_map.keys())
        for rank_id in rank_ids:
            self.id_with_table_map.setdefault(rank_id, DBNameConstant.TABLE_CLUSTER_STEP_TRACE.format(rank_id))
            self.id_with_all_reduce_map.setdefault(rank_id, DBNameConstant.TABLE_CLUSTER_ALL_REDUCE.format(rank_id))
        all_tables = self.id_with_table_map.values() + self.id_with_all_reduce_map.values()
        with ClusterStepTraceModel(self.collection_path, all_tables) as cluster_model:
            cluster_model.create_table()
            self._collect_and_save_step_trace_data(cluster_model)

    def _collect_project_paths(self: any) -> bool:
        rank_db_path = PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_RANK)
        conn, curs = DBManager.check_connect_db_path(rank_db_path)
        if not conn or not curs:
            DBManager.destroy_db_connect(conn, curs)
            logging.error("The connect to cluster rank database is failed.")
            return False
        if not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_CLUSTER_RANK):
            logging.error("The cluster rank table doesn't exist.")
            DBManager.destroy_db_connect(conn, curs)
            return False
        sql = "select rank_id, dir_name from {}".format(DBNameConstant.TABLE_CLUSTER_RANK)
        data = DBManager.fetch_all_data(curs, sql)
        DBManager.destroy_db_connect(conn, curs)
        if not data:
            logging.error("The query cluster rank info is invalid.")
            return False
        self.id_with_project_path_map = dict(data)
        return True

    def _collect_and_save_step_trace_data(self: any, cluster_model: any) -> None:
        for rank_id, path in self.id_with_project_path_map.items():
            project_path = os.path.join(self.collection_path, path)
            if not DataCheckManager.contain_info_json_data(project_path):
                continue
            InfoConfReader().load_info(project_path)
            if InfoConfReader().is_host_profiling():
                continue

            step_trace_data = self._collect_step_trace_data(project_path)
            if not step_trace_data:
                continue
            cluster_model.insert_data_to_db(self.id_with_table_map.get(rank_id), step_trace_data)

            all_reduce_data = self._collect_all_reduce_data(project_path)
            if not all_reduce_data:
                continue
            cluster_model.insert_data_to_db(self.id_with_all_reduce_map.get(rank_id), all_reduce_data)

    def _collect_all_reduce_data(self: any, project_path: str) -> list:
        sql = "select device_id,model_id,index_id,iteration_end,start*{syscnt_to_micro},end*{syscnt_to_micro} " \
              "from {0}".format(DBNameConstant.TABLE_ALL_REDUCE,
                                syscnt_to_micro=StepTraceConstant.syscnt_to_micro())
        return self._fetch_data_from_database(project_path, DBNameConstant.DB_TRACE,
                                              DBNameConstant.TABLE_ALL_REDUCE, sql)

    def _collect_step_trace_data(self: any, project_path: str) -> list:
        sql_for_ge_model_ids = "select distinct model_id from {}".format(DBNameConstant.TABLE_GE_TASK)
        model_ids = self._fetch_data_from_database(project_path, DBNameConstant.DB_GE_INFO,
                                                   DBNameConstant.TABLE_GE_TASK, sql_for_ge_model_ids)
        model_ids = list(chain.from_iterable(model_ids))
        sql_for_step_trace = self._sql_for_step_trace()
        step_trace_data = self._fetch_data_from_database(project_path, DBNameConstant.DB_TRACE,
                                                         DBNameConstant.TABLE_TRAINING_TRACE, sql_for_step_trace)
        if not step_trace_data:
            logging.error("Can't query step trace data.")
            return step_trace_data
        step_trace_data = self._append_ge_model_tag(step_trace_data, model_ids)
        return step_trace_data
