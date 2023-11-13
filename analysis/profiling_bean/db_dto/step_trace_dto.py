#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from collections import namedtuple
from dataclasses import dataclass

from common_func.ms_constant.number_constant import NumberConstant
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class StepTraceOriginDto(metaclass=InstanceCheckMeta):
    """
    step trace origin DATA dto
    """
    index_id = None
    model_id = None
    stream_id = None
    tag_id = None
    task_id = None
    timestamp = None


@dataclass
class StepTraceDto(metaclass=InstanceCheckMeta):
    """
    step trace dto
    """
    DEFAULT_ITER_ID = -1
    index_id = None
    iter_id = None
    model_id = None
    step_end = None
    step_start = None


@dataclass
class TrainingTraceDto(metaclass=InstanceCheckMeta):
    """
    Training trace dto
    """
    bp_end = None
    data_aug_bound = None
    device_id = None
    fp_bp_time = None
    fp_start = None
    grad_refresh_bound = None
    iteration_end = None
    iteration_id = None
    iteration_time = None
    model_id = None


Iteration = namedtuple("Iteration", ["model_id", "iteration_id", "iteration_count"])


class IterationRange(Iteration):
    """
    iteration range for model execute.
    """
    MAX_ITERATION_COUNT = 5

    def __repr__(self):
        if self._is_compatibility_required():
            return f'{self.iteration_id}'
        return f'{self.iteration_start}_{self.iteration_end}'

    @property
    def iteration_start(self):
        return self.iteration_id

    @property
    def iteration_end(self):
        return self.iteration_id + self.iteration_count - NumberConstant.DEFAULT_ITER_COUNT

    def get_iteration_range(self):
        return self.iteration_start, self.iteration_end

    def _is_compatibility_required(self):
        return NumberConstant.DEFAULT_ITER_COUNT == self.iteration_count
