#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import os
import json
import logging
import re
import time
from abc import abstractmethod

from common_func.common_prof_rule import CommonProfRule
from common_func.ms_constant.number_constant import NumberConstant
from common_func.msvp_common import is_number
from common_func.regex_manager import RegexManagerConstant

from config.config_manager import ConfigManager


def load_condition_files() -> dict:
    """
    load condition files
    """
    conditions = {}
    for json_condition in ConfigManager.get(ConfigManager.PROF_CONDITION).get_data():
        conditions[json_condition.get(CommonProfRule.CONDITION_ID)] = json_condition
    return conditions


def compare_number(left: any, right: any, equal_operator: bool):
    """
    compare number
    """
    if equal_operator:
        if is_number(left) and is_number(right):
            return str(get_two_decimal(left)) == str(get_two_decimal(right))
        return str(left) == str(right)

    if is_number(left) and is_number(right):
        return str(get_two_decimal(left)) != str(get_two_decimal(right))
    return str(left) != str(right)


class MetaConditionManager:
    """
    metric condition manager
    """
    COMPARE_MAP = {
        ">": lambda left, right: float(left) > float(right) if is_number(left) and is_number(right) else False,
        "<": lambda left, right: float(left) < float(right) if is_number(left) and is_number(right) else False,
        "==": lambda left, right: compare_number(left, right, equal_operator=True),
        "!=": lambda left, right: compare_number(left, right, equal_operator=False)
    }

    CALCULATE_MAP = {
        "+": lambda left, right: float(left) + float(right) if left and right else False,
        "-": lambda left, right: float(left) - float(right) if left and right else False,
        "*": lambda left, right: float(left) * float(right) if left and right else False,
        "/": lambda left, right: float(left) / float(right) if left and right else False,
        "%": lambda left, right: float(left) % float(right) if left and right else False,
        "&": lambda left, right: int(left) & int(float(right)) if is_number(left) and is_number(right) else False
    }

    REGEX_CALCULATE_MAP = {
        "+": RegexManagerConstant.REGEX_CALCULATE_ADDITION,
        "-": RegexManagerConstant.REGEX_CALCULATE_SUBTRACTION,
        "*": RegexManagerConstant.REGEX_CALCULATE_MULTIPLY,
        "/": RegexManagerConstant.REGEX_CALCULATE_DIVISION,
        "%": RegexManagerConstant.REGEX_CALCULATE_REMAINDER,
        "&": RegexManagerConstant.REGEX_CALCULATE_SRCAND
    }

    OPERATE_MAP = {
        "&&": lambda left, right: left & right if isinstance(left, set) and isinstance(right, set) else set(),
        "||": lambda left, right: left | right if isinstance(left, set) and isinstance(right, set) else set()
    }

    def __init__(self: any) -> None:
        self.conditions = load_condition_files()

    @abstractmethod
    def cal_count_condition(self: any, operator_data_list: dict, condition: dict) -> list:
        """
        calculate count condition
        """

    @abstractmethod
    def cal_accumulate_condition(self: any, operator_data_list: dict, condition: dict) -> list:
        """
        calculate accumulate condition
        """

    @staticmethod
    def __format_operator(expression: str) -> str:
        """
        format operator str
        """
        while re.findall(RegexManagerConstant.REGEX_CALCULATE_OPERATOR, expression):
            expression = expression.replace('+-', '-')
            expression = expression.replace('++', '+')
            expression = expression.replace('--', '+')
            expression = expression.replace('-+', '-')
        return expression

    @classmethod
    def calculate_expression(cls: any, expression: str) -> str:
        """
        calculate expression
        """
        expression = cls.__format_operator(expression)
        while re.search(RegexManagerConstant.REGEX_INNER_BRACKET, expression):
            sub_expression = re.search(RegexManagerConstant.REGEX_INNER_BRACKET, expression).group()
            cal_result = cls.calculate_basic_expression(sub_expression[1:-1])
            expression = expression.replace(sub_expression, cal_result, 1)
        return cls.calculate_basic_expression(expression)

    @classmethod
    def calculate_basic_expression(cls: any, sub_expression: str) -> str:
        """
        calculate basic expression
        """
        # calulate mutiply/div
        sub_expression = cls.__format_operator(sub_expression)
        high_priority_operators = re.sub(RegexManagerConstant.REGEX_HIGH_OPERATOR, '', sub_expression)
        for operator in high_priority_operators:
            sub_expression = cls.__calculate_with_operator(operator, sub_expression)

        # calculate add/subtract
        sub_expression = cls.__format_operator(sub_expression)
        low_priority_operators = re.sub(RegexManagerConstant.REGEX_LOW_OPERATOR, '', sub_expression)
        for operator in low_priority_operators:
            sub_expression = cls.__calculate_with_operator(operator, sub_expression)
        return sub_expression

    @classmethod
    def cal_formula_condition(cls: any, operator_data: dict, condition: dict) -> list:
        """
        calculate formula condition
        """
        op_names = []
        format_value = []
        for key in condition.get(CommonProfRule.CONDITION_LEFT):
            if operator_data.get(key) is not None:
                format_value.append(operator_data.get(key))
            else:
                return op_names

        expression = condition.get(CommonProfRule.COND_TYPE_FORMULA).format(*format_value)
        if condition.get(CommonProfRule.CONDITION_CMP) in cls.COMPARE_MAP:
            if cls.COMPARE_MAP.get(condition.get(
                    CommonProfRule.CONDITION_CMP))(cls.calculate_expression(expression),
                                                   condition.get(CommonProfRule.CONDITION_RIGHT)):
                op_names.append(operator_data.get("op_name"))
        return op_names

    @classmethod
    def cal_normal_condition(cls: any, operator_data: dict, condition: dict) -> list:
        """
        calculate normal condition
        """
        op_names = []
        if condition.get(CommonProfRule.CONDITION_CMP) in cls.COMPARE_MAP \
                and operator_data.get(condition.get(CommonProfRule.CONDITION_LEFT)) is not None:
            if cls.COMPARE_MAP.get(condition.get(CommonProfRule.CONDITION_CMP))(
                    operator_data.get(condition.get(CommonProfRule.CONDITION_LEFT)),
                    condition.get(CommonProfRule.CONDITION_RIGHT)):
                op_names.append(operator_data.get("op_name"))
        return op_names

    @classmethod
    def __calculate_with_operator(cls: any, operator: str, sub_expression: str) -> str:
        if operator in cls.REGEX_CALCULATE_MAP:
            match_expression = re.search(cls.REGEX_CALCULATE_MAP.get(operator), sub_expression).group()
        else:
            logging.error("Not support operator type: %s ", operator)
            return sub_expression
        if match_expression:
            cal_result = cls.CALCULATE_MAP.get(operator)(match_expression.split(operator)[0],
                                                         match_expression.split(operator)[1])
            sub_expression = sub_expression.replace(str(match_expression), str(cal_result))
        return sub_expression

    def merge_set(self: any, condition_id: str, condition_id_dict: dict) -> str:
        """
        merge set
        """
        while re.search(RegexManagerConstant.REGEX_CONNECTED, condition_id):
            cal_condition = re.search(RegexManagerConstant.REGEX_CONNECTED, condition_id).group()
            condition_ids = list(filter(None, re.split(RegexManagerConstant.REGEX_SPLIT_CONNECTED, cal_condition)))
            cmp = re.sub(RegexManagerConstant.REGEX_COMPARATOR_SYMBOLS, '', cal_condition)
            stamp_condition = str(time.time())
            condition_id_dict[stamp_condition] = self.OPERATE_MAP.get(cmp)(
                condition_id_dict.get(condition_ids[0]), condition_id_dict.get(condition_ids[1]))
            condition_id = condition_id.replace(cal_condition, stamp_condition, 1)
        return condition_id

    def cal_conditions(self: any, operator_data: list, condition_id: str) -> list:
        """
        calculate conditions
        """
        condition_ids = list(filter(None, re.split(RegexManagerConstant.REGEX_SPLIT_CONNECTED, condition_id)))
        condition_id_dict = {}
        for cond_id in condition_ids:
            condition_id_dict[cond_id] = set(self.cal_condition(operator_data, cond_id))
        while re.search(RegexManagerConstant.REGEX_INNER_BRACKET, condition_id):
            sub_condition = re.search(RegexManagerConstant.REGEX_INNER_BRACKET, condition_id).group()
            cal_result = self.merge_set(sub_condition[1:-1], condition_id_dict)
            condition_id = condition_id.replace(sub_condition, cal_result, 1)
        return list(condition_id_dict.get(self.merge_set(condition_id, condition_id_dict)))

    def cal_condition(self: any, operator_data: list, condition_id: str) -> list:
        """
        calculate conditions
        """
        condition = self.conditions.get(condition_id)
        if condition is None:
            logging.error("The condition can not be found,condition id is: %s ", condition_id)
            return []
        if condition.get(CommonProfRule.CONDITION_TYPE) == CommonProfRule.COND_TYPE_NORMAL:
            return self.cal_normal_condition(operator_data, condition)
        if condition.get(CommonProfRule.CONDITION_TYPE) == CommonProfRule.COND_TYPE_FORMULA:
            return self.cal_formula_condition(operator_data, condition)
        if condition.get(CommonProfRule.CONDITION_TYPE) == CommonProfRule.COND_TYPE_COUNT:
            return self.cal_count_condition(operator_data, condition)
        if condition.get(CommonProfRule.CONDITION_TYPE) == CommonProfRule.COND_TYPE_ACCUMULATE:
            return self.cal_accumulate_condition(operator_data, condition)
        logging.error("Not support condition type: %s ", condition.get(CommonProfRule.CONDITION_TYPE))
        return []

    def get_condition(self: any, condition_ids: list) -> list:
        """
        get conditions by ids.
        condition_ids: id for condition query.
        """
        conditions = []
        for condition_id in condition_ids:
            if self.conditions.get(condition_id):
                conditions.append(self.conditions.get(condition_id))
        return conditions


class OperatorConditionManager(MetaConditionManager):
    """
    operator condition manager
    """

    def __init__(self: any) -> None:
        super().__init__()

    @staticmethod
    def cal_count_condition(operator_data_list: dict, condition: dict) -> list:
        """
        calculate count condition
        """
        return []

    @staticmethod
    def cal_accumulate_condition(operator_data_list: dict, condition: dict) -> list:
        """
        calculate accumulate condition
        """
        return []


class NetConditionManager(MetaConditionManager):
    """
    network condition manager
    """

    def __init__(self: any) -> None:
        super().__init__()

    @classmethod
    def cal_normal_condition(cls: any, operator_data: list, condition: dict) -> list:
        """
        calculate normal condition
        """
        op_names = []
        for operator in operator_data:
            ops = super().cal_normal_condition(operator, condition)
            if ops:
                op_names.extend(ops)
        return op_names

    @classmethod
    def cal_formula_condition(cls: any, operator_data: list, condition: dict) -> list:
        """
        calculate formula condition
        """
        op_names = []
        for operator in operator_data:
            ops = super().cal_formula_condition(operator, condition)
            if ops:
                op_names.extend(ops)
        return op_names

    def cal_count_condition(self: any, operator_data_list: list, condition: dict) -> list:
        """
        calculate count condition
        """
        op_names = self.cal_condition(operator_data_list, condition.get(CommonProfRule.CONDITION_DEPENDENCY))
        cmp_mode = condition.get(CommonProfRule.CONDITION_CMP, '>')
        cpm_threshold = int(condition.get(CommonProfRule.CONDITION_THRESHOLD))
        if not self.COMPARE_MAP.get(cmp_mode)(len(op_names), cpm_threshold):
            op_names.clear()
        return op_names

    def cal_accumulate_condition(self: any, operator_data_list: list, condition: dict) -> list:
        """
        calculate accumulate condition
        """
        dependency = condition.get(CommonProfRule.CONDITION_DEPENDENCY, "")
        if dependency:
            op_names = self.cal_condition(operator_data_list, dependency)
        else:
            op_names = [''] * len(operator_data_list)
            for index, operator in enumerate(operator_data_list):
                op_names[index] = operator.get('op_name', '')
        op_names_set = set(op_names)
        compare_keys = condition.get(CommonProfRule.CONDITION_COMPARE, [])
        base = 0
        for operator in operator_data_list:
            for key in compare_keys:
                base += operator.get(key, 0)
        cmp_mode = condition.get(CommonProfRule.CONDITION_CMP, '>')
        cpm_threshold = float(condition.get(CommonProfRule.CONDITION_THRESHOLD, 0))
        keys = condition.get(CommonProfRule.CONDITION_ACCUMULATE)
        result = 0
        for operator in operator_data_list:
            if operator.get('op_name', '') not in op_names_set:
                continue
            for key in keys:
                result += operator.get(key, 0)
        if NumberConstant.is_zero(base):
            result = 0
            logging.warning("The accumulated result of comparison fields is equal to 0.")
        else:
            result = result / base
        if not self.COMPARE_MAP.get(cmp_mode)(result, cpm_threshold):
            op_names_set.clear()
        op_names = list(op_names_set)
        return op_names


def get_two_decimal(num: any) -> any:
    """
    trans number with two decimal
    """
    return round(float(num), NumberConstant.ROUND_TWO_DECIMAL) if is_number(num) else num
