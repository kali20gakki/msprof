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
from common_func.path_manager import PathManager
from framework.offset_calculator import OffsetCalculator
from msmodel.runtime.rts_track_model import RtsModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.iparser import IParser
from msparser.runtime.rts_data_bean import RtsDataBean
from profiling_bean.prof_enum.data_tag import DataTag


class RtsTrackParser(IParser, MsMultiProcess):
    """
    acl data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._rts_data = []
        self._device_id = self._sample_config.get("device_id", "0")
        self._model = RtsModel(self._project_path, DBNameConstant.DB_RTS_TRACK,
                               [DBNameConstant.TABLE_TASK_TRACK])

    def parse(self: any) -> None:
        """
        parse rts data
        """
        rts_files = self._file_list.get(DataTag.RUNTIME_TRACK)
        rts_files.sort(key=lambda x: int(x.split("_")[-1]))
        try:
            for _file in rts_files:
                _file_path = PathManager.get_data_file_path(self._project_path, _file)
                _file_size = os.path.getsize(_file_path)
                if not _file_size:
                    return
                self._read_rts_file(_file_path, _file_size)
                FileManager.add_complete_file(self._project_path, _file)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def save(self: any) -> None:
        """
        save data
        """
        if self._rts_data and self._model:
            self._model.create_rts_db(self._rts_data)

    def ms_run(self: any) -> None:
        """
        parse and save runtime task track data
        :return:
        """
        if not self._file_list or not self._file_list.get(DataTag.RUNTIME_TRACK):
            return

        self.parse()
        self.save()

    def _read_rts_file(self: any, _file_path: str, _file_size: int) -> None:
        offset_calculator = OffsetCalculator(self._file_list.get(DataTag.RUNTIME_TRACK),
                                             StructFmt.RTS_TRACK_FMT_SIZE,
                                             self._project_path)
        with open(_file_path, 'rb') as _rts_file:
            _all_rts_data = offset_calculator.pre_process(_rts_file, _file_size)
            for _index in range(len(_all_rts_data) // StructFmt.RTS_TRACK_FMT_SIZE):
                rts_data_bean = RtsDataBean.decode(
                    _all_rts_data[
                    _index * StructFmt.RTS_TRACK_FMT_SIZE:(_index + 1) * StructFmt.RTS_TRACK_FMT_SIZE])

                if rts_data_bean is not None:
                    self._rts_data.append([
                        self._device_id,
                        rts_data_bean.timestamp,
                        rts_data_bean.task_type,
                        rts_data_bean.stream_id,
                        rts_data_bean.task_id,
                        rts_data_bean.thread_id,
                        rts_data_bean.batch_id
                    ])
