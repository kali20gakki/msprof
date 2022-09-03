#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import logging
import os

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.os_manager import check_file_readable
from common_func.path_manager import PathManager
from framework.offset_calculator import OffsetCalculator
from msmodel.runtime.runtime_api_model import RuntimeApiModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.iparser import IParser
from msparser.runtime.runtime_api_bean import RunTimeApiBean
from profiling_bean.prof_enum.data_tag import DataTag


class RunTimeApiParser(IParser, MsMultiProcess):
    """
    class used to parse runtime api data
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = self._sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._model = RuntimeApiModel(self._project_path, DBNameConstant.DB_RUNTIME, [DBNameConstant.TABLE_API_CALL])
        self._data_list = []

    def parse(self: any) -> None:
        """
        to read runtime_api data
        :return: None
        """
        runtime_api_files = self._file_list.get(DataTag.RUNTIME_API)
        runtime_api_files.sort(key=lambda x: int(x.split("_")[-1]))
        try:
            for _file in runtime_api_files:
                _file_path = PathManager.get_data_file_path(self._project_path, _file)
                _file_size = os.path.getsize(_file_path)
                self.read_binary_data(_file_path, _file_size)
                FileManager.add_complete_file(self._project_path, _file)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def read_binary_data(self: any, file_path: str, file_size: int) -> None:
        """
        read binary data an decode
        :param file_path:
        :param file_size:
        :return:
        """
        check_file_readable(file_path)
        offset_calculator = OffsetCalculator(self._file_list.get(DataTag.RUNTIME_API),
                                             StructFmt.RUNTIME_API_FMT_SIZE, self._project_path)
        try:
            with open(file_path, 'rb') as api_f:
                api_call_data = offset_calculator.pre_process(api_f, file_size)
                struct_nums = file_size // StructFmt.RUNTIME_API_FMT_SIZE

                for _index in range(struct_nums):
                    api_call_bean = RunTimeApiBean.decode(api_call_data[
                        _index * StructFmt.RUNTIME_API_FMT_SIZE:(_index + 1) * StructFmt.RUNTIME_API_FMT_SIZE])
                    if api_call_bean:
                        self._data_list.append(
                            api_call_bean.time_info +
                            (api_call_bean.api,
                             api_call_bean.ret_code,
                             api_call_bean.thread,) +
                            api_call_bean.task_tag +
                            (api_call_bean.data_size, api_call_bean.copy_mes)
                        )
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error("%s: %s", os.path.basename(file_path), err, exc_info=Constant.TRACE_BACK_SWITCH)
            return

    def save(self: any) -> None:
        """
        save parser data to db
        :return: None
        """
        if self._data_list:
            self._model.init()
            self._model.flush(self._data_list)
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        parse runtime_api data and save it to db.
        :return:None
        """
        if self._file_list.get(DataTag.RUNTIME_API, []):
            self.parse()
            self.save()
