#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from framework.file_dispatch import FileDispatch
from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG

NAMESPACE = 'framework.file_dispatch'


class TestFileDispatch(unittest.TestCase):
    def test_dispatch(self):
        with mock.patch(NAMESPACE + '.PathManager.get_data_dir'), \
             mock.patch('os.listdir', return_value=['AclModule.acl_model.0.slice_0', 'AclModule.acl_rts.0.slice_0']), \
             mock.patch(NAMESPACE + '.ProfFactoryMaker.create_parser_factory'), \
             mock.patch(NAMESPACE + '.ProfFactoryMaker.create_calculator_factory'):
            check = FileDispatch(CONFIG)
            check.pick_up_files()

    def test_dispatch_parser(self):
        with mock.patch(NAMESPACE + '.FileDispatch.pick_up_files'), \
                mock.patch('framework.prof_factory_maker.ProfFactoryMaker.create_parser_factory'):
            check = FileDispatch(CONFIG)
            check._file_list = {'a': 'b'}
            check.dispatch_parser()

    def test_dispatch_calculator(self):
        InfoConfReader()._info_json = {'devices': '0'}
        with mock.patch(NAMESPACE + '.FileDispatch.pick_up_files'), \
                mock.patch('framework.prof_factory_maker.ProfFactoryMaker.create_parser_factory'):
            check = FileDispatch(CONFIG)
            check._file_list = {'a': 'b'}
            check.dispatch_calculator()
