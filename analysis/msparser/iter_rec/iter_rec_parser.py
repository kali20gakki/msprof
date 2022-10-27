#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import logging
import os
import sqlite3

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.file_manager import FileOpen
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import path_check
from common_func.path_manager import PathManager
from common_func.utils import Utils
from common_func.db_name_constant import DBNameConstant
from common_func.batch_counter import BatchCounter
from common_func.iter_recorder import IterRecorder
from common_func.msprof_exception import ProfException
from framework.offset_calculator import OffsetCalculator
from msmodel.ge.ge_info_calculate_model import GeInfoModel
from msmodel.iter_rec.iter_rec_model import HwtsIterModel
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.aic_pmu import AicPmuBean
from profiling_bean.struct_info.hwts_log import HwtsLogBean
from msparser.iter_rec.iter_info_updater.iter_info_updater import IterInfoUpdater


class IterParser(IParser, MsMultiProcess):
    HWTS_LOG_SIZE = 64
    STREAM_TASK_KEY_FMT = "{0}-{1}"
    STREAM_TASK_BATCH_KEY_FMT = "{0}-{1}-{2}"
    STATIC_ITER_ID = 0
    DEFAULT_ITER_ID = -1
    HWTS_TASK_END = 1
    AI_CORE_SIZE = 128
    DEFAULT_TASK_TIME_SIZE = 5000000

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._batch_counter = BatchCounter(self._project_path)
        self._iter_recorder = IterRecorder(self._project_path)
        self._iter_info_updater = IterInfoUpdater(self._project_path)
        self._batch_list_for_task_time = [None] * self.DEFAULT_TASK_TIME_SIZE
        self._overstep_task_cnt = 0
        self.default_index = 0
        self.hwts_iter_model = HwtsIterModel(self._project_path)
        self.ai_core_task = set()

    def save(self: any) -> None:
        """
        multiprocess to parse hwts data
        :return: None
        """
        iter_to_iter_info = self._iter_info_updater.iteration_manager.iter_to_iter_info
        try:
            if iter_to_iter_info:
                hwts_iter_data = [[iter_info.iter_id,
                                   iter_info.hwts_count,
                                   iter_info.hwts_offset,
                                   iter_info.aic_count,
                                   iter_info.aic_offset,
                                   iter_info.end_time,
                                   iter_info.hwts_count + iter_info.hwts_offset,
                                   iter_info.aic_count + iter_info.aic_offset]
                                  for iter_info in iter_to_iter_info.values()]
                self.hwts_iter_model.flush(hwts_iter_data,
                                           DBNameConstant.TABLE_HWTS_ITER_SYS)
                self.hwts_iter_model.finalize()
        except sqlite3.Error as trace_err:
            logging.error("Save hwts iter failed, "
                          "%s", str(trace_err), exc_info=Constant.TRACE_BACK_SWITCH)

    def parse(self: any) -> None:
        """
        parse hwts data by ge info and iter sys cnt
        :return: None
        """

    def ms_run(self):
        """

        :return:
        """
        pass

    def _read_hwts_data(self: any, all_bytes: bytes) -> None:
        for _chunk in Utils.chunks(all_bytes, self.HWTS_LOG_SIZE):
            _task_log = HwtsLogBean.decode(_chunk)
            if not _task_log.is_log_type():
                continue
            if self._iter_recorder.check_task_in_iteration(_task_log.sys_cnt):
                self._iter_recorder.set_current_iter_id(_task_log.sys_cnt)
                if _task_log.sys_tag == self.HWTS_TASK_END:
                    self._calculate_batch_list(_task_log)
                self._iter_info_updater.update_parallel_iter_info_pool(self._iter_recorder.current_iter_id)
                self._iter_info_updater.update_count_and_offset(_task_log, self.ai_core_task)
            else:
                self._overstep_task_cnt = self._overstep_task_cnt + 1

    def _calculate_batch_list(self: any, task_log: HwtsLogBean) -> None:
        setattr(task_log, "batch_id", self._batch_counter.calculate_batch(
            task_log.stream_id, task_log.task_id, self._iter_recorder.current_iter_id))
        if self.default_index == self.DEFAULT_TASK_TIME_SIZE:
            self.hwts_iter_model.flush(self._batch_list_for_task_time,
                                       DBNameConstant.TABLE_HWTS_BATCH)
            self._batch_list_for_task_time = [None] * self.DEFAULT_TASK_TIME_SIZE
            self.default_index = 0
        self._batch_list_for_task_time[self.default_index] = (
            task_log.stream_id, task_log.task_id, task_log.batch_id, self._iter_recorder.current_iter_id)
        self.default_index = self.default_index + 1

    def _parse_hwts_data(self: any) -> None:
        hwts_files = self._file_list.get(DataTag.HWTS, [])
        hwts_files.sort(key=lambda x: int(x.split("_")[-1]))
        _offset_calculator = OffsetCalculator(hwts_files, self.HWTS_LOG_SIZE, self._project_path)
        self.hwts_iter_model.init()
        for _hwts_file in hwts_files:
            _hwts_file = PathManager.get_data_file_path(self._project_path, _hwts_file)
            logging.info("Begin to process hwts data file: %s", os.path.basename(_hwts_file))
            with FileOpen(_hwts_file, 'rb') as _hwts_file_reader:
                all_bytes = _offset_calculator.pre_process(_hwts_file_reader.file_reader, os.path.getsize(_hwts_file))
                self._read_hwts_data(all_bytes)
        if self.default_index > 0:
            del self._batch_list_for_task_time[self.default_index:]
            self.hwts_iter_model.flush(self._batch_list_for_task_time,
                                       DBNameConstant.TABLE_HWTS_BATCH)
        if self._overstep_task_cnt > 0:
            logging.warning("HWTS overstep task number is %s", self._overstep_task_cnt)


class IterRecParser(IterParser):
    """
    this class used to parse hwts log data.
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super(IterRecParser, self).__init__(file_list, sample_config)
        self._sample_config = sample_config
        self._file_list = file_list

    def parse(self: any) -> None:
        """
        parse hwts data by ge info and iter sys cnt
        :return: None
        """
        if not path_check(PathManager.get_db_path(self._project_path, DBNameConstant.DB_GE_INFO)):
            return
        with GeInfoModel(self._project_path) as ge_info_model:
            if ge_info_model.check_table():
                self._ge_static_shape_iter_model_dict, self._ge_static_shape_model_task_dict = \
                    ge_info_model.get_ge_data(Constant.TASK_TYPE_AI_CORE, Constant.GE_STATIC_SHAPE)
                self._ge_non_static_shape_dict = ge_info_model.get_ge_data(
                    Constant.TASK_TYPE_AI_CORE, Constant.GE_NON_STATIC_SHAPE)
        if not self._ge_static_shape_iter_model_dict and not self._ge_non_static_shape_dict:
            return
        self._batch_counter.init(Constant.TASK_TYPE_AI_CORE)
        self._parse_hwts_data()

    def ms_run(self: any) -> None:
        """
        multiprocess to parse hwts data
        :return: None
        """
        try:
            if self._file_list and not ProfilingScene().is_operator():
                self.parse()
                self.save()
        except ProfException as rec_error:
            logging.warning("Iter rec parse failed, error code : %s", rec_error.code)
        finally:
            pass


class NoGeIterRecParser(IterParser):
    """
    this class used to parse hwts log data.
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super(NoGeIterRecParser, self).__init__(file_list, sample_config)
        self._file_list = file_list

    @staticmethod
    def judge_file_scene(file_dict: dict) -> bool:
        return bool(
            file_dict.get(DataTag.HWTS) and file_dict.get(DataTag.AI_CORE) and not file_dict.get(DataTag.GE_TASK))

    def parse(self: any) -> None:
        """
        parse hwts data by ge info and iter sys cnt
        :return: None
        """
        self._parse_ai_core_data()
        self._parse_hwts_data()
        if self._overstep_task_cnt > 0:
            logging.warning("overstep task number is %s", self._overstep_task_cnt)

    def ms_run(self: any) -> None:
        """
        multiprocess to parse hwts data
        :return: None
        """
        try:
            if NoGeIterRecParser.judge_file_scene(self._file_list) and not ProfilingScene().is_operator():
                self.parse()
                self.save()
        except ProfException as rec_error:
            logging.warning("Iter rec parse failed, error code : %s", rec_error.code)

    def _read_ai_core_data(self: any, all_bytes: bytes) -> None:
        for _chunk in Utils.chunks(all_bytes, self.AI_CORE_SIZE):
            _task_log = AicPmuBean.decode(_chunk)
            if _task_log:
                self.ai_core_task.add('{}-{}'.format(_task_log.stream_id, _task_log.task_id))

    def _parse_ai_core_data(self: any):
        ai_core_files = self._file_list.get(DataTag.AI_CORE, [])
        ai_core_files.sort(key=lambda x: int(x.split("_")[-1]))
        _offset_calculator = OffsetCalculator(ai_core_files, self.AI_CORE_SIZE, self._project_path)
        for ai_core_file in ai_core_files:
            ai_core_file_path = PathManager.get_data_file_path(self._project_path, ai_core_file)
            logging.info("Begin to process ai_core data file: %s with out ge data", os.path.basename(ai_core_file))
            with FileOpen(ai_core_file_path, 'rb') as _ai_core_file_reader:
                all_bytes = _offset_calculator.pre_process(
                    _ai_core_file_reader.file_reader, os.path.getsize(ai_core_file_path))
                self._read_ai_core_data(all_bytes)
