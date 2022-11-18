#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import logging

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from msmodel.ge.ge_hash_model import GeHashViewModel
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
        if not self._file_list.get(DataTag.PARALLEL_STRATEGY, []):
            return
        logging.info("Start to parse hccl operator data from ts_track!")
        self.parse()
        self.save()

    def parse(self: any) -> None:
        with TsTrackViewModel(self._project_path) as _model:
            hccl_data_list = _model.get_hccl_operator_exe_data()
        with GeHashViewModel(self._project_path) as _model:
            ge_hash_dict = _model.get_ge_hash_data()
        hccl_start_time = Constant.DEFAULT_INVALID_VALUE
        for hccl_data in hccl_data_list:
            if hccl_data.tag_id % 2 == self.END_TAG:
                hash_value = ge_hash_dict.get(hccl_data.op_name, None)
                op_name = hash_value if hash_value else hccl_data.op_name
                self._hccl_operator_data.append(
                    [hccl_data.model_id, hccl_data.index_id, op_name, hccl_data.op_type, hccl_start_time,
                     hccl_data.timestamp])
                hccl_start_time = Constant.DEFAULT_INVALID_VALUE
            elif hccl_start_time == Constant.DEFAULT_INVALID_VALUE:
                hccl_start_time = hccl_data.timestamp
            else:
                self._hccl_operator_data.append(
                    [hccl_data.model_id, hccl_data.index_id, "", "", hccl_start_time, Constant.DEFAULT_INVALID_VALUE])

    def save(self: any) -> None:
        if not self._hccl_operator_data:
            logging.error('No valid hccl operator exe data.')
            return
        with ClusterHCCLModel(self._project_path) as _model:
            _model.flush(self._hccl_operator_data, DBNameConstant.TABLE_HCCL_OPERATOR_EXE)
