#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

import logging
import os
import sqlite3

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msprof_iteration import MsprofIteration
from common_func.path_manager import PathManager
from common_func.platform.chip_manager import ChipManager
from common_func.utils import Utils
from framework.offset_calculator import OffsetCalculator
from msmodel.iter_rec.iter_rec_model import HwtsIterModel
from msmodel.ge.ge_info_calculate_model import GeInfoModel
from msparser.interface.iparser import IParser
from msparser.iter_rec.iter_info_updater.iter_info import IterInfo
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.stars.ffts_pmu import FftsPmuBean
from profiling_bean.stars.ffts_plus_pmu import FftsPlusPmuBean


class StarsIterRecParser(IParser, MsMultiProcess):
    """
    class used to process stars iter rec
    """

    PMU_LOG_SIZE = 128
    STARS_LOG_SIZE = 64
    STARS_LOG_TYPE = 0
    PMU_LOG_TYPE = 1

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._iter_info_dict = {}
        self._current_iter_id = 0
        self._iter_end_dict = {}
        self._file_list = file_list

    @staticmethod
    def _get_log_syscnt(chunk):
        # stars data fmt syscnt from 8 to 16
        return int.from_bytes(chunk[8:16], byteorder='little', signed=False)

    @classmethod
    def _get_pmu_decoder(cls: any) -> any:
        if ChipManager().is_ffts_plus_type():
            return FftsPlusPmuBean
        return FftsPmuBean

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
            logging.warning("Iter rec parse failed, error code : %s", rec_error.code,
                            exc_info=Constant.TRACE_BACK_SWITCH)

    def parse(self: any) -> None:
        """
        parse ffts profiler data
        :return:
        """
        self._iter_end_dict = self._get_iter_end_dict()
        self._parse_pmu_data()
        self._current_iter_id = 0
        self._parse_log_file_list()

    def save(self: any) -> None:
        """
        save data into database
        :return: None
        """
        iter_info_list = self.refactor_iter_info_dict()
        try:
            with HwtsIterModel(self._project_path) as hwts_iter_model:
                hwts_iter_model.flush(iter_info_list,
                                      DBNameConstant.TABLE_HWTS_ITER_SYS)
        except sqlite3.Error as trace_err:
            logging.error("Save hwts iter failed, "
                          "%s", str(trace_err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _parse_log_file_list(self):
        log_file_list = self._file_list.get(DataTag.STARS_LOG, [])
        log_file_list.sort(key=lambda x: int(x.split("_")[-1]))
        _offset_calculator = OffsetCalculator(log_file_list, self.STARS_LOG_SIZE, self._project_path)
        for log_file in log_file_list:
            log_file = PathManager.get_data_file_path(self._project_path, log_file)
            logging.info("Begin to process stars log data file: %s", os.path.basename(log_file))
            with open(log_file, 'rb') as log_reader:
                all_bytes = _offset_calculator.pre_process(log_reader, os.path.getsize(log_file))
                self._process_log_data(all_bytes)

    def _process_log_data(self, all_bytes: bytes) -> None:
        for chunk in Utils.chunks(all_bytes, self.STARS_LOG_SIZE):
            sys_cnt = self._get_log_syscnt(chunk)
            self._set_current_iter_id(sys_cnt)
            self._update_iter_info(sys_cnt, self.STARS_LOG_TYPE)

    def _parse_pmu_data(self):
        pmu_file_list = self._file_list.get(DataTag.FFTS_PMU, [])
        pmu_file_list.sort(key=lambda x: int(x.split("_")[-1]))
        _offset_calculator = OffsetCalculator(pmu_file_list, self.PMU_LOG_SIZE, self._project_path)
        for _ffts_profile_file in pmu_file_list:
            _ffts_profile_file = PathManager.get_data_file_path(self._project_path, _ffts_profile_file)
            logging.info("Begin to process ffts profile data file: %s", os.path.basename(_ffts_profile_file))
            with open(_ffts_profile_file, 'rb') as fft_pmu_reader:
                all_bytes = _offset_calculator.pre_process(fft_pmu_reader, os.path.getsize(_ffts_profile_file))
                self._process_pmu_data(all_bytes)

    def _set_current_iter_id(self: any, sys_cnt: int) -> None:
        if self._current_iter_id == 0:
            for iter_id, end_sys_cnt in self._iter_end_dict.items():
                if sys_cnt <= end_sys_cnt:
                    self._current_iter_id = iter_id
                    return
            logging.error("Ffts Pmu cannot find the iteration to which the current chunk belongs, "
                          "please check the iteration data")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)

    def _process_pmu_data(self: any, all_bytes: bytes) -> None:
        for chunk in Utils.chunks(all_bytes, self.PMU_LOG_SIZE):
            pmu_data = FftsPmuBean.decode(chunk)
            self._set_current_iter_id(pmu_data.time_list[0])
            self._update_iter_info(pmu_data.time_list[0], self.PMU_LOG_TYPE)

    def _check_current_iter_id(self: any, sys_cnt) -> None:
        iter_end = self._iter_end_dict.get(self._current_iter_id)
        return iter_end is not None and sys_cnt > iter_end

    def _update_iter_info(self, sys_cnt: int, log_type: int):
        while self._check_current_iter_id(sys_cnt):
            self._current_iter_id += 1

        iter_info = self._iter_info_dict.setdefault(self._current_iter_id,
                                                    IterInfo(self._current_iter_id,
                                                             self._iter_end_dict.get(self._current_iter_id)))
        if log_type == self.STARS_LOG_TYPE:
            iter_info.hwts_count += 1
        else:
            iter_info.aic_count += 1

    def _get_iter_end_dict(self: any) -> dict:
        iter_dict = MsprofIteration(self._project_path).get_iteration_end_dict()
        if not iter_dict:
            logging.info("No need to calculate task count at op scene.")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)
        return iter_dict

    def refactor_iter_info_dict(self: any) -> list:
        """
        refactor iter info dict
        """
        aic_offset = 0
        hwts_offset = 0

        step_trace_data = GeInfoModel(self._project_path).get_step_trace_data()
        step_trace_data.sort(key=lambda dto: dto.step_end)

        iter_info_list = []
        for step_dto in step_trace_data:
            if step_dto.iter_id not in self._iter_info_dict:
                continue
            iter_info = self._iter_info_dict.get(step_dto.iter_id)
            iter_info_list.append([step_dto.iter_id,
                                   step_dto.model_id,
                                   step_dto.index_id,
                                   iter_info.hwts_count,
                                   hwts_offset,
                                   iter_info.aic_count,
                                   aic_offset,
                                   step_dto.step_end])
            aic_offset += iter_info.aic_count
            hwts_offset += iter_info.hwts_count
        return iter_info_list
