#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.

import logging
import os.path

from common_func.utils import Utils
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager, FileOpen
from common_func.path_manager import PathManager
from common_func.msvp_common import is_valid_original_data
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from msparser.interface.iparser import IParser
from msparser.data_struct_size_constant import StructFmt
from msmodel.msproftx.msproftx_model import MsprofTxMarkExModel
from framework.offset_calculator import OffsetCalculator
from profiling_bean.struct_info.msproftx_decoder import MsprofTxMarkExDecoder
from profiling_bean.prof_enum.data_tag import DataTag


class MsprofTxMarkExParser(IParser, MsMultiProcess):
    """
    parsing MsprofTx Markex data class
    """
    def __init__(self: any, file_dict: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_dict.get(DataTag.MSPROFTX_MARKEX, [])
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH, '')
        self._msproftx_markex_data = []

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        if not self._file_list:
            return
        self.parse()
        self.save()

    def parse(self: any) -> None:
        """
        parse dat file
        :return: None
        """
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))
        for file_name in self._file_list:
            if not is_valid_original_data(file_name, self._project_path):
                continue
            self._original_data_handler(file_name)

    def save(self: any) -> None:
        """
        save data to db
        :return: None
        """
        if not self._msproftx_markex_data:
            return
        with MsprofTxMarkExModel(self._project_path) as model:
            model.flush(self._msproftx_markex_data)

    def _original_data_handler(self: any, file_name: str) -> None:
        """
        handler for msproftx markex original data
        :param file_name: data file
        :return: None
        """
        logging.info("start parsing msproftx markex data file: %s", file_name)
        status = self._read_binary_data(file_name)
        FileManager.add_complete_file(self._project_path, file_name)
        if status:
            logging.error('Parsing msproftx markex data file error.')
        logging.info("Parsing msproftx markex data file finished!")

    def _read_binary_data(self: any, file_name: str) -> None:
        """
        parsing msproftx markex data
        :param file_name: data file
        :return: None
        """
        calculate = OffsetCalculator(self._file_list, StructFmt.MSPROFTX_MARKEX_FMT_SIZE, self._project_path)
        markex_file = PathManager.get_data_file_path(self._project_path, file_name)
        try:
            with FileOpen(markex_file, 'rb') as markex_f:
                markex_data = calculate.pre_process(markex_f.file_reader, os.path.getsize(markex_file))
                for chunk in Utils.chunks(markex_data, StructFmt.MSPROFTX_MARKEX_FMT_SIZE):
                    data_object = MsprofTxMarkExDecoder.decode(chunk)
                    self._msproftx_markex_data.append((data_object.pid, data_object.tid, data_object.timestamp,
                                                       data_object.mark_id, data_object.message))

            return NumberConstant.SUCCESS
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s: %s", file_name, err, exc_info=Constant.TRACE_BACK_SWITCH)
            return NumberConstant.ERROR
