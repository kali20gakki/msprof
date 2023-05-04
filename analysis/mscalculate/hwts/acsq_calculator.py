#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import logging

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.batch_counter import BatchCounter
from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_iteration import MsprofIteration
from common_func.path_manager import PathManager
from mscalculate.interface.icalculator import ICalculator
from mscalculate.ts_task.ai_cpu.aicpu_from_ts_collector import AICpuFromTsCollector
from msmodel.iter_rec.iter_rec_model import HwtsIterModel
from msmodel.stars.acsq_task_model import AcsqTaskModel
from msmodel.step_trace.ts_track_model import TsTrackModel
from profiling_bean.prof_enum.data_tag import DataTag


class AcsqCalculator(ICalculator, MsMultiProcess):
    """
    class used to calculate stars acsq data and parse log by iter
    """
    PREP_DATA_LENGTH = 7

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._iter_range = sample_config.get(StrConstant.PARAM_ITER_ID)
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._file_list = file_list
        self._aicpu_collector = AICpuFromTsCollector(self._project_path)
        self._acsq_model = AcsqTaskModel(self._project_path, DBNameConstant.DB_SOC_LOG,
                                         [DBNameConstant.TABLE_ACSQ_TASK_TIME, DBNameConstant.TABLE_ACSQ_TASK])
        self._iter_model = HwtsIterModel(self._project_path)
        self._log_data = []

    def calculate(self: any) -> None:
        """
        calculate stars acsq data
        :return: None
        """
        self._get_data_from_task_table()

    def save(self: any) -> None:
        """
        save acsq data
        :return: None
        """
        if self._log_data:
            self._acsq_model.init()
            self._acsq_model.flush_task_time(self._add_batch_id(self._prep_data()))
            self._acsq_model.finalize()

    def ms_run(self: any) -> None:
        """
        entrance for calculating acsq
        :return: None
        """
        if self._file_list.get(DataTag.STARS_LOG):
            self.calculate()
            self.save()

    def _prep_data(self: any) -> list:
        """
        prepare data for tasktime table
        :return:
        """
        train_data = []
        self._log_data.sort(key=lambda x: x[2])
        with TsTrackModel(self._project_path, DBNameConstant.DB_STEP_TRACE,
                          [DBNameConstant.TABLE_STEP_TRACE_DATA]) as _trace:
            step_trace_data = _trace.get_step_end_list_with_iter_range(self._iter_range)
        for task_data in self._log_data:
            tmp = []
            dur_time = task_data[3] - task_data[2]
            tmp.append(task_data[0:3] + (dur_time,) +
                       (task_data[4],
                        MsprofIteration(self._project_path).get_iter_id_within_iter_range(step_trace_data, task_data[3],
                                                                                          self._iter_range),
                        self._iter_range.model_id))
            train_data.extend(tmp)
        return train_data

    def _get_data_from_task_table(self: any) -> None:
        if not DBManager.check_tables_in_db(PathManager.get_db_path(self._project_path, DBNameConstant.DB_SOC_LOG),
                                            DBNameConstant.TABLE_ACSQ_TASK):
            logging.warning("No task data collected, the data may be missing or the current iteration has no task.")
            return
        sql = "select task_id, stream_id, start_time, end_time, task_type" \
              " from {}".format(DBNameConstant.TABLE_ACSQ_TASK)

        with self._acsq_model as model:
            acsq_data = DBManager.fetch_all_data(model.cur, sql)
        self._log_data.extend(acsq_data)

    def _add_batch_id(self: any, prep_data_res: list) -> list:
        if ProfilingScene().is_operator():
            current_iter_id = NumberConstant.INVALID_ITER_ID
        else:
            iter_range = MsprofIteration(self._project_path).get_parallel_iter_range(self._iter_range)
            current_iter_id = min(iter_range)

        batch_counter = BatchCounter(self._project_path)
        batch_counter.init(Constant.TASK_TYPE_AI_CORE)
        res_data = []
        for datum in prep_data_res:
            if len(datum) != self.PREP_DATA_LENGTH:
                return []
            stream_id = datum[1]
            task_id = datum[0]
            if datum[4] == Constant.TASK_TYPE_AI_CPU:
                self._aicpu_collector.filter_aicpu_for_stars(datum, current_iter_id)
                continue
            batch_id = batch_counter.calculate_batch(stream_id, task_id, current_iter_id)
            res_data.append(datum + (batch_id,))
        return res_data + self._aicpu_collector.aicpu_list
