#!/usr/bin/python3
# coding=utf-8
"""
function: parser for ge model info
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import logging

from common_func.constant import Constant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from msmodel.ge.ge_model_load_model import GeFusionModel
from msparser.ge.ge_fusion_op_info_parser import GeFusionOpParser
from msparser.ge.ge_model_load_info_parser import GeModelLoadParser
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag


class GeModelInfoParser(IParser, MsMultiProcess):
    """
    ge model info parser
    """

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        MsMultiProcess.__init__(self, sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)

        self._parsing_obj = [GeFusionOpParser, GeModelLoadParser]
        self._model = None
        self._ge_fusion_data = {}
        self._table_list = []

    def parse(self: any) -> None:
        """
        parse ge model info data
        """
        for parse_class in self._parsing_obj:
            ge_model_data = parse_class(self._file_list, self._sample_config).ms_run()
            self._ge_fusion_data.update(ge_model_data)
            self._table_list.extend(list(ge_model_data.keys()))

    def save(self: any) -> None:
        """
        save data
        """
        self._model = GeFusionModel(self._project_path, self._table_list)
        if self._model:
            self._model.init()
            self._model.flush_all(self._ge_fusion_data)
            self._model.finalize()

    def ms_run(self: any) -> None:
        """
        parse and save ge model info data
        :return:
        """
        try:
            if self._file_list.get(DataTag.GE_MODEL_LOAD) or self._file_list.get(DataTag.GE_FUSION_OP_INFO):
                self.parse()
                self.save()
        except (OSError, IOError, SystemError, ValueError, TypeError, RuntimeError, ProfException) as err:
            logging.error(str(err), exc_info=Constant.TRACE_BACK_SWITCH)
