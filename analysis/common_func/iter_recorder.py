#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to parse hwts log.
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""
import logging

from common_func.msprof_iteration import MsprofIteration
from common_func.msprof_exception import ProfException


class IterRecorder:
    """
    common function of calculating iter id
    """

    STREAM_TASK_KEY_FMT = "{0}-{1}"
    DEFAULT_ITER_ID = -1

    def __init__(self: any, project_path) -> None:
        self._project_path = project_path
        self._iter_end_dict = MsprofIteration(self._project_path).get_iteration_end_dict()
        self._batch_list = []
        self._current_iter_id = self.DEFAULT_ITER_ID

    def _check_current_iter_id(self: any, sys_cnt: int) -> int:
        iter_end = self._iter_end_dict.get(self._current_iter_id)
        return iter_end is not None and sys_cnt > iter_end

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

    @property
    def iter_end_dict(self: any) -> int:
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
