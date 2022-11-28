#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import unittest
from unittest import mock

from common_func.info_conf_reader import InfoConfReader
from constant.constant import CONFIG
from mscalculate.biu_perf.biu_monitor_calculator import BiuMonitorCalculator
from mscalculate.biu_perf.biu_monitor_calculator import MonitorFlowCalculator
from mscalculate.biu_perf.biu_monitor_calculator import MonitorCyclesCalculator
from profiling_bean.prof_enum.data_tag import DataTag


NAMESPACE = 'mscalculate.biu_perf.biu_monitor_calculator'


class TestBiuMonitorCalculator(unittest.TestCase):
    file_list = {DataTag.AI_CORE: ['aicore.data.0.slice_0']}

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.BiuMonitorCalculator.calculate'), \
                mock.patch(NAMESPACE + '.BiuMonitorCalculator.save'):
            BiuMonitorCalculator(CONFIG).ms_run()


class TestMonitorFlowCalculator(unittest.TestCase):

    def test_calculate(self):
        class DataTest:
            stat_rcmd_num = 1
            stat_wcmd_num = 2
            stat_rlat_raw = 3
            stat_wlat_raw = 4
            stat_flux_rd = 5
            stat_flux_wr = 6
            stat_flux_rd_l2 = 7
            stat_flux_wr_l2 = 7
            timestamp = 7
            L2_cache_hit = 7
            core_id = 7
            group_id = 7
            core_type = 7

        with mock.patch(NAMESPACE + '.BiuPerfModel.get_all_data', return_value=[DataTest]):
            check = MonitorFlowCalculator(CONFIG)
            InfoConfReader().get_biu_sample_cycle = mock.Mock()
            InfoConfReader().get_biu_sample_cycle.return_value = 100
            check.calculate()

    def test_get_unit_name(self):
        check = MonitorFlowCalculator(CONFIG)
        self.assertEqual(check.get_unit_name(1, 1, '2'), 'biu_group1')

    def test_get_pid(self):
        check = MonitorFlowCalculator(CONFIG)
        self.assertEqual(check.get_pid(1, 1), 8)


class TestMonitorCyclesCalculator(unittest.TestCase):

    def test_calculate(self):
        class DataTest:
            vector_cycles = 1
            scalar_cycles = 2
            cube_cycles = 3
            lsu0_cycles = 4
            lsu1_cycles = 5
            lsu2_cycles = 6
            timestamp = 5
            core_id = 6
            group_id = 7
            core_type = 7

        with mock.patch(NAMESPACE + '.BiuPerfModel.get_all_data', return_value=[DataTest]):
            check = MonitorCyclesCalculator(CONFIG)
            InfoConfReader().get_biu_sample_cycle = mock.Mock()
            InfoConfReader().get_biu_sample_cycle.return_value = 100
            check.calculate()

    def test_get_unit_name(self):
        check = MonitorCyclesCalculator(CONFIG)
        self.assertEqual(check.get_unit_name(1, 1, '2'), '2_core1_group1')

    def test_get_pid(self):
        check = MonitorCyclesCalculator(CONFIG)
        self.assertEqual(check.get_pid(1, 1), 5)
        self.assertEqual(check.get_pid(26, 1), 7)
        self.assertEqual(check.get_pid(27, 1), 6)

    def test_save(self):
        with mock.patch(NAMESPACE + '.logging.warning'):
            MonitorCyclesCalculator(CONFIG).save()
        with mock.patch(NAMESPACE + '.logging.warning'),\
                mock.patch(NAMESPACE + '.BiuPerfModel.create_table'),\
                mock.patch(NAMESPACE + '.BiuPerfModel.flush'):
            check = MonitorCyclesCalculator(CONFIG)
            check.data = [123]
            check.save()


if __name__ == '__main__':
    unittest.main()
