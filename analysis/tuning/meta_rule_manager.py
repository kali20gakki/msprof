#!usr/bin/env python
# coding:utf-8
"""
Copyright Huawei Technologies Co., Ltd. 2020. All rights reserved.
"""


class MetaRuleManager:
    """
    metric rule manager
    """

    def __init__(self: any) -> None:
        self.rules = {}

    def register(self: any, rule: any) -> None:
        """
        register rule
        """
        self.rules[rule.metric_name] = rule

    def run(self: any) -> None:
        """
        run rules
        """
        for rule in self.rules.values():
            rule.run()


class OperatorRuleManager(MetaRuleManager):
    """
    save all operator rule
    """

    def __init__(self: any) -> None:
        MetaRuleManager.__init__(self)


class NetRuleManager(MetaRuleManager):
    """
    save all operator rule
    """

    def __init__(self: any) -> None:
        MetaRuleManager.__init__(self)
