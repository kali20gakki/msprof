#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import logging
import os
import struct

from common_func.constant import Constant
from common_func.empty_class import EmptyClass
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from msparser.aicpu.parse_aicpu_bin_data import ParseAiCpuBinData
from msparser.aicpu.parse_aicpu_data import ParseAiCpuData
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.prof_enum.data_tag import DataTag


class ParseAiCpuDataAdapter(MsMultiProcess):
    """
    support to analysis the ai cpu due to the compatibility
    """
    TAG_AICPU = "AICPU"
    BYTE_ORDER_CHAR = '='
    READ_BINARY_HEADER_FMT = 'HH'
    # The min bytes for ai cpu: 2 bytes for magic number, and 2 bytes for data tag.
    # magic number: 5A5A
    AI_CPU_MAGIC_NUM = 23130
    AI_CPU_TAG = 60

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        self._file_list = file_list
        self.sample_config = sample_config
        self.project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

    def get_ai_cpu_analysis_engine(self: any, ai_cpu_file: list) -> any:
        """
        check the ai cpu data and pick up the correct parser to analysis
        """
        judge_file, offset = self.__get_aicpu_judge_data(ai_cpu_file)
        if not judge_file:
            return ParseAiCpuData(self._file_list, self.sample_config)
        judge_file_path = PathManager.get_data_file_path(self.project_path, judge_file)
        if StructFmt.AI_CPU_FMT_SIZE - offset < struct.calcsize(self.READ_BINARY_HEADER_FMT):
            offset -= StructFmt.AI_CPU_FMT_SIZE
        try:
            with open(judge_file_path, 'rb') as cpu_f:
                file_size = os.path.getsize(judge_file_path)
                _ = cpu_f.read(file_size + offset - StructFmt.AI_CPU_FMT_SIZE)
                magic_num, data_tag = struct.unpack(self.BYTE_ORDER_CHAR + self.READ_BINARY_HEADER_FMT, cpu_f.read(
                    struct.calcsize(self.BYTE_ORDER_CHAR + self.READ_BINARY_HEADER_FMT)))
                if magic_num == self.AI_CPU_MAGIC_NUM and data_tag == self.AI_CPU_TAG:
                    return ParseAiCpuBinData(self._file_list, self.sample_config)
                return ParseAiCpuData(self._file_list, self.sample_config)
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(err, exc_info=Constant.TRACE_BACK_SWITCH)
            return EmptyClass(str(err))

    def ms_run(self: any) -> None:
        """
        main entry for parsing ai cpu data adapter
        :return:
        """
        ai_cpu_file = sorted(self._file_list.get(DataTag.AI_CPU, []), key=lambda x: int(x.split("_")[-1]),
                             reverse=True)
        if not ai_cpu_file:
            return
        parser_obj = self.get_ai_cpu_analysis_engine(ai_cpu_file)
        try:
            if parser_obj:
                parser_obj.ms_run()
        except (OSError, SystemError, ValueError, TypeError, RuntimeError) as task_rec_err:
            logging.error(str(task_rec_err), exc_info=Constant.TRACE_BACK_SWITCH)

    def __get_aicpu_judge_data(self: any, ai_cpu_file: list) -> tuple:
        total_size = 0
        size_list = []
        for file in ai_cpu_file:
            _ai_cpu_file = PathManager.get_data_file_path(self.project_path, file)
            size_list.append(os.path.getsize(_ai_cpu_file))
            total_size += os.path.getsize(_ai_cpu_file)
        if total_size < StructFmt.AI_CPU_FMT_SIZE:
            return EmptyClass(str('Insufficient file size')), 0
        judge_file = ai_cpu_file[0]
        offset = 0
        if size_list[0] < StructFmt.AI_CPU_FMT_SIZE:
            offset = size_list[0]
            judge_file = ai_cpu_file[1]
        return judge_file, offset
