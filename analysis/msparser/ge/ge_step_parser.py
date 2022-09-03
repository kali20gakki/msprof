#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from common_func.db_name_constant import DBNameConstant
from common_func.ms_multi_process import MsMultiProcess
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.ge.ge_step_bean import GeStepBean
from profiling_bean.prof_enum.data_tag import DataTag


class GeStepParser(DataParser, MsMultiProcess):
    """
    ge step data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        DataParser.__init__(self, sample_config)
        self._file_list = file_list
        self.data = []

    @staticmethod
    def read_binary_data(bean_class: any, bean_data: any) -> any:
        """
        read binary data
        """
        return bean_class().fusion_decode(bean_data)

    @staticmethod
    def _get_step_data(bean_data: any) -> list:
        if not bean_data:
            return []
        return [bean_data.model_id, bean_data.stream_id,
                bean_data.task_id, bean_data.thread_id, bean_data.timestamp,
                bean_data.index_num, bean_data.tag, bean_data.batch_id]

    def parse(self: any) -> None:
        """
        parse ge step data
        """
        fusion_step_file = self._file_list.get(DataTag.GE_STEP, [])
        if fusion_step_file:
            self.data = self.parse_bean_data(fusion_step_file,
                                             StructFmt.GE_STEP_SIZE,
                                             GeStepBean,
                                             self._get_step_data)

    def save(self: any) -> str:
        """
        save
        :return:
        """
        return str(self.__name__)

    def ms_run(self: any) -> dict:
        """
        parse and save ge step data
        :return:
        """
        if not self._file_list.get(DataTag.GE_STEP, []):
            return {}
        self.parse()
        return {DBNameConstant.TABLE_GE_STEP: self.data}
