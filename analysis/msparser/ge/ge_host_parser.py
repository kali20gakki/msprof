#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import logging
import os
import sqlite3

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from common_func.utils import Utils
from framework.offset_calculator import OffsetCalculator
from msmodel.ge.ge_host_parser_model import GeHostParserModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.ge_host_bean import GeHostBean


class GeHostParser(IParser, MsMultiProcess):
    """
    Ge host data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list.get(DataTag.GE_HOST, [])
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._model = GeHostParserModel(self._project_path, DBNameConstant.DB_GE_HOST_INFO,
                                        [DBNameConstant.TABLE_GE_HOST])
        self._ge_host_data = []

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        if not self._file_list:
            logging.info("No GE host sch data, exit without parser running.")
            return
        try:
            self.parse()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        try:
            self.save()
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def parse(self: any) -> None:
        """
        parsing data file
        """
        _offset_calculator = OffsetCalculator(self._file_list, StructFmt.GE_HOST_FMT_SIZE, self._project_path)
        for _ge_host_file in self._file_list:
            _ge_host_file = PathManager.get_data_file_path(self._project_path, _ge_host_file)
            logging.info("Begin to process GE host data file: %s", os.path.basename(_ge_host_file))
            with open(_ge_host_file, 'rb') as _ge_host_file_reader:
                all_bytes = _offset_calculator.pre_process(_ge_host_file_reader, os.path.getsize(_ge_host_file))
                self._read_binary_data(all_bytes)

    def save(self: any) -> None:
        """
        save data to db
        :return: None
        """
        if self._ge_host_data:
            self._model.init()
            self._model.flush(self._ge_host_data)
            self._model.finalize()

    def _read_binary_data(self: any, all_bytes: bytes) -> None:
        for _chunk in Utils.chunks(all_bytes, StructFmt.GE_HOST_FMT_SIZE):
            ge_host_data = GeHostBean.decode(_chunk)
            self._ge_host_data.append([
                ge_host_data.thread_id,
                ge_host_data.op_type,
                ge_host_data.event_type,
                ge_host_data.start_time,
                ge_host_data.end_time,
                ge_host_data.op_name])
