#!usr/bin/env python
# coding:utf-8
"""
Copyright Huawei Technologies Co., Ltd. 2020. All rights reserved.
"""

import json
import logging
import os
import sys

from common_func.common import CommonConstant
from common_func.common_prof_rule import CommonProfRule
from common_func.constant import Constant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.path_manager import PathManager
from common_func.msprof_common import check_path_valid
from common_func.msprof_exception import ProfException


class TuningControl:
    """
    data control for tuning
    """

    def __init__(self: any) -> None:
        self._init_tuning_data()
        self.title_map = {
            CommonProfRule.RESULT_MODEL_COMPUTATION: lambda result: self.model_computation.get(
                CommonProfRule.RESULT_KEY).append(result),
            CommonProfRule.RESULT_MODEL_MEMORY: lambda result: self.model_memory.get(
                CommonProfRule.RESULT_KEY).append(result),
            CommonProfRule.RESULT_OPERATOR_SCHEDULE: lambda result: self.operator_schedule.get(
                CommonProfRule.RESULT_KEY).append(result),
            CommonProfRule.RESULT_OPERATOR_PROCESSING: lambda result: self.operator_processing.get(
                CommonProfRule.RESULT_KEY).append(result),
            CommonProfRule.RESULT_OPERATOR_METRICS: lambda result: self.operator_metrics.get(
                CommonProfRule.RESULT_KEY).append(result)
        }

    def _init_tuning_data(self: any) -> None:
        self.model_computation = {CommonProfRule.RESULT_RULE_TYPE: CommonProfRule.RESULT_MODEL_COMPUTATION,
                                  CommonProfRule.RESULT_KEY: []}
        self.model_memory = {CommonProfRule.RESULT_RULE_TYPE: CommonProfRule.RESULT_MODEL_MEMORY,
                             CommonProfRule.RESULT_KEY: []}
        self.operator_schedule = {CommonProfRule.RESULT_RULE_TYPE: CommonProfRule.RESULT_OPERATOR_SCHEDULE,
                                  CommonProfRule.RESULT_KEY: []}
        self.operator_processing = {CommonProfRule.RESULT_RULE_TYPE: CommonProfRule.RESULT_OPERATOR_PROCESSING,
                                    CommonProfRule.RESULT_KEY: []}
        self.operator_metrics = {CommonProfRule.RESULT_RULE_TYPE: CommonProfRule.RESULT_OPERATOR_METRICS,
                                 CommonProfRule.RESULT_KEY: []}
        self.data = [self.model_computation, self.model_memory, self.operator_schedule,
                     self.operator_processing, self.operator_metrics]

    def tuning_callback(self: any, **param: dict) -> None:
        """
        entry of tuning callback
        """
        if param:
            self.title_map.get(param.pop(CommonProfRule.RESULT_RULE_TYPE))(param)

    def dump_to_file(self: any, result_dir: str, device_id: str) -> None:
        """
        write tuning result to file
        """
        file_name = CommonProfRule.RESULT_PROF_JSON.format(device_id)
        output_dir = PathManager.get_summary_dir(result_dir)
        result_file = os.path.join(output_dir, file_name)
        try:
            check_path_valid(output_dir, True)
        except ProfException as ex:
            logging.error("write tuning result file failed: %s", file_name)
            raise ProfException(ProfException.PROF_SYSTEM_EXIT) from ex

        with os.fdopen(os.open(result_file, Constant.WRITE_FLAGS,
                               Constant.WRITE_MODES), 'w') as f_write:
            os.chmod(result_file, CommonConstant.FILE_AUTHORITY)
            json.dump({"status": NumberConstant.SUCCESS, "data": self.data}, f_write)
