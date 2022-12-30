#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
"""
import struct
import unittest
from unittest import mock
from common_func.db_name_constant import DBNameConstant
from constant.constant import CONFIG
from msparser.ge.ge_info_parser import GeInfoParser


class TestGeInfoParser(unittest.TestCase):
    def test_offset_index_normal(self):
        dynamic_model_dict = {
            0: [
                [0, -1, -1, -1, -1, -1, -1, -1, -1, 11],
                [0, -1, -1, -1, -1, -1, -1, -1, -1, 12],
                [0, -1, -1, -1, -1, -1, -1, -1, -1, 13],
                [0, -1, -1, -1, -1, -1, -1, -1, -1, 14],
            ],
            1: [
                [1, -1, -1, -1, -1, -1, -1, -1, -1, 2],
                [1, -1, -1, -1, -1, -1, -1, -1, -1, 3],
                [1, -1, -1, -1, -1, -1, -1, -1, -1, 3],
                [1, -1, -1, -1, -1, -1, -1, -1, -1, 3],
            ]

        }
        parser = GeInfoParser({}, {})
        parser.offset_index(dynamic_model_dict, parser.GE_TASK)
        self.assertEqual(dynamic_model_dict[0][0][-1], 1)
        self.assertEqual(dynamic_model_dict[0][3][-1], 4)
        self.assertEqual(dynamic_model_dict[1][3][-1], 2)

    def test_offset_index_empty(self):
        dynamic_model_dict = {
            0: [
                [0, -1, -1, -1, -1, -1, -1, -1, -1, 3]
            ],
            1: []
        }
        parser = GeInfoParser({}, {})
        parser.offset_index(dynamic_model_dict, parser.GE_TASK)
        self.assertEqual(dynamic_model_dict[0][0][-1], 1)
        self.assertEqual(dynamic_model_dict[1], [])

    def test_offset_index_both_empty(self):
        dynamic_model_dict = {
            0: [],
            1: []
        }
        parser = GeInfoParser({}, {})
        parser.offset_index(dynamic_model_dict, parser.GE_TASK)

    def test_dynamic_scene_process(self):
        parser = GeInfoParser({}, {})
        parser.ge_info_data = {
            DBNameConstant.TABLE_GE_STEP: [
                    [0, -1, -1, -1, -1, 2, '1'],
                    [0, -1, -1, -1, -1, 3, '0'],
                    [0, -1, -1, -1, -1, 3, '1'],
                    [0, -1, -1, -1, -1, 4, '0'],
                    [0, -1, -1, -1, -1, 4, '1'],
                    [0, -1, -1, -1, -1, 5, '0'],
                    [3333, -1, -1, -1, -1, 55, '1']
                ],
            DBNameConstant.TABLE_GE_TASK: [
                [0, -1, -1, -1, -1, -1, -1, -1, -1, 2],
                [0, -1, -1, -1, -1, -2, -1, -1, -1, 3],
                [0, -1, -1, -1, -1, -3, -1, -1, -1, 3],
                [0, -1, -1, -1, -1, -4, -1, -1, -1, 4],
                [0, -1, -1, -1, -1, -5, -1, -1, -1, 4],
                [0, -1, -1, -1, -1, -6, -1, -1, -1, 5],
                [3333, -1, -1, -1, -7, -1, -1, -1, -1, 55]
            ]
        }
        parser.dynamic_scene_process(DBNameConstant.TABLE_GE_TASK, parser.GE_TASK)
        ret = parser.ge_info_data[DBNameConstant.TABLE_GE_TASK]
        self.assertEqual(sum([ls[5] for ls in ret]), (-2 + -3 + -4 + -5 + -6))
        self.assertEqual(ret[-1][-1], 3)
        self.assertEqual(len(parser.ge_info_data[DBNameConstant.TABLE_GE_STEP]), 7)

    def test_dynamic_scene_process_non_delete(self):
        parser = GeInfoParser({}, {})
        parser.ge_info_data = {
            DBNameConstant.TABLE_GE_STEP: [
                    [0, -1, -1, -1, -1, 3, '0'],
                    [0, -1, -1, -1, -1, 3, '1'],
                    [0, -1, -1, -1, -1, 4, '0'],
                    [0, -1, -1, -1, -1, 4, '1'],
                ],
            DBNameConstant.TABLE_GE_TASK: [
                [0, -1, -1, -1, -1, -1, -1, -1, -1, 3],
                [0, -1, -1, -1, -1, -2, -1, -1, -1, 3],
                [0, -1, -1, -1, -1, -3, -1, -1, -1, 3],
                [0, -1, -1, -1, -1, -4, -1, -1, -1, 4],
                [0, -1, -1, -1, -1, -5, -1, -1, -1, 4],
                [0, -1, -1, -1, -1, -6, -1, -1, -1, 4]
            ]
        }
        parser.dynamic_scene_process(DBNameConstant.TABLE_GE_TASK, parser.GE_TASK)
        ret = parser.ge_info_data[DBNameConstant.TABLE_GE_TASK]
        self.assertEqual(sum([ls[5] for ls in ret]), (-1 + -2 + -3 + -4 + -5 + -6))
        self.assertEqual(ret[-1][-1], 2)

    def test_dynamic_scene_process_empty(self):
        parser = GeInfoParser({}, {})
        parser.ge_info_data = {
            DBNameConstant.TABLE_GE_STEP: [
                    [3333, -1, -1, -1, -1, 55, '0']
                ],
            DBNameConstant.TABLE_GE_TASK: [
                [3333, -1, -1, -1, -1, -1, -1, -1, -1, 55],
                [3333, -1, -1, -1, -2, -1, -1, -1, -1, 55],
                [3333, -1, -1, -1, -3, -1, -1, -1, -1, 55],
                [3333, -1, -1, -1, -4, -1, -1, -1, -1, 55],
            ]
        }
        parser.dynamic_scene_process(DBNameConstant.TABLE_GE_TASK, parser.GE_TASK)
        ret = parser.ge_info_data[DBNameConstant.TABLE_GE_TASK]
        self.assertEqual(len(ret), 4)

    def test_remove_incomplete_index(self):
        ge_step_data = [
                [0, -1, -1, -1, -1, 2, '1'],
                [0, -1, -1, -1, -1, 3, '0'],
                [0, -1, -1, -1, -1, 3, '1'],
                [0, -1, -1, -1, -1, 4, '0'],
        ]
        dynamic_model_dict = {
            0: [
                [0, -1, -1, -1, -1, -1, -1, -1, -1, 2],
                [0, -1, -1, -1, -1, -1, -2, -1, -1, 3],
                [0, -1, -1, -1, -1, -1, -3, -1, -1, 3],
                [0, -1, -1, -1, -1, -1, -4, -1, -1, 4]
            ]
        }
        parser = GeInfoParser({}, {})
        parser.remove_incomplete_index(dynamic_model_dict, ge_step_data, parser.GE_TASK)
        self.assertEqual(len(dynamic_model_dict[0]), 3)

    def test_remove_incomplete_index_tag_type_incorrect(self):
        ge_step_data = [
                [0, -1, -1, -1, -1, 4, 'ad']
        ]
        dynamic_model_dict = {
            0: [
                [0, -1, -1, -1, -1, -1, -4, -1, -1, 4]
            ]
        }
        parser = GeInfoParser({}, {})
        parser.remove_incomplete_index(dynamic_model_dict, ge_step_data, parser.GE_TASK)

    def test_is_complete_iter_exist_true_1(self):
        ge_step_data = [
            [0, -1, -1, -1, -1, 2, '1'],
            [0, -1, -1, -1, -1, 3, '0'],
            [0, -1, -1, -1, -1, 3, '1'],
            [0, -1, -1, -1, -1, 4, '0'],
        ]
        parser = GeInfoParser({}, {})
        ret = parser.is_complete_iter_exist(ge_step_data)
        self.assertEqual(ret, True)

    def test_is_complete_iter_exist_true_2(self):
        ge_step_data = [
            [0, -1, -1, -1, -1, 2, '1'],
            [1, -1, -1, -1, -1, 3, '0'],
            [2, -1, -1, -1, -1, 3, '1'],
            [3, -1, -1, -1, -1, 4, '0'],
            [3, -1, -1, -1, -1, 4, '1']
        ]
        parser = GeInfoParser({}, {})
        ret = parser.is_complete_iter_exist(ge_step_data)
        self.assertEqual(ret, True)

    def test_is_complete_iter_exist_false_1(self):
        ge_step_data = [
            [0, -1, -1, -1, -1, 2, '1']
        ]
        parser = GeInfoParser({}, {})
        ret = parser.is_complete_iter_exist(ge_step_data)
        self.assertEqual(ret, False)

    def test_is_complete_iter_exist_false_2(self):
        ge_step_data = []
        parser = GeInfoParser({}, {})
        ret = parser.is_complete_iter_exist(ge_step_data)
        self.assertEqual(ret, False)

    def test_is_complete_iter_exist_false_3(self):
        ge_step_data = [
            [0, -1, -1, -1, -1, 2, '1'],
            [1, -1, -1, -1, -1, 3, '0'],
            [2, -1, -1, -1, -1, 3, '1'],
            [3, -1, -1, -1, -1, 4, '0'],
            [4, -1, -1, -1, -1, 4, '1']
        ]
        parser = GeInfoParser({}, {})
        ret = parser.is_complete_iter_exist(ge_step_data)
        self.assertEqual(ret, False)
