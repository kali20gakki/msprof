import json
import unittest
from unittest import mock
from collections import OrderedDict

from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from constant.info_json_construct import DeviceInfo
from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager
from mscalculate.ascend_task.ascend_task import TopDownTask
from profiling_bean.db_dto.task_time_dto import TaskTimeDto
from profiling_bean.db_dto.step_trace_dto import MsproftxMarkDto
from profiling_bean.prof_enum.chip_model import ChipModel
from viewer.task_time_viewer import TaskTimeViewer

NAMESPACE = 'viewer.task_time_viewer'


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


class TestTaskTimeViewer(unittest.TestCase):

    def test_get_time_timeline_header(self):
        data = (['thread', 2, 3, 4, 5],)
        configs, params = {}, {}
        InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(pid='1', tid='0').device_info])).process()
        check = TaskTimeViewer(configs, params)
        ret = check.get_time_timeline_header(data)
        self.assertEqual(ret, [['process_name', 1000, 0, 'Task Scheduler'],
                               ['thread_name', 2, 3, 'Stream 3'],
                               ['thread_sort_index', 2, 3, 3]])

    def test_get_timeline_data(self):
        configs, params = {}, {"project": ""}
        with mock.patch(NAMESPACE + '.TaskTimeViewer.get_ascend_task_data', return_value={}), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.get_trace_timeline', return_value=[]):
            check = TaskTimeViewer(configs, params)
            ret = check.get_timeline_data()
            self.assertEqual(ret, [])

    def test_get_trace_timeline(self):
        configs, params = {}, {}
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 1),
            ],
        }
        setattr(data.get('task_data_list', [])[0], 'op_name', 'Add')
        setattr(data.get('subtask_data_list', [])[0], 'op_name', 'MatMul_1_lxslice1')
        with mock.patch('common_func.trace_view_manager.TraceViewManager.time_graph_trace', return_value=[]), \
                mock.patch('common_func.trace_view_manager.TraceViewManager.metadata_event', return_value=[]), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.add_node_name', return_value=data), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.get_time_timeline_header', return_value=()):
            check = TaskTimeViewer(configs, params)
            InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
                DeviceInfo().device_info])).process()
            ret = check.get_trace_timeline(data)
            self.assertEqual(ret, [])

    def test_format_task_type_data(self):
        configs, params = {}, {'data_type': 'ffts_sub_task_time'}
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 1),
            ],
        }
        setattr(data.get('task_data_list', [])[0], 'op_name', 'Add')
        setattr(data.get('subtask_data_list', [])[0], 'op_name', 'MatMul_1_lxslice1')
        setattr(data.get('subtask_data_list', [])[0], 'thread_id', 1)
        with mock.patch('common_func.trace_view_manager.TraceViewManager.time_graph_trace', return_value=[]), \
                mock.patch('common_func.trace_view_manager.TraceViewManager.metadata_event', return_value=[]), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.add_node_name'), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.add_thread_id'), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.get_time_timeline_header', return_value=()):
            check = TaskTimeViewer(configs, params)
            InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
                DeviceInfo().device_info])).process()
            ret = check.get_trace_timeline(data)
            self.assertEqual(ret, [])

    def test_get_node_name(self):
        configs, params = {}, {}
        data = [TestData(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 1, 0)]
        with mock.patch(NAMESPACE + '.ViewModel.init'), \
                mock.patch(NAMESPACE + '.ViewModel.get_all_data', return_value=data):
            check = TaskTimeViewer(configs, params)
            ret = check.get_ge_data_dict()
            self.assertEqual(ret, {'0-1-3-0': 4})

    def test_get_device_task_type_should_return_device_type_when_device_is_not_number(self):
        configs, params = {}, {'data_type': 'ffts_sub_task_time'}
        check = TaskTimeViewer(configs, params)
        ChipManager().chip_id = ChipModel.CHIP_V3_1_0
        result = check.get_device_task_type("AI_CPU")
        self.assertEqual(result, "AI_CPU")

    def test_get_device_task_type_should_return_other_when_v3_1_0_and_device_belongs_to_sqetype(self):
        configs, params = {}, {'data_type': 'ffts_sub_task_time'}
        check = TaskTimeViewer(configs, params)
        ChipManager().chip_id = ChipModel.CHIP_V3_1_0
        result = check.get_device_task_type("0")
        self.assertEqual(result, "AI_CORE")

    def test_get_device_task_type_should_return_other_when_v3_1_0_and_device_not_belongs_to_sqetype(self):
        configs, params = {}, {'data_type': 'ffts_sub_task_time'}
        check = TaskTimeViewer(configs, params)
        ChipManager().chip_id = ChipModel.CHIP_V3_1_0
        result = check.get_device_task_type("25")
        self.assertEqual(result, "Other")

    def test_get_device_task_type_should_return_other_when_v2_1_0_and_device_is_number(self):
        configs, params = {}, {'data_type': 'ffts_sub_task_time'}
        check = TaskTimeViewer(configs, params)
        ChipManager().chip_id = ChipModel.CHIP_V2_1_0
        result = check.get_device_task_type("0")
        self.assertEqual(result, "Other")

    def test_get_device_task_type_should_return_other_when_v2_1_0_and_device_is_number(self):
        configs, params = {}, {'data_type': 'ffts_sub_task_time'}
        InfoConfReader()._info_json = {"pid": "0"}
        check = TaskTimeViewer(configs, params)
        ChipManager().chip_id = ChipModel.CHIP_V2_1_0
        result = check.get_device_task_type("0")
        self.assertEqual(result, "Other")
        InfoConfReader()._info_json = {}

    def test_get_device_task_type_should_return_sdma_when_v3_and_device_is_number(self):
        configs, params = {}, {'data_type': 'ffts_sub_task_time'}
        InfoConfReader()._info_json = {"pid": "0"}
        check = TaskTimeViewer(configs, params)
        ChipManager().chip_id = ChipModel.CHIP_V3_1_0
        result = check.get_device_task_type("9")
        self.assertEqual(result, "SDMA_SQE")
        InfoConfReader()._info_json = {}

    def test_get_device_task_type_should_return_sdma_when_stars_and_device_is_number(self):
        configs, params = {}, {'data_type': 'ffts_sub_task_time'}
        InfoConfReader()._info_json = {"pid": "0"}
        check = TaskTimeViewer(configs, params)
        ChipManager().chip_id = ChipModel.CHIP_V4_1_0
        result = check.get_device_task_type("11")
        self.assertEqual(result, "SDMA_SQE")

        ChipManager().chip_id = ChipModel.CHIP_V1_1_1
        result = check.get_device_task_type("11")
        self.assertEqual(result, "SDMA_SQE")
        InfoConfReader()._info_json = {}

    def test_add_node_name(self):
        configs, params = {}, {}
        node_dict = {
            '2-36-47-0': 'MatMul_1_lxslice1',
            '2-27-4294967295-0': 'Add',
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 1),
            ],
        }
        with mock.patch(NAMESPACE + '.TaskTimeViewer.get_ge_data_dict', return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            check.add_node_name(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'MatMul_1_lxslice1')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'Add')

    def test_add_node_name_should_return_device_when_host_is_ffts_plus(self):
        configs, params = {}, {}
        node_dict = {
            '3-36-47-0': 'MatMul_1_lxslice1',
            '3-27-4294967295-0': 'Add',
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "FFTS_PLUS", "AIV", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 1),
            ],
        }
        with mock.patch(NAMESPACE + '.TaskTimeViewer.get_ge_data_dict', return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            check.add_node_name(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'AIV')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'AIV')

    def test_add_node_name_should_return_op_name_when_op_name_exists_and_host_is_unknown(self):
        configs, params = {}, {}
        node_dict = {
            '2-36-47-0': 'MatMul_1_lxslice1',
            '2-27-4294967295-0': 'MatMul/v2_MemSet',
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "UNKNOWN", "AI_CPU", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "AIV", 1),
            ],
        }
        with mock.patch(NAMESPACE + '.TaskTimeViewer.get_ge_data_dict', return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            check.add_node_name(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'MatMul_1_lxslice1')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'MatMul/v2_MemSet')

    def test_add_node_name_should_return_other_when_host_is_unknown_and_device_not_belongs_to_sqetype(self):
        configs, params = {}, {}
        node_dict = {
            '3-36-47-0': 'MatMul/v2_MemSet',
            '3-27-4294967295-0': 'Add',
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICORE", "AI_CORE", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "25", 1),
            ],
        }
        with mock.patch(NAMESPACE + '.TaskTimeViewer.get_ge_data_dict', return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            ChipManager().chip_id = ChipModel.CHIP_V3_1_0
            check.add_node_name(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'Other')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'KERNEL_AICORE')

    def test_add_node_name_should_return_sqetype_when_host_is_unknown_and_device_belongs_to_sqetype(self):
        configs, params = {}, {}
        node_dict = {
            '3-36-47-0': 'MatMul/v2_MemSet',
            '3-27-4294967295-0': 'aclnnAdd_AddAiCore_Add',
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "FFTS_PLUS", "AI_CPU", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "5", 1),
            ],
        }
        with mock.patch(NAMESPACE + '.TaskTimeViewer.get_ge_data_dict', return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            ChipManager().chip_id = ChipModel.CHIP_V1_1_0
            check.add_node_name(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'EVENT_WAIT_SQE')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'AI_CPU')

    def test_add_node_name_should_return_device_when_v2_1_0_and_host_is_unknown_and_device_is_not_number(self):
        configs, params = {}, {}
        node_dict = {
            '3-36-47-0': 'MatMul_1_lxslice1',
            '3-27-4294967295-0': 'aclnnCast_CastAiCore_Cast',
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "FFTS_PLUS", "AI_CPU", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "AIV", 1),
            ],
        }
        with mock.patch(NAMESPACE + '.TaskTimeViewer.get_ge_data_dict', return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            ChipManager().chip_id = ChipModel.CHIP_V2_1_0
            check.add_node_name(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'AIV')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'AI_CPU')

    def test_add_node_name_should_return_device_when_chip_v1_1_0_and_host_is_unknown_and_device_is_not_number(self):
        configs, params = {}, {}
        node_dict = {
            '3-36-47-0': 'MatMul/v2_MemSet',
            '3-27-4294967295-0': 'aclnnAny_ReduceAny_MemSet',
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "FFTS_PLUS", "AI_CPU", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "SDMA", 1),
            ],
        }
        with mock.patch(NAMESPACE + '.TaskTimeViewer.get_ge_data_dict', return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            ChipManager().chip_id = ChipModel.CHIP_V1_1_0
            check.add_node_name(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'SDMA')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'AI_CPU')

    def test_add_node_name_should_return_device_when_chip_v1_1_0_and_host_is_unknown_and_device_is_not_number(self):
        configs, params = {}, {}
        node_dict = {
            '3-36-47-0': 'MatMul_1_lxslice1',
            '3-27-4294967295-0': 'aclnnAdd_AddAiCore_Add',
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "5", 1),
            ],
        }
        with mock.patch(NAMESPACE + '.TaskTimeViewer.get_ge_data_dict', return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            ChipManager().chip_id = ChipModel.CHIP_V1_1_0
            check.add_node_name(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'EVENT_WAIT_SQE')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'KERNEL_AICPU')

    def test_add_node_name_should_return_other_when_v2_1_0_and_host_is_unknown_and_device_belongs_to_sqetype(self):
        configs, params = {}, {}
        node_dict = {
            '3-36-47-0': 'aclnnAny_ReduceAny_MemSet',
            '3-27-4294967295-0': 'aclnnAdd_AddAiCore_Add',
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "15", 1),
            ],
        }
        with mock.patch(NAMESPACE + '.TaskTimeViewer.get_ge_data_dict', return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            ChipManager().chip_id = ChipModel.CHIP_V2_1_0
            check.add_node_name(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'Other')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'KERNEL_AICPU')

    def test_add_node_name_should_return_other_when_v2_1_0_and_host_is_unknown_and_device_not_belongs_to_sqetype(self):
        configs, params = {}, {}
        node_dict = {
            '3-36-47-0': 'aclnnAdd_AddAiCore_Add',
            '3-27-4294967295-0': 'aclnnAny_ReduceAny_MemSet',
        }
        data = {
            "task_data_list": [
                TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "EVENT_WAIT", "0", 0),
            ],
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "UNKNOWN", "25", 1),
            ],
        }
        with mock.patch(NAMESPACE + '.TaskTimeViewer.get_ge_data_dict', return_value=node_dict):
            check = TaskTimeViewer(configs, params)
            ChipManager().chip_id = ChipModel.CHIP_V2_1_0
            check.add_node_name(data)
            self.assertEqual(data.get('subtask_data_list', [])[0].op_name, 'Other')
            self.assertEqual(data.get('task_data_list', [])[0].op_name, 'EVENT_WAIT')

    def test_get_ascend_task_data_should_split_acsq_task_and_subtask_by_context_id(self):
        configs = {}
        params = {"project": './'}
        data = [
            TopDownTask(0, 1, 27, 2, 4294967295, 0, 38140478706523, 1510560, "KERNEL_AICPU", "AI_CPU", 0),
            TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 1),
        ]
        with mock.patch(NAMESPACE + '.AscendTaskModel.get_ascend_task_data_without_unknown', return_value=data):
            check = TaskTimeViewer(configs, params)
            ret = check.get_ascend_task_data()
            self.assertEqual(len(ret), 2)
            self.assertEqual(len(ret.get('task_data_list', [])), 1)
            self.assertEqual(ret.get('task_data_list', [])[0].context_id, 4294967295)
            self.assertEqual(len(ret.get('subtask_data_list', [])), 1)
            self.assertEqual(ret.get('subtask_data_list', [])[0].context_id, 47)

    def test_add_thread_id_should_add_thread_id_when_matched(self):
        configs = {}
        params = {"project": './'}
        data = {
            "subtask_data_list": [
                TopDownTask(0, 1, 36, 2, 47, 0, 38140480645103, 12400, "FFTS_PLUS", "AIV", 0),
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
            check = TaskTimeViewer(configs, params)
            check.add_thread_id(data)
            self.assertEqual(data.get("subtask_data_list")[0].thread_id, 10)

    def test_get_time_timeline_header_should_have_thread_name_when_thread_task_time(self):
        data = (['thread', 2, 1, 4, 5],)
        configs, params = {}, {}
        InfoJsonReaderManager(info_json=InfoJson(devices='0', DeviceInfo=[
            DeviceInfo(pid='1', tid='0').device_info])).process()
        check = TaskTimeViewer(configs, params)
        ret = check.get_time_timeline_header(data, TraceViewHeaderConstant.PROCESS_THREAD_TASK)
        self.assertEqual(ret, [['process_name', 2, 0, 'Thread Task Time'],
                               ['thread_name', 2, 1, 'Thread 0(AIV)'],
                               ['thread_sort_index', 2, 1, 1]])

    def test_get_timeline_data_with_empty_msproftx_data_with_no_device_msproftx_data(self):
        configs, params = {}, {"project": ""}
        with mock.patch('viewer.memory_copy.memory_copy_viewer.MemoryCopyViewer.get_memory_copy_timeline',
                        return_value=[]), \
                mock.patch("common_func.msvp_common.path_check", return_value=False), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.get_ascend_task_data', return_value={}), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.get_trace_timeline', return_value=[]):
            check = TaskTimeViewer(configs, params)
            ret = check.get_timeline_data()
            self.assertEqual(ret, [])

    def test_get_timeline_data_with_valid_msproftx_data_with_device_msproftx_data(self):
        InfoConfReader()._dev_cnt = 0.0
        InfoConfReader()._host_mon = 0.0
        InfoConfReader()._local_time_offset = 10.0
        device_msproftx_data_dto = MsproftxMarkDto(0, 43114577937563, 0, 0)
        expected_trace_data = [
            OrderedDict([
                ('name', 'MsTx_0'), (TraceViewHeaderConstant.TRACE_HEADER_PID, 1000), ('tid', 0),
                ('ts', '1658653708018104.830'), ('dur', 0),
                ('args', {'Physic Stream Id': 0, 'Task Id': 0}), ('ph', 'X')]),
            {
                'bp': 'e', 'cat': 'MsTx', 'id': '0', 'name': 'MsTx_0', 'ph': 'f',
                TraceViewHeaderConstant.TRACE_HEADER_PID: 1000, 'tid': 0, 'ts': '1658653708018104.830'
            }
        ]
        configs, params = {}, {"project": ""}
        with mock.patch('viewer.memory_copy.memory_copy_viewer.MemoryCopyViewer.get_memory_copy_timeline',
                        return_value=[]), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.get_msproftx_ex_task_data',
                           return_value=[device_msproftx_data_dto]), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.get_ascend_task_data', return_value={}), \
                mock.patch(NAMESPACE + '.TaskTimeViewer.get_trace_timeline', return_value=[]):
            check = TaskTimeViewer(configs, params)
            InfoJsonReaderManager(info_json=InfoJson(devices='0',
                                                     DeviceInfo=[{"hwts_frequency": "100"}], pid='0')).process()
            ret = check.get_timeline_data()
            self.assertEqual(ret, expected_trace_data)

if __name__ == '__main__':
    unittest.main()
