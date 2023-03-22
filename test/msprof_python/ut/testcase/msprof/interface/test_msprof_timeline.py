import json
import unittest
from unittest import mock

from common_func.empty_class import EmptyClass
from constant.constant import ITER_RANGE
from msinterface.msprof_timeline import MsprofTimeline

NAMESPACE = 'msinterface.msprof_timeline'


class TestMsprofTimeline(unittest.TestCase):
    def test_add_export_data(self):
        data = json.dumps(
            [{'name': 'process_name', 'pid': 0, 'tid': 0, 'args': {'name': 'Task Scheduler'}, 'ph': 'M'},
             {'name': 'trans_TransData_7', 'pid': 0, 'tid': '0', 'ts': 1210122,
              'args': {'Stream Id': 1, 'Task Id': 1}, 'ph': 'X'}])

        with mock.patch('common_func.ai_stack_data_check_manager.AiStackDataCheckManager.contain_acl_data',
                        return_value=True), \
                mock.patch(
                    'common_func.ai_stack_data_check_manager.AiStackDataCheckManager.contain_core_cpu_reduce_data',
                    return_value=True):
            key = MsprofTimeline()
            key.add_export_data(data, 'task_time')
            with mock.patch('common_func.db_manager.DBManager.fetch_all_data',
                            return_value=(
                                    (4294967295, 'Add', 1, 1, 32, 0, 'AI_CORE', 'Add', 0, 10621, 620199010400, 0),)):
                data = json.dumps(
                    [{'name': 'process_name', 'pid': 0, 'tid': 0, 'args': {'name': 'AscendCL'}, 'ph': 'M'},
                     {'name': 'aclopExecuteV2', 'pid': 0, 'tid': 0, 'ts': 620199010.277, 'dur': 569.359,
                      'args': {'Mode': 'ACL_OP'}, 'ph': 'X'}])
                key = MsprofTimeline()
                key.add_export_data(data, 'acl')

    def test_export_all_data(self):
        with mock.patch(NAMESPACE + '.StepTraceViewer.get_one_iter_timeline_data',
                        return_value=EmptyClass()):
            key = MsprofTimeline()
            key.export_all_data()

    def test_export_all_data_1(self):
        data = json.dumps([
            {'name': 'process_name', 'pid': 0, 'tid': 0, 'args': {'name': 'Step Trace'}, 'ph': 'M'},
            {'name': 'Reduce', 'pid': 0, 'ph': 'X', 'ts': 1210122}])
        with mock.patch(NAMESPACE + '.StepTraceViewer.get_one_iter_timeline_data',
                        return_value=data):
            key = MsprofTimeline()
            key.export_all_data()

    def test_is_in_iteration_1(self):
        from msinterface.msprof_timeline import MsprofTimeline
        time_stamp = {'ts': 111, 'dur': 2}
        key = MsprofTimeline()
        key._iteration_time = [1, 4]
        result = key.is_in_iteration(time_stamp)
        self.assertEqual(result, False)

    def test_is_in_iteration_2(self):
        from msinterface.msprof_timeline import MsprofTimeline
        time_stamp = {'ts': 1, 'dur': 1}
        key = MsprofTimeline()
        key._iteration_time = [1, 3]
        result = key.is_in_iteration(time_stamp)
        self.assertEqual(result, True)

    def test_set_iteration_info(self):
        with mock.patch(NAMESPACE + '.MsprofIteration.get_iteration_time', return_value=0):
            key = MsprofTimeline()
            key._iteration_time = [[1, 2], [1, 2]]
            key.set_iteration_info('test', ITER_RANGE)


if __name__ == '__main__':
    unittest.main()
