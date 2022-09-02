#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock
import multiprocessing

from common_func.info_conf_reader import InfoConfReader
from framework.collection_engine import AI

from constant.constant import CONFIG, INFO_JSON

NAMESPACE = 'framework.collection_engine'


class TestCollectionEngine(unittest.TestCase):

    def test_import_control_flow(self):
        with mock.patch(NAMESPACE + '.AI.data_analysis', side_effect=multiprocessing.ProcessError),\
                mock.patch(NAMESPACE + '.logging.error'),\
                mock.patch(NAMESPACE + '.logging.info'):
            check = AI(CONFIG)
            check.import_control_flow()

    def test_data_analysis(self):
        InfoConfReader()._info_json = {}
        with mock.patch(NAMESPACE + '.AI.formulat_list'):
            check = AI(CONFIG)
            check.data_analysis()

    def test_data_analysis_2(self):
        InfoConfReader()._info_json = INFO_JSON
        with mock.patch(NAMESPACE + '.AI.formulat_list'):
            check = AI(CONFIG)
            check.data_analysis()

    def test_project_preparation(self):
        project_dir = "test100"
        with mock.patch(NAMESPACE + '.PathManager.get_data_dir'), \
             mock.patch(NAMESPACE + '.PathManager.get_sql_dir'), \
             mock.patch(NAMESPACE + '.check_dir_writable'), \
             mock.patch(NAMESPACE + '.AI._create_collection_log'), \
             mock.patch('os.path.exists', return_value=False), \
             mock.patch('os.makedirs'), \
             mock.patch('os.remove'), \
             mock.patch('os.path.join', return_value="test"), \
             mock.patch('os.listdir', return_value=['test.complete']):
            AI.project_preparation(project_dir)

    def test_create_collection_log(self):
        project_dir = "test100"
        with mock.patch(NAMESPACE + '.check_dir_writable'), \
             mock.patch(NAMESPACE + '.PathManager.get_log_dir'), \
             mock.patch('builtins.open', side_effect=OSError), \
             mock.patch(NAMESPACE + '.error'):
            AI._create_collection_log(project_dir)
