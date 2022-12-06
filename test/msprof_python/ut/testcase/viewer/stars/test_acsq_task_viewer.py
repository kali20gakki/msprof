import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from viewer.stars.acsq_task_viewer import AcsqTaskViewer

NAMESPACE = 'viewer.stars.acsq_task_viewer'


class TestTaskData:

    def __init__(self, *args):
        self.op_name = args[0]
        self.task_type = args[1]
        self.stream_id = args[2]
        self.task_id = args[3]
        self.task_time = args[4]
        self.start_time = args[5]
        self.end_time = args[6]


class TestAcsqTaskViewer(unittest.TestCase):

    def test_get_summary_data(self):
        config = {"headers": [1], 'result_dir': 'test'}
        with mock.patch('msmodel.stars.acsq_task_model.AcsqTaskModel.get_summary_data',
                        return_value=[TestTaskData(1, 5, 6, 4, 6, 8, 0), TestTaskData(2, 3, 4, 5, 6, 7, 0)]):
            check = AcsqTaskViewer(config)
            InfoConfReader()._info_json = {"DeviceInfo": [{'hwts_frequency': 1000}]}
            ret = check.get_summary_data([1])
        self.assertEqual(ret, ([1], [[1, 5, 6, 4, 6, 8, 0], [2, 3, 4, 5, 6, 7, 0]], 2))

    def test_get_trace_timeline(self):
        acsq_data_list = [[1000, 2, 3, 'EVENT_RECORD_SQE', 66, 7, 8, 9, 54]]
        config = {}
        with mock.patch(NAMESPACE + '.TraceViewManager.time_graph_trace',
                        return_value=[]), \
             mock.patch(NAMESPACE + '.TraceViewManager.time_graph_trace',
                        return_value=[]):
            check = AcsqTaskViewer(config)
            InfoConfReader()._info_json = {'pid': 1, "DeviceInfo": [{'hwts_frequency': 1000}]}
            ret = check.get_trace_timeline(acsq_data_list)
            self.assertEqual(len(ret), 22)

    def test_get_timeline_header(self):
        config = {}
        check = AcsqTaskViewer(config)
        InfoConfReader()._info_json = {'pid': 1, "DeviceInfo": [{'hwts_frequency': 1000}]}
        ret = check.get_timeline_header()
        self.assertEqual(len(ret), 22)


if __name__ == '__main__':
    unittest.main()
