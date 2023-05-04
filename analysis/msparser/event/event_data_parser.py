#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import logging
import os
import struct

from common_func.constant import Constant
from common_func.file_manager import FileOpen
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from framework.offset_calculator import OffsetCalculator
from msmodel.event.event_data_model import EventDataModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.event_data_bean import EventDataBean


class EventDataParser(DataParser, MsMultiProcess):
    """
    event data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._event_data = []

    def parse(self: any) -> None:
        """
        parse function
        """
        event_files = self._file_list.get(DataTag.EVENT, [])
        event_files = self.group_aging_file(event_files)
        for file_list in event_files.values():
            offset_calculator = OffsetCalculator(file_list, struct.calcsize(StructFmt.EVENT_FMT),
                                                 self._project_path)
            for _file in file_list:
                if not is_valid_original_data(_file, self._project_path):
                    continue
                _file_path = self.get_file_path_and_check(_file)
                logging.info(
                    "start parsing event data file: %s", _file)
                self._read_event_data(_file_path, offset_calculator)

    def save(self: any) -> None:
        """
        save data to db
        :return:
        """
        if not self._event_data:
            return
        model = EventDataModel(self._project_path)
        with model as _model:
            _model.flush(self._event_data)

    def ms_run(self: any) -> None:
        """
        entrance for event parser
        :return:
        """
        if not self._file_list.get(DataTag.EVENT, []):
            return
        try:
            self.parse()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self.save()

    def _read_event_data(self: any, file_path: str, offset: OffsetCalculator) -> None:
        file_size = os.path.getsize(file_path)
        if not file_size:
            return
        struct_size = struct.calcsize(StructFmt.EVENT_FMT)
        with FileOpen(file_path, 'rb') as _event_file:
            _all_event_data = offset.pre_process(_event_file.file_reader, file_size)
            for _index in range(file_size // struct_size):
                data = _all_event_data[_index * struct_size:(_index + 1) * struct_size]
                self.check_magic_num(data)
                self._event_data.append(EventDataBean.decode(data))
