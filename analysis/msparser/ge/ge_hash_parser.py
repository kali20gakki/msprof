#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import logging
import sqlite3

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.utils import Utils
from msmodel.ge.ge_hash_model import GeHashModel
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag


class GeHashParser(DataParser, MsMultiProcess):
    """
    ge info data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        DataParser.__init__(self, sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._table_list = [DBNameConstant.TABLE_GE_HASH]
        self._model = GeHashModel(self._project_path, self._table_list)
        self._ge_hash_data = []

    @staticmethod
    def _get_ge_hash_data(data_lines: any) -> list:
        result = []
        for line in data_lines:
            ge_hash_list = line.strip().split(":", 1)
            if ge_hash_list[0].isdigit():
                result.append(ge_hash_list)
        return result

    def parse(self: any) -> None:
        """
        parse ge hash data
        """
        fusion_hash_file = self._file_list.get(DataTag.GE_HASH, [])
        if fusion_hash_file:
            self._ge_hash_data = self.parse_plaintext_data(fusion_hash_file, self._get_ge_hash_data)

    def save(self: any) -> None:
        """
        save data
        """
        self._model.init()
        self._model.flush(self._ge_hash_data)
        self._model.finalize()

    def ms_run(self: any) -> None:
        """
        parse and save ge hash data
        :return:
        """
        if not self._file_list.get(DataTag.GE_HASH, []):
            return
        try:
            self.parse()
        except (OSError, IOError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        try:
            self.save()
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
