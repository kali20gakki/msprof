#!usr/bin/env python
# coding:utf-8
"""
Copyright Huawei Technologies Co., Ltd. 2020. All rights reserved.
"""
from common_func.common_prof_rule import CommonProfRule
from mscalculate.trailing_calculator import TrailingCalculator


class ClusterTuning:
    """
    recommend for inference
    """

    def __init__(self: any) -> None:
        self.calculate_list = {TrailingCalculator: 'Slow node'}
        self.tuning_result = []

    @staticmethod
    def print_first_level(index: any, data: dict) -> None:
        """
        print title of first level
        :param index: index
        :param data: data
        :return: None
        """
        if data and data.get(CommonProfRule.RESULT_RULE_TYPE):
            print("{0}. {1}:".format(index, data.get(CommonProfRule.RESULT_RULE_TYPE)))

    @staticmethod
    def print_second_level(data: any) -> None:
        """
        :param data: data
        :return: None
        """
        if not data:
            print("\tN/A")
            return
        for result_index, result in enumerate(data):
            print("\t{0}){1}:".format(result_index + 1, result))

    def run(self: any, cluster_params: list) -> None:
        """
        run and recommend
        """
        for calculator, value in self.calculate_list.items():
            calculator_result = {'rule_type': value, 'result': calculator(cluster_params).ms_run()}
            self.tuning_result.append(calculator_result)
        self.show_tuning_result()

    def show_tuning_result(self: any) -> None:
        print("\nThe current data is identified as cluster data.")
        print("Cluster Tuning Report:")
        for index, every_data in enumerate(self.tuning_result):
            self.print_first_level(index + 1, every_data)
            self.print_second_level(every_data.get("result"))
        print("\n")
