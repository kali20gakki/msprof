#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.

import unittest
import os
from unittest import mock
import pytest
from tuning.cluster.cluster_parser_factory import ClusterCommunicationParserFactory
from common_func.msprof_exception import ProfException

NAMESPACE = 'tuning.cluster.cluster_parser_factory'


class Hccl:
    op_name = 'all_reduce'
    iteration = 0


class GeOp:
    op_name = 'MatMul'
    iteration = 0
    plane_id = 0


class TestClusterCommunicationParserFactory(unittest.TestCase):
    DIR = os.path.join(os.path.dirname(__file__), 'DT_ClusterTuningFacade')
    params = {"collection_path": os.path.join(os.path.dirname(__file__), 'DT_ClusterTuningFacade'),
              "npu_id": -1,
              "model_id": 0,
              "iteration_id": 1,
              "data_type": 6}

    def test_get_hccl_ops_by_iter(self):
        with mock.patch('msmodel.cluster_info.cluster_info_model.ClusterInfoViewModel.check_db', return_value=True), \
             mock.patch('msmodel.cluster_info.cluster_info_model.ClusterInfoViewModel.get_all_rank_id_and_dirnames',
                        return_value=[(0, 'hccldb_dir')]), \
             mock.patch(NAMESPACE + '.ClusterCommunicationParserFactory.get_conditions_from_db'), \
                mock.patch(NAMESPACE + '.LoadInfoManager.load_info'):
            ClusterCommunicationParserFactory(self.params).get_hccl_ops_by_iter()

    def test_get_hccl_ops_by_iter_prof(self):
        with mock.patch('msmodel.cluster_info.cluster_info_model.ClusterInfoViewModel.check_db', return_value=True), \
                mock.patch('msmodel.cluster_info.cluster_info_model.ClusterInfoViewModel.get_all_rank_id_and_dirnames',
                           return_value=[]), \
                mock.patch(NAMESPACE + '.ClusterCommunicationParserFactory.get_conditions_from_db'), \
                mock.patch(NAMESPACE + '.LoadInfoManager.load_info'), \
                pytest.raises(ProfException) as err:

            ClusterCommunicationParserFactory(self.params).get_hccl_ops_by_iter()
            self.assertEqual(ProfException.PROF_CLUSTER_INVALID_DB, err.value.code)

    def test_get_hccl_ops_by_iter_rank_less_than_2(self):
        with mock.patch('msmodel.cluster_info.cluster_info_model.ClusterInfoViewModel.get_all_rank_id_and_dirnames',
                        return_value=[(0, 'dir0')]), \
                pytest.raises(ProfException) as err:
            ClusterCommunicationParserFactory(self.params).get_hccl_ops_by_iter()
            self.assertEqual(ProfException.PROF_CLUSTER_INVALID_DB, err.value.code)

    def test_get_conditions_from_db_invalid_rank_dir(self):
        with pytest.raises(ProfException) as err:
            rank_dir = {0: None, 1: None}
            ClusterCommunicationParserFactory(self.params).get_conditions_from_db(rank_dir)
            self.assertEqual(ProfException.PROF_CLUSTER_INVALID_DB, err.value.code)

    def test_get_conditions_from_db_invalid_rank_dir(self):
        with pytest.raises(ProfException) as err:
            rank_dir = {4088: None, 5000: None}
            ClusterCommunicationParserFactory(self.params).get_conditions_from_db(rank_dir)
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)

    def test_get_conditions_from_db_no_step_trace_table(self):
        with mock.patch('msmodel.step_trace.cluster_step_trace_model.ClusterStepTraceViewModel.judge_table_exist',
                        return_value=False), \
         pytest.raises(ProfException) as err:
            rank_dir = {0: 'dir0', 1: 'dir1'}
            ClusterCommunicationParserFactory(self.params).get_conditions_from_db(rank_dir)
            self.assertEqual(ProfException.PROF_INVALID_STEP_TRACE_ERROR, err.value.code)

    def test_get_conditions_from_db_no_model_iteration_info_in_step_trace_table(self):
        with mock.patch('msmodel.step_trace.cluster_step_trace_model.ClusterStepTraceViewModel.judge_table_exist',
                        return_value=True), \
         pytest.raises(ProfException) as err:
            rank_dir = {0: 'dir0', 1: 'dir1'}
            ClusterCommunicationParserFactory(self.params).get_conditions_from_db(rank_dir)
            self.assertEqual(ProfException.PROF_INVALID_STEP_TRACE_ERROR, err.value.code)

    def test_get_conditions_from_db_no_iteration_info_in_step_trace_table(self):
        iter_model = [(0, 123), (1, 232)]
        with mock.patch('msmodel.step_trace.cluster_step_trace_model.ClusterStepTraceViewModel.judge_table_exist',
                        return_value=True), \
             mock.patch(
                 'msmodel.step_trace.cluster_step_trace_model.ClusterStepTraceViewModel.get_model_id_with_iterations',
                 return_value=iter_model), \
             pytest.raises(ProfException) as err:
            rank_dir = {0: 'dir0', 1: 'dir1'}
            ClusterCommunicationParserFactory(self.params).get_conditions_from_db(rank_dir)
            self.assertEqual(ProfException.PROF_INVALID_STEP_TRACE_ERROR, err.value.code)

    def test_get_conditions_from_db(self):
        iter_model = [(0, 123), (1, 232)]
        iter_start_end = [(44444444, 111)]
        with mock.patch('msmodel.step_trace.cluster_step_trace_model.ClusterStepTraceViewModel.judge_table_exist',
                        return_value=True), \
             mock.patch(
                 'msmodel.step_trace.cluster_step_trace_model.ClusterStepTraceViewModel.get_model_id_with_iterations',
                 return_value=iter_model), \
             mock.patch('msmodel.step_trace.cluster_step_trace_model.ClusterStepTraceViewModel.get_iter_start_end',
                        return_value=iter_start_end), \
             mock.patch(NAMESPACE + '.ClusterCommunicationParserFactory.get_hccl_events_from_db'):
            rank_dir = {0: 'dir0', 1: 'dir1'}
            ClusterCommunicationParserFactory(self.params).get_conditions_from_db(rank_dir)

    def test_get_hccl_events_from_db_invalid_connect(self):
        with pytest.raises(ProfException) as err:
            ClusterCommunicationParserFactory(self.params).get_hccl_events_from_db(0, '', [(0, 100)])
            self.assertEqual(ProfException.PROF_INVALID_STEP_TRACE_ERROR, err.value.code)

    def test_get_hccl_events_from_db_return_none(self):
        model_space = 'msmodel.cluster_info.communication_model.CommunicationModel'
        with mock.patch(model_space + '.check_db', return_value=True), \
                mock.patch(model_space + '.get_all_events_from_db', return_value=[]):
            ClusterCommunicationParserFactory(self.params).get_hccl_events_from_db(0, '', [(0, 100)])

    def test_get_hccl_events_from_db(self):
        hccl = Hccl()
        model_space = 'msmodel.cluster_info.communication_model.CommunicationModel'
        with mock.patch(model_space + '.check_db', return_value=True), \
                mock.patch(model_space + '.get_all_events_from_db', return_value=[hccl]):
            ClusterCommunicationParserFactory(self.params).get_hccl_events_from_db(0, '', [(0, 100)])

    def test_get_hccl_events_from_db_with_cpa(self):
        hccl = Hccl()
        top_hccl_ops = tuple(('allReduce_1_1', 'allReduce_2_2', 'allReduce_3_3', 'allReduce_4_4', 'allReduce_5_5'))
        with mock.patch(NAMESPACE + '.CommunicationModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.CommunicationModel.get_all_events_from_db', return_value=[hccl]):
            ClusterCommunicationParserFactory(self.params).get_hccl_events_from_db(0, '', [(0, 100)], top_hccl_ops)


class TestCommunicationMatrixParserFactory(unittest.TestCase):
    DIR = os.path.join(os.path.dirname(__file__), 'DT_ClusterTuningFacade')
    params = {"collection_path": os.path.join(os.path.dirname(__file__), 'DT_ClusterTuningFacade'),
              "npu_id": -1,
              "model_id": 0,
              "iteration_id": 1,
              "data_type": 7}


class TestCriticalPathAnalysisParserFactory(unittest.TestCase):
    DIR = os.path.join(os.path.dirname(__file__), 'DT_ClusterTuningFacade')
    params = {"collection_path": os.path.join(os.path.dirname(__file__), 'DT_ClusterTuningFacade'),
              "npu_id": -1,
              "model_id": 0,
              "iteration_id": 1,
              "data_type": 9}

    def test_get_conditions_from_db(self):
        with mock.patch(NAMESPACE + '.ClusterStepTraceViewModel.judge_table_exist', return_value=False), \
                mock.patch(NAMESPACE + '.ClusterInfoViewModel.get_all_rank_id_and_dirnames',
                           return_value=[(0, 'hccldb_dir')]),\
                mock.patch(NAMESPACE + '.ClusterInfoViewModel.check_db', return_value=True), \
                mock.patch(NAMESPACE + '.CommunicationModel.check_db', return_value=True),\
                mock.patch(NAMESPACE + '.CommunicationModel.get_all_events_from_db', return_value=[Hccl()]), \
                mock.patch(NAMESPACE + '.OpSummaryModel.get_compute_op_data', return_value=[GeOp()]):
            rank_dir = {0: 'dir0', 1: 'dir1'}
            ClusterCommunicationParserFactory(self.params).get_conditions_from_db(rank_dir)
