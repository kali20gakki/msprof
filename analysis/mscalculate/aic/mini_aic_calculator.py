#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.common import generate_config
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.utils import Utils
from mscalculate.aic.aic_utils import AicPmuUtils
from mscalculate.aic.pmu_calculator import PmuCalculator
from viewer.calculate_rts_data import insert_metric_summary_table


class MiniAicCalculator(PmuCalculator, MsMultiProcess):
    """
    class used to calculator aic pmu of mini
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._iter_range = sample_config.get(StrConstant.PARAM_ITER_ID)
        self.events_name_list = []

    def calculate(self: any) -> None:
        """
        calculate for ai core
        :return:
        """
        if ProfilingScene().is_step_trace():
            self.insert_metric_summary()
        elif ProfilingScene().is_operator():
            self.op_insert_metric_summary()

    def save(self: any) -> None:
        """
        save data for aic core
        :return:
        """
        self.events_name_list = []

    def insert_metric_summary(self: any) -> None:
        """
        insert ai core metric summary table
        :return: None
        """
        self._parse_ai_core_pmu_event()
        freq = InfoConfReader().get_freq(StrConstant.AIC)
        insert_metric_summary_table(self._project_path, freq, iter_range=self._iter_range, have_step_info=True)

    def op_insert_metric_summary(self: any) -> None:
        """
        insert ai core metric summary table
        :return: None
        """
        if DBManager.check_tables_in_db(PathManager.get_db_path(self._project_path, DBNameConstant.DB_RUNTIME),
                                        DBNameConstant.TABLE_METRICS_SUMMARY):
            return
        self._parse_ai_core_pmu_event()
        freq = InfoConfReader().get_freq(StrConstant.AIC)
        insert_metric_summary_table(self._project_path, freq, iter_range=self._iter_range)

    def ms_run(self: any) -> None:
        """
        entrance for calculate ai core
        :return: None
        """
        config = generate_config(PathManager.get_sample_json_path(self._project_path))
        if config.get('ai_core_profiling_mode') == StrConstant.AIC_SAMPLE_BASED_MODE:
            return
        self.calculate()

    def _parse_ai_core_pmu_event(self: any) -> None:
        """
        get ai_core_profiling_events from sample.json
        :return:None
        """
        config = generate_config(PathManager.get_sample_json_path(self._project_path))
        ai_core_profiling_events = config.get('ai_core_profiling_events', '')
        if ai_core_profiling_events:
            self.events_name_list = Utils.generator_to_list(AicPmuUtils.get_pmu_event_name(pmu_event)
                                                            for pmu_event in ai_core_profiling_events.split(","))
