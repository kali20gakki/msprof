import os
import unittest
from unittest import mock

from tuning.profiling_tuning import ProfilingTuning
from common_func.info_conf_reader import InfoConfReader

NAMESPACE = 'tuning.profiling_tuning'


class TestProfilingTuning(unittest.TestCase):
    def test_run(self):
        operator_list = []
        InfoConfReader()._info_json = {'devices': '1'}
        with mock.patch('tuning.data_manager.DataLoader.get_data_by_infer_id',
                        return_value=operator_list), \
                mock.patch('tuning.tuning_control.PathManager.get_summary_dir',
                           return_value=os.path.join(os.path.dirname(__file__), 'test', 'summary')):
            ProfilingTuning.run('test', 1)
        for root, dirs, files in os.walk(os.path.join(os.path.dirname(__file__), 'test'), topdown=False):
            for name in files:
                print(name)
                os.remove(os.path.join(root, name))
            for name in dirs:
                print(name)
                os.rmdir(os.path.join(root, name))
        os.rmdir(os.path.join(os.path.dirname(__file__), 'test'))

