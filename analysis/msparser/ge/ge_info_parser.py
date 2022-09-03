#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import logging
import sqlite3

from common_func.constant import Constant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from msmodel.ge.ge_info_model import GeModel
from msparser.ge.ge_session_parser import GeSessionParser
from msparser.ge.ge_step_parser import GeStepParser
from msparser.ge.ge_task_parser import GeTaskParser
from msparser.ge.ge_tensor_parser import GeTensorParser
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag


class GeInfoParser(IParser, MsMultiProcess):
    """
    ge info data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._parsing_obj = [GeTaskParser, GeStepParser, GeTensorParser, GeSessionParser]
        self._model = None
        self._ge_info_data = {}
        self._table_list = []

    def parse(self: any) -> None:
        """
        parse rts data
        """
        for parse_class in self._parsing_obj:
            ge_info_data = parse_class(self._file_list, self._sample_config).ms_run()
            self._ge_info_data.update(ge_info_data)
            self._table_list.extend(list(ge_info_data.keys()))

    def save(self: any) -> None:
        """
        save data
        """
        self._model = GeModel(self._project_path, self._table_list)
        self._model.init()
        self._model.flush_all(self._ge_info_data)
        self._model.finalize()

    def ms_run(self: any) -> None:
        """
        parse and save ge info data
        :return:
        """
        if not self._file_list.get(DataTag.GE_TASK, []):
            return
        try:
            self.parse()
        except (OSError, IOError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        try:
            self.save()
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
