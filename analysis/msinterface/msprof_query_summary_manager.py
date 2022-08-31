#!/usr/bin/python3
# coding=utf-8
"""
This script is used to dispatch query summary data task.
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
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
from msparser.cluster.data_preprocess_parser import DataPreprocessParser
from msparser.cluster.fops_parser import FopsParser
from msparser.cluster.cluster_communication_parser import ClusterCommunicationParser
from msparser.cluster.step_trace_summary import StepTraceSummay


class MsprofQuerySummaryManager:
    """
    The class for dispatching query summary data task.
    """
    CLUSTER_SCENE = '1'
    NOT_CLUSTER_SCENE = '0'
    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any, args: any) -> None:
        self.collection_path = os.path.realpath(args.collection_path)
        self.data_type = args.data_type
        self.npu_id = args.id
        self.model_id = args.model_id
        self.iteration_id = args.iteration_id
        self.is_cluster_scene = False

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
                if not DataCheckManager.contain_info_json_data(device_path):
                    continue
                InfoConfReader().load_info(device_path)
                if InfoConfReader().get_rank_id() != Constant.NA:
                    return True
        return False

    @staticmethod
    def check_cluster_scene(collection_path: str) -> None:
        if MsprofQuerySummaryManager.check_rank_id(collection_path):
            print_msg(json.dumps({'status': NumberConstant.SUCCESS, 'info': '', 'data':
                MsprofQuerySummaryManager.CLUSTER_SCENE}))
        else:
            print_msg(json.dumps({'status': NumberConstant.SUCCESS, 'info': '', 'data':
                MsprofQuerySummaryManager.NOT_CLUSTER_SCENE}))

    @staticmethod
    def _check_integer_with_min_value(arg: any, min_value: int = None, nullable: bool = False) -> bool:
        if nullable and arg is None:
            return True
        if arg is not None and isinstance(arg, int):
            if min_value is not None:
                return arg >= min_value
        return False

    def process(self: any) -> None:
        self._check_data_type_valid()
        self._dispatch()

    def _dispatch(self: any) -> None:
        if self.data_type == QueryDataType.CLUSTER_SCENE:
            MsprofQuerySummaryManager.check_cluster_scene(self.collection_path)
            return
        self._check_cluster_scene()
        params = {"collection_path": self.collection_path,
                  "is_cluster": self.is_cluster_scene,
                  "npu_id": self.npu_id,
                  "model_id": self.model_id,
                  "iteration_id": self.iteration_id}
        if self.data_type == QueryDataType.DATA_PREPARATION:
            DataPreprocessParser(params).process()
        self._check_arguments_valid()
        if self.data_type == QueryDataType.STEP_TRACE:
            StepTraceSummay(params).process()
        if self.data_type == QueryDataType.FOPS_ANALYSE:
            FopsParser(params).process()
        if self.data_type == QueryDataType.COLLECTIVE_COMMUNICATION:
            ClusterCommunicationParser(params).process()

    def _check_cluster_scene(self: any) -> None:
        cluster_rank_file = PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_RANK)
        if os.path.exists(cluster_rank_file):
            self.is_cluster_scene = True
            prepare_log(self.collection_path)
        else:
            if MsprofQuerySummaryManager.check_rank_id(self.collection_path):
                error(MsprofQuerySummaryManager.FILE_NAME,
                      "To query cluster data, please import cluster info data first.")
                raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR)
            self.is_cluster_scene = False

    def _check_data_type_valid(self: any) -> None:
        if self.data_type is None or self.data_type not in QueryDataType.__members__.values():
            error(MsprofQuerySummaryManager.FILE_NAME,
                  "The query data type is wrong. Please enter a valid value.")
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)

    def _check_arguments_valid(self: any) -> None:
        if not MsprofQuerySummaryManager._check_integer_with_min_value(self.npu_id, min_value=-1):
            error(MsprofQuerySummaryManager.FILE_NAME,
                  "The query id is wrong. Please enter a valid value.")
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)
        if not MsprofQuerySummaryManager._check_integer_with_min_value(self.model_id, min_value=0):
            error(MsprofQuerySummaryManager.FILE_NAME,
                  "The query model id is wrong. Please enter a valid value.")
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)
        if not MsprofQuerySummaryManager._check_integer_with_min_value(self.iteration_id, min_value=1, nullable=True):
            error(MsprofQuerySummaryManager.FILE_NAME,
                  "The query iteration id is wrong. Please enter a valid value.")
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)


class QueryDataType(IntEnum):
    CLUSTER_SCENE = 0
    STEP_TRACE = 1
    FOPS_ANALYSE = 2
    DATA_PREPARATION = 3
    COLLECTIVE_COMMUNICATION = 5
