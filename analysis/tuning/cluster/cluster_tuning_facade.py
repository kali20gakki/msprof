#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
import logging
import os
from enum import IntEnum
import json

from common_func.common import print_msg
from common_func.ms_constant.number_constant import NumberConstant
from common_func.common import error
from common_func.db_name_constant import DBNameConstant
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager
from common_func.msprof_query_data import QueryArgumentCheck
from common_func.msvp_common import create_json_for_dict
from common_func.ms_constant.str_constant import StrConstant
from tuning.cluster.cluster_parser_factory import ClusterCommunicationParserFactory
from tuning.cluster.cluster_parser_factory import CommunicationMatrixParserFactory
from tuning.cluster.cluster_calculator_factory import SlowRankCalculatorFactory
from tuning.cluster.cluster_calculator_factory import SlowLinkCalculatorFactory
from tuning.cluster.cluster_calculator_factory import MatrixCalculatorFactory


class QueryDataType(IntEnum):
    RUN_ALL_TUNING = -1
    CLUSTER_COMMUNICATION = 6
    COMMUNICATION_MATRIX = 7


class ClusterTuningFacade:
    """
    interface for tuning presentation and tuning suggestions
    """

    FILE_NAME = os.path.basename(__file__)
    CLUSTER_ALL_DEVICE_SCENE = -1

    def __init__(self: any, params: dict) -> None:
        self.args = params
        self._collection_path = os.path.realpath(params.get("collection_path", ''))
        self._npu_id = params.get("npu_id", -2)
        self._model_id = params.get("model_id", 0)
        self._iteration_id = params.get("iteration_id", -1)
        self.data_type = params.get("data_type", -1)
        self.query_data_type_dispatcher = {QueryDataType.CLUSTER_COMMUNICATION: self.cluster_communication,
                                           QueryDataType.COMMUNICATION_MATRIX: self.communication_matrix}

    def process(self: any) -> None:
        self._check_params_valid()
        self.dispatch()

    def _check_params_valid(self: any) -> None:
        if not self._is_cluster_all_device_scene():
            self._npu_id = -1
            print_msg(json.dumps(
                {'status': NumberConstant.WARN,
                 'info': f"and the \'--id\' parameter has been set to (-1) automatically."
                    f"The collective communication data only supports exporting data by all devices, ",
                 'data': ''}))
        self._check_data_type_valid()
        QueryArgumentCheck.check_arguments_valid(self._npu_id, self._model_id, self._iteration_id)
        if not self._check_collection_dir_valid():
            error(ClusterTuningFacade.FILE_NAME,
                  "To query cluster or summary data, please execute import --cluster first")
            raise ProfException(ProfException.PROF_CLUSTER_DIR_ERROR)

    def dispatch(self: any) -> None:
        # export command entry
        if self.data_type == QueryDataType.RUN_ALL_TUNING:
            for every_data_type in self.query_data_type_dispatcher:
                self.query_data_type_dispatcher.get(every_data_type)()
        # query command entry
        else:
            self.query_data_type_dispatcher.get(self.data_type)(print_flag=False)

    def cluster_communication(self: any, print_flag=True) -> None:
        """
        cluster communication parse and calculate
        """
        logging.info('start to parse cluster communication information!')
        parser_factory = ClusterCommunicationParserFactory(self.args)
        communication_parser = parser_factory.generate_parser()
        logging.info('start to parse hccl events')
        op_info = communication_parser.run()
        logging.info('start to give suggestions according to rules')
        slow_rank_calculator = SlowRankCalculatorFactory(op_info).generate_calculator()
        slow_rank_calculator.run()
        slow_rank_calculator.add_suggestions(op_info)
        slow_link_calculator = SlowLinkCalculatorFactory(op_info).generate_calculator()
        slow_link_calculator.run()
        slow_link_calculator.add_suggestions(op_info)
        output_file_name = "communication_{}_{}_{}.json".format(
            self._npu_id, parser_factory.max_iters_model_id, self._iteration_id)
        if print_flag:
            print_msg(StrConstant.SUGGESTION + ': ' +
                      op_info.get(StrConstant.TOTAL, {}).get(StrConstant.SLOW_RANK_SUGGESTION, ''))
        self.dump_dict_to_json(output_file_name, op_info)

    def communication_matrix(self: any, print_flag=True) -> None:
        """
        communication matrix parse and calculate
        """
        logging.info('start to parse communication matrix information!')
        parser_factory = CommunicationMatrixParserFactory(self.args)
        matrix_parser = parser_factory.generate_parser()
        logging.info('start to parse hccl events')
        op_info = matrix_parser.run()
        logging.info('start to give suggestions according to rules')
        matrix_calculator = MatrixCalculatorFactory(op_info).generate_calculator()
        matrix_calculator.run()
        matrix_calculator.add_suggestions(op_info)
        output_file_name = "matrix_{}_{}_{}.json".format(
            self._npu_id, parser_factory.max_iters_model_id, self._iteration_id)
        if print_flag:
            matrix_calculator.print_suggestion(op_info)
        self.dump_dict_to_json(output_file_name, op_info)

    def dump_dict_to_json(self: any, output_file_name: str, dict_result: dict):
        output_file_path = PathManager.get_query_result_path(self._collection_path, output_file_name)
        result = create_json_for_dict(output_file_path, dict_result)
        result_json = json.loads(result)
        if result_json["status"] == NumberConstant.SUCCESS:
            print_msg(result)
        else:
            print_msg(json.dumps(
                {'status': NumberConstant.ERROR,
                 'info': f'communication data generation failed, '
                         f'maybe you can check the directory({self._collection_path}) permissions.',
                 'data': ''}))

    def _check_collection_dir_valid(self: any) -> bool:
        return os.path.exists(PathManager.get_db_path(self._collection_path, DBNameConstant.DB_CLUSTER_RANK))

    def _check_data_type_valid(self: any) -> None:
        if self.data_type not in QueryDataType.__members__.values():
            error(ClusterTuningFacade.FILE_NAME,
                  "The query data type is wrong. Please enter a valid value.")
            raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)

    def _is_cluster_all_device_scene(self: any) -> bool:
        return self._npu_id == self.CLUSTER_ALL_DEVICE_SCENE

