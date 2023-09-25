#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import unittest
from unittest import mock

from common_func.file_slice_helper import FileSliceHelper
from common_func.ms_constant.str_constant import StrConstant
from msinterface.msprof_output_summary import MsprofOutputSummary

NAMESPACE = 'msinterface.msprof_output_summary'


class TestMsprofOutputSummary(unittest.TestCase):

    def test_export_when_not_in_prof_file_then_pass(self):
        with mock.patch(NAMESPACE + '.MsprofOutputSummary._is_in_prof_file', return_value=False):
            MsprofOutputSummary('test').export()

    def test_export_when_in_prof_file_then_pass(self):
        with mock.patch(NAMESPACE + '.MsprofOutputSummary._is_in_prof_file', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._make_new_folder'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._export_msprof_summary'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._export_msprof_timeline'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._export_readme_file'):
            MsprofOutputSummary('test').export()

    def test_is_in_prof_file_when_in_prof_file_then_return_True(self):
        with mock.patch('os.listdir', return_value=["host", "123"]):
            check = MsprofOutputSummary('test')
            self.assertTrue(check._is_in_prof_file())

        with mock.patch('os.listdir', return_value=["device_1", "123"]):
            check = MsprofOutputSummary('test')
            self.assertTrue(check._is_in_prof_file())

    def test_is_in_prof_file_when_not_in_prof_file_then_return_False(self):
        with mock.patch('os.listdir', return_value=["123"]):
            check = MsprofOutputSummary('test')
            self.assertFalse(check._is_in_prof_file())

    def test_export_msprof_summary_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + '.MsprofOutputSummary._copy_host_summary'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._merge_device_summary'):
            MsprofOutputSummary('test')._export_msprof_summary()

    def test_copy_host_summary_when_normal_then_pass(self):
        with mock.patch('os.path.join', return_value="test"), \
                mock.patch('os.path.realpath', return_value="test"), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.listdir', return_value=["op_summary_0_1_1_20230905213000.csv"]), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary.get_newest_file_list',
                           return_value=["op_summary_0_1_1_20230905213000.csv"]), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._get_file_name', return_value="op_summary"), \
                mock.patch(NAMESPACE + '.make_export_file_name',
                           return_value=["op_summary_0_1_1_20230905213000.csv"]), \
                mock.patch('shutil.copy'):
            MsprofOutputSummary('test')._copy_host_summary()

    def test_merge_device_summary_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + '.get_path_dir', return_value=['host', 'device_0', 'device_1']), \
                mock.patch('os.path.realpath', return_value="test"), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch(NAMESPACE + '.DataCheckManager.contain_info_json_data', return_value=True), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._get_summary_file_name'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._save_summary_data'):
            MsprofOutputSummary('test')._merge_device_summary()

    def test_get_summary_file_name_when_normal_then_pass(self):
        with mock.patch('os.path.join', return_value="test"), \
                mock.patch('os.path.realpath', return_value="test"), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.listdir', return_value=["op_summary.csv", "npu_mem.csv"]), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary.get_newest_file_list',
                           return_value=["op_summary_0_1_1_20230905213000.csv"]), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._get_file_name', return_value="op_summary"):
            file_set = MsprofOutputSummary('test')._get_summary_file_name("test")
            self.assertEqual(len(file_set), 1)

    def test_save_summary_data_when_normal_then_pass(self):
        helper = FileSliceHelper("test", "op_summary", "summary")
        with mock.patch('os.path.join', return_value="test"), \
                mock.patch('os.path.realpath', return_value="test"), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.listdir', return_value=["op_summary.csv", "npu_mem.csv"]), \
                mock.patch('os.path.basename', return_value="0"), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary.get_newest_file_list',
                           return_value=["op_summary_0_1_1_20230905213000.csv"]), \
                mock.patch(NAMESPACE + '.FileOpen'), \
                mock.patch('common_func.file_manager.check_path_valid'), \
                mock.patch('common_func.file_slice_helper.FileSliceHelper.insert_data'):
            MsprofOutputSummary('test')._save_summary_data("op_summary", "test", helper)

    def test_get_newest_file_list_when_csv_normal_filename_then_pass(self):
        _file_list = [
            "op_summary_20230905191336.csv", "op_summary_20230904191336.csv",
            "step_trace_20230905191336.csv", "step_trace_20230905161336.csv",
            "msprof_20230905191336.json", "msprof_20230905171336.json",
            "step_trace_20230905191336.json", "step_trace_20230805191336.json"
        ]
        data_type = StrConstant.FILE_SUFFIX_CSV
        check = MsprofOutputSummary('test')
        self.assertEqual(check.get_newest_file_list(_file_list, data_type),
                         ["op_summary_20230905191336.csv", "step_trace_20230905191336.csv"])

    def test_get_newest_file_list_when_json_normal_filename_then_pass(self):
        _file_list = [
            "op_summary_20230905191336.csv", "op_summary_20230904191336.csv",
            "step_trace_20230905191336.csv", "step_trace_20230905161336.csv",
            "msprof_20230905191336.json", "msprof_20230905171336.json",
            "step_trace_20230905191336.json", "step_trace_20230805191336.json"
        ]
        data_type = StrConstant.FILE_SUFFIX_JSON
        check = MsprofOutputSummary('test')
        self.assertEqual(check.get_newest_file_list(_file_list, data_type),
                         ["msprof_20230905191336.json", "step_trace_20230905191336.json"])

    def test_get_newest_file_list_when_invalid_filename_then_return_empty(self):
        _file_list = ["op_summary_20230905191336", "op.csv", "step_trace_qwer.csv"]
        data_type = StrConstant.FILE_SUFFIX_JSON
        check = MsprofOutputSummary('test')
        self.assertEqual(check.get_newest_file_list(_file_list, data_type), [])

    def test_get_file_name_when_normal_name_then_pass(self):
        check = MsprofOutputSummary('test')
        self.assertEqual(check._get_file_name("msprof_20230905191336.json"), "msprof")

    def test_get_file_name_when_invalid_name_then_pass(self):
        check = MsprofOutputSummary('test')
        self.assertEqual(check._get_file_name("msprof.json"), "invalid")

        self.assertEqual(check._get_file_name("2msprof.json"), "invalid")

    def test_export_msprof_timeline_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + '.MsprofOutputSummary._export_all_timeline_data'):
            MsprofOutputSummary('test')._export_msprof_timeline()

    def test_export_all_timeline_data_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + '.get_path_dir', return_value=['device']), \
                mock.patch('os.path.realpath', return_value='test'), \
                mock.patch('os.path.join', return_value='test'), \
                mock.patch(NAMESPACE + '.check_path_valid'), \
                mock.patch('common_func.data_check_manager.DataCheckManager.contain_info_json_data',
                           return_value=True), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._get_timeline_file_with_slice',
                           return_value=({}, 2)), \
                mock.patch('common_func.utils.Utils.get_json_data'), \
                mock.patch('common_func.file_slice_helper.FileSliceHelper.dump_json_data'):
            MsprofOutputSummary('test')._export_all_timeline_data()

    def test_get_timeline_file_with_slice_when_not_exist_path_then_return_empty(self):
        with mock.patch('os.path.realpath', return_value='test'), \
                mock.patch('os.path.join', return_value='test'), \
                mock.patch('os.path.exists', return_value=False):
            check = MsprofOutputSummary('test')
            self.assertEqual(check._get_timeline_file_with_slice("qwer", "test_path", {}),
                             ({}, 0))

    def test_get_timeline_file_with_slice_when_normal_then_get_timeline_file_with_slice_count(self):
        with mock.patch('os.path.realpath', return_value='test'), \
                mock.patch('os.path.join', return_value='test'), \
                mock.patch('os.path.exists', return_value=True), \
                mock.patch('os.listdir', return_value=[]), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary.get_newest_file_list',
                           return_value=["msprof_slice_1_20230923193400.json",
                                         "step_trace_slice_1_20230923193400.json"]):
            check = MsprofOutputSummary('test')
            self.assertEqual(check._get_timeline_file_with_slice("msprof", "test_path", {}),
                             ({1: ['test']}, 1))

    def test_export_readme_file_normal_then_pass(self):
        with mock.patch('msconfig.config_manager.ConfigManager.get',
                        DATA={"timeline": [], "summary": []}), \
                mock.patch('os.listdir'), \
                mock.patch('os.path.join'), \
                mock.patch('os.open'), \
                mock.patch('os.fdopen'), \
                mock.patch('os.fdopen.write'), \
                mock.patch(NAMESPACE + '.FdOpen.__enter__', mock.mock_open(read_data='123')), \
                mock.patch(NAMESPACE + '.FdOpen.__exit__'), \
                mock.patch(NAMESPACE + '.MsprofOutputSummary._get_readme_info'):
            MsprofOutputSummary('test')._export_readme_file()

    def test_get_readme_info_when_normal_then_get_desc(self):
        file_dict = {"begin": "hello", "step_trace": "qwer", "123": "zxc"}
        file_set = set()
        file_set.add("step_trace")
        self.assertEqual(MsprofOutputSummary('test')._get_readme_info(file_set, file_dict, ".json"),
                         "hello1.step_trace.json:qwer\n")
