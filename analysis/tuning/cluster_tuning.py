#!usr/bin/env python
# coding:utf-8
"""
Copyright Huawei Technologies Co., Ltd. 2020. All rights reserved.
"""
from common_func.common_prof_rule import CommonProfRule
from mscalculate.trailing_calculator import TrailingCalculator
from tuning.base_turing_view import BaseTuningView


class ClusterTuning(BaseTuningView):
    """
    recommend for inference
    """

    def __init__(self: any, cluster_params: list) -> None:
        super().__init__()
        self.cluster_params = cluster_params
        self.calculate_list = {TrailingCalculator: 'slow node, threshold value 20%'}
        self.data = []
        self.turing_start = "Cluster Tuning Report"

    @staticmethod
    def print_second_level(data: any) -> None:
        """
        :param data: data
        :return: None
        """
        if not data:
            print("\tN/A")
            return
        for result_index, result in enumerate(data.items()):
            print("\t{0}) {1}: \n\t {2}".format(result_index + 1,
                                                result[0],
                                                "\n".join(list(map(str, result[1])))
                                                ))

    def run(self: any) -> None:
        """
        run and recommend
        """
        self.tuning_report()

    def get_tuning_data(self: any) -> None:
        """
        get turing data
        :return:
        """
        for calculator, value in self.calculate_list.items():
            calculator_result = {
                CommonProfRule.RESULT_RULE_TYPE: value, 'result': calculator(self.cluster_params).run()}
            self.data.append(calculator_result)
