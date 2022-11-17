#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from config.config_manager import ConfigManager
from common_func.empty_class import EmptyClass
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
        return ConfigManager.get(ConfigManager.PROF_RULE).get_data()

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
