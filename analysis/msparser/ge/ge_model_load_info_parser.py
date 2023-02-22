#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from common_func.db_name_constant import DBNameConstant
from common_func.ms_multi_process import MsMultiProcess
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.ge.ge_model_load_bean import GeModelLoadBean
from profiling_bean.prof_enum.data_tag import DataTag


class GeModelLoadParser(DataParser, MsMultiProcess):
    """
    ge model load info data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        super(DataParser, self).__init__(sample_config)
        self._file_list = file_list
        self.data = []

    @staticmethod
    def read_binary_data(bean_class: any, bean_data: any) -> any:
        """
        read binary data
        """
        return bean_class().fusion_decode(bean_data)

    @staticmethod
    def _get_model_load_data(bean_data: any) -> list:
        if not bean_data:
            return []
        model_name = bean_data.model_name
        if not bean_data.is_hash_type():
            model_name = bean_data.model_name.partition(b'\x00')[0].decode('utf-8', 'ignore')
            return [bean_data.model_id, model_name, bean_data.start_time,
                    bean_data.end_time]
        return [bean_data.model_id, model_name, bean_data.start_time,
                bean_data.end_time]

    def parse(self: any) -> None:
        """
        parse model load data
        """
        model_load_file = self._file_list.get(DataTag.GE_MODEL_LOAD, [])
        if model_load_file:
            self.data = self.parse_bean_data(model_load_file,
                                             StructFmt.GE_MODEL_LOAD_SIZE,
                                             GeModelLoadBean,
                                             self._get_model_load_data)

    def save(self: any) -> str:
        """
        save
        :return:
        """
        return str(self.__name__)

    def ms_run(self: any) -> dict:
        """
        parse ge model load info  data
        :return:
        """
        if not self._file_list.get(DataTag.GE_MODEL_LOAD, []):
            return {}
        self.parse()
        return {DBNameConstant.TABLE_GE_MODEL_LOAD: self.data}
