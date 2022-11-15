import unittest
from unittest import mock

from analyzer.get_op_table_task_time import GetOpTableTsTime
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant

NAMESPACE = 'analyzer.get_op_table_task_time'
NAMESPACE_BASEMODEL = 'msmodel.interface.base_model.BaseModel'
CONFIG = {'result_dir': 'test', 'device_id': '0', 'iter_id': 1, 'model_id': -1}


class GeData:
    def __init__(self, ge_data):
        self.batch_id = ge_data[0]
        self.context_id = ge_data[1]
        self.op_name = ge_data[2]
        self.stream_id = ge_data[3]
        self.task_id = ge_data[4]
        self.task_type = ge_data[5]


class TestGetOpTableTsTime(unittest.TestCase):

    def setUp(self) -> None:
        ProfilingScene().init("")

    def test_get_task_time_data_return_empty_data_when_table_not_exist(self):
        with mock.patch(NAMESPACE + '.ViewModel'), \
                mock.patch(NAMESPACE + '.ViewModel.check_table', return_value=False):
            check = GetOpTableTsTime(CONFIG)
            ret = check.get_task_time_data()
            self.assertEqual([], ret)

    def test_get_task_time_data_single_op_scene_return_empty_data_when_table_exist(self):
        with mock.patch(NAMESPACE_BASEMODEL + '.init'), \
                mock.patch(NAMESPACE_BASEMODEL + '.finalize'), \
                mock.patch(NAMESPACE_BASEMODEL + '.check_table', return_value=True), \
                mock.patch('common_func.utils.Utils.get_scene', return_value="single_op"), \
                mock.patch(NAMESPACE + '.ViewModel.get_sql_data', return_value=[]):
            check = GetOpTableTsTime(CONFIG)
            ret = check.get_task_time_data()
            self.assertEqual([], ret)

    def test_get_task_time_data_step_scene_return_empty_data_when_table_exist(self):
        with mock.patch(NAMESPACE_BASEMODEL + '.init'), \
                mock.patch(NAMESPACE_BASEMODEL + '.finalize'), \
                mock.patch(NAMESPACE_BASEMODEL + '.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.ViewModel.get_sql_data', return_value=[]), \
                mock.patch('common_func.utils.Utils.get_scene', return_value="single_op"), \
                mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=[(1, 2)]):
            check = GetOpTableTsTime(CONFIG)
            ret = check.get_task_time_data()
            self.assertEqual([], ret)

    def test_get_task_time_data(self):
        ge_data = [GeData([0, 4294967295, None, 25, 0, 'AI_CORE']),
                   GeData([0, 0, None, 25, 1, 'AI_CORE']),
                   GeData([0, 0, None, 25, 0, 'MIX_AIC'])]
        with mock.patch(NAMESPACE_BASEMODEL + '.init'), \
                mock.patch(NAMESPACE_BASEMODEL + '.finalize'), \
                mock.patch(NAMESPACE_BASEMODEL + '.check_table', return_value=True), \
                mock.patch(NAMESPACE + '.ViewModel.get_sql_data', return_value=[]), \
                mock.patch('common_func.utils.Utils.get_scene', return_value="single_op"), \
                mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=[(1, 2)]):
            check = GetOpTableTsTime(CONFIG)
            ret = check.get_task_time_data(ge_data)
            self.assertEqual([], ret)
