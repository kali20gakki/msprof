import unittest
from unittest import mock

from msparser.cluster.cluster_parallel_parser import ClusterParallelParser

NAMESPACE = 'msparser.cluster.cluster_parallel_parser'


class TestClusterParallelParser(unittest.TestCase):
    params = {"collection_path": '1', "model_id": 1, "iteration_id": 1}
    rank_ids = [1, 2, 3]

    def test_process(self):
        with mock.patch(NAMESPACE + ".ClusterParallelParser._parallel_analysis"), \
                mock.patch(NAMESPACE + ".ClusterParallelParser._storage_parallel_analysis_result"):
            check = ClusterParallelParser(self.params)
            check.process()