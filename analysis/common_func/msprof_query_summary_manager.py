#!/usr/bin/python3
# coding=utf-8
"""
This script is used to dispatch query summary data task.
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import json
import os
from enum import IntEnum

from common_func.common import error
from common_func.data_check_manager import DataCheckManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.msprof_common import MsProfCommonConstant, get_path_dir
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from profiling_bean.cluster_collection.step_trace_summary import StepTraceSummay


class MsprofQuerySummaryManager:
    """
    The class for dispatching query summary data task.
    """

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
                if InfoConfReader().get_rank_id() is not None:
                    return True
        return False

    @staticmethod
    def check_cluster_task(collection_path: str) -> None:
        if MsprofQuerySummaryManager.check_rank_id(collection_path):
            json.dumps({'ClusterTask': 1})
        else:
            json.dumps({'ClusterTask': 0})

    @staticmethod
    def _check_integer_with_min_value(arg: any, min_value: int = None, nullable: bool = False) -> bool:
        if nullable and arg is None:
            return True
        if arg is not None and isinstance(arg, int):
            if min_value is not None:
                return arg >= min_value
        return False

    def process(self: any) -> None:
        self._preprocess()
        self._dispatch()

    def _preprocess(self: any) -> None:
        self._check_argument()
        self._check_cluster_scene()

    def _dispatch(self: any) -> None:
        params = {"collection_path": self.collection_path,
                  "is_cluster": self.is_cluster_scene,
                  "npu_id": self.npu_id,
                  "model_id": self.model_id,
                  "iteration_id": self.iteration_id}
        if self.data_type == QueryDataType.CLUSTER_TASK:
            MsprofQuerySummaryManager.check_cluster_task(self.collection_path)
        if self.data_type == QueryDataType.STEP_TRACE:
            StepTraceSummay(params).process()

    def _check_cluster_scene(self: any) -> None:
        cluster_rank_file = PathManager.get_db_path(self.collection_path, DBNameConstant.DB_CLUSTER_RANK)
        self.is_cluster_scene = True if os.path.exists(cluster_rank_file) else False

    def _check_argument(self: any) -> None:
        if self.data_type is None or self.data_type not in QueryDataType.__members__.values():
            error(MsProfCommonConstant.COMMON_FILE_NAME,
                  "The query data type is wrong. Please enter a valid value.")
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)
        if not MsprofQuerySummaryManager._check_integer_with_min_value(self.npu_id, min_value=-1):
            error(MsProfCommonConstant.COMMON_FILE_NAME,
                  "The query id is wrong. Please enter a valid value.")
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)
        if not MsprofQuerySummaryManager._check_integer_with_min_value(self.model_id, min_value=1):
            error(MsProfCommonConstant.COMMON_FILE_NAME,
                  "The query model id is wrong. Please enter a valid value.")
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)
        if not MsprofQuerySummaryManager._check_integer_with_min_value(self.iteration_id, min_value=1, nullable=True):
            error(MsProfCommonConstant.COMMON_FILE_NAME,
                  "The query iteration id is wrong. Please enter a valid value.")
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)


class QueryDataType(IntEnum):
    CLUSTER_TASK = 0
    STEP_TRACE = 1
