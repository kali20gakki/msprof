#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import json
import logging
import os

from common_func.empty_class import EmptyClass
from common_func.msvp_common import MsvpCommonConst
from common_func.file_manager import FileOpen
from common_func.common_prof_rule import CommonProfRule
from tuning.meta_metric_manager import NetMetricManager
from tuning.metric_factory import NetMetricFactory
from tuning.rule_bean import RuleBean


class Network:
    """
    net information, contains all operator
    """

    def __init__(self: any, net_rule_mgr: any, network_condition_mgr: any, operator_mgr: any) -> None:
        self.net_rule_mgr = net_rule_mgr
        self.network_condition_mgr = network_condition_mgr
        self.operator_mgr = operator_mgr
        self.net_metric_mgr = NetMetricManager()
        factory = NetMetricFactory(self.operator_mgr, self.network_condition_mgr)
        rules = self._load_rules_json()
        for rule in rules:
            rule_bean = RuleBean(**rule)
            net_metric = factory.generate_metric(rule_bean.get_rule_id())
            if isinstance(net_metric, EmptyClass):
                continue
            self.net_metric_mgr.register(rule_bean.get_rule_id(), net_metric)

    @staticmethod
    def _load_rules_json() -> list:
        rule_path = os.path.join(MsvpCommonConst.CONFIG_PATH, CommonProfRule.PROF_RULE_JSON)
        net_rules = []
        try:
            with FileOpen(rule_path, "r") as rule_reader:
                rule_json = json.load(rule_reader.file_reader)
            return rule_json.get(CommonProfRule.RULE_PROF) if rule_json.get(CommonProfRule.RULE_PROF) else net_rules
        except FileNotFoundError:
            logging.error("Read rule file failed: %s", os.path.basename(rule_path))
            return net_rules
        finally:
            pass

    def run(self: any) -> None:
        """
        run rules
        :return: None
        """
        self.net_rule_mgr.run()

    def get_metric(self: any, metric_name: str) -> any:
        """
        get metrics
        :param metric_name: metric name
        :return: metric object
        """
        return self.net_metric_mgr.get_metric(metric_name)
