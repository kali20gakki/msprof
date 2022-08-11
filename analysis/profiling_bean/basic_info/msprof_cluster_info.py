# !/usr/bin/python
# coding=utf-8
"""
This script is used to construct the cluster info for ide.
Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
"""
import json
import os.path
from collections import OrderedDict
from itertools import chain

from common_func.common import print_msg
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import create_json
from common_func.path_manager import PathManager


class MsProfClusterInfo:
    """
    The class for querying cluster info data.
    """
    OUTPUT_FILE_NAME = "cluster_info.json"

    def __init__(self: any, project_path: str) -> None:
        self.project_path = os.path.realpath(project_path)
        self.info_collection = []

    @staticmethod
    def _fetch_data_from_database(db_dir: str, db_name: str, db_table: str, sql: str) -> list:
        data = []
        db_path = PathManager.get_db_path(db_dir, db_name)
        conn, curs = DBManager.check_connect_db_path(db_path)
        if not conn or not curs:
            DBManager.destroy_db_connect(conn, curs)
            return data
        if not DBManager.judge_table_exist(curs, db_table):
            DBManager.destroy_db_connect(conn, curs)
            return data
        data = DBManager.fetch_all_data(curs, sql)
        DBManager.destroy_db_connect(conn, curs)
        return data

    def run(self: any) -> None:
        """
        run cluster info
        :return: None
        """
        self._collect_cluster_info_data(self.project_path)
        if not self.info_collection:
            return
        output_file_path = PathManager.get_query_result_path(self.project_path, MsProfClusterInfo.OUTPUT_FILE_NAME)
        result = create_json(output_file_path, ["Rank Id", "Models"], self.info_collection, save_old_file=False)
        result_json = json.loads(result)
        if result_json["status"] == NumberConstant.SUCCESS:
            print_msg(result)
        else:
            print_msg(json.dumps({'status': NumberConstant.ERROR, 'info': "Get the cluster info failed", 'data': ""}))

    def _collect_cluster_info_data(self: any, project_path: str) -> None:
        sql_for_rank_ids = "select rank_id from {}".format(DBNameConstant.TABLE_CLUSTER_RANK)
        rank_ids = MsProfClusterInfo._fetch_data_from_database(project_path, DBNameConstant.DB_CLUSTER_RANK,
                                                               DBNameConstant.TABLE_CLUSTER_RANK, sql_for_rank_ids)
        rank_ids = list(chain.from_iterable(rank_ids))
        if not rank_ids:
            return
        step_trace_db = PathManager.get_db_path(project_path, DBNameConstant.DB_CLUSTER_STEP_TRACE)
        conn, curs = DBManager.check_connect_db_path(step_trace_db)
        if not conn or not curs:
            DBManager.destroy_db_connect(conn, curs)
            return
        for rank_id in rank_ids:
            step_trace_table = "step_trace_{}".format(rank_id)
            if not DBManager.judge_table_exist(curs, step_trace_table):
                continue
            sql_for_total_iterations = "select model_id, max(iteration_id) " \
                                       "from {} group by model_id".format(step_trace_table)
            iteration_data = DBManager.fetch_all_data(curs, sql_for_total_iterations)
            model_list = []
            for each in iteration_data:
                model_list.append(OrderedDict(list(zip(["Model Id", "Iterations"], each))))
            self.info_collection.append([rank_id, model_list])
        DBManager.destroy_db_connect(conn, curs)
