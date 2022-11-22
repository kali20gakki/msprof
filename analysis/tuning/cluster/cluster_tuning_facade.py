#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import os
from enum import IntEnum
import json

from common_func.common import error
from common_func.db_name_constant import DBNameConstant
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from tuning.cluster.cluster_parser_factory import ClusterCommunicationParserFactory
from tuning.cluster.cluster_calculator_factory import SlowRankCalculatorFactory
from tuning.cluster.cluster_calculator_factory import SlowLinkCalculatorFactory


class QueryDataType(IntEnum):
    RUN_ALL_TUNING = -1
    CLUSTER_COMMUNICATION = 6
    COMMUNICATION_MATRIX = 7


class ClusterTuningFacade:
    """
    interface for tuning presentation and tuning suggestions
    """

    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any, params: dict) -> None:
        self.args = params
        self._collection_path = os.path.realpath(params.get("collection_path", ''))
        self._npu_id = params.get("npu_id", -2)
        self._model_id = params.get("model_id", -1)
        self._iteration_id = params.get("iteration_id", -1)
        self.data_type = params.get("data_type", -1)
        self.query_data_type_dispatcher = {QueryDataType.CLUSTER_COMMUNICATION: self.cluster_communication}

    def process(self: any) -> None:
        self._check_data_type_valid()
        if not self._check_collection_dir_valid():
            error(ClusterTuningFacade.FILE_NAME,
                  "To query cluster or summary data, please execute import --cluster first")
            raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR)
        self.dispatch()

    def dispatch(self: any) -> None:
        # export command entry
        if self.data_type == QueryDataType.RUN_ALL_TUNING:
            for key in self.query_data_type_dispatcher.keys():
                self.query_data_type_dispatcher.get(key)()
        # query command entry
        else:
            self.query_data_type_dispatcher.get(self.data_type)()

    def cluster_communication(self: any) -> None:
        """
        cluster communication parse and calculate
        """
        ClusterCommunicationParserFactory(self.args).generate_parser()
        SlowRankCalculatorFactory(self.args).generate_calculator()
        SlowLinkCalculatorFactory(self.args).generate_calculator()

    def _check_collection_dir_valid(self: any) -> bool:
        return os.path.exists(PathManager.get_db_path(self._collection_path, DBNameConstant.DB_CLUSTER_RANK))

    def _check_data_type_valid(self: any) -> None:
        if self.data_type not in QueryDataType.__members__.values():
            error(ClusterTuningFacade.FILE_NAME,
                  "The query data type is wrong. Please enter a valid value.")
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)


