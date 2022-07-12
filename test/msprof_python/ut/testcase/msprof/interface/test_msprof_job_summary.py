import unittest
from unittest import mock

from msinterface.msprof_job_summary import MsprofJobSummary

NAMESPACE = 'msinterface.msprof_job_summary'


class TestMsprofJobSummary(unittest.TestCase):
    def test_export(self):
        with mock.patch(NAMESPACE + '.MsprofJobSummary._export_msprof_timeline'):
            MsprofJobSummary('test').export()

    def test_export_msprof_timeline(self):
        with mock.patch(NAMESPACE + '.MsprofJobSummary._get_host_timeline_data', return_value=[]), \
                mock.patch(NAMESPACE + '.logging.info'):
            MsprofJobSummary('test')._export_msprof_timeline()
        with mock.patch(NAMESPACE + '.MsprofJobSummary._get_host_timeline_data', return_value=[1]), \
                mock.patch(NAMESPACE + '.MsprofJobSummary._export_all_device_data'):
            MsprofJobSummary('test')._export_msprof_timeline()

    def test_export_all_device_data(self):
        with mock.patch(NAMESPACE + '.get_path_dir', return_value=['device']), \
                mock.patch('os.path.join', return_value='test'), \
                mock.patch('os.path.realpath', return_value='test'), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofJobSummary._generate_json_file'):
            MsprofJobSummary('test')._export_all_device_data()

    def test_get_host_timeline_data(self):
        with mock.patch('os.path.join', return_value='test'), \
                mock.patch(NAMESPACE + '.MsprofJobSummary.get_msprof_json_file', return_value=[1]):
            result = MsprofJobSummary('test')._get_host_timeline_data()
        self.assertEqual(result, [1])

    def test_generate_json_file(self):
        with mock.patch('os.path.join', return_value='test'), \
                mock.patch(NAMESPACE + '.MsprofJobSummary.get_msprof_json_file', return_value=[{}]), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.write_json_files', return_value=(0, [])):
            check = MsprofJobSummary('test')
            check._host_data = [{'ts': 1}]
            check._generate_json_file('test')
        with mock.patch('os.path.join', return_value='test'), \
                mock.patch(NAMESPACE + '.MsprofJobSummary.get_msprof_json_file', return_value=[{}]):
            with mock.patch('os.open'), \
                    mock.patch('os.fdopen', mock.mock_open(read_data='')), \
                    mock.patch(NAMESPACE + '.MsprofDataStorage.write_json_files', return_value=(0, [])):
                check = MsprofJobSummary('test')
                check._host_data = [{'ts': 1}]
                check._generate_json_file('test')
        with mock.patch('os.path.join', return_value='test'), \
                mock.patch(NAMESPACE + '.MsprofJobSummary.get_msprof_json_file', return_value=[{}]):
            with mock.patch(NAMESPACE + '.MsprofJobSummary.get_msprof_json_file', return_value=[{}]), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.write_json_files', return_value=(1, 'test')),\
                    mock.patch('os.fdopen', mock.mock_open(read_data='')),\
                    mock.patch('common_func.utils.logging.error'):
                check = MsprofJobSummary('test')
                check._host_data = [{'ts': 1}]
                check._generate_json_file('test')

    def test_get_msprof_json_file(self):
        with mock.patch(NAMESPACE + '.get_path_dir', return_value=['device']), \
                mock.patch('os.path.join', return_value='test'), \
                mock.patch('os.path.realpath', return_value='test'):
            with mock.patch('os.path.exists', return_value=False):
                result = MsprofJobSummary('test').get_msprof_json_file('test')
                self.assertEqual(result, [])
            with mock.patch('os.path.exists', return_value=True), \
                    mock.patch('os.listdir', return_value=['msprof_1_1.json']), \
                    mock.patch(NAMESPACE + '.Utils.get_json_data', return_value=[123]):
                result = MsprofJobSummary('test').get_msprof_json_file('test')
                self.assertEqual(result, [123])

    def test_get_json_data(self):
        with mock.patch(NAMESPACE + '.get_path_dir', return_value=['device']), \
                mock.patch('os.path.join', return_value='test'), \
                mock.patch('os.listdir', return_value=['msprof_1_1.json']), \
                mock.patch('os.path.realpath', return_value='test'):
            with mock.patch('os.path.exists', return_value=True), \
                    mock.patch('os.path.isfile', return_value=False), \
                    mock.patch(NAMESPACE + '.check_path_valid', return_value=True):
                result = MsprofJobSummary('test').get_msprof_json_file('test')
                self.assertEqual(result, [])
            with mock.patch('os.path.exists', return_value=True), \
                    mock.patch('os.path.isfile', return_value=True), \
                    mock.patch(NAMESPACE + '.check_path_valid', return_value=True), \
                    mock.patch('builtins.open', mock.mock_open(read_data='[]')), \
                    mock.patch('logging.error'):
                result = MsprofJobSummary('test').get_msprof_json_file('test')
                self.assertEqual(result, [])
