import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from msmodel.hardware.netdev_stats_model import NetDevStatsModel

NAMESPACE = "msmodel.hardware.netdev_stats_model"


class TestNetDevStatsModel(unittest.TestCase):

    def test_flush(self):
        data = [0, 980783386442350, 0, 0, 17794086897794, 17794908949726, 0, 0,
                273574917, 273636579, 0, 0, 0, 0, 1, 204255952, 973902252]
        with mock.patch(NAMESPACE + ".NetDevStatsModel.insert_data_to_db"):
            InfoConfReader()._info_json = {"devices": "0"}
            parser = NetDevStatsModel("test")
            parser.flush(data)
