import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from msparser.cluster.cluster_info_parser import ClusterInfoParser

NAMESPACE = 'msparser.cluster.cluster_info_parser'


class TestClusterInfoParser(unittest.TestCase):
    collect_path = '1'
    cluster_device_paths = ['2']
    info_json = {"rank_id": 1, "jobInfo": 1, "devices": 1, }
    start_info = {"collectionDateBegin": 1}
    end_info = {"collectionDateEnd": 2}

    def test_ms_run(self):
        with mock.patch(NAMESPACE + ".ClusterInfoParser.parse"), \
                mock.patch(NAMESPACE + ".ClusterInfoParser.save"):
            check = ClusterInfoParser(self.collect_path, self.cluster_device_paths)
            check.ms_run()

    def test_save(self):
        check = ClusterInfoParser(self.collect_path, self.cluster_device_paths)
        check.cluster_info_list = [[1, 1, 1, 1, 1]]
        check.save()