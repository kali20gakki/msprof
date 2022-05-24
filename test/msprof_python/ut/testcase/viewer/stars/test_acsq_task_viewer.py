import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from viewer.stars.acsq_task_viewer import AcsqTaskViewer

NAMESPACE = 'viewer.stars.acsq_task_viewer'


class TestAcsqTaskViewer(unittest.TestCase):

    def test_get_summary_data(self):
        config = {"headers": [1]}
        params = {}
        with mock.patch(NAMESPACE + '.AcsqTaskViewer.get_data_from_db',
                        return_value=[[1, 5, 6, 4, 6, 8, 0], [2, 3, 4, 5, 6, 7, 0]]):
            check = AcsqTaskViewer(config, params)
            InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 1000}]}
            ret = check.get_summary_data()
        self.assertEqual(ret, ([1], [[1, 5, 6, 4, 6.0, 8.0, 2.0], [2, 3, 4, 5, 6.0, 7.0, 1.0]], 2))

    def test_get_trace_timeline(self):
        acsq_data_list = [[1000, 2, 3, 'EVENT_RECORD_SQE', 66, 7, 8, 9, 54]]
        config = {}
        params = {}
        with mock.patch(NAMESPACE + '.TraceViewManager.time_graph_trace',
                        return_value=[]), \
             mock.patch(NAMESPACE + '.TraceViewManager.time_graph_trace',
                        return_value=[]):
            check = AcsqTaskViewer(config, params)
            InfoConfReader()._info_json = {'pid': 1, "DeviceInfo": [{'hwts_frequency': 1000}]}
            ret = check.get_trace_timeline(acsq_data_list)
            self.assertEqual(len(ret), 19)

    def test_get_timeline_header(self):
        config = {}
        params = {}
        check = AcsqTaskViewer(config, params)
        InfoConfReader()._info_json = {'pid': 1, "DeviceInfo": [{'hwts_frequency': 1000}]}
        ret = check.get_timeline_header()
        self.assertEqual(len(ret), 19)


if __name__ == '__main__':
    unittest.main()
