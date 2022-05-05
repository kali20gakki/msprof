#!/usr/bin/python3
# coding=utf-8
"""
function: calculator for biu perf data.
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import logging

from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.constant import Constant
from mscalculate.biu_perf.biu_monitor_calculator import Monitor0Calculator
from mscalculate.biu_perf.biu_monitor_calculator import Monitor1Calculator


class BiuPerfCalculator(MsMultiProcess):
    """
    calculate biu perf data
    """

    def __init__(self: any, _: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self.sample_config = sample_config
        self.monitor_calculator_list = [Monitor0Calculator, Monitor1Calculator]

    def ms_run(self: any) -> None:
        """
        calculate for biu perf
        :return: None
        """
        try:
            for monitor_calculator in self.monitor_calculator_list:
                monitor_calculator(self.sample_config).ms_run()
        except ProfException as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
