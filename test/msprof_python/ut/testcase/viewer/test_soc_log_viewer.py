import json
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.info_json_construct import DeviceInfo
from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager
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
        self.batch_id = args[14]


class TestFftsLogViewer(unittest.TestCase):

    def test_get_time_timeline_header(self):
        data = (['thread', 2, "3", 4, 5],)
        configs, params = {}, {}
        InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(pid='1', tid='0').device_info])).process()
        check = FftsLogViewer(configs, params)
        ret = check.get_time_timeline_header(data)
        self.assertEqual(ret, [['process_name', 0, 0, 'Task Scheduler'],
                               ['thread_name', 2, '3', 'Stream 3'],
                               ['thread_sort_index', 2, '3', '3']])

    def test_get_timeline_data(self):
        configs, params = {}, {}
        with mock.patch(NAMESPACE + '.FftsLogViewer.get_data_from_db', return_value=[]), \
                mock.patch(NAMESPACE + '.FftsLogViewer.get_trace_timeline', return_value=[]):
            check = FftsLogViewer(configs, params)
            ret = check.get_timeline_data()
            self.assertEqual(ret, json.dumps(
                {"status": 2, "info": "Can not export ffts sub task time data, the current chip does not support "
                                      "exporting this data or the data may be not collected."}))

    def test_get_trace_timeline(self):
        configs, params = {}, {}
        data = {"acsq_task_list": [TestData(105, 2, 30, 1, 'FFTS+', 949987561536, 3580, 'NA', 8, 9, 1, 2, 3, 1, 0)],
                "thread_data_list": [TestData(105, 2, 30, 1, 'FFTS+', 949987561536, 3580, 'NA', 8, 9, 1, 2, 3, 1, 0)],
                "subtask_data_list": [TestData(105, 2, 30, 1, 'FFTS+', 949987561536, 3580, 'NA', 8, 9, 1, 2, 3, 1, 0)]}
        with mock.patch('common_func.trace_view_manager.TraceViewManager.time_graph_trace', return_value=[]), \
                mock.patch('common_func.trace_view_manager.TraceViewManager.metadata_event', return_value=[]), \
                mock.patch(NAMESPACE + '.FftsLogViewer.add_node_name', return_value=data), \
                mock.patch(NAMESPACE + '.FftsLogViewer.get_time_timeline_header', return_value=()):
            check = FftsLogViewer(configs, params)
            InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
                DeviceInfo().device_info])).process()
            ret = check.get_trace_timeline(data)
            self.assertEqual(ret, [])

    def test_format_task_type_data(self):
        configs, params = {}, {'data_type': 'ffts_sub_task_time'}
        data = {"acsq_task_list": [TestData(105, 2, 30, 1, 'FFTS+', 949987561536, 3580, 'NA', 8, 9, 1, 2, 3, 1, 0)],
                "thread_data_list": [TestData(105, 2, 30, 1, 'FFTS+', 949987561536, 3580, 'NA', 8, 9, 1, 2, 3, 1, 0)],
                "subtask_data_list": [TestData(105, 2, 30, 1, 'FFTS+', 949987561536, 3580, 'NA', 8, 9, 1, 2, 3, 1, 0)]}
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
        data = [TestData(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 1, 0)]
        with mock.patch(NAMESPACE + '.ViewModel.init'), \
                mock.patch(NAMESPACE + '.ViewModel.get_all_data', return_value=data):
            check = FftsLogViewer(configs, params)
            ret = check.get_ge_data_dict()
            self.assertEqual(ret, ({'0-1-3': 4}, {}))

    def test_add_node_name(self):
        configs, params = {}, {}
        node_dict = {'1-2-3': 'translate'}
        data = {"thread_data_list": [TestData(3, 2, 1, 1, 'FFTS+', 949987561536, 3580, 7, 8, 9, 1, 2, 3, 1, 0)],
                "acsq_task_list": [TestData(0, 2, 1, 1, 'FFTS+', 949987561536, 3580, 7, 8, 9, 1, 2, 3, 1, 0)],
                "subtask_data_list": [TestData(3, 2, 1, 1, 'FFTS+', 949987561536, 3580, 7, 8, 9, 1, 2, 3, 1, 0)]}
        with mock.patch(NAMESPACE + '.FftsLogViewer.get_ge_data_dict', return_value=(node_dict, {})):
            check = FftsLogViewer(configs, params)
            check.add_node_name(data)
            self.assertTrue(isinstance(data.get('subtask_data_list', [])[0], TestData))


if __name__ == '__main__':
    unittest.main()
