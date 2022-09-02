#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import struct
import unittest
from unittest import mock

from common_func.db_name_constant import DBNameConstant
from constant.constant import CONFIG
from msparser.ge.ge_model_parser import GeModelInfoParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.ge.ge_model_parser'


class TestGeModelInfoParser(unittest.TestCase):
    def test_ms_run(self):
        check = GeModelInfoParser({}, CONFIG)
        check.ms_run()

        with mock.patch(NAMESPACE + '.GeModelInfoParser.parse'), \
             mock.patch(NAMESPACE + '.GeModelInfoParser.save',
                        side_effect=ValueError), \
             mock.patch(NAMESPACE + '.logging.error'):
            check = GeModelInfoParser({DataTag.GE_HOST: ['Framework.step_info.0.slice_0']}, CONFIG)
            check.ms_run()

    def test_parse(self):
        with mock.patch(NAMESPACE + '.GeFusionOpParser.ms_run',
                        return_value={DBNameConstant.TABLE_GE_FUSION_OP_INFO: []}), \
             mock.patch(NAMESPACE + '.GeModelLoadParser.ms_run',
                        return_value={DBNameConstant.TABLE_GE_MODEL_LOAD: []}):
            check = GeModelInfoParser({DataTag.GE_HOST: ['Framework.step_info.0.slice_0']}, CONFIG)
            check.parse()
            self.assertEqual(len(check._ge_fusion_data), 2)

    def test_save(self):
        with mock.patch(NAMESPACE + '.GeFusionModel.init'), \
             mock.patch(NAMESPACE + '.GeFusionModel.flush'), \
             mock.patch(NAMESPACE + '.GeFusionModel.finalize'):
            check = GeModelInfoParser({DataTag.GE_HOST: ['Framework.step_info.0.slice_0']}, CONFIG)
            check.save()
