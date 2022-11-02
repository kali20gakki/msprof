#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import itertools
import logging
import os

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.common import generate_config
from common_func.config_mgr import ConfigMgr
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_iteration import MsprofIteration
from common_func.path_manager import PathManager
from common_func.utils import Utils
from framework.offset_calculator import FileCalculator
from framework.offset_calculator import OffsetCalculator
from msmodel.aic.aic_pmu_model import AicPmuModel
from msmodel.iter_rec.iter_rec_model import HwtsIterModel
from mscalculate.aic.aic_utils import AicPmuUtils
from mscalculate.calculate_ai_core_data import CalculateAiCoreData
from mscalculate.interface.icalculator import ICalculator
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.aic_pmu import AicPmuBean


class AicCalculator(ICalculator, MsMultiProcess):
    """
    class used to parse aicore data by iter
    """
    AICORE_LOG_SIZE = 128

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._iter_model = HwtsIterModel(self._project_path)
        self._file_list = file_list.get(DataTag.AI_CORE, [])
        self._aic_data_list = []
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    def calculate(self: any) -> None:
        """
        calculate the ai core
        :return: None
        """
        if ProfilingScene().is_operator():
            self._parse_all_file()
        else:
            self._parse_by_iter()

    def calculate_pmu_list(self: any, data: any, profiling_events: list, data_list: list) -> None:
        """
        calculate pmu
        :param data: pmu data
        :param profiling_events: pmu events list
        :param data_list: out args
        :return:
        """
        pmu_list = {}
        _, pmu_list = CalculateAiCoreData(self._project_path).compute_ai_core_data(
            Utils.generator_to_list(profiling_events), pmu_list, data.total_cycle, data.pmu_list)

        AicPmuUtils.remove_redundant(pmu_list)
        data_list.append(
            [data.total_cycle, *list(itertools.chain.from_iterable(pmu_list.values())), data.task_id, data.stream_id,
             ])

    def save(self: any) -> None:
        """
        save ai core data
        :return: None
        """
        if self._aic_data_list:
            aic_pmu_model = AicPmuModel(self._project_path)
            aic_pmu_model.init()
            aic_pmu_model.flush(self._aic_data_list)
            aic_pmu_model.finalize()

    def ms_run(self: any) -> None:
        """
        entrance or ai core calculator
        :return: None
        """
        config = generate_config(PathManager.get_sample_json_path(self._project_path))
        if config.get('ai_core_profiling_mode') == 'sample-based':
            return

        if not self._file_list:
            return
        self.calculate()
        self.save()

    def _get_total_aic_count(self: any) -> int:
        sum_file_size = 0
        for file in self._file_list:
            sum_file_size += os.path.getsize(PathManager.get_data_file_path(self._project_path, file))
        return sum_file_size // self.AICORE_LOG_SIZE

    def _get_offset_and_total(self: any, model_id: int, index_id: int) -> (int, int):
        """
        :param iter_id:
        :return: offset count and total aic count
        """
        offset_count, total_count = self._iter_model.get_task_offset_and_sum(
            model_id, index_id, HwtsIterModel.AI_CORE_TYPE)
        _total_aic_count = self._get_total_aic_count()
        _sql_aic_count = self._iter_model.get_aic_sum_count()
        # get offset by all aic count and sql record count
        #offset_count = _total_aic_count - _sql_aic_count + offset_count
        if offset_count < 0:
            total_count += offset_count
            offset_count = 0
        return offset_count, total_count

    def _parse_by_iter(self: any) -> None:
        """
        Parse the specified iteration data
        :return: None
        """
        if self._iter_model.check_db() and self._iter_model.check_table():
            offset_count, total_count = self._get_offset_and_total(self._sample_config.get("model_id"),
                                                                   self._sample_config.get("iter_id"))
            if total_count <= 0:
                logging.warning("The ai core data that is not satisfied by the specified iteration!")
                return
            _file_calculator = FileCalculator(self._file_list, self.AICORE_LOG_SIZE, self._project_path,
                                              offset_count, total_count)
            self._parse(_file_calculator.prepare_process())
            self._iter_model.finalize()

    def _parse_all_file(self: any) -> None:
        _offset_calculator = OffsetCalculator(self._file_list, self.AICORE_LOG_SIZE, self._project_path)
        for _file in self._file_list:
            _file = PathManager.get_data_file_path(self._project_path, _file)
            logging.info("start parsing ai core data file: %s", os.path.basename(_file))
            with open(_file, 'rb') as _aic_reader:
                self._parse(_offset_calculator.pre_process(_aic_reader, os.path.getsize(_file)))

    def _parse(self: any, all_log_bytes: bytes) -> None:
        aic_pmu_events = AicPmuUtils.get_pmu_events(
            ConfigMgr.read_sample_config(self._project_path).get('ai_core_profiling_events'))
        for log_data in Utils.chunks(all_log_bytes, self.AICORE_LOG_SIZE):
            _aic_pmu_log = AicPmuBean.decode(log_data)
            self.calculate_pmu_list(_aic_pmu_log, aic_pmu_events, self._aic_data_list)
