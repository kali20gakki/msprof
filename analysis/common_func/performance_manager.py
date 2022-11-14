#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import os
import re

from common_func.constant import Constant


class PerformanceManager:
    """
    Performance metrics Manager
    """

    class PerformanceError(Exception):
        """
        Performance Exception
        """

        def __init__(self: any, message: str) -> None:
            super().__init__(message)
            self.message = message

    PROF_METRIC_FILE = "prof_metric.json"

    @staticmethod
    def check_metric_file(file_path: str) -> bool:
        """
        check validity of file path.
        :param file_path: file path
        :return: result of check file size
        """
        return os.path.exists(file_path) and os.path.getsize(file_path) <= Constant.MAX_READ_FILE_BYTES

    @classmethod
    def filter_case_underscore(cls: any, metric_name: str) -> str:
        """
        filter case and underscore for metric name
        :param metric_name: metric name
        :return: ignore the case under score
        """
        if metric_name:
            metric_name = metric_name.lower().replace("_", " ")
            metric_name = re.sub(r'\s+', " ", metric_name)
        return metric_name

    @classmethod
    def format_performance_explanation(cls: any, metric_name: str, explanation: str) -> str:
        """
        format performance explanation
        :param metric_name: metric name
        :param explanation: explanation for the metric name
        :return: metric name and explanation
        """
        if explanation:
            return "{metric_name} : {explanation}".format(
                metric_name=cls.filter_case_underscore(metric_name), explanation=explanation)
        return "Unrecognized metric name: {metric_name}".format(metric_name=metric_name)
