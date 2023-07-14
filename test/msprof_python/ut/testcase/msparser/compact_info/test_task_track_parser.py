#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from msparser.compact_info.task_track_parser import TaskTrackParser
from msparser.compact_info.task_track_bean import TaskTrackBean
from profiling_bean.prof_enum.data_tag import DataTag
from constant.constant import CONFIG

NAMESPACE = 'msparser.compact_info.task_track_parser'
MODEL_NAMESPACE = 'msmodel.compact_info.task_track_model'
GE_HASH_MODEL_NAMESPACE = 'msmodel.ge.ge_hash_model'


class TestTaskTrackParser(unittest.TestCase):
    file_list = {DataTag.TASK_TRACK: ['aging.compact.task_track.slice_0', 'unaging.compact.task_track.slice_0']}

    def test_reformat_data(self):
        InfoConfReader()._start_info = {"clockMonotonicRaw": "0", "cntvct": "0"}
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        hash_key = {
            'runtime': {
                '1000': 'task_track',
            },
        }
        with mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.init'), \
                mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.get_type_hash_data', return_value=hash_key), \
                mock.patch(GE_HASH_MODEL_NAMESPACE + '.GeHashViewModel.finalize'):
            check = TaskTrackParser(self.file_list, CONFIG)
            bean_data = TaskTrackBean([23130, 5000, 1000, 270722, 12, 75838889645892, 0, 0, 0, 0, 1])
            data = check.reformat_data([bean_data])
            self.assertEqual(len(data), 1)
            self.assertEqual(len(data[0]), 10)
            self.assertEqual(data[0][7], '1000')
            data = check.reformat_data([])
            self.assertEqual(data, [])

    def test_parse(self):
        with mock.patch(NAMESPACE + '.TaskTrackParser.parse_bean_data'), \
                mock.patch(NAMESPACE + '.TaskTrackParser.reformat_data'):
            check = TaskTrackParser(self.file_list, CONFIG)
            check.parse()

    def test_save(self):
        with mock.patch(MODEL_NAMESPACE + '.TaskTrackModel.init'), \
                mock.patch(MODEL_NAMESPACE + '.TaskTrackModel.check_db'), \
                mock.patch(MODEL_NAMESPACE + '.TaskTrackModel.create_table'), \
                mock.patch(MODEL_NAMESPACE + '.TaskTrackModel.insert_data_to_db'), \
                mock.patch(MODEL_NAMESPACE + '.TaskTrackModel.finalize'):
            check = TaskTrackParser(self.file_list, CONFIG)
            check._task_track_data = [0, 75838889645892, 'CREATE_STREAM', 0, 0, 270772, 0, 'task_track', 'runtime', 12]
            check.save()

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.TaskTrackParser.parse'), \
                mock.patch(NAMESPACE + '.TaskTrackParser.save'):
            TaskTrackParser(self.file_list, CONFIG).ms_run()
        TaskTrackParser({DataTag.TASK_TRACK: []}, CONFIG).ms_run()
