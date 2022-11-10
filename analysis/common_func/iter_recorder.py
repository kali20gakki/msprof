#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import logging

from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.msprof_exception import ProfException
from common_func.msprof_iteration import MsprofIteration


class IterRecorder:
    """
    common function of calculating iter id
    """

    STREAM_TASK_KEY_FMT = "{0}-{1}"
    DEFAULT_ITER_ID = -1
    DEFAULT_ITER_TIME = -1

    def __init__(self: any, project_path) -> None:
        self._project_path = project_path
        self._iter_end_dict = MsprofIteration(self._project_path).get_iteration_end_dict()
        self._max_iter_time = self._get_max_iter_time()
        self._current_iter_id = self.DEFAULT_ITER_ID

    @property
    def iter_end_dict(self: any) -> dict:
        """
        get iter end dict
        :return: iter end dict
        """
        return self._iter_end_dict

    @property
    def current_iter_id(self: any) -> int:
        """
        get iter id
        :return: iter id
        """
        return self._current_iter_id

    def check_task_in_iteration(self: any, sys_cnt: int) -> bool:
        if self._max_iter_time == self.DEFAULT_ITER_TIME:
            return True
        return self._max_iter_time >= sys_cnt

    def set_current_iter_id(self: any, sys_cnt: int) -> None:
        """
        set current iter id
        :params: sys cnt
        :return: int
        """
        if self._current_iter_id == self.DEFAULT_ITER_ID:
            for iter_id, end_sys_cnt in self._iter_end_dict.items():
                if sys_cnt <= end_sys_cnt:
                    self._current_iter_id = iter_id
                    return
            logging.error("Data cannot be found in any iteration.")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)

        while self._check_current_iter_id(sys_cnt):
            self._current_iter_id += 1

    def _check_current_iter_id(self: any, sys_cnt: int) -> int:
        iter_end = self._iter_end_dict.get(self._current_iter_id)
        return iter_end is not None and sys_cnt > iter_end

    def _get_max_iter_time(self: any) -> int:
        if self._iter_end_dict.values():
            return max(self._iter_end_dict.values())
        return self.DEFAULT_ITER_TIME
