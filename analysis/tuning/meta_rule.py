#!usr/bin/env python
# coding:utf-8
"""
Copyright Huawei Technologies Co., Ltd. 2020. All rights reserved.
"""

from abc import abstractmethod

from common_func.empty_class import EmptyClass


class MetaRule:
    """
    metric rules
    """

    def __init__(self: any, rule_bean: any, callback: any) -> None:
        self.metric_name = rule_bean.rule_id
        self.rule_type = rule_bean.rule_type
        self.rule_subtype = rule_bean.rule_subtype
        self.tips = rule_bean.rule_suggestion
        self.condition = rule_bean.rule_condition
        self.callback = callback

    def __compare(self: any) -> any:
        return self.get_metric()

    @abstractmethod
    def get_metric(self: any) -> any:
        """
        get metric data
        :return: metrics
        """
        return EmptyClass()

    def run(self: any) -> None:
        """
        run rule validate
        :return: None
        """
        param = self.__compare()
        if param and self.callback:
            self.callback(**param)


class OperatorRule(MetaRule):
    """
    init operator from rule config file
    """

    def __init__(self: any, operator: any, rule_bean: any, callback: any) -> None:
        super(OperatorRule, self).__init__(rule_bean, callback)
        self.operator = operator

    def get_metric(self: any) -> any:
        """
        get metric data by operator
        :return: metrics
        """
        return self.operator.get_metric(self.metric_name)


class NetRule(MetaRule):
    """
    init network from rule config file
    """

    def __init__(self: any, network: any, rule_bean: any, callback: any) -> None:
        super(NetRule, self).__init__(rule_bean, callback)
        self.network = network

    def get_metric(self: any) -> any:
        """
        get metric data by network
        :return: metrics
        """
        return self.network.get_metric(self.metric_name)
