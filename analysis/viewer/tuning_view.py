"""

Copyright Huawei Technologies Co., Ltd. 2020. All rights reserved.
"""

import json
import logging
import os

from common_func.common_prof_rule import CommonProfRule
from common_func.path_manager import PathManager


class TuningView:
    """
    view for tuning
    """

    def __init__(self: any, result_dir: str, sample_config: dict) -> None:
        self.result_dir = result_dir
        self.sample_config = sample_config

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

    def show_by_dev_id(self: any, dev_id: any) -> None:
        """
        show data by device id
        :param dev_id: device id
        :return: None
        """
        self.tuning_report(dev_id)

    def tuning_report(self: any, dev_id: any) -> None:
        """
        tuning report
        :param dev_id: device id
        :return: None
        """
        load_data = self._load_result_file(dev_id)
        if not load_data or not load_data.get("data"):
            return
        print("\nPerformance Summary Report:")
        for index, every_data in enumerate(load_data.get("data")):
            self.print_first_level(index + 1, every_data)
            self.print_second_level(every_data.get("result"))
        print("\n")

    def _load_result_file(self: any, dev_id: any) -> dict:
        prof_rule_path = os.path.join(PathManager.get_summary_dir(self.result_dir),
                                      CommonProfRule.RESULT_PROF_JSON.format(dev_id))
        try:
            if os.path.exists(prof_rule_path):
                with open(prof_rule_path, "r") as rule_reader:
                    return json.load(rule_reader)
            return {}
        except FileNotFoundError:
            logging.error("Read rule file failed: %s", os.path.basename(prof_rule_path))
            return {}
        finally:
            pass
