#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from constant.constant import CONFIG
from msparser.acl.acl_parser import AclParser
from msparser.factory.parser_factory import ParserFactory
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.factory.parser_factory'


class TestParserFactory(unittest.TestCase):
    def test_run(self):
        file_list = {DataTag.ACL: ['AclModule.acl_model.0.slice_0', 'AclModule.acl_rts.0.slice_0'],
                     DataTag.ACL_HASH: ['AclModule.hash_dic.0.slice_0']}
        with mock.patch(NAMESPACE + '.ConfigDataParsers.get_parsers'):
            check = ParserFactory(file_list, CONFIG, '310')
            check.run()

    def test_launch_parser_list(self):
        file_list = dict()
        with mock.patch(NAMESPACE + '.ConfigDataParsers.get_parsers', side_effect=[{'acl': [AclParser]}]), \
                mock.patch('msparser.acl.acl_parser.AclParser.start'), \
                mock.patch('msparser.acl.acl_parser.AclParser.join'):
            check = ParserFactory(file_list, CONFIG, '310')

            check.run()
