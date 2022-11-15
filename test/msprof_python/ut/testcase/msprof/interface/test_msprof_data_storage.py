import configparser
import json
import unittest
from unittest import mock

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from msinterface.msprof_data_storage import MsprofDataStorage
from profiling_bean.prof_enum.chip_model import ChipModel
from viewer.stars.stars_soc_view import StarsSocView

NAMESPACE = 'msinterface.msprof_data_storage'


class TestMsprofDataStorage(unittest.TestCase):

    def test_slice_data_list(self):
        data_list = [{'name': 'process_name', 'pid': 21532, 'tid': 0, 'args': {'name': 'AscendCL'}, 'ph': 'M'},
                     {'name': 'thread_name', 'pid': 21532, 'tid': 26964, 'args': {'name': 'Thread 26964'}, 'ph': 'M'},
                     {'name': 'aclrtMalloc', 'pid': 21532, 'tid': 21532, 'ts': 661374107216.124, 'dur': 94.977,
                      'args': {'Mode': 'ACL_RTS', 'Process Id': 21532, 'Thread Id': 21532}, 'ph': 'X'},
                     {'name': 'aclrtSetDevice', 'pid': 21532, 'tid': 21591, 'ts': 661374107249.786, 'dur': 19.988,
                      'args': {'Mode': 'ACL_RTS', 'Process Id': 21532, 'Thread Id': 21591}, 'ph': 'X'}]
        with mock.patch(NAMESPACE + '.MsprofDataStorage.read_slice_config', return_value=('on', 0, 0)):
            key = MsprofDataStorage()
            result = key.slice_data_list(data_list)
        self.assertEqual(len(result[1][0]), 4)
        data_list = [{'name': 'process_name', 'pid': 21532, 'tid': 0, 'args': {'name': 'AscendCL'}, 'ph': 'M'},
                     {'name': 'thread_name', 'pid': 21532, 'tid': 26964, 'args': {'name': 'Thread 26964'}, 'ph': 'M'},
                     {'name': 'aclrtMalloc', 'pid': 21532, 'tid': 21532, 'ts': 661374107216.124, 'dur': 94.977,
                      'args': {'Mode': 'ACL_RTS', 'Process Id': 21532, 'Thread Id': 21532}, 'ph': 'X'},
                     {'name': 'aclrtSetDevice', 'pid': 21532, 'tid': 21591, 'ts': 661374107249.786, 'dur': 19.988,
                      'args': {'Mode': 'ACL_RTS', 'Process Id': 21532, 'Thread Id': 21591}, 'ph': 'X'}]
        with mock.patch(NAMESPACE + '.MsprofDataStorage.read_slice_config', return_value=('on', 0, 0)), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.get_slice_times', return_value=2):
            key = MsprofDataStorage()
            result = key.slice_data_list(data_list)
        self.assertEqual(len(result[1][0]), 3)

    def test_export_summary_data_1(self):
        headers = 2
        data = 2
        params = {"export_format": "csv"}
        with mock.patch(NAMESPACE + '.MsprofDataStorage._make_export_file_name', return_value=2), \
                mock.patch(NAMESPACE + '.check_file_writable'), \
                mock.patch(NAMESPACE + '.create_csv', return_value=1):
            key = MsprofDataStorage()
            result = key.export_summary_data(headers, data, params)
        self.assertEqual(result, 1)

    def test_export_summary_data_2(self):
        headers = 1
        data = 1
        params = {"export_format": "json"}
        with mock.patch(NAMESPACE + '.MsprofDataStorage._make_export_file_name', return_value=1), \
                mock.patch(NAMESPACE + '.check_file_writable'), \
                mock.patch(NAMESPACE + '.create_json', return_value=1):
            key = MsprofDataStorage()
            result = key.export_summary_data(headers, data, params)
        self.assertEqual(result, 1)

    def test_export_summary_data_3(self):
        headers = None
        data = 2
        params = {"export_format": "1"}
        key = MsprofDataStorage()
        result = key.export_summary_data(headers, data, params)
        self.assertEqual(result, 2)

    def test_export_timeline_data_to_json_1(self):
        result = {'test1': 1, 'status': 2}
        params = {"data_type": 1}
        key = MsprofDataStorage()
        res = key.export_timeline_data_to_json(result, params)
        self.assertEqual(res, result)

    def test_export_timeline_data_to_json_2(self):
        result = {'test1': 1}
        params = {"data_type": 1}
        with mock.patch(NAMESPACE + '.MsprofDataStorage._make_export_file_name', return_value=1), \
                mock.patch(NAMESPACE + '.check_file_writable'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.remove', return_value=True), \
                mock.patch('os.open', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.clear_timeline_dir'), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.slice_data_list', return_value=(1, [1])), \
                mock.patch('os.fdopen', mock.mock_open(read_data='123')):
            key = MsprofDataStorage()
            res = key.export_timeline_data_to_json(result, params)
        params = {"data_type": 'str', "device_id": 1, "model_id": 1, "iter_id": 1,
                  "export_type": "summary", "export_format": "json", "project": 123}
        with mock.patch(NAMESPACE + '.check_file_writable'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.remove', return_value=True), \
                mock.patch('os.open', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.clear_timeline_dir'), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.slice_data_list', return_value=(1, [1])), \
                mock.patch('os.fdopen', mock.mock_open(read_data='123')):
            with mock.patch(NAMESPACE + '.PathManager.get_summary_dir', return_value="train"), \
                    mock.patch('os.path.join', return_value=True),\
                    mock.patch('common_func.utils.Utils.is_step_scene', return_value=True):
                key = MsprofDataStorage()
                result = key.export_timeline_data_to_json(result, params)
        self.assertEqual(res, '{"status": 0, "data": [1]}')
        self.assertEqual(result, '{"status": 0, "data": [true]}')

    def test_export_timeline_data_to_json_3(self):
        result = '123'
        params = {"data_type": 2}
        with mock.patch(NAMESPACE + '.json.loads', return_value={'status': 1}), \
                mock.patch(NAMESPACE + '.check_file_writable'), \
                mock.patch('os.path.exists', return_value=False):
            key = MsprofDataStorage()
            res = key.export_timeline_data_to_json(result, params)
        self.assertEqual(res, '123')

    def test_export_timeline_data_to_json_4(self):
        result = None
        params = {"data_type": 3}
        key = MsprofDataStorage()
        res = key.export_timeline_data_to_json(result, params)
        self.assertEqual(res, '{"status": 2, "info": "Unable to get 3 data. Maybe the data is not collected, '
                              'or the data may fail to be analyzed."}')

    def test_read_slice_config(self):
        with mock.patch(NAMESPACE + '.check_file_writable'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.remove', return_value=True), \
                mock.patch('os.open', return_value=True), \
                mock.patch('os.fdopen', mock.mock_open(
                    read_data='{"slice_switch": "on","slice_file_size": 0,"slice_method": 0}')):
            key = MsprofDataStorage()
            res = key.read_slice_config()
            self.assertEqual(res, ('on', 0, 0))

    def test_timeline_dir(self):
        with mock.patch(NAMESPACE + '.PathManager.get_timeline_dir', return_value='test'),\
                mock.patch('os.listdir', return_value=['acl_0_0_1.json']),\
                mock.patch(NAMESPACE + '.check_file_writable'),\
                mock.patch('os.remove'):
            MsprofDataStorage.clear_timeline_dir({'data_type': 'acl'})

    def test_get_slice_times1(self):
        with mock.patch(NAMESPACE + '.logging.warning'):
            key = MsprofDataStorage()
            key.data_list = [{'test': 'test'} for _ in range(10000000)]
            key.tid_set = {i for i in range(100)}
            key.get_slice_times('a', 1)

    def test_get_slice_times2(self):
        with mock.patch(NAMESPACE + '.logging.warning'):
            key = MsprofDataStorage()
            key.data_list = []
            key.tid_set = {i for i in range(10000000)}
            key.get_slice_times('a', 1)

if __name__ == '__main__':
    unittest.main()
