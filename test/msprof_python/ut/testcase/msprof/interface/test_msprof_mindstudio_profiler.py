#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import unittest
from unittest import mock

from common_func.file_slice_helper import FileSliceHelper
from common_func.ms_constant.str_constant import StrConstant
from msinterface.msprof_mindstudio_profiler import MsprofMindStudioProfiler

NAMESPACE = 'msinterface.msprof_mindstudio_profiler'


class TestMsprofMindStudioProfiler(unittest.TestCase):
    def test_export_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._make_new_folder'), \
                mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._export_msprof_summary'), \
                mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._export_msprof_timeline'), \
                mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._export_readme_file'):
            MsprofMindStudioProfiler('test').export()

    def test_export_msprof_summary_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._move_host_summary'), \
                mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._merge_device_summary'):
            MsprofMindStudioProfiler('test')._export_msprof_summary()

    def test_move_host_summary_when_normal_then_pass(self):
        with mock.patch('os.path.join', return_value="test"), \
                mock.patch('os.path.realpath', return_value="test"), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.listdir', return_value=["op_summary_0_1_1_20230905213000.csv"]), \
                mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._update_file_list',
                           return_value=["op_summary_0_1_1_20230905213000.csv"]), \
                mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._get_file_name', return_value="op_summary"), \
                mock.patch(NAMESPACE + '.make_export_file_name',
                           return_value=["op_summary_0_1_1_20230905213000.csv"]), \
                mock.patch('shutil.copy'):
            MsprofMindStudioProfiler('test')._move_host_summary()

    def test_merge_device_summary_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + '.get_path_dir', return_value=['host', 'device_0', 'device_1']), \
                mock.patch('os.path.realpath', return_value="test"), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._get_summary_file_name'), \
                mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._get_summary_data'):
            MsprofMindStudioProfiler('test')._merge_device_summary()

    def test_get_summary_file_name_when_normal_then_pass(self):
        file_set = set()
        with mock.patch('os.path.join', return_value="test"), \
                mock.patch('os.path.realpath', return_value="test"), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.listdir', return_value=["op_summary.csv", "npu_mem.csv"]), \
                mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._update_file_list',
                           return_value=["op_summary_0_1_1_20230905213000.csv"]), \
                mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._get_file_name', return_value="op_summary"):
            MsprofMindStudioProfiler('test')._get_summary_file_name(file_set, "test")
            self.assertEqual(len(file_set), 1)

    def test_get_summary_data_when_normal_then_pass(self):
        helper = FileSliceHelper("test", "op_summary", "summary")
        with mock.patch('os.path.join', return_value="test"), \
                mock.patch('os.path.realpath', return_value="test"), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.listdir', return_value=["op_summary.csv", "npu_mem.csv"]), \
                mock.patch('os.path.basename', return_value="0"), \
                mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._update_file_list',
                           return_value=["op_summary_0_1_1_20230905213000.csv"]), \
                mock.patch('builtins.open', mock.mock_open(read_data="")):
            MsprofMindStudioProfiler('test')._get_summary_data("op_summary", "test", helper)

    def test_update_file_list_when_csv_normal_filename_then_pass(self):
        _file_list = ["op_summary_20230905191336.csv", "op_summary_20230904191336.csv",
                      "step_trace_20230905191336.csv", "step_trace_20230905161336.csv",
                      "msprof_20230905191336.json", "msprof_20230905171336.json",
                      "step_trace_20230905191336.json", "step_trace_20230805191336.json"]
        data_type = StrConstant.FILE_SUFFIX_CSV
        check = MsprofMindStudioProfiler('test')
        self.assertEqual(check._update_file_list(_file_list, data_type),
                         ["op_summary_20230905191336.csv", "step_trace_20230905191336.csv"])

    def test_update_file_list_when_json_normal_filename_then_pass(self):
        _file_list = ["op_summary_20230905191336.csv", "op_summary_20230904191336.csv",
                      "step_trace_20230905191336.csv", "step_trace_20230905161336.csv",
                      "msprof_20230905191336.json", "msprof_20230905171336.json",
                      "step_trace_20230905191336.json", "step_trace_20230805191336.json"]
        data_type = StrConstant.FILE_SUFFIX_JSON
        check = MsprofMindStudioProfiler('test')
        self.assertEqual(check._update_file_list(_file_list, data_type),
                         ["msprof_20230905191336.json", "step_trace_20230905191336.json"])

    def test_update_file_list_when_invalid_filename_then_return_empty(self):
        _file_list = ["op_summary_20230905191336", "op.csv", "step_trace_qwer.csv"]
        data_type = StrConstant.FILE_SUFFIX_JSON
        check = MsprofMindStudioProfiler('test')
        self.assertEqual(check._update_file_list(_file_list, data_type), [])

    def test_get_file_name_when_normal_name_then_pass(self):
        check = MsprofMindStudioProfiler('test')
        self.assertEqual(check._get_file_name("msprof_20230905191336.json"), "msprof")

    def test_get_file_name_when_invalid_name_then_pass(self):
        check = MsprofMindStudioProfiler('test')
        self.assertEqual(check._get_file_name("msprof.json"), "invalid")

        self.assertEqual(check._get_file_name("2msprof.json"), "invalid")

    def test_get_json_data_when_normal_then_pass(self):
        params = {StrConstant.PARAM_DATA_TYPE: "msprof"}
        with mock.patch('os.path.join', return_value='test'), \
                mock.patch('os.path.realpath', return_value='test'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.listdir',
                           return_value=["msprof_20230905191336.json"]), \
                mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._update_file_list',
                           return_value=["msprof_20230905191336.json"]), \
                mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._get_file_name',
                           return_value="msprof"), \
                mock.patch('common_func.utils.Utils.get_json_data', return_value=["data"]):
            check = MsprofMindStudioProfiler('test')
            self.assertEqual(check._get_json_data("test", params), ["data"])

    def test_export_msprof_timeline_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._export_all_timeline_data'):
            MsprofMindStudioProfiler('test')._export_msprof_timeline()

    def test_export_all_timeline_data_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + '.get_path_dir', return_value=['device']), \
                mock.patch('os.path.realpath', return_value='test'), \
                mock.patch('os.path.join', return_value='test'), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch('common_func.data_check_manager.DataCheckManager.contain_info_json_data',
                           return_value=True), \
                mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._get_json_data'), \
                mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._data_deduplication'), \
                mock.patch(NAMESPACE + '.MsprofMindStudioProfiler._generate_json_file'):
            MsprofMindStudioProfiler('test')._export_all_timeline_data()

    def test_generate_json_file_when_write_failed_then_error(self):
        with mock.patch(NAMESPACE + '.MsprofDataStorage.slice_data_list'), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.write_json_files', return_value=(1, [])):
            MsprofMindStudioProfiler('test')._generate_json_file({}, [])

    def test_generate_json_file_when_write_success_then_success(self):
        with mock.patch(NAMESPACE + '.MsprofDataStorage.slice_data_list'), \
                mock.patch(NAMESPACE + '.MsprofDataStorage.write_json_files', return_value=(0, [])):
            MsprofMindStudioProfiler('test')._generate_json_file({}, [])

    def test_data_deduplication_when_first_data_with_cann_then_return_all_data(self):
        json_data = [{"name": "acl@qwer"}, {"name": "abcd"}, {"name": "efgh"}]
        timeline_data = []
        check = MsprofMindStudioProfiler('test')
        check._data_deduplication(json_data, timeline_data)
        self.assertEqual(timeline_data, json_data)

    def test_data_deduplication_when_exsist_data_with_cann_then_return_all_data(self):
        json_data = [{"name": "acl@qwer"}, {"name": "abcd"}, {"name": "efgh"}]
        timeline_data = [{"name": "acl@1234"}]
        check = MsprofMindStudioProfiler('test')
        check._data_deduplication(json_data, timeline_data)
        self.assertEqual(timeline_data, [{"name": "acl@1234"}, {"name": "abcd"}, {"name": "efgh"}])

    def test_write_json_files_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + '.make_export_file_name', return_value="test"), \
                mock.patch('os.open'), \
                mock.patch('os.fdopen'), \
                mock.patch('os.fdopen.write'):
            MsprofMindStudioProfiler('test')._write_json_files((0, [1, 2]), {})

    def test_export_readme_file_normal_then_pass(self):
        with mock.patch('msconfig.config_manager.ConfigManager.get',
                        DATA={"timeline": [], "summary": []}), \
                mock.patch('os.listdir'), \
                mock.patch('os.path.join'), \
                mock.patch('builtins.open', mock.mock_open(read_data="")):
            MsprofMindStudioProfiler('test')._export_readme_file()
