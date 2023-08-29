#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
import logging
 
from collections import OrderedDict
from common_func.profiling_scene import ProfilingScene
from common_func.common import generate_config
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.utils import Utils
from common_func.msvp_common import read_cpu_cfg
from mscalculate.aic.aic_utils import AicPmuUtils
from mscalculate.aic.pmu_calculator import PmuCalculator
from profiling_bean.db_dto.step_trace_dto import IterationRange
from viewer.calculate_rts_data import get_metrics_from_sample_config, get_limit_and_offset, create_metric_table


class MiniAicCalculator(PmuCalculator, MsMultiProcess):
    """
    class used to calculator aic pmu of mini
    """
    TYPE = "ai_core"

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._iter_range = sample_config.get(StrConstant.PARAM_ITER_ID)
        self.events_name_list = []
        self.conn = None
        self.cur = None

    def calculate(self: any) -> None:
        """
        calculate for ai core
        :return:
        """
        try:
            self.init()
        except ValueError:
            logging.warning("calculate subtask failed, maybe the data is not in fftsplus mode")
            return
        finally:
            self._parse_ai_core_pmu_event()
            freq = InfoConfReader().get_freq(StrConstant.AIC)
            if ProfilingScene().is_step_trace():
                self.insert_metric_summary_table(freq, iter_range=self._iter_range,
                                                 have_step_info=True)
            elif ProfilingScene().is_operator():
                self.insert_metric_summary_table(freq, iter_range=self._iter_range)
            DBManager().destroy_db_connect(self.conn, self.cur)

    def save(self: any) -> None:
        """
        save data for aic core
        :return:
        """
        self.events_name_list = []

    def ms_run(self: any) -> None:
        """
        entrance for calculate ai core
        :return: None
        """
        if not DBManager.check_tables_in_db(
                PathManager.get_db_path(self._project_path, DBNameConstant.DB_RUNTIME),
                DBNameConstant.TABLE_EVENT_COUNTER):
            logging.warning("unable to create metrics db, because it can’t find the event_counter table")
            return
        config = generate_config(PathManager.get_sample_json_path(self._project_path))
        if config.get('ai_core_profiling_mode') == StrConstant.AIC_SAMPLE_BASED_MODE:
            return
        self.calculate()

    def init(self: any) -> None:
        self.conn, self.cur = DBManager.create_connect_db(
            PathManager.get_db_path(self._project_path, DBNameConstant.DB_METRICS_SUMMARY))
        if not self.conn or not self.cur:
            raise ValueError
        if DBManager.judge_table_exist(self.cur, DBNameConstant.TABLE_METRIC_SUMMARY):
            DBManager.drop_table(self.conn, DBNameConstant.TABLE_METRIC_SUMMARY)
 
    def insert_metric_summary_table(self, freq: float, iter_range: IterationRange,
                                    have_step_info: bool = False) -> None:
        """
        insert metric summary table
        """
        metrics_datas = self.get_metric_summary_data(freq, iter_range, have_step_info)
        DBManager.insert_data_into_table(self.conn, DBNameConstant.TABLE_METRIC_SUMMARY, metrics_datas)
 
    def get_metric_summary_data(self, freq: float, iter_range: IterationRange, have_step_info: bool) -> []:
        sql = self.get_metric_summary_sql(freq, have_step_info)
        conn, curs = DBManager.check_connect_db_path(PathManager.get_db_path(self._project_path,
                                                                             DBNameConstant.DB_RUNTIME))
        if not conn or not curs or not DBManager.judge_table_exist(curs, DBNameConstant.TABLE_EVENT_COUNT):
            logging.warning("unable to get metrics data, because it can’t find the event_count table")
 
        if have_step_info:
            limit_and_offset = get_limit_and_offset(self._project_path, iter_range)
            if not limit_and_offset:
                return []
            metric_results = DBManager.fetch_all_data(curs, sql, (limit_and_offset[0], limit_and_offset[1]))
        else:
            metric_results = DBManager.fetch_all_data(curs, sql)
        if not metric_results:
            return []
        return metric_results
 
    def get_metric_summary_sql(self, freq: float, have_step_info: bool) -> "":
        metrics = get_metrics_from_sample_config(self._project_path)
        if not metrics or not create_metric_table(self.conn, metrics,
                                                       DBNameConstant.TABLE_METRIC_SUMMARY):
            return ""
 
        algos = []
        field_dict = read_cpu_cfg(MiniAicCalculator.TYPE, 'metrics')
        if field_dict is None:
            return ''
        metrics_res = []
        for metric in metrics:
            replaced_metric = metric.replace("(GB/s)", "(gb/s)")
            replaced_field = field_dict.get(replaced_metric, replaced_metric).replace("freq", str(freq))
            metrics_res.append((metric, replaced_field))
        field_dict = OrderedDict(metrics_res)
        for field in field_dict:
            algo = field_dict[field]
            algos.append(algo)
        algo_lst = Utils.generator_to_list("cast(" + algo + " as decimal(8,2)) " for algo in algos)
        sql = "SELECT " + ",".join(algo_lst) \
              + ", task_id, stream_id, '0' FROM EventCount"
        if have_step_info:
            sql = "SELECT " + ",".join(algo_lst) \
                  + ", task_id, stream_id, '0' " \
                    "FROM EventCount limit ? offset ?"
        return sql

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
