#!/usr/bin/python3
# coding=utf-8
"""
function: parser for ge model time
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import logging
import sqlite3

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from model.ge.ge_model_time_load import GeModelTimeModel
from msparser.data_struct_size_constant import StructFmt
from msparser.interface.data_parser import DataParser
from profiling_bean.ge.ge_model_time_bean import GeModelTimeBean
from profiling_bean.prof_enum.data_tag import DataTag


class GeModelTimeParser(DataParser, MsMultiProcess):
    """
    ge model time data parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        DataParser.__init__(self, sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._ge_model_time_data = []
        self._table_list = [DBNameConstant.TABLE_GE_MODEL_TIME]
        self._model = None

    def parse(self: any) -> None:
        """
        parse ge model time data
        """
        self._parse_model_time_data()

    def _parse_model_time_data(self: any) -> None:
        fusion_time_file = self._file_list.get(DataTag.GE_MODEL_TIME, [])
        if fusion_time_file:
            self._ge_model_time_data = self.parse_bean_data(fusion_time_file,
                                                            StructFmt.GE_MODEL_TIME_SIZE,
                                                            GeModelTimeBean,
                                                            self._get_model_time_data)

    @staticmethod
    def _get_model_time_data(bean_data: any) -> list:
        if not bean_data:
            return []
        model_name = bean_data.model_name
        if not bean_data.is_hash_type():
            model_name = bean_data.model_name.partition(b'\x00')[0].decode('utf-8', 'ignore')
        return [model_name, bean_data.model_id, bean_data.request_id,
                bean_data.thread_id, bean_data.input_data_start_time, bean_data.input_data_end_time,
                bean_data.infer_start_time, bean_data.infer_end_time, bean_data.output_data_start_time,
                bean_data.output_data_end_time]

    @staticmethod
    def read_binary_data(bean_class: any, bean_data: any) -> any:
        """
        read binary data
        """
        return bean_class().fusion_decode(bean_data)

    def save(self: any) -> None:
        """
        save data
        """
        self._model = GeModelTimeModel(self._project_path, self._table_list)
        if self._model:
            self._model.init()
            self._model.flush(self._ge_model_time_data)
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        parse and save ge model time data
        :return:
        """
        if not self._file_list.get(DataTag.GE_MODEL_TIME, []):
            return
        try:
            self.parse()
        except (OSError, IOError, SystemError, ValueError, TypeError, RuntimeError) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
            return
        try:
            self.save()
        except sqlite3.Error as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
