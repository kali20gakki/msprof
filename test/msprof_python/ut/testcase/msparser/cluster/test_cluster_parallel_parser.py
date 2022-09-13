import os
import pathlib
import unittest
from unittest import mock

import pytest

from common_func.msprof_exception import ProfException
from constant.constant import clear_dt_project
from msparser.cluster.cluster_parallel_parser import ClusterParallelParser

NAMESPACE = 'msparser.cluster.cluster_parallel_parser'


class TestClusterParallelParser(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_ClusterParallelParser')
    params = {"collection_path": DIR_PATH, "npu_id": -1, "model_id": None, "iteration_id": None}
    device_and_rank_ids_0 = []
    device_and_rank_ids_1 = [[0, 0], [1, 1]]
    device_and_rank_ids_2 = [[0, 'N/A'], [1, 'N/A']]
    side_effect_1 = [0.05, 0.06]
    side_effect_2 = [0.2, 0.3]
    side_effect_3 = [0.2, 0.05]

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))
        os.makedirs(os.path.join(self.DIR_PATH, 'sqlite'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_process_should_raise_prof_exception_when_without_cluster_db(self):
        with pytest.raises(ProfException) as err:
            check = ClusterParallelParser(self.params)
            check.process()
        self.assertEqual(ProfException.PROF_CLUSTER_INVALID_DB, err.value.code)

    def test_process_should_raise_prof_exception_when_without_cluster_table(self):
        pathlib.Path(os.path.join(self.DIR_PATH, 'sqlite', 'cluster_rank.db')).touch()
        pathlib.Path(os.path.join(self.DIR_PATH, 'sqlite', 'cluster_step_trace.db')).touch()
        with pytest.raises(ProfException) as err:
            check = ClusterParallelParser(self.params)
            check.process()
        self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)

    def test_process_should_raise_prof_exception_when_device_or_rank_id_invalid(self):
        with mock.patch(NAMESPACE + '.ClusterParallelParser._get_device_and_rank_ids',
                        return_value=self.device_and_rank_ids_0):
            with pytest.raises(ProfException) as err:
                check = ClusterParallelParser(self.params)
                check.process()
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)

    def test_process_should_raise_prof_exception_when_communication_time_ratio_invalid(self):
        with mock.patch(NAMESPACE + '.ClusterParallelParser._get_device_and_rank_ids',
                        return_value=self.device_and_rank_ids_1), \
                mock.patch(NAMESPACE + '.ClusterCommunicationModel.get_communication_time_ratio',
                           return_value=None):
            with pytest.raises(ProfException) as err:
                check = ClusterParallelParser(self.params)
                check.process()
            self.assertEqual(ProfException.PROF_INVALID_DATA_ERROR, err.value.code)

    def test_process_should_return_type_0_when_communication_time_ratio_below_10per(self):
        with mock.patch(NAMESPACE + '.ClusterParallelParser._get_device_and_rank_ids',
                        return_value=self.device_and_rank_ids_1), \
                mock.patch(NAMESPACE + '.ClusterCommunicationModel.get_communication_time_ratio',
                           side_effect=self.side_effect_1):
            check = ClusterParallelParser(self.params)
            check.process()
            self.assertEqual(check.parallel_analysis_result["type"], 0)

    def test_process_should_return_type_1_when_communication_time_ratio_over_10per(self):
        with mock.patch(NAMESPACE + '.ClusterParallelParser._get_device_and_rank_ids',
                        return_value=self.device_and_rank_ids_1), \
                mock.patch(NAMESPACE + '.ClusterCommunicationModel.get_communication_time_ratio',
                           side_effect=self.side_effect_2):
            check = ClusterParallelParser(self.params)
            check.process()
            self.assertEqual(check.parallel_analysis_result["type"], 1)

    def test_process_should_return_type_2_when_part_of_communication_time_ratio_over_10per(self):
        with mock.patch(NAMESPACE + '.ClusterParallelParser._get_device_and_rank_ids',
                        return_value=self.device_and_rank_ids_1), \
                mock.patch(NAMESPACE + '.ClusterCommunicationModel.get_communication_time_ratio',
                           side_effect=self.side_effect_3):
            check = ClusterParallelParser(self.params)
            check.process()
            self.assertEqual(check.parallel_analysis_result["type"], 2)

    def test_process_should_return_type_1_when_without_rank_id(self):
        with mock.patch(NAMESPACE + '.ClusterParallelParser._get_device_and_rank_ids',
                        return_value=self.device_and_rank_ids_2), \
                mock.patch(NAMESPACE + '.ClusterCommunicationModel.get_communication_time_ratio',
                           side_effect=self.side_effect_2):
            check = ClusterParallelParser(self.params)
            check.process()
            self.assertEqual(check.parallel_analysis_result["type"], 1)
            self.assertEqual(check.parallel_analysis_result["value"][0]["device_id"], 1)
