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

    def __init__(self: any, collect_path: str, cluster_device_paths: list) -> None:
        self.collect_path = collect_path
        self.cluster_device_paths = cluster_device_paths
        self.cluster_info_list = []
        self.rank_id_set = set()

    def ms_run(self: any):
        self.parse()
        self.save()
        logging.info("cluster_rank.db created successful!")

    def parse(self: any) -> None:
        logging.info("Start to parse cluster rank data!")
        for cluster_device_path in self.cluster_device_paths:
            check_path_valid(cluster_device_path, False)
            InfoConfReader().load_info(cluster_device_path)
            rank_id = InfoConfReader().get_rank_id()
            if rank_id in self.rank_id_set:
                logging.error("Parsing not supported! There are different collect data in the dir(%s)"
                              , cluster_device_path)
                raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR)
            self.rank_id_set.add(rank_id)
            cluster_info = InfoConfReader().get_job_basic_info()
            if cluster_info:
                cluster_info.append(os.sep.join(cluster_device_path.split(os.sep)[-2:]))
                self.cluster_info_list.append(cluster_info)

    def save(self: any) -> None:
        logging.info("Starting to save cluster_rank data to db!")
        if not self.cluster_info_list:
            logging.error('no valid cluster data!')
            return
        self.cluster_info_list.sort(key=lambda x:x[3])
        with ClusterInfoModel(self.collect_path) as cluster_info_model:
            cluster_info_model.flush(self.cluster_info_list)