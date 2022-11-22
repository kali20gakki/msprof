#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import os
import logging
from abc import abstractmethod
from msmodel.step_trace.cluster_step_trace_model import ClusterStepTraceViewModel
from msmodel.cluster_info.cluster_info_model import ClusterInfoViewModel
from msmodel.cluster_info.communication_model import CommunicationModel
from msparser.cluster.meta_parser import MetaParser
from common_func.db_name_constant import DBNameConstant
from common_func.msprof_exception import ProfException
from common_func.common import error
from common_func.ms_constant.number_constant import NumberConstant


class ClusterParserFactory:
    """
    parser factory interface for cluster scene
    """
    def __init__(self: any) -> None:
        pass

    @abstractmethod
    def generate_parser(self):
        """
        generate_parse_method
        """
        return MetaParser()


class ClusterCommunicationParserFactory(ClusterParserFactory):
    """
    factory which creates cluster communication parser
    provide data preparation for created parser
    """

    FILE_NAME = os.path.basename(__file__)

    def __init__(self: any, params: dict) -> None:
        super().__init__()
        self.iteration_id = params.get("iteration_id", -1)
        self.max_iters_model_id = -1
        self.collection_path = os.path.realpath(params.get("collection_path"))
        self._cluster_info_model = ClusterInfoViewModel(self.collection_path)
        self.cluster_step_trace_model = ClusterStepTraceViewModel(self.collection_path)
        self.rank_hccl_data_dict = {}

    def generate_parser(self: any):
        return

    def get_hccl_ops_by_iter(self):
        """
        get op events of all rank by iteration start and end time
        """
        with self._cluster_info_model as _model:
            if not _model.check_db():
                logging.error("Fail to connect %s, communication parser is interrupted", DBNameConstant.DB_CLUSTER_RANK)
                raise ProfException(ProfException.PROF_INVALID_CONNECT_ERROR)
            rank_dirnames = _model.get_all_rank_id_and_dirnames()
        if not rank_dirnames:
            logging.error("no info useful in %s, communication parser is interrupted", DBNameConstant.DB_CLUSTER_RANK)
            raise ProfException(ProfException.PROF_CLUSTER_INVALID_DB)
        self.get_conditions_from_db(rank_dirnames)

    def get_conditions_from_db(self, rank_dirnames) -> None:
        """
        get max iteration model id, iteration start and end time
        """
        with self.cluster_step_trace_model as model:
            for rank_dir in rank_dirnames:
                if len(rank_dir) < 2:
                    logging.error("no info enough in %s, communication parser is interrupted",
                                  DBNameConstant.DB_CLUSTER_RANK)
                    raise ProfException(ProfException.PROF_CLUSTER_INVALID_DB)
                rank_id = rank_dir[0]
                dirname = rank_dir[1]
                if rank_id is None or dirname is None:
                    logging.error("no valid information in %s, communication parser is interrupted",
                                  DBNameConstant.DB_CLUSTER_RANK)
                    raise ProfException(ProfException.PROF_CLUSTER_INVALID_DB)
                if rank_id >= NumberConstant.MAX_RANK_NUMS:
                    logging.error("Number of ranks is %s !, exceeds the limited upper bound:%s ",
                                  str(rank_id), NumberConstant.MAX_RANK_NUMS)
                    raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)
                rank_path = os.path.join(self.collection_path, dirname)
                step_trace_table = DBNameConstant.TABLE_CLUSTER_STEP_TRACE.format(rank_id)
                if not model.judge_table_exist(step_trace_table):
                    logging.error("%s doesn't exist!", step_trace_table)
                    raise ProfException(ProfException.PROF_INVALID_STEP_TRACE_ERROR)
                # find model id that has most iterations
                model_iteration = model.get_model_id_with_iterations(step_trace_table)
                if not model_iteration:
                    logging.error("%s doesn't have model information", step_trace_table)
                    raise ProfException(ProfException.PROF_INVALID_STEP_TRACE_ERROR)
                model_iteration = sorted(model_iteration, key=lambda x: x[1], reverse=True)
                self.max_iters_model_id = model_iteration[0][0]
                # get iteration end and start
                if self.iteration_id is None or self.iteration_id <= 0:
                    error(ClusterCommunicationParserFactory.FILE_NAME,
                          "invalid iteration id!".format(self.iteration_id))
                    logging.error("invalid iteration id!")
                    raise ProfException(ProfException.PROF_INVALID_PARAM_ERROR)
                iter_start_end = model.get_iter_start_end(self.iteration_id, self.max_iters_model_id, step_trace_table)
                if not iter_start_end:
                    error(ClusterCommunicationParserFactory.FILE_NAME,
                          "fail to get no.{} iteration end and start time".format(self.iteration_id))
                    logging.error("%s doesn't have %s iteration information", step_trace_table, str(self.iteration_id))
                    raise ProfException(ProfException.PROF_INVALID_STEP_TRACE_ERROR)
                self.get_hccl_events_from_db(rank_id, rank_path, iter_start_end)

    def get_hccl_events_from_db(self: any, rank_id, rank_path: str, iter_start_end: list) -> None:
        """
        get op events of all rank by iteration start and end time
        """
        with CommunicationModel(rank_path) as _model:
            if not _model.check_db():
                logging.error("Fail to connect %s, communication parser is interrupted", DBNameConstant.DB_HCCL)
                raise ProfException(ProfException.PROF_INVALID_CONNECT_ERROR)
            hccl_names = _model.get_distinct_op_name()
            for hccl_name in hccl_names:
                conditions = {'op_name': hccl_name.op_name,
                              'iter_start': iter_start_end[0][0],
                              'iter_end': iter_start_end[0][1]}
                events_data = _model.get_hccl_data_by_conditions(conditions)
                if not events_data:
                    error(ClusterCommunicationParserFactory.FILE_NAME,
                          "fail to get no.{} iteration hccl data".format(self.iteration_id))
                    logging.error("Fail to get information from %s in %s iteration, communication parser is interrupted"
                                  , str(self.iteration_id), DBNameConstant.DB_HCCL)
                    raise ProfException(ProfException.PROF_CLUSTER_INVALID_DB)
                # only get hccl data with first iter
                events_data = [event for event in events_data if event.iteration == events_data[0].iteration]
                if hccl_name.op_name not in self.rank_hccl_data_dict:
                    self.rank_hccl_data_dict[hccl_name.op_name] = {}
                self.rank_hccl_data_dict[hccl_name.op_name][rank_id] = events_data
