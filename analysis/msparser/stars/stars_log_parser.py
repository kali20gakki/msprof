#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

import os

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.file_manager import FileOpen
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.utils import Utils
from framework.offset_calculator import FileCalculator, OffsetCalculator
from msmodel.iter_rec.iter_rec_model import HwtsIterModel
from mscalculate.interface.icalculator import ICalculator
from msparser.stars.parser_dispatcher import ParserDispatcher
from profiling_bean.prof_enum.data_tag import DataTag


class StarsLogCalCulator(ICalculator, MsMultiProcess):
    """
    to read and parse stars data
    """

    DEFAULT_FMT_SIZE = 64

    STRUCT_SIZE_MAP = {
        '101000': 128,
        '101001': 128
    }

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._parser_dispatcher = None
        self._file_list = file_list.get(DataTag.STARS_LOG, [])
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    def ms_run(self: any) -> None:
        """
        parser and save stars_log data
        :return: NA
        """
        if not self._file_list:
            return
        self.calculate()
        self.save()

    def calculate(self: any) -> None:
        """
        parse stars soc log buffer
        :return: NA
        """
        self.init_dispatcher()
        if ProfilingScene().is_step_trace():
            self._parse_by_iter()
        else:
            self._parse_all_file()

    def save(self: any) -> None:
        """
        save data to db
        :return: NA
        """
        self._parser_dispatcher.flush_all_parser()

    def init_dispatcher(self: any) -> None:
        """
        init stars parser dispatcher
        :return: NA
        """
        self._parser_dispatcher = ParserDispatcher(self._project_path)
        self._parser_dispatcher.init()

    def _parse_all_file(self):
        offset_calculator = OffsetCalculator(self._file_list, self.DEFAULT_FMT_SIZE, self._project_path)
        for _file in self._file_list:
            file_name = PathManager.get_data_file_path(self._project_path, _file)
            with FileOpen(file_name, 'rb') as file_reader:
                file_data = offset_calculator.pre_process(file_reader.file_reader, os.path.getsize(file_name))
                self._parse_data(file_data)

    def _parse_by_iter(self):
        with HwtsIterModel(self._project_path) as iter_model:
            offset_count, total_count = iter_model.get_task_offset_and_sum(
                self._sample_config.get("iter-id"), 'task_count')
            if not total_count:
                return
            _file_calculator = FileCalculator(self._file_list, self.DEFAULT_FMT_SIZE, self._project_path,
                                              offset_count, total_count)
            self._parse_data(_file_calculator.prepare_process())

    def _parse_data(self: any, all_log_bytes: bytes) -> None:
        for chunk in Utils.chunks(all_log_bytes, self.DEFAULT_FMT_SIZE):
            header = int.from_bytes(chunk[0:1], byteorder='little', signed=False)
            func_type = Utils.get_func_type(header)
            self._parser_dispatcher.dispatch(func_type, chunk)
