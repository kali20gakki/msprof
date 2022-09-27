#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import os
import unittest
from unittest import mock

from constant.constant import clear_dt_project
from msparser.iter_rec.iter_rec_parser import IterRecParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.iter_rec.iter_rec_parser'


class TestIterRecParser(unittest.TestCase):
    DIR_PATH = os.path.join(os.path.dirname(__file__), 'DT_IterRecParser')
    sample_config = {
        'result_dir': DIR_PATH, 'device_id': '0', 'iter_id': 1,
        'job_id': 'job_default', 'ip_address': '127.0.0.1', 'model_id': -1
    }
    file_list = {DataTag.HWTS: ['hwts.data.0.slice_0']}
    non_static_data = {1: ['1-1-0', '1-2-0', '1-3-0'], 2: ['2-1-0', '2-2-0', '2-3-0']}

    def setUp(self) -> None:
        os.makedirs(os.path.join(self.DIR_PATH, 'PROF1', 'device_0'))
        os.makedirs(os.path.join(self.DIR_PATH, 'sqlite'))

    def tearDown(self) -> None:
        clear_dt_project(self.DIR_PATH)

    def test_parse_should_return_none_when_no_ge_data(self):
        with mock.patch(NAMESPACE + '.GeInfoModel.check_table'), \
                mock.patch(NAMESPACE + '.GeInfoModel.get_all_ge_static_shape_data', return_value=[{}, {}]), \
                mock.patch(NAMESPACE + '.GeInfoModel.get_all_ge_non_static_shape_data', return_value={}):
            check = IterRecParser(self.file_list, self.sample_config)
            check.parse()
