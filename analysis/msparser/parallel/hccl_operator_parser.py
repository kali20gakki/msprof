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
    END_TAG = 1

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
            hccl_data_list = _model.get_hccl_operator_exe_data()
        hccl_start_time = []
        for hccl_data in hccl_data_list:
            if hccl_data.tag == self.END_TAG:
                if hccl_start_time:
                    self._hccl_operator_data.append(
                        [hccl_data.model_id, hccl_data.index_id, hccl_data.op_name, hccl_data.op_type,
                         hccl_start_time[-1], hccl_data.timestamp])
                    hccl_start_time = []
            else:
                hccl_start_time.append(hccl_data.timestamp)

    def save(self: any) -> None:
        if not self._hccl_operator_data:
            logging.error('No valid hccl operator exe data.')
            return
        with ClusterHCCLModel(self._project_path) as _model:
            _model.flush(self._hccl_operator_data, DBNameConstant.TABLE_HCCL_OPERATOR_EXE)
