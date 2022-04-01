#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import struct
import unittest
from unittest import mock

from constant.constant import CONFIG
from msparser.acl.acl_parser import AclParser
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.acl.acl_parser'


class TestAclParser(unittest.TestCase):
    file_list = {DataTag.ACL: ['AclModule.acl_model.0.slice_0', 'AclModule.acl_rts.0.slice_0'],
                 DataTag.ACL_HASH: ['AclModule.hash_dic.0.slice_0']}

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.AclParser.parse'), \
                mock.patch(NAMESPACE + '.AclParser.parse'), \
                mock.patch(NAMESPACE + '.AclParser.save'):
            check = AclParser(self.file_list, CONFIG)
            check.ms_run()
        with mock.patch(NAMESPACE + '.AclParser.parse', side_effect=SystemError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = AclParser(self.file_list, CONFIG)
            check.ms_run()

    def test_group_file_list(self):
        result = AclParser.group_file_list(['AclModule.acl_model.1.slice_0', 'AclModule.acl_op.1.slice_0'])
        self.assertEqual(result, {'acl_model': ['AclModule.acl_model.1.slice_0'],
                                  'acl_op': ['AclModule.acl_op.1.slice_0'],
                                  'acl_others': [],
                                  'acl_rts': []})

    def test_save(self):
        with mock.patch(NAMESPACE + '.AclSqlParser.create_acl_data_table'), \
                mock.patch(NAMESPACE + '.AclSqlParser.insert_acl_data_to_db'), \
                mock.patch(NAMESPACE + '.logging.info'):
            check = AclParser(self.file_list, CONFIG)
            check.save()

    def test_insert_acl_data(self):
        acl_data_new = struct.pack('=HHIQQII64s4Q', 23130, 0, 3, 46, 6, 8, 989, b'test', 8, 56, 2, 5)
        with mock.patch('builtins.open', mock.mock_open(read_data=acl_data_new)), \
                mock.patch(NAMESPACE + '.OffsetCalculator.pre_process', return_value=acl_data_new):
            check = AclParser(self.file_list, CONFIG)
            check._insert_acl_data('test', 128, [])
        self.assertEqual(check._acl_data, [['test', 3, 46, 6, 8, 989, '0']])

    def test_parse(self):
        _acl_data = (23130, 0, '3074377499921637268', '3', 102026687320964, 102026687968135, 15489, 15489, 0)
        with mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch('os.path.getsize', return_value=0):
            check = AclParser(self.file_list, CONFIG)
            check.parse()
        with mock.patch('os.path.getsize', return_value=128), \
                mock.patch(NAMESPACE + '.PathManager.get_data_file_path', return_value='test'), \
                mock.patch(NAMESPACE + '.logging.info'), \
                mock.patch(NAMESPACE + '.FileManager.add_complete_file'), \
                mock.patch(NAMESPACE + '.AclParser._insert_acl_data'):
            check = AclParser(self.file_list, CONFIG)
            check.parse()
