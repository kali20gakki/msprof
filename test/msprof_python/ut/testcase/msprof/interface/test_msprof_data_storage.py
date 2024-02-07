#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import json
import unittest
from datetime import datetime
from datetime import timezone
from unittest import mock

from common_func.constant import Constant
from common_func.profiling_scene import ProfilingScene
from common_func.profiling_scene import ExportMode
from msinterface.msprof_data_storage import MsprofDataStorage

NAMESPACE = 'msinterface.msprof_data_storage'


class TestMsprofDataStorage(unittest.TestCase):

    def test_slice_data_list(self):
        data_list = [
            {'name': 'process_name', 'pid': 21532, 'tid': 0, 'args': {'name': 'AscendCL'}, 'ph': 'M'},
            {'name': 'thread_name', 'pid': 21532, 'tid': 26964, 'args': {'name': 'Thread 26964'}, 'ph': 'M'},
            {
                'name': 'aclrtMalloc', 'pid': 21532, 'tid': 21532, 'ts': 661374107216.124, 'dur': 94.977,
                'args': {'Mode': 'ACL_RTS', 'Process Id': 21532, 'Thread Id': 21532}, 'ph': 'X'
            },
            {
                'name': 'aclrtSetDevice', 'pid': 21532, 'tid': 21591, 'ts': 661374107249.786, 'dur': 19.988,
                'args': {'Mode': 'ACL_RTS', 'Process Id': 21532, 'Thread Id': 21591}, 'ph': 'X'
            }
        ]
        with mock.patch(NAMESPACE + '.MsprofDataStorage.read_slice_config', return_value=('on', 0, 0)):
            key = MsprofDataStorage()
            result = key.slice_data_list(data_list)
        self.assertEqual(len(result[1][0]), 4)
        data_list = [
            {'name': 'process_name', 'pid': 21532, 'tid': 0, 'args': {'name': 'AscendCL'}, 'ph': 'M'},
            {'name': 'thread_name', 'pid': 21532, 'tid': 26964, 'args': {'name': 'Thread 26964'}, 'ph': 'M'},
            {
                'name': 'aclrtMalloc', 'pid': 21532, 'tid': 21532, 'ts': 661374107216.124, 'dur': 94.977,
                'args': {'Mode': 'ACL_RTS', 'Process Id': 21532, 'Thread Id': 21532}, 'ph': 'X'
            },
            {
                'name': 'aclrtSetDevice', 'pid': 21532, 'tid': 21591, 'ts': 661374107249.786, 'dur': 19.988,
                'args': {'Mode': 'ACL_RTS', 'Process Id': 21532, 'Thread Id': 21591}, 'ph': 'X'
            }
        ]
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
        self.assertEqual(res, json.dumps(result))

    def test_export_timeline_data_to_json_2(self):
        result = {'test1': 1}
        params = {"data_type": 1}
        with mock.patch(NAMESPACE + '.MsprofDataStorage._make_export_file_name', return_value=1), \
                mock.patch(NAMESPACE + '.check_file_writable'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.remove', return_value=True), \
                mock.patch('os.open', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.clear_timeline_dir'), \
                mock.patch('common_func.path_manager.PathManager.get_device_count', return_value=2), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.slice_data_list', return_value=(1, [1])), \
                mock.patch(NAMESPACE + '.FdOpen.__enter__', mock.mock_open(read_data='123')), \
                mock.patch(NAMESPACE + '.FdOpen.__exit__'), \
                mock.patch('os.fdopen', mock.mock_open(read_data='123')):
            key = MsprofDataStorage()
            res = key.export_timeline_data_to_json(result, params)
        params = {
            "data_type": 'str', "device_id": 1, "model_id": 1, "iter_id": 1,
            "export_type": "summary", "export_format": "json", "project": 123
        }
        with mock.patch(NAMESPACE + '.check_file_writable'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.remove', return_value=True), \
                mock.patch('os.open', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.clear_timeline_dir'), \
                mock.patch('common_func.path_manager.PathManager.get_device_count', return_value=2), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.slice_data_list', return_value=(1, [1])), \
                mock.patch(NAMESPACE + '.FdOpen.__enter__', mock.mock_open(read_data='123')), \
                mock.patch(NAMESPACE + '.FdOpen.__exit__'), \
                mock.patch('os.fdopen', mock.mock_open(read_data='123')):
            with mock.patch(NAMESPACE + '.PathManager.get_summary_dir', return_value="train"), \
                    mock.patch('os.path.join', return_value=True), \
                    mock.patch('common_func.utils.Utils.is_step_scene', return_value=True):
                key = MsprofDataStorage()
                result = key.export_timeline_data_to_json(result, params)
        self.assertEqual(res, '{"status": 0, "data": [1]}')
        self.assertEqual(result, '{"status": 0, "data": [true]}')

    def test_export_timeline_data_should_return_directly_when_input_contains_status(self):
        result = json.loads('{"status": "123"}')
        params = {"data_type": 2}
        with mock.patch(NAMESPACE + '.check_file_writable'), \
                mock.patch('os.path.exists', return_value=False):
            key = MsprofDataStorage()
            res = key.export_timeline_data_to_json(result, params)
        self.assertEqual(res, '{"status": "123"}')

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
        with mock.patch(NAMESPACE + '.PathManager.get_timeline_dir', return_value='test'), \
                mock.patch('os.listdir', return_value=['acl_0_0_1.json']), \
                mock.patch(NAMESPACE + '.check_file_writable'), \
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

    def test_get_current_time_str_should_return_success_when_input_data_is_valid(self):
        utc_time = datetime.now(tz=timezone.utc)
        current_time = utc_time.replace(tzinfo=timezone.utc).astimezone(tz=None)
        self.assertEqual(MsprofDataStorage.get_current_time_str(), current_time.strftime('%Y%m%d%H%M%S'))

    def test_get_current_time_str_should_return_success_when_data_length_is_14(self):
        self.assertEqual(len(MsprofDataStorage.get_current_time_str()), 14)

    def test_get_export_prefix_file_name_should_return_name_when_normal(self):
        params = {"data_type": "abc", "device_id": "0", "model_id": "1", "iter_id": "1"}
        ProfilingScene().init(" ")
        ProfilingScene()._scene = Constant.STEP_INFO
        ProfilingScene().set_mode(ExportMode.GRAPH_EXPORT)
        with mock.patch(NAMESPACE + '.MsprofDataStorage.get_current_time_str', return_value="20230905200300"):
            self.assertEqual(MsprofDataStorage.get_export_prefix_file_name(params, 0, True),
                             "abc_0_1_1_slice_0_20230905200300")
        ProfilingScene().set_mode(ExportMode.ALL_EXPORT)

    def test_make_export_file_name_should_return_json_name_when_give_timeline_json(self):
        params = {
            "data_type": "abc", "device_id": "0", "model_id": "1", "iter_id": "1",
            "export_type": "timeline"
        }
        with mock.patch(NAMESPACE + '.MsprofDataStorage.get_export_prefix_file_name',
                        return_value="abc_0_1_1_slice_0_20230905200300"):
            self.assertEqual(MsprofDataStorage._make_export_file_name(params),
                             "abc_0_1_1_slice_0_20230905200300.json")

    def test_make_export_file_name_should_return_json_name_when_give_summary_json(self):
        params = {
            "data_type": "abc", "device_id": "0", "model_id": "1", "iter_id": "1",
            "export_type": "summary", "export_format": "json"
        }
        with mock.patch(NAMESPACE + '.MsprofDataStorage.get_export_prefix_file_name',
                        return_value="abc_0_1_1_slice_0_20230905200300"):
            self.assertEqual(MsprofDataStorage._make_export_file_name(params),
                             "abc_0_1_1_slice_0_20230905200300.json")

    def test_make_export_file_name_should_return_csv_name_when_give_summary_csv(self):
        params = {
            "data_type": "abc", "device_id": "0", "model_id": "1", "iter_id": "1",
            "export_type": "summary", "export_format": "csv"
        }
        with mock.patch(NAMESPACE + '.MsprofDataStorage.get_export_prefix_file_name',
                        return_value="abc_0_1_1_slice_0_20230905200300"):
            self.assertEqual(MsprofDataStorage._make_export_file_name(params),
                             "abc_0_1_1_slice_0_20230905200300.csv")


if __name__ == '__main__':
    unittest.main()
