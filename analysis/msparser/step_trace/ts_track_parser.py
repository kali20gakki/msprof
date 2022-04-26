# -*-coding:utf-8 -*-
"""
This script is used to create database for runtime data
Copyright Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.
"""

import logging
import os
import struct

from common_func.common import CommonConstant
from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msvp_common import is_valid_original_data
from common_func.utils import Utils
from framework.offset_calculator import OffsetCalculator
from model.step_trace.ts_track_model import TsTrackModel
from mscalculate.step_trace.create_step_table import StepTableBuilder
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from msparser.step_trace.helper.model_with_q_parser import ModelWithQParser
from msparser.step_trace.ts_binary_data_reader.step_trace_reader import StepTraceReader
from msparser.step_trace.ts_binary_data_reader.ts_memcpy_reader import TsMemcpyReader
from msparser.step_trace.ts_binary_data_reader.task_type_reader import TaskTypeReader
from profiling_bean.prof_enum.data_tag import DataTag


class TstrackParser(DataParser, MsMultiProcess):
    """
    parsing ts track data
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        DataParser.__init__(self, sample_config)
        self.sample_config = sample_config
        self.project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH, [])
        self._file_list = file_list
        self._model = TsTrackModel(self.project_path,
                                   DBNameConstant.DB_STEP_TRACE,
                                   [DBNameConstant.TABLE_STEP_TRACE,
                                    DBNameConstant.TABLE_TS_MEMCPY,
                                    DBNameConstant.TABLE_MODEL_WITH_Q,
                                    DBNameConstant.TABLE_TASK_TYPE])
        self.tag_reader = {
            CommonConstant.STEP_TRACE_TAG: StepTraceReader(),
            CommonConstant.TS_MEMCPY_TAG: TsMemcpyReader(),
            CommonConstant.MODEL_WITH_Q_TAG: ModelWithQParser(),
            CommonConstant.TS_TASK_TYPE_TAG: TaskTypeReader()
        }

    def parse_binary_data(self: any, file_list: list, format_size: int, tag_fmt: str) -> None:
        """
        parsing binary data
        """
        _offset_calculator = OffsetCalculator(file_list, format_size, self.project_path)
        file_list.sort(key=lambda x: int(x.split("_")[-1]))
        for file_name in file_list:
            if not is_valid_original_data(file_name, self.project_path):
                continue
            logging.info("start parsing rts data file: %s", file_name)
            FileManager.add_complete_file(self.project_path, file_name)
            file_path = self.get_file_path_and_check(file_name)
            file_size = os.path.getsize(file_path)
            with FileOpen(file_path, 'rb') as file:
                all_log_bytes = _offset_calculator.pre_process(file.file_reader, file_size)
                for bean_data in Utils.chunks(all_log_bytes, format_size):
                    _, tag = struct.unpack_from(tag_fmt, bean_data)
                    reader = self.tag_reader.get(tag)
                    if reader:
                        reader.read_binary_data(bean_data)

    def parse(self: any) -> None:
        """
        start parsing the data
        """
        ts_track_file = sorted(self._file_list.get(DataTag.TS_TRACK, []))
        helper_file = sorted(self._file_list.get(DataTag.HELPER_MODEL_WITH_Q, []))
        self.parse_binary_data(ts_track_file, StructFmt.STEP_TRACE_FMT_SIZE, StructFmt.STEP_HEADER_FMT)
        self.parse_binary_data(helper_file, StructFmt.HELPER_MODEL_WITH_Q_FMT_SIZE, StructFmt.HELPER_HEADER_FMT)

    def save(self: any) -> None:
        """
        save ts track data
        """
        if any(reader.data for reader in self.tag_reader.values()):
            self._model.init()
            for reader in self.tag_reader.values():
                if reader.data:
                    self._model.create_table(reader.table_name)
                    self._model.flush(reader.table_name, reader.data)
            self._model.finalize()

    def parse_and_save(self: any) -> None:
        self.parse()
        self.save()
        StepTableBuilder.run(self.sample_config)

    def ms_run(self: any) -> None:
        """
        run function
        """
        if not self._file_list:
            return
        try:
            self.parse_and_save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
