import unittest
from unittest import mock

from constant.info_json_construct import InfoJsonReaderManager
from constant.info_json_construct import DeviceInfo
from constant.info_json_construct import InfoJson
from common_func.info_conf_reader import InfoConfReader
from viewer.stars.ffts_log_viewer import FftsLogViewer

NAMESPACE = 'viewer.stars.ffts_log_viewer'


class TestData:

    def __init__(self, *args):
        self.task_id = args[0]
        self.stream_id = args[1]
        self.subtask_id = args[2]
        self.context_id = args[3]
        self.op_name = args[4]
        self.task_type = args[5]
        self.ffts_type = args[6]
        self.subtask_type = args[7]
        self.task_time = args[8]
        self.dur_time = args[9]
        self.start_time = args[10]
        self.end_time = args[11]
        self.ffts_type = args[12]
        self.thread_id = args[13]


class TestFftsLogViewer(unittest.TestCase):

    def test_get_time_timeline_header(self):
        data = (['thread', 2, "3", 4, 5],)
        configs, params = {}, {}
        InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(pid='1', tid='0').device_info])).process()
        check = FftsLogViewer(configs, params)
        ret = check.get_time_timeline_header(data)
        self.assertEqual(ret, [['process_name', 1000, 0, 'Subtask Time'],
                               ['thread_name', 2, '3', '3']])

    def test_get_trace_timeline(self):
        configs, params = {}, {}
        data = {"thread_data_list": [TestData(105, 2, 30, 1, 'FFTS+', 949987561536, 3580, 'NA', 8, 9, 1, 2, 3, 1)],
                "subtask_data_list": [TestData(105, 2, 30, 1, 'FFTS+', 949987561536, 3580, 'NA', 8, 9, 1, 2, 3, 1)]}
        with mock.patch('common_func.trace_view_manager.TraceViewManager.time_graph_trace', return_value=[]), \
                mock.patch('common_func.trace_view_manager.TraceViewManager.metadata_event', return_value=[]), \
                mock.patch(NAMESPACE + '.FftsLogViewer.add_node_name', return_value=data), \
                mock.patch(NAMESPACE + '.FftsLogViewer.get_time_timeline_header', return_value=()):
            check = FftsLogViewer(configs, params)
            InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
                DeviceInfo().device_info])).process()
            ret = check.get_trace_timeline(data)
            self.assertEqual(ret, [])

    def test_get_node_name(self):
        configs, params = {}, {}
        data = [TestData(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 1)]
        with mock.patch(NAMESPACE + '.ViewModel.init'), \
                mock.patch(NAMESPACE + '.ViewModel.get_all_data', return_value=data):
            check = FftsLogViewer(configs, params)
            ret = check.get_node_name()
            self.assertEqual(ret, {'0-1-3': 4})

    def test_add_node_name(self):
        configs, params = {}, {}
        node_dict = {'1-2-3': 'translate'}
        data = {"thread_data_list": [TestData(3, 2, 1, 1, 'FFTS+', 949987561536, 3580, 7, 8, 9, 1, 2, 3, 1)],
                "subtask_data_list": [TestData(3, 2, 1, 1, 'FFTS+', 949987561536, 3580, 7, 8, 9, 1, 2, 3, 1)]}
        with mock.patch(NAMESPACE + '.FftsLogViewer.get_node_name', return_value=node_dict):
            check = FftsLogViewer(configs, params)
            ret = check.add_node_name(data)
            self.assertTrue(isinstance(ret.get('subtask_data_list', [])[0], TestData))


if __name__ == '__main__':
    unittest.main()
