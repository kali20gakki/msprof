#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import logging

from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from msmodel.parallel.cluster_hccl_model import ClusterHCCLModel
from msmodel.step_trace.ts_track_model import TsTrackViewModel
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag


class HCCLOperatiorParser(IParser, MsMultiProcess):
    def __init__(self: any, file_list: dict, sample_config: dict):
        super().__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._hccl_operator_data = []

    def ms_run(self: any) -> None:
        self.parse()
        self.save()

    def parse(self: any) -> None:
        parallel_files = self._file_list.get(DataTag.PARALLEL_STRATEGY, [])
        if not parallel_files:
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        logging.info("Start to parse hccl operator data from ts_track!")
        with TsTrackViewModel(self._project_path) as _model:
            self._hccl_operator_data = _model.get_hccl_operator_exe_data()

    def save(self: any) -> None:
        if not self._hccl_operator_data:
            logging.error('No valid hccl operator exe data.')
            return
        with ClusterHCCLModel(self._project_path) as _model:
            _model.flush(self._hccl_operator_data, DBNameConstant.TABLE_HCCL_OPERATOR_EXE)
