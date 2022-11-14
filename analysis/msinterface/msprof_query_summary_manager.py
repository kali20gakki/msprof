#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import json
import os
from enum import IntEnum

from common_func.common import error, print_msg
from common_func.constant import Constant
from common_func.data_check_manager import DataCheckManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_common import get_path_dir, prepare_log
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from msmodel.cluster_info.cluster_info_model import ClusterInfoViewModel
from msparser.cluster.cluster_communication_parser import ClusterCommunicationParser
from msparser.cluster.cluster_parallel_parser import ClusterParallelParser
from msparser.cluster.cluster_data_preparation_parser import ClusterDataPreparationParser
from msparser.cluster.fops_parser import FopsParser
from msparser.cluster.step_trace_summary import StepTraceSummay


class QueryDataType(IntEnum):
    CLUSTER_SCENE = 0
    STEP_TRACE = 1
    FOPS_ANALYSE = 2
    DATA_PREPARATION = 3
    PARALLEL_ANALYSIS = 4
    COLLECTIVE_COMMUNICATION = 5


class MsprofQuerySummaryManager:
    """
    The class for dispatching query summary data task.
    """
    CLUSTER_SCENE = '1'
    NOT_CLUSTER_SCENE = '0'
    FILE_NAME = os.path.basename(__file__)
    QUERY_DATA_TYPE_PARSER = {
        QueryDataType.STEP_TRACE: StepTraceSummay,
        QueryDataType.FOPS_ANALYSE: FopsParser,
        QueryDataType.DATA_PREPARATION: ClusterDataPreparationParser,
        QueryDataType.PARALLEL_ANALYSIS: ClusterParallelParser,
        QueryDataType.COLLECTIVE_COMMUNICATION: ClusterCommunicationParser
    }

    def __init__(self: any, args: any) -> None:
        self.collection_path = os.path.realpath(args.collection_path)
        self.data_type = args.data_type
        self.npu_id = args.id
        self.model_id = args.model_id
        self.iteration_id = args.iteration_id

    @staticmethod
    def check_rank_id(collection_path: str) -> bool:
        prof_dirs = get_path_dir(collection_path)
        for prof_dir in prof_dirs:
            prof_path = os.path.join(collection_path, prof_dir)
            if not os.path.isdir(prof_path):
                continue
            device_dirs = os.listdir(prof_path)
            for device_dir in device_dirs:
                device_path = os.path.join(prof_path, device_dir)
                if not DataCheckManager.contain_info_json_data(device_path, device_info_only=True):
                    continue
                InfoConfReader().load_info(device_path)
                return InfoConfReader().get_rank_id() != Constant.NA
        return False

    @staticmethod
    def check_cluster_scene(collection_path: str) -> None:
        if MsprofQuerySummaryManager.check_rank_id(collection_path):
            print_msg(json.dumps({'status': NumberConstant.SUCCESS, 'info': '', 'data':
                MsprofQuerySummaryManager.CLUSTER_SCENE}))
        else:
            print_msg(json.dumps({'status': NumberConstant.SUCCESS, 'info': '', 'data':
                MsprofQuerySummaryManager.NOT_CLUSTER_SCENE}))

    def process(self: any) -> None:
        self._check_data_type_valid()
        self._dispatch()

    def _dispatch(self: any) -> None:
        if self.data_type == QueryDataType.CLUSTER_SCENE:
            MsprofQuerySummaryManager.check_cluster_scene(self.collection_path)
            return
        if not self._check_collection_dir_valid():
            error(MsprofQuerySummaryManager.FILE_NAME,
                  "To query cluster or summary data, please execute import --cluster first")
            raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR)
        prepare_log(self.collection_path)
        params = {"collection_path": self.collection_path,
                  "npu_id": self.npu_id,
                  "model_id": self.model_id,
                  "iteration_id": self.iteration_id}
        self.QUERY_DATA_TYPE_PARSER.get(self.data_type)(params).process()

    def _check_collection_dir_valid(self: any) -> bool:
        if not os.path.exists(PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_RANK)):
            return False
        with ClusterInfoViewModel(self.collection_path) as cluster_info_model:
            return cluster_info_model.check_table()

    def _check_data_type_valid(self: any) -> None:
        if self.data_type is None or self.data_type not in QueryDataType.__members__.values():
            error(MsprofQuerySummaryManager.FILE_NAME,
                  "The query data type is wrong. Please enter a valid value.")
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)
