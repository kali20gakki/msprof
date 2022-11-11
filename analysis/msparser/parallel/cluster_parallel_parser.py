#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

import logging

from common_func.constant import Constant
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.ms_multi_process import MsMultiProcess
from common_func.msprof_exception import ProfException
from msmodel.iter_rec.iter_rec_model import HwtsIterViewModel
from msmodel.parallel.cluster_hccl_model import ClusterHCCLViewModel
from msmodel.parallel.parallel_model import ParallelModel
from msmodel.step_trace.ts_track_model import TsTrackViewModel
from msparser.interface.iparser import IParser
from profiling_bean.prof_enum.data_tag import DataTag


class ClusterParallelParser(IParser, MsMultiProcess):

    def __init__(self: any, file_list: dict, sample_config: dict) -> None:
        super().__init__(sample_config)
        self._file_list = file_list
        self._sample_config = sample_config
        self._project_path = sample_config.get(StrConstant.SAMPLE_CONFIG_PROJECT_PATH)
        self._hccl_op_data = []
        self._merged_compute_op_data = []
        self._iter_time_data = []
        self._hccl_overlap_time_data = []
        self._iter_compute_time_data = []

    def ms_run(self: any) -> None:
        self.parse()
        self.save()

    def parse(self: any) -> None:
        if not self._file_list.get(DataTag.PARALLEL_STRATEGY, []):
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        logging.info("Start to parse parallel index related data.")
        if not self._prepare_for_parse():
            raise ProfException(ProfException.PROF_SYSTEM_EXIT)
        hccl_op_overlap_time = self._compute_overlap_time(self._hccl_op_data, self._merged_compute_op_data)
        iter_overlap_time = self._compute_overlap_time(self._iter_time_data, self._merged_compute_op_data)
        for hccl_op in hccl_op_overlap_time:
            self._hccl_overlap_time_data.append(
                [hccl_op.model_id, hccl_op.index_id, hccl_op.op_name, hccl_op.op_type, hccl_op.start_time,
                 hccl_op.end_time, hccl_op.overlap_time])
        for iter_data in iter_overlap_time:
            self._iter_compute_time_data.append(
                [iter_data.model_id, iter_data.index_id, iter_data.end_time - iter_data.start_time,
                 iter_data.overlap_time])

    def save(self: any) -> None:
        if not self._hccl_overlap_time_data:
            logging.warning('No valid parallel index related data!')
            return
        if not self._iter_compute_time_data:
            logging.warning('No valid parallel index related data!')
            return
        with ParallelModel(self._project_path) as _model:
            _model.flush(DBNameConstant.TABLE_HCCL_OPERATOR_OVERLAP, self._hccl_overlap_time_data)
            _model.flush(DBNameConstant.TABLE_COMPUTATION_TIME, self._iter_compute_time_data)

    def _prepare_for_parse(self: any) -> bool:
        with ClusterHCCLViewModel(self._project_path) as _model:
            self._hccl_op_data = _model.get_hccl_op_data()
        if not self._hccl_op_data:
            logging.warning("Invalid hccl op data from ts_track!")
            return False
        with HwtsIterViewModel(self._project_path) as _model:
            ai_core_op_data = _model.get_ai_core_op_data()
        with TsTrackViewModel(self._project_path) as _model:
            ai_cpu_op_data = _model.get_ai_cpu_op_data()
            self._iter_time_data = _model.get_iter_time_data()
        if not ai_core_op_data and not ai_cpu_op_data:
            logging.warning("Invalid compute op data from hwts and ts_track!")
            return False
        if not self._iter_time_data:
            logging.warning("Invalid step trace data from ts_track!")
            return False
        self._merge_compute_op_data(ai_core_op_data, ai_cpu_op_data)
        return True

    def _merge_compute_op_data(self: any, ai_core_op_data: list, ai_cpu_op_data: list) -> None:
        current_op = None
        while ai_core_op_data or ai_cpu_op_data:
            if not ai_cpu_op_data:
                compare_op = ai_core_op_data.pop()
            elif not ai_core_op_data:
                compare_op = ai_cpu_op_data.pop()
            elif ai_cpu_op_data[-1].start_time < ai_core_op_data[-1].start_time:
                compare_op = ai_cpu_op_data.pop()
            else:
                compare_op = ai_core_op_data.pop()

            if not current_op:
                current_op = compare_op
            elif compare_op.start_time > current_op.end_time:
                self._merged_compute_op_data.append(current_op)
                current_op = compare_op
            else:
                current_op.end_time = max(current_op.end_time, compare_op.end_time)
        self._merged_compute_op_data.append(current_op)

    def _compute_overlap_time(self: any, master_time_section_list: list, slave_time_section_list: list) -> list:
        current_slava_key = Constant.DEFAULT_VALUE
        for master_time_section in master_time_section_list:
            overlap_time = Constant.DEFAULT_VALUE
            while current_slava_key < len(slave_time_section_list):
                if slave_time_section_list[current_slava_key].end_time <= master_time_section.start_time:
                    current_slava_key += 1
                elif slave_time_section_list[current_slava_key].start_time >= master_time_section.end_time:
                    break
                elif self._merged_compute_op_data[current_slava_key].end_time > master_time_section.end_time:
                    overlap_time = overlap_time + (master_time_section.end_time - max(
                        self._merged_compute_op_data[current_slava_key].start_time, master_time_section.start_time))
                    break
                else:
                    overlap_time = overlap_time + (self._merged_compute_op_data[current_slava_key].end_time - max(
                        self._merged_compute_op_data[current_slava_key].start_time, master_time_section.start_time))
                    current_slava_key += 1
            master_time_section.overlap_time = overlap_time
        return master_time_section_list
