#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import unittest
from unittest import mock

from common_func.file_slice_helper import FileSliceHelper
from common_func.file_slice_helper import make_export_file_name

NAMESPACE = 'common_func.file_slice_helper'


class TestFileSliceHelper(unittest.TestCase):
    def test_set_header_when_normal_then_pass(self):
        check = FileSliceHelper('test', "file", "summary")
        check.set_header(["1"])

    def test_check_header_is_empty_when_empty_return_false(self):
        check = FileSliceHelper('test', "file", "summary")
        check.header = ["1"]
        self.assertFalse(check.check_header_is_empty())

    def test_check_header_is_empty_when_exist_return_True(self):
        check = FileSliceHelper('test', "file", "summary")
        self.assertTrue(check.check_header_is_empty())

    def test_insert_data_when_normal_then_pass(self):
        with mock.patch(NAMESPACE + '.FileSliceHelper.dump_csv_data'):
            check = FileSliceHelper('test', "file", "summary")
            check.insert_data(["1"])

    def test_dump_csv_data_when_force_then_pass(self):
        with mock.patch(NAMESPACE + '.make_export_file_name'), \
                mock.patch(NAMESPACE + '.create_csv'):
            check = FileSliceHelper('test', "file", "summary")
            check.data_list = ["1"]
            check.dump_csv_data(force=True)

    def test_dump_csv_data_when_not_force_but_exceeded_limit_then_pass(self):
        with mock.patch(NAMESPACE + '.make_export_file_name'), \
                mock.patch(NAMESPACE + '.create_csv'):
            check = FileSliceHelper('test', "file", "summary")
            check.data_list = ["1"] * 1000001
            check.dump_csv_data()


def test_make_export_file_name_should_return_csv_name_when_give_summary_json():
    params = {
        "data_type": "abc", "device_id": "0", "model_id": "1", "iter_id": "1",
        "export_type": "summary", "project": ""
    }
    with mock.patch('msinterface.msprof_data_storage.MsprofDataStorage.get_export_prefix_file_name',
                    return_value="abc_0_1_1_slice_0_20230905200300"):
        unittest.TestCase().assertEqual(make_export_file_name(params),
                                        "abc_0_1_1_slice_0_20230905200300.csv")


def test_make_export_file_name_should_return_json_name_when_give_timeline_json():
    params = {
        "data_type": "abc", "device_id": "0", "model_id": "1", "iter_id": "1",
        "export_type": "timeline", "project": ""
    }
    with mock.patch('msinterface.msprof_data_storage.MsprofDataStorage.get_export_prefix_file_name',
                    return_value="abc_0_1_1_slice_0_20230905200300"):
        unittest.TestCase().assertEqual(make_export_file_name(params),
                                        "abc_0_1_1_slice_0_20230905200300.json")


if __name__ == '__main__':
    unittest.main()
