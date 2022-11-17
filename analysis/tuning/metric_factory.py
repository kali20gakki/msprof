#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from abc import abstractmethod

from common_func.empty_class import EmptyClass
from config.config_manager import ConfigManager
from tuning.meta_metric import NetWorkMetric
from tuning.meta_metric import OperatorMetric


class MetricFactory:
    """
    metric factory
    """

    def __init__(self: any) -> None:
        pass

    @staticmethod
    def get_support_rules() -> list:
        """
        get rules to check
        :param option: option
        :return: rules
        """
        return ConfigManager.get(ConfigManager.TUNING_RULE).get_data()

    @abstractmethod
    def generate_metric(self: any, metric_name: str) -> any:
        """
        generate metrics
        :param metric_name:metric name
        :return: metric object
        """


class OpMetricFactory(MetricFactory):
    """
    operator metric factory
    """

    def __init__(self: any, operator: any) -> None:
        super().__init__()
        self.operator = operator
        self.rule_support = self.get_support_rules()

    def generate_metric(self: any, metric_name: str) -> any:
        """
        generate metrics
        :param metric_name: metric name
        :return: metric object
        """
        if metric_name in self.rule_support:
            return OperatorMetric(self.operator)
        return EmptyClass()


class NetMetricFactory(MetricFactory):
    """
    network metric factory
    """

    def __init__(self: any, operator_mgr: any, network_condition_mgr: any) -> None:
        super().__init__()
        self.operator_mgr = operator_mgr
        self.network_condition_mgr = network_condition_mgr
        self.rule_support = self.get_support_rules()

    def generate_metric(self: any, metric_name: str) -> any:
        """
        generate metrics
        :param metric_name: metric name
        :return: metric object
        """
        if metric_name in self.rule_support:
            return NetWorkMetric(self.operator_mgr, self.network_condition_mgr)
        return EmptyClass()
