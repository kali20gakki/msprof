#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import json
import os

from common_func.common import print_info, error
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_common import MsProfCommonConstant
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from msmodel.parallel.cluster_parallel_model import ClusterParallelViewModel
from msparser.parallel.parallel_query.cluster_parallel_analysis import ClusterParallelAnalysis


class ClusterParallelAnalysisTuning:
    def __init__(self: any, params: dict) -> None:
        self._collection_path = params["collection_path"]
        self._params = params
        self._npu_id = params["npu_id"]
        self._model_id = params["model_id"]
        self._iteration_id = params["iteration_id"]
        self._tuning_suggestion = {}

    def process(self: any) -> None:
        self._prepare_parallel_analysis()
        with ClusterParallelViewModel(self._collection_path) as _model:
            parallel_table_name = _model.get_table_name()
        if parallel_table_name == Constant.NA:
            error(MsProfCommonConstant.COMMON_FILE_NAME, "Cannot find the cluster_parallel table!")
            raise ProfException(ProfException.PROF_CLUSTER_INVALID_DB)
        self._tuning_suggestion = ClusterParallelAnalysis(parallel_table_name, self._params).get_tuning_suggestion()
        output_file_name = "cluster_parallel_suggestion_-1_-1_-1.json"
        FileManager.storage_query_result_json_file(self._collection_path, self._tuning_suggestion, output_file_name)

    def _prepare_parallel_analysis(self: any) -> None:
        if not os.path.exists(PathManager.get_db_path(self._collection_path, DBNameConstant.DB_CLUSTER_PARALLEL)):
            error(MsProfCommonConstant.COMMON_FILE_NAME, "Cannot find the cluster_parallel.db or Permission denied!")
            raise ProfException(ProfException.PROF_CLUSTER_INVALID_DB)
        if self._npu_id != Constant.DEFAULT_INVALID_VALUE:
            error(MsProfCommonConstant.COMMON_FILE_NAME, "Invalid params! --id must be -1.")
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)
        if self._model_id != Constant.DEFAULT_INVALID_VALUE:
            error(MsProfCommonConstant.COMMON_FILE_NAME, "Invalid params! --model-id must be -1.")
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)
        if self._iteration_id != Constant.DEFAULT_INVALID_VALUE:
            error(MsProfCommonConstant.COMMON_FILE_NAME, "Invalid params! --iteration-id must be -1.")
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)
