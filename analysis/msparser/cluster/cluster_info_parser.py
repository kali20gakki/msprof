"""
This script is used to parse cluster rank_id data to db.
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import json
import logging
import os
import sqlite3

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import check_path_valid
from common_func.file_name_manager import get_info_json_compiles
from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_common import get_path_dir
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from msmodel.cluster_info.cluster_info_model import ClusterInfoModel
from msparser.interface.iparser import IParser


class ClusterInfoParser(IParser):
    """
    parser of rank_id data
    """
    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any, collect_path: str) -> None:
        self.collect_path = collect_path
        self.cluster_info_list = []
        self.rank_id_set = set()

    def ms_run(self: any):
        self.parse()
        self.save()

    def parse(self: any) -> None:
        first_sub_dirs = get_path_dir(self.collect_path)
        for first_sub_dir in first_sub_dirs:
            if first_sub_dir == 'sqlite' or first_sub_dir == 'log' or first_sub_dir == 'query':
                continue
            first_sub_path = os.path.realpath(
                os.path.join(self.collect_path, first_sub_dir))
            second_sub_dirs = get_path_dir(first_sub_path)
            for second_sub_dir in second_sub_dirs:
                if second_sub_dir.find('device') == -1:
                    continue
                second_sub_path = os.path.realpath(
                        os.path.join(first_sub_path, second_sub_dir))
                cluster_info = self._get_cluster_info(second_sub_path)
                if cluster_info:
                    cluster_info.append(first_sub_dir)
                    self.cluster_info_list.append(cluster_info)

    def save(self: any) -> None:
        if os.path.exists(PathManager.get_db_path(self.collect_path, DBNameConstant.DB_CLUSTER_RANK)):
            os.remove(PathManager.get_db_path(self.collect_path, DBNameConstant.DB_CLUSTER_RANK))
        if not self.cluster_info_list:
            logging.error('no valid cluster data')
            return
        cluster_model = ClusterInfoModel(self.collect_path)
        try:
            if cluster_model.init():
                cluster_model.flush(self.cluster_info_list)
                cluster_model.finalize()
        except sqlite3.Error as trace_err:
            logging.error("Save cluster rank_id failed, "
                          "%s", str(trace_err), exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass

    def _get_cluster_info(self: any, second_sub_path) -> list:
        check_path_valid(second_sub_path, False)
        InfoConfReader().load_info(second_sub_path)
        device_id_list = InfoConfReader().get_device_list()
        rank_id = InfoConfReader().get_rank_id()
        if rank_id == -1:
            logging.error("the data is not collected in a clustered environment, please check the directory: %s",
                          second_sub_path)
            raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR)
        if rank_id in self.rank_id_set:
            logging.error("parsing not supported! There are different collect data in the dir(%s)"
                          , second_sub_path)
            raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR)
        if rank_id is None:
            logging.error("No rank id found in the file of info.json, "
                          "please check the info.json under the directory: %s", second_sub_path)
            return []
        self.rank_id_set.add(rank_id)
        if not device_id_list:
            logging.error("No device id found in the file of info.json, "
                          "please check the info.json under the directory: %s", second_sub_path)
            return []
        device_id = device_id_list[0]
        collection_time, _ = InfoConfReader().get_collect_date()
        if not collection_time:
            logging.error("No collection start time found in the file of start.info, "
                                  "please check the start.info under the directory: %s", second_sub_path)
            return []
        return [InfoConfReader().get_job_info(), device_id, collection_time, rank_id]