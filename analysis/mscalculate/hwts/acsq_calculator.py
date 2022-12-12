#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import logging

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msprof_iteration import MsprofIteration
from common_func.platform.chip_manager import ChipManager
from mscalculate.interface.icalculator import ICalculator
from msmodel.iter_rec.iter_rec_model import HwtsIterModel
from msmodel.stars.acsq_task_model import AcsqTaskModel
from msmodel.step_trace.ts_track_model import TsTrackModel
from profiling_bean.prof_enum.data_tag import DataTag


class AcsqCalculator(ICalculator, MsMultiProcess):
    """
    class used to calculate stars acsq data and parse log by iter
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._iter_range = sample_config.get(StrConstant.PARAM_ITER_ID)
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._file_list = file_list
        self._acsq_model = AcsqTaskModel(self._project_path, DBNameConstant.DB_ACSQ,
                                         [DBNameConstant.TABLE_ACSQ_TASK_TIME])
        self._iter_model = HwtsIterModel(self._project_path)
        self._log_data = []

    @staticmethod
    def _get_iter_id_within_iter_range(step_trace_data, timestamp):
        while step_trace_data:
            step_trace = step_trace_data[0]
            if InfoConfReader().time_from_syscnt(step_trace.step_end) < timestamp:
                step_trace_data.pop(0)
                continue
            return step_trace.index_id
        logging.warning("Task exceeds the scope of iteration, there may be something wrong.")
        raise ProfException(ProfException.PROF_INVALID_STEP_TRACE_ERROR)

    def calculate(self: any) -> None:
        """
        calculate stars acsq data
        :return: None
        """
        if ProfilingScene().is_operator():
            self._parse_all_file()
        else:
            self._parse_by_iter()

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
        try:
            if ChipManager().is_ffts_type() and self._file_list.get(DataTag.STARS_LOG):
                self.calculate()
                self.save()
        except RuntimeError as err:
            logging.error(err)

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
                       (task_data[4], self._get_iter_id_within_iter_range(step_trace_data, task_data[3]),
                        self._iter_range.model_id))
            train_data.extend(tmp)
        return train_data

    def _parse_by_iter(self: any) -> None:
        """
        Parse the specified iteration data
        :return: None
        """
        _iter_time = MsprofIteration(self._project_path).get_iter_interval(self._iter_range)
        if not _iter_time:
            return

        sql = "select task_id, stream_id, start_time, end_time, task_type" \
              " from {} where start_time >= ? and end_time <= ?".format(DBNameConstant.TABLE_ACSQ_TASK)
        cpu_sql = []
        with self._acsq_model as model:
            if self._acsq_model.check_table():
                model.drop_table(DBNameConstant.TABLE_ACSQ_TASK_TIME)
                cpu_sql = DBManager.fetch_all_data(model.cur, sql, _iter_time)
        self._log_data.extend(cpu_sql)

    def _parse_all_file(self: any) -> None:
        sql = "select task_id, stream_id, start_time, end_time, task_type" \
              " from {}".format(DBNameConstant.TABLE_ACSQ_TASK)

        cpu_sql = []
        with self._acsq_model as model:
            if model.check_table():
                model.drop_table(DBNameConstant.TABLE_ACSQ_TASK_TIME)
                cpu_sql = DBManager.fetch_all_data(model.cur, sql)
        self._log_data.extend(cpu_sql)

    def _add_batch_id(self: any, prep_data_res: list) -> list:
        res = []
        for task_data in prep_data_res:
            train_data = []
            train_data.append(task_data + (0,))
            res.extend(train_data)
        return res
