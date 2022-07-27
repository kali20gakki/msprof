#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to parse hwts log.
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

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

    def __init__(self: any, project_path) -> None:
        self._project_path = project_path
        self._iter_end_dict = MsprofIteration(self._project_path).get_iteration_end_dict()
        self._iteration_end_time_max = max(
            self._iter_end_dict.values()) if self._iter_end_dict.values() else self.DEFAULT_ITER_ID
        self._current_iter_id = self.DEFAULT_ITER_ID
        self._current_op_iter = self.DEFAULT_ITER_ID
        self._op_iter_dict = MsprofIteration(self._project_path).get_op_iteration_dict()
        self._op_iter_queue = sorted(self._op_iter_dict.keys(), reverse=True)
        self._graph_iter_dict = MsprofIteration(self._project_path).get_graph_iteration_dict()
        self._graph_iter_queue = sorted(self._graph_iter_dict.keys(), reverse=True)


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

    @property
    def current_op_iter(self: any) -> int:
        """
        get op iter id
        :return: op iter id
        """
        return self._current_op_iter

    def check_task_in_iteration(self: any, sys_cnt: int) -> bool:
        if self._iteration_end_time_max == -1:
            return True
        return self._iteration_end_time_max >= sys_cnt

    def set_current_iter_id(self: any, sys_cnt: int) -> None:
        """
        set current iter id
        :params: sys cnt
        :return: int
        """
        if ProfilingScene().is_mix_operator_and_graph():
            self.set_current_mix_iter_id(sys_cnt)
            return
        if self._current_iter_id == self.DEFAULT_ITER_ID:
            for iter_id, end_sys_cnt in self._iter_end_dict.items():
                if sys_cnt <= end_sys_cnt:
                    self._current_iter_id = iter_id
                    self._current_op_iter = self._current_iter_id
                    return
            logging.error("Data cannot be found in any iteration.")
            raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)

        while self._check_current_iter_id(sys_cnt):
            self._current_iter_id += 1
            self._current_op_iter = self._current_iter_id

    def set_current_mix_iter_id(self: any, sys_cnt: int) -> None:
        self.set_current_graph_iter(sys_cnt)
        self.set_current_op_iter(sys_cnt)
        if not self._op_iter_queue:
            return
        if self._op_iter_queue and self._graph_iter_queue and \
                sys_cnt >= self._graph_iter_dict.get(self._graph_iter_queue[-1])[0]:
            self._current_op_iter = self._op_iter_queue[-1]
            self._current_iter_id = self._graph_iter_queue[-1]
        else:
            self._current_op_iter = self._op_iter_queue[-1]
            self._current_iter_id = self._current_op_iter

    def set_current_graph_iter(self: any, sys_cnt: int) -> None:
        while self._graph_iter_queue:
            if sys_cnt > self._graph_iter_dict.get(self._graph_iter_queue[-1])[1]:
                self._graph_iter_queue.pop()
            else:
                break

    def set_current_op_iter(self: any, sys_cnt: int) -> bool:
        while self._op_iter_queue:
            if sys_cnt > self._op_iter_dict.get(self._op_iter_queue[-1])[1]:
                self._op_iter_queue.pop()
            else:
                break

    def _check_current_iter_id(self: any, sys_cnt: int) -> int:
        iter_end = self._iter_end_dict.get(self._current_iter_id)
        return iter_end is not None and sys_cnt > iter_end