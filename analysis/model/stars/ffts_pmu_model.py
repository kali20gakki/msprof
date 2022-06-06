# coding=utf-8
"""
ffts task pmu model
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

import itertools
import logging
import sqlite3

from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.utils import Utils
from model.interface.parser_model import ParserModel
from mscalculate.aic.aic_utils import AicPmuUtils
from mscalculate.calculate_ai_core_data import CalculateAiCoreData
from viewer.calculate_rts_data import get_metrics_from_sample_config


class FftsPmuModel(ParserModel):
    """
    ffts pmu model.
    """

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)
        self.events_name_list = []
        self.aic_profiling_events = None  # ai core pmu event
        self.aiv_profiling_events = None

    def create_table(self: any) -> None:
        """
        create aic and aiv table by sample.json
        :return:
        """
        # aic metrics table
        self.aic_profiling_events = get_metrics_from_sample_config(self.result_dir)
        column_list = AicPmuUtils.remove_unused_column(self.aic_profiling_events)
        if column_list:
            self._insert_metric_value(column_list, DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY)
        # aiv metrics table
        self.aiv_profiling_events = get_metrics_from_sample_config(self.result_dir, StrConstant.AIV_PROFILING_METRICS)
        column_list = AicPmuUtils.remove_unused_column(self.aiv_profiling_events)
        if column_list:
            self._insert_metric_value(column_list, DBNameConstant.TABLE_AIV_METRIC_SUMMARY)

    def insert_task_pmu_data(self: any, pmu_data_list: list) -> None:
        """
        insert sub task pmu data to db
        """
        aic_data_list = []
        aiv_data_list = []
        aic_pmu_events = AicPmuUtils.get_pmu_events(
            ConfigMgr.read_sample_config(self.result_dir).get('ai_core_profiling_events'))
        aiv_pmu_events = AicPmuUtils.get_pmu_events(
            ConfigMgr.read_sample_config(self.result_dir).get('aiv_profiling_events'))
        for data in pmu_data_list:
            # splite data by data type
            if data.is_aic_data():
                self.calculate_pmu_list(data, aic_pmu_events, aic_data_list)
            else:
                self.calculate_pmu_list(data, aiv_pmu_events, aiv_data_list)
        if self.aic_profiling_events:
            self.insert_data_to_db(DBNameConstant.TABLE_AI_CORE_METRIC_SUMMARY, aic_data_list)
        if self.aiv_profiling_events:
            self.insert_data_to_db(DBNameConstant.TABLE_AIV_METRIC_SUMMARY, aiv_data_list)

    def calculate_pmu_list(self: any, data: any, profiling_events: list, data_list: list) -> None:
        """
        calculate pmu
        :param result_dir:
        :param data: pmu data
        :param profiling_events: pmu events list
        :param data_list: out args
        :return:
        """
        pmu_list = {}
        _, pmu_list = CalculateAiCoreData(self.result_dir).compute_ai_core_data(
            Utils.generator_to_list(profiling_events), pmu_list, data.total_cycle, data.pmu_list)

        AicPmuUtils.remove_redundant(pmu_list)
        data_list.append(
            [data.total_cycle, *list(itertools.chain.from_iterable(pmu_list.values())), data.task_id, data.stream_id,
             data.subtask_id, data.subtask_type, data.time_list[0], data.time_list[1]])

    def flush(self: any, data_list: list) -> None:
        """
        insert data into database
        :param data_list: ffts pmu data list
        :return: None
        """
        self.insert_task_pmu_data(data_list)

    def _insert_metric_value(self: any, metrics: list, table_name: str) -> None:
        """insert event value into metric op_summary"""
        sql = 'CREATE TABLE IF NOT EXISTS {name}({column})'.format(
            column=','.join(metric.replace('(ms)', '').replace('(GB/s)', '')
                            + ' numeric' for metric in metrics) + ', task_id INT, '
                                                                  'stream_id INT,'
                                                                  'ctx_id INT,'
                                                                  'task_type INT, '
                                                                  'start_time INT,'
                                                                  'end_time INT', name=table_name)
        try:
            DBManager.execute_sql(self.conn, sql)
        except sqlite3.Error as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass
