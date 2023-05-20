#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved.
import logging
import os
from typing import List

from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.path_manager import PathManager
from mscalculate.interface.icalculator import ICalculator
from msmodel.hccl.hccl_model import HcclViewModel
from profiling_bean.db_dto.hccl_dto import HcclDto


class HcclCalculator(ICalculator, MsMultiProcess):

    def __init__(self, file_list, sample_config):
        super().__init__(sample_config)
        self._file_list = file_list
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._model = HcclViewModel(self._project_path, DBNameConstant.DB_HCCL,
                                    [DBNameConstant.TABLE_HCCL_OP, DBNameConstant.TABLE_HCCL_TASK])
        self._hccl_data = []

    @staticmethod
    def _is_next_hccl_op(cur_op_name, data, first_timestamp_record):
        return cur_op_name != data.op_name and cur_op_name in first_timestamp_record

    def calculate(self: any) -> None:
        hccl_data = self._get_hccl_data()
        if hccl_data:
            self._generate_hccl_op_info(hccl_data)

    def save(self: any) -> None:
        if not self._hccl_data:
            return
        with self._model as hccl_model:
            hccl_model.rebuild_hccl_table()
            hccl_model.insert_data_to_db(DBNameConstant.TABLE_HCCL_ALL_REDUCE, self._hccl_data)

    def ms_run(self: any) -> None:
        self.calculate()
        self.save()

    def _generate_hccl_op_info(self, hccl_data: List[HcclDto]):
        first_timestamp_record = {}
        cur_op_name = hccl_data[0].op_name
        for data in hccl_data:
            data.first_timestamp = first_timestamp_record.setdefault(data.op_name, data.timestamp)

            if self._is_next_hccl_op(cur_op_name, data, first_timestamp_record):
                first_timestamp_record.pop(cur_op_name)
                cur_op_name = data.op_name
            self._hccl_data.append([data.model_id, data.index_id, data.op_name, data.iteration,
                                    data.hccl_name, data.first_timestamp, data.plane_id, data.timestamp,
                                    data.duration, data.is_dynamic, data.task_type, data.op_type, str(data.args)])

    def _get_hccl_data(self):
        if not os.path.exists(PathManager.get_db_path(self._project_path, DBNameConstant.DB_HCCL)):
            return []
        with self._model as hccl_model:
            if not hccl_model.check_table():
                logging.warning("The HCCL table does not exist, so there is no need to continue associating operators.")
                return []
            return hccl_model.get_hccl_communication_data()
