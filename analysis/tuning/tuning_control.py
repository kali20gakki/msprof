#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import json
import logging
import os

from common_func.common import CommonConstant
from common_func.common_prof_rule import CommonProfRule
from common_func.constant import Constant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msprof_common import check_path_valid
from common_func.msprof_exception import ProfException
from common_func.path_manager import PathManager


class TuningControl:
    """
    data control for tuning
    """

    def __init__(self: any) -> None:
        self._init_tuning_data()
        self.title_map = {
            CommonProfRule.RESULT_COMPUTATION: self._model_computation_tuning,
            CommonProfRule.RESULT_MEMORY: self._model_memory_tuning,
            CommonProfRule.RESULT_OPERATOR_SCHEDULE: self._operator_schedule_tuning,
            CommonProfRule.RESULT_OPERATOR_PROCESSING: self._operator_process_tuning,
            CommonProfRule.RESULT_OPERATOR_METRICS: self._operator_metrics_tuning,
            CommonProfRule.RESULT_OPERATOR_PARALLELISM: self._operator_parallel_tuning,
            CommonProfRule.RESULT_MODEL_BOTTLENECK: self._model_bottleneck_tuning
        }

    def add_tuning_result(self: any, **param: dict) -> None:
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
        if device_id == str(NumberConstant.HOST_ID):
            file_name = CommonProfRule.RESULT_PROF_JSON_HOST
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

    def _init_tuning_data(self: any) -> None:
        self.model_computation = {
            CommonProfRule.RESULT_RULE_TYPE: CommonProfRule.RESULT_COMPUTATION,
            CommonProfRule.RESULT_RULE_DESCRIPTION: CommonProfRule.RESULT_COMPUTATION_DESCRIPTION,
            CommonProfRule.RESULT_KEY: []
        }
        self.model_memory = {
            CommonProfRule.RESULT_RULE_TYPE: CommonProfRule.RESULT_MEMORY,
            CommonProfRule.RESULT_RULE_DESCRIPTION: CommonProfRule.RESULT_MEMORY_DESCRIPTION,
            CommonProfRule.RESULT_KEY: []
        }
        self.operator_schedule = {
            CommonProfRule.RESULT_RULE_TYPE: CommonProfRule.RESULT_OPERATOR_SCHEDULE,
            CommonProfRule.RESULT_RULE_DESCRIPTION: CommonProfRule.RESULT_SCHEDULE_DESCRIPTION,
            CommonProfRule.RESULT_KEY: []
        }
        self.operator_processing = {
            CommonProfRule.RESULT_RULE_TYPE: CommonProfRule.RESULT_OPERATOR_PROCESSING,
            CommonProfRule.RESULT_RULE_DESCRIPTION: CommonProfRule.RESULT_PROCESSING_DESCRIPTION,
            CommonProfRule.RESULT_KEY: []
        }
        self.operator_metrics = {
            CommonProfRule.RESULT_RULE_TYPE: CommonProfRule.RESULT_OPERATOR_METRICS,
            CommonProfRule.RESULT_RULE_DESCRIPTION: CommonProfRule.RESULT_METRICS_DESCRIPTION,
            CommonProfRule.RESULT_KEY: []
        }
        self.operator_parallelism = {
            CommonProfRule.RESULT_RULE_TYPE: CommonProfRule.RESULT_OPERATOR_PARALLELISM,
            CommonProfRule.RESULT_RULE_DESCRIPTION: CommonProfRule.RESULT_PARALLEL_DESCRIPTION,
            CommonProfRule.RESULT_KEY: []
        }
        self.model_bottleneck = {
            CommonProfRule.RESULT_RULE_TYPE: CommonProfRule.RESULT_MODEL_BOTTLENECK,
            CommonProfRule.RESULT_RULE_DESCRIPTION: CommonProfRule.RESULT_BOTTLENECK_DESCRIPTION,
            CommonProfRule.RESULT_KEY: []
        }
        self.data = {
            CommonProfRule.TUNING_OPERATOR: [self.model_computation, self.model_memory, self.operator_schedule,
                                             self.operator_processing, self.operator_metrics],
            CommonProfRule.TUNING_OP_PARALLEL: [self.operator_parallelism],
            CommonProfRule.TUNING_MODEL: [self.model_bottleneck]
        }

    def _model_computation_tuning(self, result):
        return self.model_computation.get(CommonProfRule.RESULT_KEY).append(result)

    def _model_memory_tuning(self, result):
        return self.model_memory.get(CommonProfRule.RESULT_KEY).append(result)

    def _operator_schedule_tuning(self, result):
        return self.operator_schedule.get(CommonProfRule.RESULT_KEY).append(result)

    def _operator_process_tuning(self, result):
        return self.operator_processing.get(CommonProfRule.RESULT_KEY).append(result)

    def _operator_metrics_tuning(self, result):
        return self.operator_metrics.get(CommonProfRule.RESULT_KEY).append(result)

    def _operator_parallel_tuning(self, result):
        return self.operator_parallelism.get(CommonProfRule.RESULT_KEY).append(result)

    def _model_bottleneck_tuning(self, result):
        return self.model_bottleneck.get(CommonProfRule.RESULT_KEY).append(result)
