#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from common_func.common_prof_rule import CommonProfRule
from config.config_manager import ConfigManager
from tuning.data_manager import DataManager
from tuning.rule_bean import RuleBean
from tuning.meta_condition_manager import NetConditionManager
from tuning.tuning_control import TuningControl


class RuleManager:
    """
    rule manager
    """

    def __init__(self: any, project: str, device_id: str, iter_id: str) -> None:
        self.rule_list = []
        self.data_mgr = DataManager(project, device_id, iter_id)
        self.condition_mgr = NetConditionManager()
        self.rules = self._load_rules()
        self.tuning_control = TuningControl()
        self.project = project
        self.device_id = device_id

    @staticmethod
    def _get_support_rules() -> list:
        """
        get rules to check
        :return: rules
        """
        return ConfigManager.get(ConfigManager.TUNING_RULE).get_data()

    @classmethod
    def _load_rules(cls: any) -> list:
        rules_json = ConfigManager.get(ConfigManager.PROF_RULE).get_data()
        support_rules = set(cls._get_support_rules())
        rules = []
        for rule in rules_json:
            if rule.get(CommonProfRule.RULE_ID, '') in support_rules:
                rules.append(rule)
        return rules

    def run(self: any) -> None:
        """
        run rules
        """
        for rule in self.rules:
            rule = RuleBean(**rule)
            condition = rule.get_rule_condition()
            tuning_type = CommonProfRule.TUNING_OPERATOR
            tuning_data = self.data_mgr.get_data(tuning_type)
            data_names = self.condition_mgr.cal_conditions(tuning_data, condition)
            if not data_names:
                continue
            param = {
                CommonProfRule.RESULT_RULE_TYPE: rule.rule_type,
                CommonProfRule.RESULT_RULE_SUBTYPE: rule.rule_subtype,
                CommonProfRule.RESULT_RULE_SUGGESTION: rule.rule_suggestion,
                CommonProfRule.RESULT_OP_LIST: data_names
            }
            self.tuning_control.add_tuning_result(**param)
        self.tuning_control.dump_to_file(self.project, self.device_id)
