#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from constant.info_json_construct import InfoJson
from constant.info_json_construct import InfoJsonReaderManager
from msmodel.aic.aiv_pmu_model import AivPmuModel

NAMESPACE = 'msmodel.aic.aiv_pmu_model'


class TestAivPmuModel(unittest.TestCase):

    @staticmethod
    def setup_class():
        InfoJsonReaderManager(InfoJson(devices='0')).process()

    def test_init(self):
        with mock.patch('msmodel.interface.base_model.BaseModel.init'), \
                mock.patch(NAMESPACE + '.AivPmuModel.create_table'):
            check = AivPmuModel('test')
            check.init()

    def test_create_table(self):
        with mock.patch(NAMESPACE + '.AivPmuModel.clear'), \
                mock.patch(NAMESPACE + '.get_metrics_from_sample_config',
                           return_value={'ai_core_profiling_events': '0x64,0x65,0x66'}), \
                mock.patch(NAMESPACE + '.insert_metric_value'):
            check = AivPmuModel('test')
            check.create_table()

    def test_flush(self):
        with mock.patch(NAMESPACE + '.AivPmuModel.insert_data_to_db'):
            check = AivPmuModel('test')
            check.flush([])

    def test_clear(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path'), \
                mock.patch(NAMESPACE + '.DBManager.check_tables_in_db', return_value=True), \
                mock.patch(NAMESPACE + '.DBManager.drop_table'):
            check = AivPmuModel('test')
            check.clear()
