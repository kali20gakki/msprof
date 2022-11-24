#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_constant.str_constant import OpAnalysisType
from mscalculate.cluster.meta_calculator import MetaCalculator


class SlowRankProf:
    PROF_COMMUNICATION_ISSUE = "There is a low efficiency issue in the current cluster communication, " \
                     "ID of the slowest rank is: {}. The max wait ratio is: {:.2f} \n "
    PROF_NO_SLOW_RANK = "There is no slow rank in the current cluster communication"
    PROF_REASON_SLOW_RANK = "The reason is that the the ranks are not synchronized. " \
                            "Rank {} has more longer computation time than other ranks, " \
                            "whose synchronization time ratio is: {:.2f} "
    PROF_REASON_SLOW_LINK = "The possible reason is that the communication bandwidths between ranks are inconsistent"


class SlowRankCalculator(MetaCalculator):
    def __init__(self, data: list, op_name_list: list):
        super().__init__()
        self.data = data
        self.op_name_list = op_name_list

    def run(self):
        for rank_dict in self.data:
            self.suggestions.append(self.calculate(rank_dict))

    @staticmethod
    def calculate(rank_dict: dict) -> str:
        max_item = max(rank_dict.items(),
                       key=lambda x: x[1][StrConstant.COMMUNICATION_TIME_INFO][OpAnalysisType.WAIT_TIME_RATIO])
        rank_with_max_time_ratio = max_item[0]
        max_wait_time_ratio = max_item[1][StrConstant.COMMUNICATION_TIME_INFO][OpAnalysisType.WAIT_TIME_RATIO]
        suggestion = ''
        if max_wait_time_ratio > NumberConstant.WAIT_TIME_THRESHOLD:
            suggestion += SlowRankProf.PROF_COMMUNICATION_ISSUE.format(rank_with_max_time_ratio, max_wait_time_ratio)
            synchronization_ratio = rank_dict[rank_with_max_time_ratio][StrConstant.COMMUNICATION_TIME_INFO]\
                [OpAnalysisType.SYNCHRONIZATION_TIME_RATIO]
            if synchronization_ratio > NumberConstant.WAIT_TIME_THRESHOLD:
                suggestion += SlowRankProf.PROF_REASON_SLOW_RANK.format(rank_with_max_time_ratio, synchronization_ratio)
            else:
                suggestion += SlowRankProf.PROF_REASON_SLOW_LINK
        else:
            suggestion += SlowRankProf.PROF_NO_SLOW_RANK
        return suggestion

    def add_suggestions(self: any, op_info: dict) -> None:
        """
        add suggestion to dict
        """
        for idx, hccl_name in enumerate(self.op_name_list):
            rank_dict = op_info.get(hccl_name, {})
            rank_dict[StrConstant.SLOW_RANK_SUGGESTION] = self.suggestions[idx]






