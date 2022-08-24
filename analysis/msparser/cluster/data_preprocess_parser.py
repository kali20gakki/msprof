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


class DataPreprocessParser(IParser):
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
        logging.info("Start to save cluster step trace data to db!")
        self.save()
        logging.info("Query cluster step trace data finished!")

    def parse(self: any) -> None:
        if not self._collect_project_paths():
            error(ClusterStepTraceParser.FILE_NAME,
                  "The cluster step trace parsing is failed.")

    def save(self: any) -> None:
        tables = self._generate_cluster_table_name()
        if not tables:
            logging.error("The step trace source database or table is not found.")
            return
        with ClusterStepTraceModel(self.collection_path, tables) as cluster_model:
            cluster_model.create_table()
            self._collect_and_save_step_trace_data(cluster_model)

    def _check_collection_path_valid(self: any) -> bool:
        db_path = PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_RANK)
        return os.path.exists(db_path)
