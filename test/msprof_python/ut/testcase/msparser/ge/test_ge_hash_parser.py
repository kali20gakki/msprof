#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from constant.constant import CONFIG
from msparser.ge.ge_hash_parser import GeHashParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.ge.ge_hash_parser'


class TestGeHashParser(unittest.TestCase):
    def test_ms_run(self):
        check = GeHashParser({}, CONFIG)
        check.ms_run()

        with mock.patch(NAMESPACE + '.GeHashParser.parse'), \
             mock.patch(NAMESPACE + '.GeHashParser.save', side_effect=ValueError), \
             mock.patch(NAMESPACE + '.logging.error'):
            check = GeHashParser({DataTag.GE_FUSION_OP_INFO: ['Framework.Framework.hash_dic.0.slice_0']}, CONFIG)
            check.ms_run()

    def test_parse(self):
        data = "16013848712347040169:ConstantFolding/loss_scale/gradients/fp32_vars/conv2d_10/Conv2D_grad/ShapeN-matshapes-1"
        with mock.patch('builtins.open', mock.mock_open(read_data=data)), \
             mock.patch('os.path.getsize', return_value=256):
            check = GeHashParser({DataTag.GE_FUSION_OP_INFO: ['Framework.Framework.hash_dic.0.slice_0']}, CONFIG)
            check.parse()

    def test_save(self):
        with mock.patch(NAMESPACE + '.GeHashModel.init'), \
             mock.patch(NAMESPACE + '.GeHashModel.flush'), \
             mock.patch(NAMESPACE + '.GeHashModel.finalize'):
            check = GeHashParser({DataTag.GE_FUSION_OP_INFO: ['Framework.Framework.hash_dic.0.slice_0']}, CONFIG)
            check.save()
