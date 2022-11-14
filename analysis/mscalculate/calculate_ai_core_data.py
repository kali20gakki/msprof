#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.

import logging

from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.platform.chip_manager import ChipManager


class CalculateAiCoreData:
    """
    calculate aicore data
    """

    def __init__(self: any, project_path: str) -> None:
        self.project_path = project_path

    @staticmethod
    def get_vector_num() -> list:
        """
        get vector fops index coefficient
        :return: vector_num
        """
        if ChipManager().is_chip_v3():
            vector_num = [64.0, 16.0, 16.0]
        else:
            vector_num = [64.0, 64.0, 32.0]
        return vector_num

    @staticmethod
    def update_fops_data(field: str, algo: str) -> str:
        """
        update fops data in sample_based
        :return: algo
        """
        if field != "vector_fops":
            return algo
        if ChipManager().is_chip_v1():
            algo = algo.replace("r4b_num", "32.0")
            algo = algo.replace("r4e_num", "64.0")
            algo = algo.replace("r4f_num", "32.0")
        elif ChipManager().is_chip_v3():
            algo = algo.replace("r4b_num", "64.0")
            algo = algo.replace("r4e_num", "16.0")
            algo = algo.replace("r4f_num", "16.0")
        else:
            algo = algo.replace("r4b_num", "64.0")
            algo = algo.replace("r4e_num", "64.0")
            algo = algo.replace("r4f_num", "32.0")
        return algo

    @staticmethod
    def _cal_pmu_metrics(ai_core_profiling_events: dict, events_name_list: list, pmu_data: list, task_cyc: int) -> None:
        freq = InfoConfReader().get_freq(StrConstant.AIC)
        for pmu_name, pmu_value in zip(events_name_list, pmu_data):
            if task_cyc and pmu_name in list(Constant.AI_CORE_CALCULATION_FORMULA.keys()):
                if StrConstant.BANDWIDTH in pmu_name:
                    ai_core_profiling_events.setdefault(pmu_name, []).append(
                        Constant.AI_CORE_CALCULATION_FORMULA.get(pmu_name)(pmu_value, task_cyc, freq))
                else:
                    ai_core_profiling_events.setdefault(pmu_name, []).append(
                        Constant.AI_CORE_CALCULATION_FORMULA.get(pmu_name)(pmu_value, task_cyc))
            else:
                ai_core_profiling_events.setdefault(pmu_name, []).append(pmu_value)

    def add_fops_header(self: any, metric_key: str, metrics: list) -> None:
        """
        add fops index item to metrics in ArithmeticUtilization
        :param metric_key: metric key
        :param metrics: metrics
        :return: None
        """
        if metric_key == StrConstant.AI_CORE_PROFILING_METRICS:
            sample_config = ConfigMgr.read_sample_config(self.project_path)
            if sample_config.get(StrConstant.AI_CORE_PROFILING_METRICS) == "ArithmeticUtilization":
                metrics.extend(["cube_fops", "vector_fops"])

    def compute_ai_core_data(self: any, events_name_list: list, ai_core_profiling_events: dict, task_cyc: int,
                             pmu_data: list) -> tuple:
        """
        compute ai core pmu event
        :return: events_name_list, ai_core_profiling_events
        """
        try:
            self._cal_pmu_metrics(ai_core_profiling_events, events_name_list, pmu_data, task_cyc)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s", str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return events_name_list, ai_core_profiling_events
        self.__cal_addition(events_name_list, ai_core_profiling_events, task_cyc)
        return events_name_list, ai_core_profiling_events

    def add_vector_data(self: any, events_name_list: list, ai_core_profiling_events: dict, task_cyc: int) -> None:
        """
        calculate vector fops index data
        :return:
        """
        names = [
            "vec_fp16_128lane_ratio", "vec_fp16_64lane_ratio",
            "vec_fp32_ratio", "vec_int32_ratio", "vec_misc_ratio"
        ]
        if all(map(lambda name: name in events_name_list, names)):
            vector_num = self.get_vector_num()
            vec_fp16_128lane_ratio = ai_core_profiling_events["vec_fp16_128lane_ratio"][-1] * task_cyc * 128
            vec_fp16_64lane_ratio = ai_core_profiling_events["vec_fp16_64lane_ratio"][-1] * task_cyc * 64
            vec_fp32_ratio = ai_core_profiling_events["vec_fp32_ratio"][-1] * task_cyc * vector_num[0]
            vec_int32_ratio = ai_core_profiling_events["vec_int32_ratio"][-1] * task_cyc * vector_num[1]
            vec_misc_ratio = ai_core_profiling_events["vec_misc_ratio"][-1] * task_cyc * vector_num[2]
            vector_fops = vec_fp16_128lane_ratio + vec_fp16_64lane_ratio + \
                          vec_fp32_ratio + vec_int32_ratio + vec_misc_ratio
            ai_core_profiling_events.setdefault("vector_fops",
                                                []).append(vector_fops)

    def __cal_addition(self: any, events_name_list: list, ai_core_profiling_events: dict, task_cyc: int) -> None:
        """
        calculate additional ai core metrics
        :return:
        """
        names = ["vec_fp16_128lane_ratio", "vec_fp16_64lane_ratio"]
        if all(map(lambda name: name in events_name_list, names)):
            vec_fp16_ratio = ai_core_profiling_events["vec_fp16_128lane_ratio"][-1] + \
                             ai_core_profiling_events["vec_fp16_64lane_ratio"][-1]
            ai_core_profiling_events.setdefault("vec_fp16_ratio",
                                                []).append(vec_fp16_ratio)

        names = ["mac_fp16_ratio", "mac_int8_ratio"]
        if all(map(lambda name: name in events_name_list, names)):
            cube_fops = ai_core_profiling_events["mac_fp16_ratio"][-1] * task_cyc * 16 * 16 * 16 * 2 + \
                        ai_core_profiling_events["mac_int8_ratio"][-1] * task_cyc * 16 * 16 * 32 * 2
            ai_core_profiling_events.setdefault("cube_fops", []).append(cube_fops)

        names = [
            "mte1_iq_full_ratio", "mte2_iq_full_ratio", "mte3_iq_full_ratio",
            "cube_iq_full_ratio", "vec_iq_full_ratio"
        ]
        if all(map(lambda name: name in events_name_list, names)):
            iq_full_ratio = ai_core_profiling_events["mte1_iq_full_ratio"][-1] + \
                            ai_core_profiling_events["mte2_iq_full_ratio"][-1] + \
                            ai_core_profiling_events["mte3_iq_full_ratio"][-1] + \
                            ai_core_profiling_events["cube_iq_full_ratio"][-1] + \
                            ai_core_profiling_events["vec_iq_full_ratio"][-1]
            ai_core_profiling_events.setdefault("iq_full_ratio", []).append(iq_full_ratio)

        names = ["icache_miss_rate", "icache_req_ratio"]
        if all(map(lambda name: name in events_name_list, names)):
            if ai_core_profiling_events["icache_req_ratio"][-1] != 0:
                icache_miss_rate = ai_core_profiling_events["icache_miss_rate"][-1] \
                                   / ai_core_profiling_events["icache_req_ratio"][-1]

                ai_core_profiling_events.get("icache_miss_rate").pop()
                ai_core_profiling_events.setdefault("icache_miss_rate",
                                                    []).append(icache_miss_rate)
        self.add_vector_data(events_name_list, ai_core_profiling_events, task_cyc)
