#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant


class StepTraceDto:
    """
    step trace dto
    """
    DEFAULT_ITER_ID = -1

    def __init__(self: any) -> None:
        self._index_id = None
        self._model_id = None
        self._step_start = None
        self._step_end = None
        self._iter_id = None

    @property
    def index_id(self: any) -> any:
        return self._index_id

    @index_id.setter
    def index_id(self: any, value: any) -> None:
        self._index_id = value

    @property
    def model_id(self: any) -> any:
        return self._model_id

    @model_id.setter
    def model_id(self: any, value: any) -> None:
        self._model_id = value

    @property
    def step_start(self: any) -> any:
        return self._step_start

    @step_start.setter
    def step_start(self: any, value: any) -> None:
        self._step_start = value

    @property
    def step_end(self: any) -> any:
        return self._step_end

    @step_end.setter
    def step_end(self: any, value: any) -> None:
        self._step_end = value

    @property
    def iter_id(self: any) -> any:
        return self._iter_id

    @iter_id.setter
    def iter_id(self: any, value: any) -> None:
        self._iter_id = value


class TrainingTraceDto:
    """
    Training trace dto
    """
    def __init__(self: any) -> None:
        self._device_id = None
        self._model_id = None
        self._iteration_id = None
        self._fp_start = None
        self._bp_end = None
        self._iteration_end = None
        self._iteration_time = None
        self._fp_bp_time = None
        self._grad_refresh_bound = None
        self._data_aug_bound = None

    @property
    def device_id(self: any) -> any:
        return self._device_id

    @device_id.setter
    def device_id(self: any, value: any) -> None:
        self._device_id = value

    @property
    def model_id(self: any) -> any:
        return self._model_id

    @model_id.setter
    def model_id(self: any, value: any) -> None:
        self._model_id = value

    @property
    def iteration_id(self: any) -> any:
        return self._iteration_id

    @iteration_id.setter
    def iteration_id(self: any, value: any) -> None:
        self._iteration_id = value

    @property
    def fp_start(self: any) -> any:
        return self._fp_start

    @fp_start.setter
    def fp_start(self: any, value: any) -> None:
        if value == NumberConstant.NULL_NUMBER:
            self._fp_start = value
        else:
            self._fp_start = InfoConfReader().time_from_syscnt(value, NumberConstant.MICRO_SECOND)

    @property
    def bp_end(self: any) -> any:
        return self._bp_end

    @bp_end.setter
    def bp_end(self: any, value: any) -> None:
        if value == NumberConstant.NULL_NUMBER:
            self._bp_end = value
        else:
            self._bp_end = InfoConfReader().time_from_syscnt(value, NumberConstant.MICRO_SECOND)

    @property
    def iteration_end(self: any) -> any:
        return self._iteration_end

    @iteration_end.setter
    def iteration_end(self: any, value: any) -> None:
        if value == NumberConstant.NULL_NUMBER:
            self._iteration_end = value
        else:
            self._iteration_end = InfoConfReader().time_from_syscnt(value, NumberConstant.MICRO_SECOND)

    @property
    def iteration_time(self: any) -> any:
        return self._iteration_time

    @iteration_time.setter
    def iteration_time(self: any, value: any) -> None:
        self._iteration_time = value

    @property
    def fp_bp_time(self: any) -> any:
        return self._fp_bp_time

    @fp_bp_time.setter
    def fp_bp_time(self: any, value: any) -> None:
        self._fp_bp_time = value

    @property
    def grad_refresh_bound(self: any) -> any:
        return self._grad_refresh_bound

    @grad_refresh_bound.setter
    def grad_refresh_bound(self: any, value: any) -> None:
        self._grad_refresh_bound = value

    @property
    def data_aug_bound(self: any) -> any:
        return self._data_aug_bound

    @data_aug_bound.setter
    def data_aug_bound(self: any, value: any) -> None:
        self._data_aug_bound = value
