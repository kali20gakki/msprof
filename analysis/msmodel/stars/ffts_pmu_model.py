#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

import itertools
import logging
import sqlite3

from common_func.config_mgr import ConfigMgr
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from mscalculate.aic.aic_utils import AicPmuUtils
from mscalculate.calculate_ai_core_data import CalculateAiCoreData
from msmodel.interface.parser_model import ParserModel
from viewer.calculate_rts_data import get_metrics_from_sample_config


class PmuMetrics:
    """
    Pmu metrics for ai core and ai vector core.
    """

    def __init__(self, pmu_dict: dict):
        self.init(pmu_dict)

    def init(self, pmu_dict: dict):
        """
        construct ffts pmu data.
        """
        for pmu_name, pmu_value in pmu_dict.items():
            setattr(self, pmu_name, pmu_value)

    def get_pmu_by_event_name(self, event_name_list):
        """
        get pmu value list order by pmu event name list.
        """
        AicPmuUtils.remove_redundant(event_name_list)
        return [getattr(self, event_name, 0) for event_name in event_name_list if hasattr(self, event_name)]


class FftsPmuManager:

    PMU_LENGTH = 8

    def __init__(self, result_dir):
        self._result_dir = result_dir
        self._sample_config = ConfigMgr.read_sample_config(result_dir)
        self._aic_pmu_events = AicPmuUtils.get_pmu_events(self._sample_config.get(StrConstant.AI_CORE_PMU_EVENTS))
        self._aiv_pmu_events = AicPmuUtils.get_pmu_events(
            self._sample_config.get(StrConstant.AI_VECTOR_CORE_PMU_EVENTS))
        self.block_dict = {}
        self.mix_pmu_dict = {}

    @staticmethod
    def _get_total_cycle_and_pmu_data(data: any, is_true: bool):
        """
        default value for pmu cycle list can be set to zero.
        """
        return (data.total_cycle, data.pmu_list) if is_true else (0, [0] * 8)

    @staticmethod
    def __get_mix_type(data: any) -> str:
        """
        get mix type
        """
        mix_type = ''
        if data.ffts_type == 4 and data.subtask_type == 6:
            mix_type = Constant.TASK_TYPE_MIX_AIC
        if data.ffts_type == 4 and data.subtask_type == 7:
            mix_type = Constant.TASK_TYPE_MIX_AIV
        return mix_type

    def calculate_pmu_list(self: any, data: any, pmu_data_list: list) -> None:
        """
        calculate pmu
        :param data: pmu data
        :param pmu_data_list: out args
        :return:
        """
        task_type = 0 if data.is_aic_data() else 1

        aic_pmu_value, aiv_pmu_value, aic_total_cycle, aiv_total_cycle = self.get_total_cycle_info(data, task_type)

        pmu_data = [aic_total_cycle, *list(itertools.chain.from_iterable(aic_pmu_value)), aiv_total_cycle,
                    *list(itertools.chain.from_iterable(aiv_pmu_value)),
                    data.task_id, data.stream_id, data.subtask_id, data.subtask_type,
                    InfoConfReader().time_from_syscnt(data.time_list[0]),
                    InfoConfReader().time_from_syscnt(data.time_list[1]), task_type, data.ffts_type]
        pmu_data_list.append(pmu_data)

    def get_total_cycle_info(self, data: any, task_type: int) -> tuple:
        """
        add block mix pmu to context pmu
        :return: tuple
        """
        aic_pmu_value = self._get_pmu_value(*self._get_total_cycle_and_pmu_data(data, data.is_aic_data()),
                                            self._aic_pmu_events)
        aiv_pmu_value = self._get_pmu_value(*self._get_total_cycle_and_pmu_data(data, not data.is_aic_data()),
                                            self._aiv_pmu_events)
        data_key = "{0}-{1}-{2}".format(data.task_id, data.stream_id, data.subtask_id)
        mix_pmu_info = self.mix_pmu_dict.get(data_key, {})
        aic_total_cycle = data.total_cycle if not task_type else 0
        mix_total_cycle = data.total_cycle if task_type else 0
        if mix_pmu_info:
            mix_total_cycle = mix_pmu_info.get('total_cycle')
            mix_data_type = mix_pmu_info.get('mix_type')
            if mix_data_type == 'mix_aiv':
                aic_pmu_value = mix_pmu_info.get('pmu')
                aic_total_cycle, mix_total_cycle = mix_total_cycle, aic_total_cycle
            else:
                aiv_pmu_value = mix_pmu_info.get('pmu')
        res_tuple = aic_pmu_value, aiv_pmu_value, aic_total_cycle, mix_total_cycle
        return res_tuple

    def calculate_block_pmu_list(self: any, data: any) -> None:
        """
        assortment mix pmu data
        :return: None
        """
        task_key = "{0}-{1}-{2}".format(data.task_id, data.stream_id, data.subtask_id)
        aic_pmu_value = self._get_total_cycle_and_pmu_data(data, data.is_aic_data())
        aiv_pmu_value = self._get_total_cycle_and_pmu_data(data, not data.is_aic_data())
        mix_type = self.__get_mix_type(data)
        if not mix_type:
            return
        pmu_list = aic_pmu_value if mix_type == Constant.TASK_TYPE_MIX_AIC else aiv_pmu_value
        pmu_data = [
            mix_type,
            pmu_list
        ]
        self.block_dict.setdefault(task_key, []).append(pmu_data)

    def add_block_pmu_list(self):
        """
        add block log pmu which have same key: task_id-stream_id-subtask_id
        demo input: {'1-1-1': [['mix_aic', (100, [1,1,2,2,3,3,4,4])], ['mix_aic', (100, [4,4,3,3,2,2,1,1])]]}
            output: {'1-1-1': [['mix_aic', 200, [5,5,5,5,5,5,5,5]}
        """
        if not self.block_dict:
            return
        for key, value in self.block_dict.items():
            mix_type = tuple(set([mix_type[0] for mix_type in value]))
            # One mix operator has only one mix_type.
            if len(mix_type) != 1:
                logging.debug('Pmu data type error, task key: task_id-stream_id-subtask_id: %s', key)
                continue
            total_cycle = sum([cycle[1][0] for cycle in value])
            cycle_list = [sum(cycle[1][1][index] for cycle in value) for index in range(self.PMU_LENGTH)]
            if mix_type[0] == Constant.TASK_TYPE_MIX_AIC:
                pmu = self._get_pmu_value(total_cycle, cycle_list,
                                          self._aiv_pmu_events)
            elif mix_type[0] == Constant.TASK_TYPE_MIX_AIV:
                pmu = self._get_pmu_value(total_cycle, cycle_list,
                                          self._aic_pmu_events)
            else:
                logging.debug('Mix type error, the key: task_id-stream_id-subtask_id: %s', key)
                pmu = cycle_list
            self.mix_pmu_dict[key] = {
                'mix_type': mix_type[0],
                'total_cycle': total_cycle,
                'pmu': pmu
            }

    def _get_pmu_value(self, total_cycle, pmu_list, pmu_metrics):
        _, pmu_list = CalculateAiCoreData(self._result_dir).compute_ai_core_data(pmu_metrics, {}, total_cycle, pmu_list)
        return PmuMetrics(pmu_list).get_pmu_by_event_name(pmu_list)


class FftsPmuModel(ParserModel):
    """
    ffts pmu model.
    """

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        super().__init__(result_dir, db_name, table_list)
        self.events_name_list = []
        self.profiling_events = {'aic': [], 'aiv': []}

    def create_table(self: any) -> None:
        """
        create aic and aiv table by sample.json
        :return:
        """
        # aic metrics table
        column_list = []
        self.profiling_events['aiv'] = get_metrics_from_sample_config(self.result_dir,
                                                                      StrConstant.AIV_PROFILING_METRICS)
        self.profiling_events['aic'] = get_metrics_from_sample_config(self.result_dir)
        self.update_pmu_list(column_list)
        self._insert_metric_value(column_list, DBNameConstant.TABLE_METRICS_SUMMARY)

    def update_pmu_list(self: any, column_list: list) -> None:
        for core_type, pmu_list in self.profiling_events.items():
            core_column_list = AicPmuUtils.remove_unused_column(pmu_list)
            for column in core_column_list:
                column_list.append('{0}_{1}'.format(core_type, column))

    def flush(self: any, data_dict: dict) -> None:
        """
        insert data into database
        :param data_dict: ffts pmu data list
        :return: None
        """
        if not data_dict:
            logging.warning("ffts pmu data is empty, no data found.")
            return

        pmu_data = []
        ffts_pmu_manager = FftsPmuManager(self.result_dir)
        for data in data_dict.get(StrConstant.BLOCK_PMU_TYPE, []):
            ffts_pmu_manager.calculate_block_pmu_list(data)
        ffts_pmu_manager.add_block_pmu_list()
        for data in data_dict.get(StrConstant.CONTEXT_PMU_TYPE, []):
            ffts_pmu_manager.calculate_pmu_list(data, pmu_data)
        pmu_data.sort(key=lambda x: x[-3])
        self.insert_data_to_db(DBNameConstant.TABLE_METRICS_SUMMARY, pmu_data)

    def _insert_metric_value(self: any, metrics: list, table_name: str) -> None:
        """
        insert event value into metric op_summary
        """
        sql = 'CREATE TABLE IF NOT EXISTS {name}({column})'.format(
            column=','.join(metric.replace('(ms)', '').replace('(GB/s)', '')
                            + ' numeric' for metric in metrics) + ', task_id INT, '
                                                                  'stream_id INT,'
                                                                  'subtask_id INT,'
                                                                  'task_type INT, '
                                                                  'start_time INT,'
                                                                  'end_time INT, '
                                                                  'core_type INT,'
                                                                  'ffts_type INT', name=table_name)
        try:
            DBManager.execute_sql(self.conn, sql)
        except sqlite3.Error as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
