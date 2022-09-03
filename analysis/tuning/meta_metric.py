#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import logging
from abc import abstractmethod

from common_func.common_prof_rule import CommonProfRule


class MetaMetric:
    """
    meta metric interface
    support get metric method
    """

    def __init__(self: any) -> None:
        pass

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return MetaMetric.__name__

    @abstractmethod
    def get_metric(self: any, metric_name: str) -> any:
        """
        calculate metric
        :param metric_name: metric name
        :return:
        """


class OperatorMetric(MetaMetric):
    """
    operator
    """

    def __init__(self: any, operator: any) -> None:
        super().__init__()
        self.operator = operator

    def get_metric(self: any, metric_name: str) -> any:
        operator_rule_mgr = self.operator.operator_rule_mgr
        operator_data = self.operator.operator_data
        operator_condition_mgr = self.operator.operator_condition_mgr
        param = {}
        rule = operator_rule_mgr.rules.get(metric_name)
        if rule is None:
            logging.warning("No rule matching the current operator,metric name is %s", metric_name)
            return param
        if operator_data.get("op_name") is None:
            logging.warning("The current operator does not have an operator name, "
                            "and a sample-based mode may have been set. "
                            "Please check the parameters you delivered.")
        elif operator_condition_mgr.cal_conditions(operator_data, rule.condition):
            param = {CommonProfRule.RESULT_RULE_TYPE: rule.rule_type,
                     CommonProfRule.RESULT_RULE_SUBTYPE: rule.rule_subtype,
                     CommonProfRule.RESULT_RULE_SUGGESTION: rule.tips,
                     CommonProfRule.RESULT_OP_LIST: [operator_data.get("op_name")]}
        return param


class NetWorkMetric(MetaMetric):
    """
    Network
    """

    def __init__(self: any, operator_mgr: any, network_condition_mgr: any) -> None:
        super().__init__()
        self.operator_mgr = operator_mgr
        self.network_condition_mgr = network_condition_mgr

    def get_metric(self: any, metric_name: str) -> any:
        param = {}
        if len(self.operator_mgr.op_list) > 0:
            operator_rule_mgr = self.operator_mgr.op_list[0].operator_rule_mgr
            rule = operator_rule_mgr.rules.get(metric_name)
            if rule is None:
                logging.warning("No rule matching the current operator for network,metric name is %s", metric_name)
                return param
            op_names = self.network_condition_mgr.cal_conditions(self.operator_mgr.op_list,
                                                                 rule.condition)
            if op_names:
                param = {CommonProfRule.RESULT_RULE_TYPE: rule.rule_type,
                         CommonProfRule.RESULT_RULE_SUBTYPE: rule.rule_subtype,
                         CommonProfRule.RESULT_RULE_SUGGESTION: rule.tips,
                         CommonProfRule.RESULT_OP_LIST: op_names}
        return param
