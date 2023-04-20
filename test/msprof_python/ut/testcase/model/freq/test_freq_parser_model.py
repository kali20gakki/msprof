#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import unittest
from unittest import mock
from msmodel.freq.freq_parser_model import FreqParserModel
from common_func.db_name_constant import DBNameConstant

NAMESPACE = 'msmodel.freq.freq_parser_model'
PATH_NAMESPACE = 'common_func.path_manager'
DB_NAMESPACE = 'common_func.db_manager'


class TestFreqParserModel(unittest.TestCase):
    def test_construct(self):
        check = FreqParserModel('test', [DBNameConstant.TABLE_FREQ_PARSE])
        self.assertEqual(check.result_dir, 'test')
        self.assertEqual(check.db_name, DBNameConstant.DB_FREQ)
        self.assertEqual(check.table_list, [DBNameConstant.TABLE_FREQ_PARSE])

    def test_get_freq_data(self):
        with mock.patch(PATH_NAMESPACE + '.PathManager.get_db_path'), \
                mock.patch(DB_NAMESPACE + '.DBManager.check_connect_db_path', return_value=(0, 0)), \
                mock.patch(DB_NAMESPACE + '.DBManager.destroy_db_connect'):
            check = FreqParserModel('test', [DBNameConstant.TABLE_FREQ_PARSE])
            self.assertEqual(check.get_freq_data('test'), [])
        with mock.patch(PATH_NAMESPACE + '.PathManager.get_db_path'), \
                mock.patch(DB_NAMESPACE + '.DBManager.check_connect_db_path', return_value=(1, 1)), \
                mock.patch(DB_NAMESPACE + '.DBManager.judge_table_exist', return_value=True), \
                mock.patch(DB_NAMESPACE + '.DBManager.fetch_all_data', return_value=[[100, 200]]), \
                mock.patch(DB_NAMESPACE + '.DBManager.destroy_db_connect'):
            check = FreqParserModel('test', [DBNameConstant.TABLE_FREQ_PARSE])
            self.assertEqual(check.get_freq_data('test'), [[100, 200]])

    def test_flush(self):
        with mock.patch(NAMESPACE + '.FreqParserModel.insert_data_to_db'):
            check = FreqParserModel('test', [DBNameConstant.TABLE_FREQ_PARSE])
            check.flush([[100, 200]])
