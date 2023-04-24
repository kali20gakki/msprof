#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import logging
import os
import struct

from common_func.constant import Constant
from common_func.file_manager import FileOpen
from common_func.ms_constant.level_type_constant import LevelDataType
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from framework.offset_calculator import OffsetCalculator
from msmodel.api.api_data_model import ApiDataModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.prof_enum.data_tag import DataTag
from profiling_bean.struct_info.api_data_bean import ApiDataBean


class ApiDataParser(DataParser, MsMultiProcess):
    """
    api data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._api_data = []

    def parse(self: any) -> None:
        """
        parse function
        """
        api_files = self._file_list.get(DataTag.API, [])
        api_files = self.group_aging_file(api_files)
        for file_list in api_files.values():
            offset_calculator = OffsetCalculator(file_list, struct.calcsize(StructFmt.API_FMT),
                                                 self._project_path)
            for _file in file_list:
                if not is_valid_original_data(_file, self._project_path):
                    continue
                _file_path = self.get_file_path_and_check(_file)
                logging.info(
                    "start parsing api data file: %s", _file)
                self._read_api_data(_file_path, offset_calculator)

    def save(self: any) -> None:
        """
        save data to db
        :return:
        """
        if not self._api_data:
            return
        model = ApiDataModel(self._project_path)
        with model as _model:
            _model.flush(self._api_data)

    def ms_run(self: any) -> None:
        """
        entrance for api parser
        :return:
        """
        if not self._file_list.get(DataTag.API, []):
            return
        try:
            self.parse()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        self.save()

    def _read_api_data(self: any, file_path: str, offset: OffsetCalculator) -> None:
        file_size = os.path.getsize(file_path)
        if not file_size:
            return
        struct_size = struct.calcsize(StructFmt.API_FMT)
        with FileOpen(file_path, 'rb') as _api_file:
            _all_api_data = offset.pre_process(_api_file.file_reader, file_size)
            for _index in range(file_size // struct_size):
                data = _all_api_data[_index * struct_size:(_index + 1) * struct_size]
                self.check_magic_num(data)
                self._api_data.append(ApiDataBean.decode(data))
