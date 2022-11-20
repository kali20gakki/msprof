#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import json
import os

from common_func.common import error, print_info
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_common import MsProfCommonConstant
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from msmodel.parallel.cluster_parallel_model import ClusterParallelViewModel
from msparser.parallel.parallel_query.cluster_parallel_analysis import ClusterParallelAnalysis


class ClusterParallelAnalysisParser:
    def __init__(self: any, params: dict) -> None:
        self._collection_path = params["collection_path"]
        self._params = params
        self._npu_id = params["npu_id"]
        self._model_id = params["model_id"]
        self._iteration_id = params["iteration_id"]
        self._parallel_table_name = Constant.NA
        self.parallel_data_result = {}

    def process(self: any) -> None:
        self._prepare_parallel_analysis()
        self.parallel_data_result = ClusterParallelAnalysis(self._parallel_table_name, self._params).get_parallel_data()
        self._storage_parallel_analysis_result()

    def _prepare_parallel_analysis(self: any) -> None:
        if not os.path.exists(PathManager.get_db_path(self._collection_path, DBNameConstant.DB_CLUSTER_PARALLEL)):
            error(MsProfCommonConstant.COMMON_FILE_NAME, "Cannot find the cluster_parallel.db or Permission denied!")
            raise ProfException(ProfException.PROF_CLUSTER_INVALID_DB)
        npu_ids = []
        model_iteration_ids = {}
        with ClusterParallelViewModel(self._collection_path) as _model:
            self._parallel_table_name = _model.get_table_name()
            if self._parallel_table_name == Constant.NA:
                error(MsProfCommonConstant.COMMON_FILE_NAME,
                      "Cannot find the cluster parallel table or Permission denied!")
                raise ProfException(ProfException.PROF_CLUSTER_INVALID_DB)
            npu_ids = _model.get_npu_ids(self._parallel_table_name)
            model_iteration_ids = _model.get_model_iteration_ids(self._parallel_table_name)
        self._check_arguments_valid(npu_ids, model_iteration_ids)

    def _check_arguments_valid(self: any, npu_ids: list, model_iteration_ids: dict) -> None:
        if self._npu_id == Constant.DEFAULT_INVALID_VALUE:
            if self._model_id not in model_iteration_ids.keys():
                error(MsProfCommonConstant.COMMON_FILE_NAME,
                      "Invalid arguments! The argument '--model-id' should be between {} and {}.".format(
                          min(model_iteration_ids.keys()), max(model_iteration_ids.keys())))
                raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)
            if str(self._iteration_id) not in model_iteration_ids.get(self._model_id, []):
                error(MsProfCommonConstant.COMMON_FILE_NAME,
                      "Invalid arguments! The argument '--iteration-id' should be between {} and {}.".format(
                          min(model_iteration_ids.get(self._model_id, [])),
                          max(model_iteration_ids.get(self._model_id, []))))
                raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)
            return
        if self._iteration_id == Constant.DEFAULT_INVALID_VALUE:
            if self._npu_id not in npu_ids:
                error(MsProfCommonConstant.COMMON_FILE_NAME,
                      "Invalid arguments! The argument '--id' should be on the list {}.".format(str(npu_ids)))
                raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)
            if self._model_id not in model_iteration_ids.keys():
                error(MsProfCommonConstant.COMMON_FILE_NAME,
                      "Invalid arguments! The argument '--model-id' should be between {} and {}.".format(
                          min(model_iteration_ids.keys()), max(model_iteration_ids.keys())))
                raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)
            return
        error(MsProfCommonConstant.COMMON_FILE_NAME,
              "Query arguments error! One of the arguments '--id' or '--model-id' must be -1.")
        raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)

    def _storage_parallel_analysis_result(self: any) -> None:
        if not self.parallel_data_result:
            return
        output_file_name = "cluster_parallel_analysis_{}_{}_{}.json".format(self._npu_id, self._model_id,
                                                                            self._iteration_id)
        query_path = os.path.join(self._collection_path, PathManager.QUERY_CLUSTER)
        if not os.path.exists(query_path):
            try:
                os.makedirs(query_path)
            except OSError:
                error(MsProfCommonConstant.COMMON_FILE_NAME,
                      "Storing data failed, you may not have the permission to write files in the current path.")
            os.chmod(query_path, NumberConstant.DIR_AUTHORITY)
        output_file_path = PathManager.get_query_result_path(self._collection_path, output_file_name)
        try:
            with os.fdopen(os.open(output_file_path, Constant.WRITE_FLAGS,
                                   Constant.WRITE_MODES), 'w') as file:
                os.chmod(output_file_path, NumberConstant.FILE_AUTHORITY)
                json.dump(self.parallel_data_result, file)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            error(MsProfCommonConstant.COMMON_FILE_NAME, err)
        else:
            print_info(MsProfCommonConstant.COMMON_FILE_NAME, "The data has stored successfully, "
                                                              "file path: {}".format(output_file_path))
