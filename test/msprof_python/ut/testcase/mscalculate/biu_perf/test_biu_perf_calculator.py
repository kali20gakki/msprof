#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import unittest
from unittest import mock

from common_func.msprof_exception import ProfException
from constant.constant import CONFIG
from mscalculate.biu_perf.biu_perf_calculator import BiuPerfCalculator
from profiling_bean.prof_enum.data_tag import DataTag

NAMESPACE = 'mscalculate.biu_perf.biu_perf_calculator'


class TestBiuPerfCalculator(unittest.TestCase):
    file_list = {DataTag.AI_CORE: ['aicore.data.0.slice_0']}

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.MonitorFlowCalculator.ms_run'), \
                mock.patch(NAMESPACE + '.MonitorCyclesCalculator.ms_run'):
            BiuPerfCalculator({}, CONFIG).ms_run()

    with mock.patch(NAMESPACE + '.MonitorFlowCalculator.ms_run'), \
            mock.patch(NAMESPACE + '.MonitorCyclesCalculator.ms_run', side_effect=ProfException(0)):
        BiuPerfCalculator({}, CONFIG).ms_run()


if __name__ == '__main__':
    unittest.main()