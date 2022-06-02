"""

Copyright Huawei Technologies Co., Ltd. 2020. All rights reserved.
"""

import json
import logging
import os

from common_func.common_prof_rule import CommonProfRule
from common_func.path_manager import PathManager


class BaseTuningView:
    """
    view for tuning
    """

    def __init__(self: any) -> None:
        self.data = {}
        self.turing_start = "Performance Summary Report"

    def get_tuning_data(self: any) -> None:
        """
        get turing data
        :return:
        """

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
        sub_rule_dict = {}
        for result_index, result in enumerate(data):
            rule_sub_type = result.get(CommonProfRule.RESULT_RULE_SUBTYPE, "")
            if rule_sub_type:
                sub_rule_dict.get(rule_sub_type, []).append(result)
            else:
                print("\t{0}){2}: [{1}]".format(result_index + 1,
                                                ",".join(list(map(str, result.get(CommonProfRule.RESULT_OP_LIST, [])))),
                                                result.get(CommonProfRule.RESULT_RULE_SUGGESTION, "")))
        if sub_rule_dict:
            for sub_key_index, sub_key in enumerate(sub_rule_dict.keys()):
                print("\t{0}){1}:".format(sub_key_index + 1, sub_key))
                for value_index, value in enumerate(sub_rule_dict.get(sub_key)):
                    print(
                        "\t\t{0}){2}: [{1}]".format(value_index + 1,
                                                    ",".join(value.get(CommonProfRule.RESULT_OP_LIST)),
                                                    value.get(CommonProfRule.RESULT_RULE_SUGGESTION)))

    def tuning_report(self: any) -> None:
        """
        tuning report
        :return: None
        """
        self.get_tuning_data()
        if not self.data:
            return
        print("\n{0}:".format(self.turing_start))
        for index, every_data in enumerate(self.data):
            self.print_first_level(index + 1, every_data)
            self.print_second_level(every_data.get("result"))
        print("\n")
