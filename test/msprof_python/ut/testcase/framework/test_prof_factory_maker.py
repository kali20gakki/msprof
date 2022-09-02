#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.platform.chip_manager import ChipManager
from framework.prof_factory_maker import ProfFactoryMaker
from profiling_bean.prof_enum.chip_model import ChipModel
from profiling_bean.prof_enum.data_tag import DataTag

from constant.constant import CONFIG

NAMESPACE = 'framework.prof_factory_maker'


class TestProfFactoryMaker(unittest.TestCase):
    def test_create_parser_factory(self):
        file_list = {DataTag.ACL: ['AclModule.acl_model.0.slice_0', 'AclModule.acl_rts.0.slice_0'],
                     DataTag.ACL_HASH: ['AclModule.hash_dic.0.slice_0']}
        with mock.patch(NAMESPACE + '.ProfFactoryMaker._get_chip_model'), \
             mock.patch(NAMESPACE + '.ParserFactory.run'):
            check = ProfFactoryMaker(CONFIG)
            check.create_parser_factory(file_list)
        with mock.patch(NAMESPACE + '.logging.warning'):
            check = ProfFactoryMaker(CONFIG)
            check.create_parser_factory(False)

    def test_create_calculator_factory(self):
        with mock.patch(NAMESPACE + '.ProfFactoryMaker._get_chip_model'), \
             mock.patch(NAMESPACE + '.CalculatorFactory.run'):
            check = ProfFactoryMaker(CONFIG)
            check.create_calculator_factory([])
            check.create_calculator_factory(['a'])

    def test_get_chip_model(self):
        check = ProfFactoryMaker(CONFIG)
        InfoConfReader()._info_json = {Constant.PLATFORM_VERSION: "1"}
        ret = check._get_chip_model()
        self.assertEqual(ChipModel.CHIP_V2_1_0, ret)
