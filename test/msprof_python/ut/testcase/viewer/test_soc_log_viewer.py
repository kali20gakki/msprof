import json
import unittest
from unittest import mock

from constant.info_json_construct import DeviceInfo
from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from viewer.stars.ffts_log_viewer import FftsLogViewer
from mscalculate.ascend_task.ascend_task import TopDownTask
from profiling_bean.db_dto.task_time_dto import TaskTimeDto

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
        with mock.patch(NAMESPACE + '.FftsLogViewer.get_ascend_task_data', return_value={}), \
                mock.patch(NAMESPACE + '.FftsLogViewer.get_trace_timeline', return_value=[]):
            check = FftsLogViewer(configs, params)
            ret = check.get_timeline_data()
            self.assertEqual(ret, json.dumps(
                {"status": 2, "info": "Can not export ffts sub task time data, the current chip does not support "
                                      "exporting this data or the data may be not collected."}))

    def test_get_trace_timeline(self):
        configs, params = {}, {}
        data = {
            "acsq_task_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU"),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV"),
            ],
        }
        setattr(data.get('acsq_task_list', [])[0], 'op_name', 'Add')
        setattr(data.get('subtask_data_list', [])[0], 'op_name', 'MatMul_1_lxslice1')
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
        data = {
            "acsq_task_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU"),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV"),
            ],
        }
        setattr(data.get('acsq_task_list', [])[0], 'op_name', 'Add')
        setattr(data.get('subtask_data_list', [])[0], 'op_name', 'MatMul_1_lxslice1')
        setattr(data.get('subtask_data_list', [])[0], 'thread_id', 1)
        with mock.patch('common_func.trace_view_manager.TraceViewManager.time_graph_trace', return_value=[]), \
                mock.patch('common_func.trace_view_manager.TraceViewManager.metadata_event', return_value=[]), \
                mock.patch(NAMESPACE + '.FftsLogViewer.add_node_name'), \
                mock.patch(NAMESPACE + '.FftsLogViewer.add_thread_id'), \
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
            self.assertEqual(ret, ({'0-1-3-0': 4}, {}))

    def test_add_node_name(self):
        configs, params = {}, {}
        node_dict = {'2-36-47-0': 'MatMul_1_lxslice1',
                     '2-27-4294967295-0': 'Add',
                     }
        data = {
            "acsq_task_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU"),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV"),
            ],
        }
        with mock.patch(NAMESPACE + '.FftsLogViewer.get_ge_data_dict', return_value=(node_dict, {})):
            check = FftsLogViewer(configs, params)
            check.add_node_name(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'MatMul_1_lxslice1')
            self.assertEqual(data.get('acsq_task_list', [])[0].op_name, 'Add')

    def test_get_ascend_task_data_should_split_acsq_task_and_subtask_by_context_id(self):
        configs = {}
        params = {"project": './'}
        data = [
            TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU"),
            TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV"),
        ]
        with mock.patch(NAMESPACE + '.AscendTaskModel.get_ascend_task_data_without_unknown', return_value=data):
            check = FftsLogViewer(configs, params)
            ret = check.get_ascend_task_data()
            self.assertEqual(len(ret), 2)
            self.assertEqual(len(ret.get('acsq_task_list', [])), 1)
            self.assertEqual(ret.get('acsq_task_list', [])[0].context_id, 4294967295)
            self.assertEqual(len(ret.get('subtask_data_list', [])), 1)
            self.assertEqual(ret.get('subtask_data_list', [])[0].context_id, 47)

    def test_add_thread_id_should_add_thread_id_when_matched(self):
        configs = {}
        params = {"project": './'}
        data = {
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV"),
            ],
        }
        subtask_data = [TaskTimeDto()]
        subtask_data[0].stream_id = 36
        subtask_data[0].task_id = 2
        subtask_data[0].subtask_id = 47
        subtask_data[0].start_time = 38140480645103
        subtask_data[0].thread_id = 10
        with mock.patch(NAMESPACE + '.SubTaskTimeModel.get_all_data', return_value=subtask_data), \
                mock.patch('msmodel.interface.parser_model.ParserModel.init'):
            check = FftsLogViewer(configs, params)
            check.add_thread_id(data)
            self.assertEqual(data.get("subtask_data_list")[0].thread_id, 10)

    def test_get_time_timeline_header_should_have_thread_name_when_thread_task_time(self):
        data = (['thread', 2, "Thread 0(AIV)", 4, 5],)
        configs, params = {}, {}
        InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(pid='1', tid='0').device_info])).process()
        check = FftsLogViewer(configs, params)
        ret = check.get_time_timeline_header(data, TraceViewHeaderConstant.PROCESS_THREAD_TASK)
        self.assertEqual(ret, [['process_name', 2, 0, 'Thread Task Time'],
                               ['thread_name', 2, 'Thread 0(AIV)', 'Thread 0(AIV)'],
                               ['thread_sort_index', 2, 'Thread 0(AIV)', 'Thread 0(AIV)']])


if __name__ == '__main__':
    unittest.main()
