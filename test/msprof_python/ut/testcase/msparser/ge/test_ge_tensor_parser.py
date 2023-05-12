#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2023. All rights reserved.
"""
import struct

import unittest

from msparser.ge.ge_tensor_parser import GeTensorParser

NAMESPACE = 'msparser.ge.ge_tensor_parser'


class TestGeTensorParser(unittest.TestCase):
    def test_update_hash_dict_data1(self):
        check = GeTensorParser({'0': {'Framework.tensor_data_info.0.slice_0': 1}}, {'0': {'0': '0'}}, )
        hash_dict = {
            '1': {
                'tensor_num': 3,
                'input_format': 'FORMAT_ND;FORMAT_ND',
                'input_data_type': 'INT64;INT64',
                'input_shape': ';',
                'output_format': 'FORMAT_ND',
                'output_data_type': 'INT64',
                'output_shape': ''
            }
        }
        key = '1'
        data = [
            11, 32, 116, 5, 'FORMAT_ND;FORMAT_ND;FORMAT_ND;FORMAT_ND;FORMAT_ND',
            'FLOAT;FLOAT;FLOAT;FLOAT;FLOAT', ';;;;', '', '', '', 0, 0, 0,
        ]
        check._update_hash_dict_data(hash_dict, key, data)
        self.assertEqual(hash_dict, {
            '1': {
                'tensor_num': 8,
                'input_format': 'FORMAT_ND;FORMAT_ND;FORMAT_ND;'
                                'FORMAT_ND;FORMAT_ND;FORMAT_ND;FORMAT_ND',
                'input_data_type': 'INT64;INT64;FLOAT;FLOAT;FLOAT;FLOAT;FLOAT',
                'input_shape': ';;;;;;',
                'output_format': 'FORMAT_ND;',
                'output_data_type': 'INT64;',
                'output_shape': ';'}
        })

    def test_update_hash_dict_data2(self):
        check = GeTensorParser({'0': {'Framework.tensor_data_info.0.slice_0': 1}}, {'0': {'0': '0'}}, )
        hash_dict = {
            '1': {
                'tensor_num': 3,
                'input_format': 'FORMAT_ND;FORMAT_ND',
                'input_data_type': 'INT64;INT64',
                'input_shape': ';',
                'output_format': 'FORMAT_ND',
                'output_data_type': 'INT64',
                'output_shape': ''
            }
        }
        key = '1'
        data = [11, 32, 116, 1, '', '', '', '', '', '', 0, 0, 0, ]
        check._update_hash_dict_data(hash_dict, key, data)
        self.assertEqual(hash_dict, {
            '1': {
                'tensor_num': 4,
                'input_format': 'FORMAT_ND;FORMAT_ND;',
                'input_data_type': 'INT64;INT64;',
                'input_shape': ';;',
                'output_format': 'FORMAT_ND;',
                'output_data_type': 'INT64;',
                'output_shape': ';'
            }
        })
