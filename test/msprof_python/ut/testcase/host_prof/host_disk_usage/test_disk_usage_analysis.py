import unittest
import pytest
from unittest import mock
from host_prof.host_disk_usage.disk_usage_analysis import DiskUsageAnalysis

NAMESPACE = 'host_prof.host_disk_usage.disk_usage_analysis'


class TestDiskUsageAnalysis(unittest.TestCase):

    def test_ms_run(self):
        with mock.patch('os.path.exists', return_value=False),\
                mock.patch(NAMESPACE + '.logging.error'):
            check = DiskUsageAnalysis({'result': 'test'})
            check.ms_run()

        with mock.patch('os.path.exists', return_value=True), \
                mock.patch(NAMESPACE + '.PathManager.get_data_dir',
                           return_value=True), \
                mock.patch('os.listdir', return_value=['host_disk.data.slice_0']), \
                mock.patch(NAMESPACE + '.is_valid_original_data', return_value=True), \
                mock.patch(NAMESPACE + '.HostDiskUsagePresenter.run'), \
                mock.patch('os.path.isfile', return_value=False), \
                mock.patch('host_prof.host_prof_base.host_prof_presenter_base.'
                           'PathManager.get_data_file_path', return_value='test'):
            check = DiskUsageAnalysis({'result': 'test'})
            check.ms_run()
