#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.

import logging
import os

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.file_manager import FileManager
from common_func.file_manager import FileOpen
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from common_func.msvp_common import is_valid_original_data
from common_func.path_manager import PathManager
from framework.offset_calculator import OffsetCalculator
from msmodel.hardware.ccu_channel_model import CcuChannelModel
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.hardware.ccu_bean import CCUChannelBean
from profiling_bean.prof_enum.data_tag import DataTag


class CCUChannelParser(MsMultiProcess):
    """
    class used to parser ccu channel data
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._sample_config = sample_config
        self._file_list = file_list.get(DataTag.CCU_CHANNEL, [])
        self._project_path = sample_config.get("result_dir", "")
        self._calculate = OffsetCalculator(self._file_list, StructFmt.CCU_CHANNEL_FMT_SIZE, self._project_path)
        self._model = CcuChannelModel(self._project_path, DBNameConstant.DB_CCU, [DBNameConstant.TABLE_CCU_CHANNEL])
        self.channel_data = []

    def read_binary_data(self: any, file_name: str) -> int:
        """
        parsing ccu channel data
        """
        files = PathManager.get_data_file_path(self._project_path, file_name)
        if not os.path.exists(files):
            return NumberConstant.ERROR
        _file_size = os.path.getsize(files)
        if _file_size < StructFmt.CCU_CHANNEL_FMT_SIZE:
            logging.error("CCU channel data struct is incomplete, please check the file.")
            return NumberConstant.ERROR
        with FileOpen(files, "rb") as f:
            channel_bin = self._calculate.pre_process(f.file_reader, _file_size)
            struct_nums = _file_size // StructFmt.CCU_CHANNEL_FMT_SIZE
            for i in range(struct_nums):
                single_bean = CCUChannelBean().decode(
                    channel_bin[i * StructFmt.CCU_CHANNEL_FMT_SIZE: (i + 1) * StructFmt.CCU_CHANNEL_FMT_SIZE])
                if single_bean and len(single_bean.channels_bw_data) == 128:
                    self.channel_data.extend(single_bean.channels_bw_data)
        return NumberConstant.SUCCESS

    def parse(self: any) -> None:
        """
        parsing data file
        """
        logging.info("Start parsing CCU channel data file.")
        for file_name in self._file_list:
            if is_valid_original_data(file_name, self._project_path):
                self._handle_original_data(file_name)
        logging.info("Create CCU channel table finished!")

    def save(self: any) -> None:
        """
        save data to db
        :return: None
        """
        if self.channel_data:
            with self._model as model:
                model.flush(self.channel_data, DBNameConstant.TABLE_CCU_CHANNEL)

    def ms_run(self: any) -> None:
        """
        main
        :return: None
        """
        try:
            if self._file_list:
                self.parse()
                self.save()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)

    def _handle_original_data(self: any, file_name: str) -> None:
        if self.read_binary_data(file_name) == NumberConstant.ERROR:
            logging.error('Parse CCU channel data file: %s error.', file_name)
        FileManager.add_complete_file(self._project_path, file_name)
