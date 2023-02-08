import unittest
from unittest import mock

from analyzer.get_op_table_task_time import GetOpTableTsTime
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from profiling_bean.db_dto.step_trace_dto import IterationRange

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

    def test_get_sub_task_sql(self):
        check = GetOpTableTsTime({'iter_id': IterationRange(model_id=1,
                                                            iteration_id=1,
                                                            iteration_count=1), 'model_id': -1})
        ret = check._get_sub_task_sql()
        self.assertEqual("select task_id, stream_id, start_time, dur_time, "
                         "(case when subtask_type='AIC' then 'AI_CORE' when subtask_type='AIV' then 'AI_VECTOR_CORE' "
                         "else subtask_type end), 1, 1, 0, subtask_id from SubtaskTime order by start_time", ret)
