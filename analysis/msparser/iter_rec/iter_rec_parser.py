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


class IterInfo:
    """
    class used to record iter info.
    """

    def __init__(self: any, iter_id: int, iter_end_sys_cnt: int) -> None:
        self.aic_count = 0
        self.task_count = 0
        self.iter_id = iter_id
        self.iter_end = iter_end_sys_cnt

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return "IterInfo"

    @staticmethod
    def file_name() -> str:
        """
        file name
        """
        return "iter_rec_parser"


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
        self._iter_info_dict = {}
        self._ge_static_shape_iter_model_dict = {}
        self._ge_static_shape_model_task_dict = {}
        self._ge_non_static_shape_dict = {}
        self._batch_list_for_task_time = [None] * self.DEFAULT_TASK_TIME_SIZE
        self._overstep_task_cnt = 0
        self.default_index = 0
        self.hwts_iter_model = HwtsIterModel(self._project_path)

    def save(self: any) -> None:
        """
        multiprocess to parse hwts data
        :return: None
        """
        try:
            if self._iter_info_dict:
                self.hwts_iter_model.flush(Utils.obj_list_to_list(self._iter_info_dict.values()),
                                           DBNameConstant.TABLE_HWTS_ITER_SYS)
                self.hwts_iter_model.finalize()
        except sqlite3.Error as trace_err:
            logging.error("Save hwts iter failed, "
                          "%s", str(trace_err), exc_info=Constant.TRACE_BACK_SWITCH)
        finally:
            pass

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

    def _calculate_task_count(self: any, task_log: HwtsLogBean) -> None:
        iter_info = self._iter_info_dict.setdefault(self._iter_recorder.current_op_iter,
                                                    IterInfo(self._iter_recorder.current_op_iter,
                                                             self._iter_recorder.iter_end_dict.get(
                                                                 self._iter_recorder.current_op_iter)))
        iter_info.task_count += 1
        if task_log.sys_tag == self.HWTS_TASK_END \
                and self._is_ai_core_task(task_log.stream_id, task_log.task_id, task_log.batch_id):
            iter_info.aic_count += 1

    def _read_hwts_data(self: any, all_bytes: bytes) -> None:
        for _chunk in Utils.chunks(all_bytes, self.HWTS_LOG_SIZE):
            _task_log = HwtsLogBean.decode(_chunk)
            if not _task_log.is_log_type():
                continue
            if self._iter_recorder.check_task_in_iteration(_task_log.sys_cnt):
                self._iter_recorder.set_current_iter_id(_task_log.sys_cnt)
                if _task_log.sys_tag == self.HWTS_TASK_END:
                    self._calculate_batch_list(_task_log)
                self._calculate_task_count(_task_log)
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
        with GeInfoModel(self._project_path) as ge_info_model:
            if ge_info_model.check_table():
                self._ge_static_shape_iter_model_dict, self._ge_static_shape_model_task_dict = \
                    ge_info_model.get_all_ge_static_shape_data(Constant.TASK_TYPE_AI_CORE)
                self._ge_non_static_shape_dict = ge_info_model.get_all_ge_non_static_shape_data(
                    Constant.TASK_TYPE_AI_CORE)
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

    def _is_ai_core_task(self: any, stream_id: int, task_id: int, batch_id: int) -> bool:
        stream_task_batch_value = self.STREAM_TASK_BATCH_KEY_FMT.format(stream_id, task_id, batch_id)
        if stream_task_batch_value in self._ge_non_static_shape_dict.get(self._iter_recorder.current_iter_id, set()):
            return True
        model_id = self._ge_static_shape_iter_model_dict.get(self._iter_recorder.current_iter_id)
        if not model_id:
            return False
        if stream_task_batch_value in self._ge_static_shape_model_task_dict.get(model_id, set()):
            return True
        return False


class NoGeIterRecParser(IterParser):
    """
    this class used to parse hwts log data.
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super(NoGeIterRecParser, self).__init__(file_list, sample_config)
        self._file_list = file_list
        self.ai_core_task = set()

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

    def _is_ai_core_task(self: any, stream_id: int, task_id: int, batch_id: int) -> bool:
        stream_task_value = self.STREAM_TASK_KEY_FMT.format(stream_id, task_id)
        if stream_task_value in self.ai_core_task:
            return True
        return False

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
