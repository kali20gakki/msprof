#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import logging

from common_func.common_prof_rule import CommonProfRule
from common_func.info_conf_reader import InfoConfReader
from tuning.data_manager import DataManager
from tuning.meta_condition_manager import NetConditionManager
from tuning.meta_condition_manager import OperatorConditionManager
from tuning.meta_rule import NetRule
from tuning.meta_rule import OperatorRule
from tuning.meta_rule_manager import NetRuleManager
from tuning.meta_rule_manager import OperatorRuleManager
from tuning.ms_operator import Operator
from tuning.ms_operator import OperatorManager
from tuning.network import Network
from tuning.rule_bean import RuleBean
from tuning.tuning_control import TuningControl

from config.config_manager import ConfigManager


class ProfilingTuning:
    """
    recommend for inference
    """

    @staticmethod
    def _generate_rule_bean(rule: any) -> any:
        rule_dic = {
            CommonProfRule.RULE_ID: rule.get(CommonProfRule.RULE_ID),
            CommonProfRule.RULE_CONDITION: rule.get(CommonProfRule.RULE_CONDITION),
            CommonProfRule.RULE_TYPE: rule.get(CommonProfRule.RULE_TYPE),
            CommonProfRule.RULE_SUBTYPE: rule.get(CommonProfRule.RULE_SUBTYPE),
            CommonProfRule.RULE_SUGGESTION: rule.get(CommonProfRule.RULE_SUGGESTION)
        }
        rule_bean = RuleBean(**rule_dic)
        return rule_bean

    @classmethod
    def tuning_operator(cls: any, project: str, device_id: str, iter_id: str) -> None:
        """
        recommend for operator
        """
        operator_dicts = DataManager.get_data_by_infer_id(project, device_id, iter_id)
        if not operator_dicts:
            logging.warning("The operator data is not found, no recommendation is necessary.")
            return
        operator_condition_mgr = OperatorConditionManager()
        operator_mgr = OperatorManager()  # init op mgr
        operator_rule_mgr = OperatorRuleManager()  # init op rule mgr
        operator = Operator(operator_rule_mgr, operator_condition_mgr, operator_dicts[-1])
        rule_json = cls._load_rules()
        tuning_control = TuningControl()
        cls._register_operator_rules(operator, operator_rule_mgr, rule_json, tuning_control)
        operator_mgr.register(operator)  # register op
        operator_mgr.run()
        tuning_control.dump_to_file(project, device_id)

    @classmethod
    def tuning_network(cls: any, project: str, device_id: str, iter_id: str) -> None:
        """
        recommend for network
        """
        operator_dicts = DataManager.get_data_by_infer_id(project, device_id, iter_id)
        operator_mgr = OperatorManager()  # init op mgr
        network_condition_mgr = NetConditionManager()
        operator_condition_mgr = OperatorConditionManager()
        rule_json = cls._load_rules()
        net_rule_mgr = NetRuleManager()
        tuning_control = TuningControl()
        net = Network(net_rule_mgr, network_condition_mgr, operator_mgr)
        cls._register_network_rules(net, net_rule_mgr, rule_json, tuning_control)
        for operator_dict in operator_dicts:
            operator_rule_mgr = OperatorRuleManager()
            operator = Operator(operator_rule_mgr, operator_condition_mgr, operator_dict)
            operator_mgr.register(operator)
            cls._register_operator_rules(operator, operator_rule_mgr, rule_json, tuning_control)
        net.run()
        tuning_control.dump_to_file(project, device_id)

    @classmethod
    def run(cls: any, result_dir: str, iter_id: int = 1) -> None:
        """
        run and recommend
        """
        devices = InfoConfReader().get_device_list()
        if devices and all(i.isdigit() for i in devices):
            for dev_id in devices:
                # default iteration id for report_analysis is 1
                if DataManager.is_network(result_dir, dev_id):
                    cls.tuning_network(result_dir, dev_id, iter_id)
                else:
                    cls.tuning_operator(result_dir, dev_id, iter_id)

    @classmethod
    def _load_rules(cls: any) -> dict:
        return ConfigManager.get(ConfigManager.PROF_RULE).get_data()

    @classmethod
    def _register_network_rules(cls: any, net: any, net_rule_mgr: any, rule_json: dict, tuning_control: any) -> None:
        for rule in rule_json:
            # init rule
            rule_bean = cls._generate_rule_bean(rule)
            net_rule = NetRule(net, rule_bean, tuning_control.tuning_callback)
            net_rule_mgr.register(net_rule)

    @classmethod
    def _register_operator_rules(cls: any, operator: dict, operator_rule_mgr: any, rule_json: dict,
                                 tuning_control: any) -> None:
        for rule in rule_json:
            rule_bean = cls._generate_rule_bean(rule)
            operator_rule = OperatorRule(operator, rule_bean, tuning_control.tuning_callback)
            operator_rule_mgr.register(operator_rule)
