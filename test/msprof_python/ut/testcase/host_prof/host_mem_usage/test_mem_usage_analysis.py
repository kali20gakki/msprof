import unittest
import pytest
from unittest import mock
from host_prof.host_mem_usage.mem_usage_analysis import MemUsageAnalysis

NAMESPACE = 'host_prof.host_mem_usage.mem_usage_analysis'


class TestMemUsageAnalysis(unittest.TestCase):

    def test_ms_run(self):
        with mock.patch('os.path.exists', return_value=False),\
                mock.patch(NAMESPACE + '.logging.error'):
            check = MemUsageAnalysis({'result': 'test'})
            check.ms_run()
        with mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.PathManager.get_data_dir',
                           return_value=True), \
                mock.patch('os.listdir', return_value=['host_mem.data.slice_0']), \
                mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch('host_prof.host_cpu_usage.presenter.'
                           'host_cpu_usage_presenter.HostCpuUsagePresenter.run'), \
                mock.patch('os.path.isfile', return_value=False), \
                mock.patch('host_prof.host_prof_base.host_prof_presenter_base.'
                           'PathManager.get_data_file_path', return_value='test'):
            check = MemUsageAnalysis({'result': 'test'})
            check.ms_run()
