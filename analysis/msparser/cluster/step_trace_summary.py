# !/usr/bin/python
# coding=utf-8
"""
This script is used to process querying step trace summary data.
Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
"""

import json
import os
import logging
from itertools import chain

from common_func.common import error, print_msg
from common_func.constant import Constant
from common_func.data_check_manager import DataCheckManager
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_common import get_path_dir
from common_func.msprof_exception import ProfException
from common_func.msvp_common import create_json
from common_func.path_manager import PathManager
from common_func.step_trace_constant import StepTraceConstant


class StepTraceSummay:
    """
    The class for querying step trace summary data.
    """
    FILE_NAME = os.path.basename(__file__)
    HEADERS = ["ID", "Model ID", "Iteration ID", "Iteration Time", "FP to BP Time", "Iteration Interval",
               "Iteration Refresh", "Iteration Start", "FP Start", "BP End", "Iteration End"]
    ID_NUM_FOR_ALL_DEVICES = -1
    ID_NUM_FOR_ALL_ITERATIONS = -1
    NUMBER_0F_DECIMAL_PLACE = 2

    def __init__(self: any, params: dict) -> None:
        self.collection_path = params["collection_path"]
        self.is_cluster_scene = params["is_cluster"]
        self.npu_id = params["npu_id"]
        self.model_id = params["model_id"]
        self.iteration_id = params["iteration_id"]
        self.data_collection = []
        self.all_devices = False

    def process(self: any) -> None:
        self._check_query_all_devices()
        if not self._check_iteration_id_valid():
            return
        if self.is_cluster_scene:
            self._process_in_cluster_scene()
        else:
            self._process_in_non_cluster_scene()
        if self.data_collection:
            self._storage_summary_data()

    def _storage_summary_data(self: any) -> None:
        output_file_name = "step_trace_{}_{}_{}.json".format(self.npu_id, self.model_id, self.iteration_id)
        output_file_path = PathManager.get_query_result_path(self.collection_path, output_file_name)
        result = create_json(output_file_path, StepTraceSummay.HEADERS, self.data_collection, save_old_file=False)
        result_json = json.loads(result)
        if result_json["status"] == NumberConstant.SUCCESS:
            print_msg(result)
        else:
            print_msg(json.dumps({'status': NumberConstant.ERROR, 'info': 'query step trace data failed', 'data': ''}))

    def _process_in_cluster_scene(self: any) -> None:
        if not self._check_cluster_db_valid():
            return
        rank_id_collection = self._get_all_rank_ids()
        if not rank_id_collection:
            logging.error("Get cluster rank id info failed.")
            return
        query_data = self._query_in_cluster_scene(rank_id_collection)
        if not query_data:
            logging.error("Get cluster step trace data failed.")
            return
        self.data_collection = query_data

    def _check_cluster_db_valid(self: any) -> bool:
        db_file = PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_STEP_TRACE)
        if not os.path.exists(db_file):
            logging.error("There is not step trace cluster database file. Please check the input dir.")
            return False
        return True

    def _check_query_all_devices(self: any) -> None:
        self.all_devices = self.npu_id == StepTraceSummay.ID_NUM_FOR_ALL_DEVICES

    def _check_iteration_id_valid(self: any) -> bool:
        if self.iteration_id is None:
            self.iteration_id = StepTraceSummay.ID_NUM_FOR_ALL_ITERATIONS
        if self.all_devices and self.iteration_id == StepTraceSummay.ID_NUM_FOR_ALL_ITERATIONS:
            logging.error("For querying all devices data, you should input a valid iteration id.")
            return False
        if not self.all_devices and self.iteration_id != StepTraceSummay.ID_NUM_FOR_ALL_ITERATIONS:
            logging.error("For querying single device data, you should not input a iteration id.")
            return False
        return True

    def _query_in_cluster_scene(self: any, rank_id_collection: list) -> list:
        data = []
        db_file = PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_STEP_TRACE)
        conn, curs = DBManager.check_connect_db_path(db_file)
        if not (conn and curs):
            DBManager.destroy_db_connect(conn, curs)
            logging.error("The connect to cluster step trace database is failed.")
            return data
        if self.all_devices:
            data = self._query_data_from_all_device(curs, rank_id_collection)
        else:
            if self.npu_id not in rank_id_collection:
                return data
            data = self._query_data_from_single_device(curs)
        DBManager.destroy_db_connect(conn, curs)
        return data

    def _get_all_rank_ids(self: any) -> list:
        rank_ids = []
        db_file = PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_RANK)
        conn, curs = DBManager.check_connect_db_path(db_file)
        if not (conn and curs):
            logging.error("The connect to cluster rank database is failed.")
            DBManager.destroy_db_connect(conn, curs)
            return rank_ids
        if not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_CLUSTER_RANK):
            logging.error("The cluster rank table doesn't exist.")
            DBManager.destroy_db_connect(conn, curs)
            return rank_ids
        sql = "select distinct rank_id from {}".format(DBNameConstant.TABLE_CLUSTER_RANK)
        rank_ids = DBManager.fetch_all_data(curs, sql)
        rank_ids = list(chain.from_iterable(rank_ids))
        DBManager.destroy_db_connect(conn, curs)
        return rank_ids

    def _query_data_from_all_device(self: any, curs: any, rank_id_collection: list) -> list:
        data_colleciton = []
        for rank_id in rank_id_collection:
            table = "step_trace_{}".format(rank_id)
            if not DBManager.judge_table_exist(curs, table):
                logging.error("The %s table doesn't exist.", table)
                continue
            sql = self._sql_for_query_all_iteration(table, rank_id) + " and iteration_id={}".format(self.iteration_id)
            data = DBManager.fetch_all_data(curs, sql)
            data_colleciton.append(list(chain.from_iterable(data)))
        return data_colleciton

    def _query_data_from_single_device(self, curs: any) -> list:
        table = "step_trace_{}".format(self.npu_id)
        sql = self._sql_for_query_all_iteration(table, self.npu_id)
        data = DBManager.fetch_all_data(curs, sql)
        return data

    def _sql_for_query_all_iteration(self: any, table_name: str, npu_id: int) -> str:
        sql = "select {0}, (case when model_id={1} then 'N/A' else model_id end), " \
              "iteration_id, " \
              "(case when iteration_time={2} then 'N/A' else round(iteration_time, {3}) end), " \
              "(case when fp_bp_time={2} then 'N/A' else round(fp_bp_time, {3}) end), " \
              "(case when data_aug_bound={2} then 'N/A' else round(data_aug_bound, {3}) end), " \
              "(case when bp_end={2} then 'N/A' else round(iteration_end - bp_end, {3}) end), " \
              "(case when iteration_time={2} or iteration_end={2} then 'N/A' else " \
              "round(iteration_end - iteration_time, {3}) end), " \
              "(case when fp_start={2} then 'N/A' else round(FP_start, {3}) end), " \
              "(case when bp_end={2} then 'N/A' else round(BP_end, {3}) end), " \
              "(case when iteration_end={2} then 'N/A' else round(iteration_end, {3}) end) " \
              "from {4} where model_id={5}".format(
            npu_id,
            NumberConstant.DEFAULT_MODEL_ID,
            NumberConstant.NULL_NUMBER,
            StepTraceSummay.NUMBER_0F_DECIMAL_PLACE,
            table_name,
            self.model_id)
        return sql

    def _process_in_non_cluster_scene(self: any) -> None:
        project_dir = self._find_info_file_dir()
        if project_dir:
            self.data_collection = self._query_in_non_cluster_scene(project_dir)
        else:
            logging.error("The input dir is wrong. Please check.")

    def _find_info_file_dir(self: any) -> str:
        prof_dirs = get_path_dir(self.collection_path)
        result = ""
        is_found = False
        for prof_dir in prof_dirs:
            prof_path = os.path.join(self.collection_path, prof_dir)
            if not os.path.isdir(prof_path):
                continue
            device_dirs = os.listdir(prof_path)
            for device_dir in device_dirs:
                device_path = os.path.join(prof_path, device_dir)
                if not DataCheckManager.contain_info_json_data(device_path):
                    continue
                InfoConfReader().load_info(device_path)
                if InfoConfReader().get_rank_id() != Constant.NA:
                    logging.error("To query cluster step trace data, please import cluster info data first.")
                    return ""
                if InfoConfReader().is_host_profiling():
                    continue
                devices = InfoConfReader().get_device_list()
                if str(self.npu_id) in devices and not is_found:
                    result = device_path
                    is_found = True
                elif str(self.npu_id) in devices and is_found:
                    logging.error("There are duplicate device id numbers in input dir. "
                                  "Please input a valid dir")
                    return ""
        return result

    def _query_in_non_cluster_scene(self: any, project_dir: str) -> list:
        data = []
        InfoConfReader().load_info(project_dir)
        db_file = PathManager.get_db_path(project_dir, DBNameConstant.DB_TRACE)
        conn, curs = DBManager.check_connect_db_path(db_file)
        if not (conn and curs):
            logging.error("The connect to database is failed.")
            DBManager.destroy_db_connect(conn, curs)
            return data
        if not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_TRAINING_TRACE):
            logging.error("The %s table doesn't exist.", DBNameConstant.TABLE_TRAINING_TRACE)
            DBManager.destroy_db_connect(conn, curs)
            return data
        sql = self._sql_for_non_cluster(DBNameConstant.TABLE_TRAINING_TRACE)
        data = DBManager.fetch_all_data(curs, sql)
        DBManager.destroy_db_connect(conn, curs)
        return data

    def _sql_for_non_cluster(self: any, table_name: str) -> str:
        sql = "select device_id, " \
              "(case when model_id={0} then 'N/A' else model_id end), " \
              "iteration_id, " \
              "(case when iteration_time={1} then 'N/A' else round(iteration_time*{2}, {3}) end), " \
              "(case when fp_bp_time={1} then 'N/A' else round(fp_bp_time*{2}, {3} end), " \
              "(case when data_aug_bound={1} then 'N/A' else round(data_aug_bound*{2}, {3}) end), " \
              "(case when bp_end={1} then 'N/A' else round((iteration_end - bp_end)*{2}, {3}) end), " \
              "(case when iteration_time={1} or iteration_end={1} then 'N/A' else " \
              "round((iteration_end - iteration_time)*{2}, {3}) end), " \
              "(case when FP_start={1} then 'N/A' else round(FP_start*{2}, {3}) end), " \
              "(case when BP_end={1} then 'N/A' else round(BP_end*{2}, {3}) end), " \
              "(case when iteration_end={1} then 'N/A' else round(iteration_end*{2}, {3}) end) " \
              "from {4} where model_id={5}".format(
            NumberConstant.DEFAULT_MODEL_ID,
            NumberConstant.NULL_NUMBER,
            StepTraceConstant.syscnt_to_micro(),
            StepTraceSummay.NUMBER_0F_DECIMAL_PLACE,
            table_name,
            self.model_id)
        return sql
