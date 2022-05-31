#!usr/bin/env python
# coding:utf-8
"""
Copyright Huawei Technologies Co., Ltd. 2020. All rights reserved.
"""

from mscalculate.trailing_calculator import TrailingCalculator


class ClusterTuning:
    """
    recommend for inference
    """

    def __init__(self: any) -> None:
        self.calculate_list = [TrailingCalculator]
        self.tuning_result = {}

    def run(self: any, cluster_params: list) -> None:
        """
        run and recommend
        """
        for calculator in self.calculate_list:
            self.tuning_result[calculator] = calculator(cluster_params).run()
        self.show_tuning_result()

    def show_tuning_result(self: any) -> None:
        pass
