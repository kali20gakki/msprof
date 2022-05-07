#!usr/bin/env python
# coding:utf-8
"""
Copyright Huawei Technologies Co., Ltd. 2020. All rights reserved.
"""

import json
import logging
import os

from common_func.empty_class import EmptyClass
from common_func.msvp_common import MsvpCommonConst
from common_func.file_manager import FileOpen
from common_func.common_prof_rule import CommonProfRule
from tuning.meta_metric_manager import OperatorMetricManager
from tuning.metric_factory import OpMetricFactory
from tuning.rule_bean import RuleBean


class Operator:
    """
    operator information, contains op_name, op_type, op_duration and so on
    """

    def __init__(self: any, operator_rule_mgr: any, operator_condition_mgr: any, operator_data: any) -> None:
        self.operator_rule_mgr = operator_rule_mgr
        self.operator_condition_mgr = operator_condition_mgr
        self.operator_metric_mgr = OperatorMetricManager()
        self.operator_data = operator_data
        factory = OpMetricFactory(self)
        rules = self._load_rules_json()
        for rule in rules:
            rule_bean = RuleBean(**rule)
            op_metric = factory.generate_metric(rule_bean.get_rule_id())
            if isinstance(op_metric, EmptyClass):
                continue
            self.operator_metric_mgr.register(rule_bean.get_rule_id(), op_metric)

    @staticmethod
    def _load_rules_json() -> list:
        rule_path = os.path.join(MsvpCommonConst.CONFIG_PATH, CommonProfRule.PROF_RULE_JSON)
        rules = []
        try:
            with FileOpen(rule_path, "r") as rule_reader:
                rule_json = json.load(rule_reader.file_reader)
            return rule_json.get(CommonProfRule.RULE_PROF) if rule_json.get(CommonProfRule.RULE_PROF) else rules
        except FileNotFoundError:
            logging.error("Read rule file failed: %s", os.path.basename(rule_path))
            return rules
        finally:
            pass

    def run(self: any) -> None:
        """
        run rules
        :return: None
        """
        self.operator_rule_mgr.run()

    def get_metric(self: any, metric_name: str) -> any:
        """
        get metric object
        :param metric_name:metric name
        :return: metric object
        """
        return self.operator_metric_mgr.get_metric(metric_name)


class OperatorManager:
    """
    save all operators
    """

    def __init__(self: any) -> None:
        self.op_list = []

    def register(self: any, operator: any) -> None:
        """
        register operator
        :param operator: operator object
        :return: None
        """
        self.op_list.append(operator)

    def run(self: any) -> None:
        """
        run operator
        :return: None
        """
        for operator in self.op_list:
            operator.run()
