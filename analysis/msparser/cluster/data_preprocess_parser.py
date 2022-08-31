# !/usr/bin/python
# coding=utf-8
"""
This script is used to parse step trace data for cluster.
Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
"""
import json
import logging
import os
from collections import OrderedDict

from common_func.common import error, warn, print_info
from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant
from common_func.data_check_manager import DataCheckManager
from common_func.db_manager import DBManager, ClassRowType
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import check_path_valid
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_step import MsprofStep
from common_func.msvp_common import check_file_writable
from common_func.path_manager import PathManager
from msmodel.ai_cpu.data_queue_model import DataQueueModel
from profiling_bean.db_dto.cluster_rank_dto import ClusterRankDto


class DataPreprocessParser:
    """
    Step trace data parser for cluster scene.
    """
    FILE_NAME = os.path.basename(__file__)
    QUERY_FILE_NAME = 'query'

    def __init__(self: any, params: dict) -> None:
        self.collection_path = params.get('collection_path')
        self.data_type = params.get('data_type')
        self.rank_id = params.get('npu_id')

    @staticmethod
    def calculate_queue_data(queue_list: list, trace_data: dict) -> dict:
        """
        calculate data queue
        :return: json data list
        """
        total_info = {
            "step_count": len(trace_data),
            "empty_queue": 0,
            "total_time": 0,
            "avg_time": 0
        }
        total_time = 0
        empty_queue = 0
        data_list = []
        for data_index, data in enumerate(trace_data):
            if 2 * data_index + 1 > len(queue_list):
                data_list.append({"step": data_index + 1, "duration": 0, "queue_size": 0})
                continue
            queue_size = queue_list[2 * data_index][1] + queue_list[2 * data_index + 1][1]
            duration = queue_list[2 * data_index][4] + queue_list[2 * data_index + 1][4]
            total_time += duration
            empty_queue += int(bool(queue_size == 0))
            data_list.append({"step": data_index + 1, "duration": duration, "queue_size": queue_size})
        total_info["empty_queue"] = empty_queue
        total_info["total_time"] = total_time
        total_info["avg_time"] = round(total_time / len(trace_data), NumberConstant.ROUND_FOUR_DECIMAL)
        return {"total_info": total_info, "data_list": data_list}

    def calculate(self: any) -> None:
        """
        calculate data and data storage
        :return: None
        """
        if not self.check_id_valid():
            logging.warning(self.FILE_NAME, "Parameter settings are incorrect, please check input: --id. ")
            return
        self._query_data()

    def check_id_valid(self: any) -> bool:
        rank_conn, rank_cur = DBManager.check_connect_db_path(
            PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_RANK))
        rank_sql = 'select * from {} where rank_id=?'.format(DBNameConstant.TABLE_CLUSTER_RANK)
        rank_cur.row_factory = ClassRowType.class_row(ClusterRankDto)
        rank_data = DBManager.fetch_all_data(rank_cur, rank_sql, (self.rank_id,))
        if not rank_data:
            DBManager.destroy_db_connect(rank_conn, rank_cur)
            return False
        DBManager.destroy_db_connect(rank_conn, rank_cur)
        self.collection_path = os.path.join(self.collection_path, rank_data[0].dir_name)
        return True

    def query_data_queue_data(self: any) -> None:
        """
        query cluster data
        :return: None
        """
        data_queue_data = self.get_data_queue_data()
        step_trace_data = self.get_step_trace_data()
        if not (data_queue_data and step_trace_data):
            logging.error(self.FILE_NAME, "Query data failed, maybe import command has not run successfully yet, "
                                          "please run import command first")
            return
        json_data = self.calculate_queue_data(data_queue_data, step_trace_data)
        self.storage_data(json_data)

    def get_data_queue_data(self: any) -> list:
        data_queue_data = []
        model = DataQueueModel(self.collection_path, [DBNameConstant.TABLE_DATA_QUEUE])
        with model as _model:
            if not _model.check_db() or not _model.check_table():
                return data_queue_data
            data_queue_data = _model.get_all_data(DBNameConstant.TABLE_DATA_QUEUE)
        return data_queue_data

    def get_step_trace_data(self: any) -> dict:
        with MsprofStep(self.collection_path) as step_trace:
            trace_data = step_trace.data
        iter_dict = OrderedDict()
        for index, data in enumerate(trace_data):
            start_time = 0 if data.iter_id == 1 else trace_data[index - 1].step_end
            end_time = data.step_end
            iter_dict.setdefault(data.iter_id, [start_time, end_time])
        return iter_dict

    def storage_data(self: any, json_data: dict) -> None:
        """
        save data into file
        :return: None
        """
        logging.info(self.FILE_NAME, "Data queue query complete, start to storage data into json file")
        file_name = 'data_queue_{0}.json'.format(self.rank_id)
        file_path = self.get_cluster_path(file_name)
        check_file_writable(file_path)
        if os.path.exists(file_path):
            os.remove(file_path)
        try:
            with os.fdopen(os.open(file_path, Constant.WRITE_FLAGS,
                                   Constant.WRITE_MODES), "w") as _file:
                os.chmod(file_path, NumberConstant.FILE_AUTHORITY)
                _file.write(json.dumps(json_data))
        except (OSError, SystemError, RuntimeError, TypeError):
            logging.error(self.FILE_NAME,
                          "Storing data failed, you may not have the permission to write files in the current path.")
        else:
            print_info({"status": 0, "info": "", "data": file_path})

    def get_cluster_path(self: any, file_name: str) -> str:
        query_path = os.path.realpath(os.path.join(self.collection_path, '..', '..', self.QUERY_FILE_NAME))
        if not os.path.exists(query_path):
            try:
                os.makedirs(query_path)
            except OSError:
                logging.error(self.FILE_NAME,
                              "Storing data failed, you may not have the permission to write files in the current path.")
        return os.path.realpath(os.path.join(query_path, file_name))

    def process(self: any) -> None:
        """
        entrance for calculating data queue
        :return: None or dict
        """
        if self.rank_id is None:
            logging.warning(self.FILE_NAME,
                            "To query data queue,  id is required")
            return
        self.calculate()

    def _query_data(self):
        check_path_valid(self.collection_path, False)
        if DataCheckManager.contain_info_json_data(self.collection_path):
            InfoConfReader().load_info(self.collection_path)
            self.query_data_queue_data()
        else:
            logging.warning(self.FILE_NAME,
                            'Invalid parsing dir("%s"), there is no PROF file in this path' % self.collection_path)
