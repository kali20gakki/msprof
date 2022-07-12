#!/usr/bin/python3
# coding: utf-8
"""
This script is used to provide methods for creating db methods
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

import logging
import os

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.os_manager import check_file_readable
from common_func.path_manager import PathManager
from common_func.utils import Utils
from framework.offset_calculator import OffsetCalculator
from msmodel.msproftx.msproftx_model import MsprofTxModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.msproftx_decoder import MsprofTxDecoder


class MsprofTxParser(IParser, MsMultiProcess):
    """
    parsing MsprofTx data class
    """
    EVENT_DICT = {
        NumberConstant.MARKER: 'marker',
        NumberConstant.PUSH_AND_POP: 'push/pop',
        NumberConstant.START_AND_END: 'start/end'
    }

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        self._file_list = file_list.get(DataTag.MSPROFTX, [])
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH, '')
        self._model = MsprofTxModel(self._project_path, DBNameConstant.DB_MSPROFTX,
                                    [DBNameConstant.TABLE_MSPROFTX])
        self._msproftx_data = []
        self._file_list.sort(key=lambda x: int(x.split("_")[-1]))

    def read_binary_data(self: any, file_name: str) -> int:
        """
        parsing msproftx data and insert into msproftx.db
        """
        calculate = OffsetCalculator(self._file_list, StructFmt.MSPROFTX_FMT_SIZE,
                                     self._project_path)
        msproftx_file = PathManager.get_data_file_path(self._project_path, file_name)
        check_file_readable(msproftx_file)
        _file_size = os.path.getsize(msproftx_file)
        try:
            with open(msproftx_file, 'rb') as msproftx_f:
                msproftx_data = calculate.pre_process(msproftx_f, _file_size)
                for chunk in Utils.chunks(msproftx_data, StructFmt.MSPROFTX_FMT_SIZE):
                    data_object = MsprofTxDecoder.decode(chunk)
                    self._msproftx_data.append(
                        data_object.id_data + (data_object.category, self.EVENT_DICT.get(data_object.event_type, ''))
                        + data_object.mix_message + (data_object.message_type, data_object.message))
                return NumberConstant.SUCCESS
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s: %s", file_name, err, exc_info=Constant.TRACE_BACK_SWITCH)
            return NumberConstant.ERROR

    def parse(self: any) -> None:
        """
        parsing data file
        """
        try:
            for file_name in self._file_list:
                if is_valid_original_data(file_name, self._project_path):
                    self._original_data_handler(file_name)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def save(self: any) -> None:
        """
        save data to db
        :return: None
        """
        if self._msproftx_data and self._model:
            self._model.init()
            self._model.flush(self._msproftx_data)
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        try:
            if self._file_list:
                self.parse()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _original_data_handler(self: any, file_name: str) -> None:
        logging.info(
            "start parsing msproftx data file: %s", file_name)
        status_ = self.read_binary_data(file_name)
        FileManager.add_complete_file(self._project_path, file_name)
        if status_:
            logging.error('Insert MsprofTx data error.')
        logging.info("Create MSPROFTX DB finished!")
