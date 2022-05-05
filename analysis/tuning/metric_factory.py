#!usr/bin/env python
# coding:utf-8
"""
Copyright Huawei Technologies Co., Ltd. 2020. All rights reserved.
"""

import logging
import configparser
import os
from abc import abstractmethod

from common_func.empty_class import EmptyClass
from common_func.msvp_common import MsvpCommonConst
from common_func.file_manager import check_path_valid
from common_func.common_prof_rule import CommonProfRule
from common_func.constant import Constant
from tuning.meta_metric import NetWorkMetric
from tuning.meta_metric import OperatorMetric


class MetricFactory:
    """
    metric factory
    """

    def __init__(self: any) -> None:
        pass

    @abstractmethod
    def generate_metric(self: any, metric_name: str) -> any:
        """
        generate metrics
        :param metric_name:metric name
        :return: metric object
        """

    @staticmethod
    def get_support_rules(option: str) -> list:
        """
        get rules to check
        :param option: option
        :return: rules
        """
        rule_path = os.path.join(MsvpCommonConst.CONFIG_PATH, CommonProfRule.SUPPORT_RULE_CONF)
        check_path_valid(rule_path, True)
        rule_parser = configparser.ConfigParser()
        try:
            rule_parser.read(rule_path)
            return rule_parser.get(CommonProfRule.PROF_RULES, option).split(",")
        except configparser.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return []


class OpMetricFactory(MetricFactory):
    """
    operator metric factory
    """

    def __init__(self: any, operator: any) -> None:
        super().__init__()
        self.operator = operator
        self.rule_support = self.get_support_rules(CommonProfRule.OPERATOR_RULES)

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
        self.rule_support = self.get_support_rules(CommonProfRule.NETWORK_RULES)

    def generate_metric(self: any, metric_name: str) -> any:
        """
        generate metrics
        :param metric_name: metric name
        :return: metric object
        """
        if metric_name in self.rule_support:
            return NetWorkMetric(self.operator_mgr, self.network_condition_mgr)
        return EmptyClass()
