#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import unittest
from unittest import mock
from msmodel.compact_info.task_track_model import TaskTrackModel
from common_func.db_name_constant import DBNameConstant

NAMESPACE = 'msmodel.compact_info.task_track_model'


class TestTaskTrackModel(unittest.TestCase):
    def test_construct(self):
        check = TaskTrackModel('test')
        self.assertEqual(check.result_dir, 'test')
        self.assertEqual(check.db_name, DBNameConstant.DB_RTS_TRACK)
        self.assertEqual(check.table_list, [DBNameConstant.TABLE_TASK_TRACK])

    def test_flush(self):
        with mock.patch(NAMESPACE + '.TaskTrackModel.insert_data_to_db'):
            check = TaskTrackModel('test')
            check.flush([0, 75838889645892, 'CREATE_STREAM', 0, 0, 270772, 0, 'task_track', 'runtime', 12])
