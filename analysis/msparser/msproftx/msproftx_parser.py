#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2024. All rights reserved.

import logging
import os

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager, FileOpen
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from common_func.utils import Utils
from framework.offset_calculator import OffsetCalculator
from msmodel.msproftx.msproftx_model import MsprofTxModel, MsprofTxExModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.msproftx_decoder import MsprofTxDecoder, MsprofTxExDecoder


class MsprofTxParser(IParser, MsMultiProcess):
    """
    parsing MsprofTx data class
    """
    EVENT_DICT = {
        NumberConstant.MARKER: 'marker',
        NumberConstant.PUSH_AND_POP: 'push/pop',
        NumberConstant.START_AND_END: 'start/end',
        NumberConstant.MARKER_EX: 'marker_ex'
    }

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = {
            DataTag.MSPROFTX: file_list.get(DataTag.MSPROFTX, []),
            DataTag.MSPROFTX_EX: file_list.get(DataTag.MSPROFTX_EX, [])
        }
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH, '')

        self._file_tag = DataTag.MSPROFTX
        self._cur_file_list = []
        self._msproftx_data = []
        self._msproftx_ex_data = []

    def parse(self: any) -> None:
        """
        parsing data file
        """
        for file_name in self._cur_file_list:
            if is_valid_original_data(file_name, self._project_path):
                self._original_data_handler(file_name)

    def save(self: any) -> None:
        """
        save data to db
        :return: None
        """
        if self._msproftx_data:
            with MsprofTxModel(self._project_path, DBNameConstant.DB_MSPROFTX,
                               [DBNameConstant.TABLE_MSPROFTX]) as tx_model:
                tx_model.flush(self._msproftx_data)
        if self._msproftx_ex_data:
            with MsprofTxExModel(self._project_path) as tx_ex_model:
                tx_ex_model.flush(self._msproftx_ex_data)

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        for file_tag, file_list in self._file_list.items():
            if file_list:
                self._file_tag = file_tag
                self._cur_file_list = sorted(file_list, key=lambda x: int(x.split("_")[-1]))
                self.parse()
        try:
            self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _read_tx_binary_data(self: any, file_name: str) -> int:
        """
        parsing msproftx data and insert into msproftx.db
        """
        calculate = OffsetCalculator(self._cur_file_list, StructFmt.MSPROFTX_FMT_SIZE,
                                     self._project_path)
        msproftx_file = PathManager.get_data_file_path(self._project_path, file_name)
        try:
            with FileOpen(msproftx_file, 'rb') as msproftx_f:
                msproftx_data = calculate.pre_process(msproftx_f.file_reader, os.path.getsize(msproftx_file))
                for chunk in Utils.chunks(msproftx_data, StructFmt.MSPROFTX_FMT_SIZE):
                    data_object = MsprofTxDecoder.decode(chunk)
                    self._msproftx_data.append((data_object.process_id, data_object.thread_id,
                                                data_object.category, self.EVENT_DICT.get(data_object.event_type, ''),
                                                data_object.payload_type, data_object.payload_value,
                                                data_object.start_time, data_object.end_time,
                                                data_object.message_type,
                                                data_object.message,
                                                self._file_tag.value))
                return NumberConstant.SUCCESS
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s: %s", file_name, err, exc_info=Constant.TRACE_BACK_SWITCH)
            return NumberConstant.ERROR

    def _read_tx_ex_binary_data(self: any, file_name: str) -> None:
        """
        parsing msproftx ex data
        :param file_name: data file
        :return: None
        """
        calculate = OffsetCalculator(self._cur_file_list, StructFmt.MSPROFTX_EX_FMT_SIZE, self._project_path)
        msproftx_ex_file = PathManager.get_data_file_path(self._project_path, file_name)
        try:
            with FileOpen(msproftx_ex_file, 'rb') as msproftx_ex_f:
                msproftx_ex_data = calculate.pre_process(msproftx_ex_f.file_reader, os.path.getsize(msproftx_ex_file))
                for chunk in Utils.chunks(msproftx_ex_data, StructFmt.MSPROFTX_EX_FMT_SIZE):
                    data_object = MsprofTxExDecoder.decode(chunk)
                    self._msproftx_ex_data.append((data_object.pid, data_object.tid,
                                                   self.EVENT_DICT.get(data_object.event_type, ''),
                                                   data_object.start_time, data_object.end_time,
                                                   data_object.mark_id, data_object.message))

            return NumberConstant.SUCCESS
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s: %s", file_name, err, exc_info=Constant.TRACE_BACK_SWITCH)
            return NumberConstant.ERROR

    def _original_data_handler(self: any, file_name: str) -> None:
        file_tag_dict = {
            DataTag.MSPROFTX: self._read_tx_binary_data,
            DataTag.MSPROFTX_EX: self._read_tx_ex_binary_data
        }
        if self._file_tag not in file_tag_dict.keys():
            logging.error("unsupported file tag to parse")
            return
        logging.info("start parsing %s data file: %s", self._file_tag.name.lower(), file_name)
        status_ = file_tag_dict[self._file_tag](file_name)
        FileManager.add_complete_file(self._project_path, file_name)
        if status_:
            logging.error("Parsing %s data file error.", self._file_tag.name.lower())
        logging.info("Parsing %s data file finished!", self._file_tag.name.lower())


