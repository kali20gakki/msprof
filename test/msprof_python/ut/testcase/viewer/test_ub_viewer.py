import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from profiling_bean.db_dto.ub_dto import UBDto
from viewer.ub_viewer import UBViewer

NAMESPACE = 'msmodel.hardware.ub_model'


class TestUBViewer(unittest.TestCase):
    def test_get_trace_timeline_should_return_timeline_format(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "50"}]}
        configs = {}
        param = {}
        datas = [
            UBDto(7, 0, 508694802700, 800, 1000, 780, 500, 1000, 900, 10, 0, 870, 600, 1200, 80, 8, 3),
            UBDto(6, 0, 508965580266, 620, 1100, 560, 550, 999, 880, 8, 2, 888, 200, 1211, 85, 9, 1)
        ]
        with mock.patch(NAMESPACE + '.UBViewModel.check_table', return_value=True):
            check = UBViewer(configs, param)
            ret = check.get_trace_timeline(datas)
            self.assertEqual(5, len(ret))

    def test_format_ub_summary_data_should_return_formatted_summary_data(self):
        InfoConfReader()._info_json = {"pid": 1, "tid": 1, "DeviceInfo": [{"hwts_frequency": "50"}]}
        configs = {}
        param = {}
        datas = [
            UBDto(7, 0, 508694802700, 800, 1000, 780, 500, 1000, 900, 10, 0, 870, 600, 1200, 80, 8, 3),
            UBDto(6, 0, 508965580266, 620, 1100, 560, 550, 999, 880, 8, 2, 888, 200, 1211, 85, 9, 1)
        ]
        with mock.patch(NAMESPACE + '.UBViewModel.check_table', return_value=True):
            check = UBViewer(configs, param)
            ret = check.format_ub_summary_data(datas)
            self.assertEqual(2, len(ret))
