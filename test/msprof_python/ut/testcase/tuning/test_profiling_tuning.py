import os
import unittest
from unittest import mock

from tuning.profiling_tuning import ProfilingTuning
from common_func.info_conf_reader import InfoConfReader

NAMESPACE = 'tuning.profiling_tuning'


class TestOperatorMetric(unittest.TestCase):

    def test_tuning_operator(self):
        with mock.patch(NAMESPACE + '.DataManager.get_data_by_infer_id',
                        return_value=False), \
                mock.patch(NAMESPACE + '.logging.warning'):
            ProfilingTuning.tuning_operator('test', '4', '1')
        operator_dicts = [
            {'model_name': 'ge_default_20210326081018_131', 'model_id': 12, 'task_id': 168, 'stream_id': 50,
             'infer_id': 1, 'op_name': 'Momentum', 'op_type': 'AssignAdd', 'task_type': 'AI_CORE',
             'task_start_time': 18997971010140, 'task_duration': 1.79, 'task_wait_time': 0.11, 'block_dim': 1,
             'input_shapes': '";"', 'input_data_types': 'DT_INT64;DT_INT64', 'input_formats': 'ND;ND',
             'output_shapes': '""', 'output_data_types': 'DT_INT64', 'output_formats': 'ND', 'aicore_time': 1.379,
             'total_cycles': 1378, 'vec_time': 0.002, 'vec_ratio': 0.00145, 'mac_time': 0.0, 'mac_ratio': 0.0,
             'scalar_time': 0.099, 'scalar_ratio': 0.071791, 'mte1_time': 0.0, 'mte1_ratio': 0.0, 'mte2_time': 0.774,
             'mte2_ratio': 0.561276, 'mte3_time': 0.126, 'mte3_ratio': 0.091371, 'icache_miss_rate': 0.000745,
             'memory_bound': '387.086897', 'core_num': 32, 'memory_workspace': 0}]
        with mock.patch(NAMESPACE + '.DataManager.get_data_by_infer_id',
                        return_value=operator_dicts):
            ProfilingTuning.tuning_operator('test', '4', '1')
        for root, dirs, files in os.walk('test', topdown=False):
            for name in files:
                print(name)
                os.remove(os.path.join(root, name))
            for name in dirs:
                print(name)
                os.rmdir(os.path.join(root, name))

    def test_load_rules(self):
        with mock.patch('builtins.open', side_effect=FileNotFoundError),\
                mock.patch(NAMESPACE + '.logging.error'):
            result = ProfilingTuning._load_rules()
        self.assertEqual(result, {})

    def test_tuning_network(self):
        operator_dicts = [
            {'model_name': 'ge_default_20210326081018_131', 'model_id': 12, 'task_id': 168, 'stream_id': 50,
             'infer_id': 1, 'op_name': 'Momentum', 'op_type': 'AssignAdd', 'task_type': 'AI_CORE',
             'task_start_time': 18997971010140, 'task_duration': 1.79, 'task_wait_time': 0.11, 'block_dim': 1,
             'input_shapes': '";"', 'input_data_types': 'DT_INT64;DT_INT64', 'input_formats': 'ND;ND',
             'output_shapes': '""', 'output_data_types': 'DT_INT64', 'output_formats': 'ND', 'aicore_time': 1.379,
             'total_cycles': 1379, 'vec_time': 0.002, 'vec_ratio': 0.00145, 'mac_time': 0.0, 'mac_ratio': 0.0,
             'scalar_time': 0.099, 'scalar_ratio': 0.071791, 'mte1_time': 0.0, 'mte1_ratio': 0.0, 'mte2_time': 0.774,
             'mte2_ratio': 0.561276, 'mte3_time': 0.126, 'mte3_ratio': 0.091371, 'icache_miss_rate': 0.000745,
             'memory_bound': '387.086897', 'core_num': 32, 'memory_workspace': 0}]
        with mock.patch(NAMESPACE + '.DataManager.get_data_by_infer_id',
                        return_value=operator_dicts):
            ProfilingTuning.tuning_network('test', '4', '1')
        for root, dirs, files in os.walk('test', topdown=False):
            for name in files:
                print(name)
                os.remove(os.path.join(root, name))
            for name in dirs:
                print(name)
                os.rmdir(os.path.join(root, name))

    def test_run(self):
        InfoConfReader()._info_json = {'devices': '4'}
        with mock.patch(NAMESPACE + '.DataManager.is_network', return_value=True), \
                mock.patch(NAMESPACE + '.ProfilingTuning.tuning_network'):
            ProfilingTuning.run('test')
        with mock.patch(NAMESPACE + '.DataManager.is_network', return_value=False), \
                mock.patch(NAMESPACE + '.ProfilingTuning.tuning_operator'):
            ProfilingTuning.run('test')
