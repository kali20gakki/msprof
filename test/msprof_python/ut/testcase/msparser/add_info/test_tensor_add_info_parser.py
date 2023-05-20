#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
"""
import sqlite3
import struct
import unittest
from unittest import mock

from common_func.hash_dict_constant import HashDictData
from constant.constant import CONFIG
from msparser.add_info.tensor_add_info_parser import TensorAddInfoParser
from msparser.add_info.tensor_add_info_bean import TensorAddInfoBean
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'msparser.add_info.tensor_add_info_parser'


class TestTensorAddInfoParser(unittest.TestCase):
    file_list = {
        DataTag.TENSOR_ADD_INFO: ['aging.additional.tensor_info.slice_0', 'unaging.additional.tensor_info.slice_0']
    }

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.TensorAddInfoParser.parse'), \
                mock.patch(NAMESPACE + '.TensorAddInfoParser.save'):
            check = TensorAddInfoParser(self.file_list, CONFIG)
            check.ms_run()
        with mock.patch(NAMESPACE + '.TensorAddInfoParser.parse', side_effect=SystemError), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = TensorAddInfoParser(self.file_list, CONFIG)
            check.ms_run()
        with mock.patch(NAMESPACE + '.TensorAddInfoParser.save', side_effect=sqlite3.Error), \
                mock.patch(NAMESPACE + '.TensorAddInfoParser.parse'), \
                mock.patch(NAMESPACE + '.logging.error'):
            check = TensorAddInfoParser(self.file_list, CONFIG)
            check.ms_run()

    def test_save(self):
        check = TensorAddInfoParser(self.file_list, CONFIG)
        check.save()
        HashDictData('test')._type_hash_dict = {'node': {}}
        HashDictData('test')._ge_hash_dict = {}
        check = TensorAddInfoParser(self.file_list, CONFIG)
        check._ge_tensor_info_data = [[1, 2, 3, 4, 5], [4, 5, 6, 7, 8]]
        check.save()

    def test_parse(self):
        with mock.patch(NAMESPACE + '.TensorAddInfoParser.parse_bean_data',
                        return_value=[['node', '0', 0, 0, '0', 0, '', '', '', '', '', '']]):
            check = TensorAddInfoParser(self.file_list, CONFIG)
            check.parse()

    def test_get_tensor_info_data(self):
        ctx_data = (23130, 10000, *(0,) * 61)
        struct_data = struct.pack("HHIIIQQI55I", *ctx_data)
        data = TensorAddInfoBean.decode(struct_data)
        check = TensorAddInfoParser(self.file_list, CONFIG)
        result = check._get_tensor_info_data(data)
        self.assertEqual(result, ['node', '0', 0, 0, '0', 0, '', '', '', '', '', ''])


if __name__ == '__main__':
    unittest.main()
