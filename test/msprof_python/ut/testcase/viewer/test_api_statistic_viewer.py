#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2023. All rights reserved.
"""
import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from common_func.msvp_constant import MsvpConstant
from viewer.api_statistic_viewer import ApiStatisticViewer

NAMESPACE = 'viewer.api_statistic_viewer'


class TestApiStatisticViewer(unittest.TestCase):

    def test_get_api_summary_data_should_return_success_when_model_init_ok_and_data_exists(self):
        config = {
            'headers': [
                'level', 'API Name', 'Time(us)', 'Count',
                'Avg(us)', 'Min(us)', 'Max(us)', 'Variance'
            ]
        }
        params = {
            "project": "test_get_api_summary_data",
            "model_id": 1,
            "iter_id": 1
        }
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        data = [(0, 1, 2, 3, 4, 5, 6, 7, 8)]
        with mock.patch(NAMESPACE + '.ApiDataViewModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.ApiDataViewModel.get_api_statistic_data', return_value=data), \
                mock.patch(NAMESPACE + '.ApiDataViewModel.get_api_statistic_data_for_variance', return_value=[(0, 0)]):
            check = ApiStatisticViewer(config, params)
            ret = check.get_api_summary_data()
            self.assertEqual([(7, 0, 0.002, 3, 0.004, 0.005, 0.006, 0)], ret)

    def test_get_api_summary_data_should_return_empty_list_when_model_init_fail(self):
        config = {
            'headers': [
                'level', 'API Name', 'Time(us)', 'Count',
                'Avg(us)', 'Min(us)', 'Max(us)', 'Variance'
            ]
        }
        params = {
            "project": "test_get_api_summary_data",
            "model_id": 2,
            "iter_id": 2
        }
        check = ApiStatisticViewer(config, params)
        ret = check.get_api_summary_data()
        self.assertEqual(MsvpConstant.EMPTY_LIST, ret)

    def test_get_api_summary_data_should_return_empty_when_model_init_ok_but_data_misses(self):
        config = {
            'headers': [
                'level', 'API Name', 'Time(us)', 'Count',
                'Avg(us)', 'Min(us)', 'Max(us)', 'Variance'
            ]
        }
        params = {
            "project": "test_get_api_summary_data",
            "model_id": 3,
            "iter_id": 3
        }
        with mock.patch(NAMESPACE + '.ApiDataViewModel.init', return_value=True), \
                mock.patch(NAMESPACE + ".ApiDataViewModel.get_api_statistic_data", return_value=[]), \
                mock.patch(NAMESPACE + ".ApiDataViewModel.get_api_statistic_data_for_variance", return_value=[]):
            check = ApiStatisticViewer(config, params)
            ret = check.get_api_summary_data()
            self.assertEqual(MsvpConstant.EMPTY_LIST, ret)

    def test_get_api_statistic_data_should_return_success_when_model_init_ok_and_data_exists(self):
        config = {
            'headers': [
                'level', 'API Name', 'Time(us)', 'Count',
                'Avg(us)', 'Min(us)', 'Max(us)', 'Variance'
            ]
        }
        params = {
            "project": "test_get_api_statistic_data",
            "model_id": 4,
            "iter_id": 4
        }
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        data = [(0, 1, 2, 3, 4, 5, 6, 7, 8)]
        with mock.patch(NAMESPACE + '.ApiDataViewModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.ApiDataViewModel.get_api_statistic_data', return_value=data), \
                mock.patch(NAMESPACE + '.ApiDataViewModel.get_api_statistic_data_for_variance', return_value=[(0, 0)]):
            check = ApiStatisticViewer(config, params)
            ret = check.get_api_statistic_data()
            self.assertEqual((["level", "API Name", "Time(us)", "Count",
                              "Avg(us)", "Min(us)", "Max(us)", "Variance"],
                              [(7, 0, 0.002, 3, 0.004, 0.005, 0.006, 0)],
                              1), ret)

    def test_get_api_statistic_data_should_return_success_when_model_init_ok_and_data_is_double(self):
        config = {
            'headers': [
                'level', 'API Name', 'Time(us)', 'Count',
                'Avg(us)', 'Min(us)', 'Max(us)', 'Variance'
            ]
        }
        params = {
            "project": "test_get_api_statistic_data",
            "model_id": 5,
            "iter_id": 5
        }
        InfoConfReader()._host_freq = None
        InfoConfReader()._info_json = {'CPU': [{'Frequency': "1000"}]}
        data = [(0, 1, 2, 3, 4, 5, 6, 7, 8)]
        with mock.patch(NAMESPACE + '.ApiDataViewModel.init', return_value=True), \
                mock.patch(NAMESPACE + '.ApiDataViewModel.get_api_statistic_data', return_value=data + data), \
                mock.patch(NAMESPACE + '.ApiDataViewModel.get_api_statistic_data_for_variance', return_value=[(0, 0)]):
            check = ApiStatisticViewer(config, params)
            ret = check.get_api_statistic_data()
            self.assertEqual((["level", "API Name", "Time(us)", "Count",
                              "Avg(us)", "Min(us)", "Max(us)", "Variance"],
                              [(7, 0, 0.002, 3, 0.004, 0.005, 0.006, 0),
                               (7, 0, 0.002, 3, 0.004, 0.005, 0.006, 0)],
                              2), ret)
