#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to parse hwts log.
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

import logging

from analyzer.scene_base.profiling_scene import ProfilingScene
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
        self._current_iter_id = self.DEFAULT_ITER_ID
        self._current_op_iter = 0
        self._current_graph_iter = 0
        self._op_iter_dict = MsprofIteration(self._project_path).get_op_iteration_dict()
        self._graph_iter_dict = MsprofIteration(self._project_path).get_iteration_dict()

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

    def _check_current_iter_id(self: any, sys_cnt: int) -> int:
        iter_end = self._iter_end_dict.get(self._current_iter_id)
        return iter_end is not None and sys_cnt > iter_end

    def set_current_mix_iter_id(self: any, sys_cnt: int) -> None:
        if self._current_iter_id == self.DEFAULT_ITER_ID:
            self.set_current_graph_iter(sys_cnt)
            if not self.set_current_op_iter(sys_cnt):
                logging.error("Data cannot be found in any iteration.")
                raise ProfException(ProfException.PROF_INVALID_DATA_ERROR)

        self._update_current_iter_id(sys_cnt)

    def set_current_graph_iter(self: any, sys_cnt: int) -> None:
        for iter_id, iter_sys_cnt in self._graph_iter_dict.items():
            # iter_sys_cnt include start_sys_cnt and end_sys_cnt
            if sys_cnt <= iter_sys_cnt[1]:
                self._current_iter_id = iter_id
                self._current_graph_iter = iter_id
                break

    def set_current_op_iter(self: any, sys_cnt: int) -> bool:
        for iter_id, iter_sys_cnt in self._op_iter_dict.items():
            # iter_sys_cnt include start_sys_cnt and end_sys_cnt
            if iter_sys_cnt[0] <= sys_cnt <= iter_sys_cnt[1]:
                self._current_op_iter = iter_id
                return True
        return False

    def _check_mix_current_iter_id(self: any, sys_cnt: int) -> int:
        iter_time = self._graph_iter_dict.get(self._current_iter_id)
        return iter_time is not None and (sys_cnt > iter_time[1] or sys_cnt < iter_time[0])

    def _update_current_iter_id(self: any, sys_cnt: int) -> None:
        self._current_iter_id = self._current_graph_iter
        while self._check_mix_current_iter_id(sys_cnt):
            if self._op_iter_dict.get(self._current_op_iter) and self._op_iter_dict.get(self._current_op_iter)[
                0] <= sys_cnt \
                    <= self._op_iter_dict.get(self._current_op_iter)[1]:
                # sys time in interval [op_iter_start, op_iter_end]

                if self._graph_iter_dict.get(self._current_graph_iter + 1) \
                        and (self._graph_iter_dict.get(self._current_graph_iter + 1)[0] <= sys_cnt
                             <= self._graph_iter_dict.get(self._current_graph_iter + 1)[1]):
                    # sys in interval [iter+1_start, iter+1_end]
                    self._current_graph_iter = self._current_graph_iter + 1
                    self._current_iter_id = self._current_graph_iter
                    break
                # sys in interval [op_iter_start, iter_start] or [iter_end, iter+1_start]
                self._current_iter_id = self._current_op_iter
                break
            self._current_op_iter = self._current_graph_iter + 1
            self._current_graph_iter = self._current_graph_iter + 2
            self._current_iter_id = self._current_graph_iter

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
    def current_graph_iter(self: any) -> int:
        """
        get graph iter id
        :return: iter id
        """
        return self._current_graph_iter

    @property
    def current_op_iter(self: any) -> int:
        """
        get op iter id
        :return: iter id
        """
        return self._current_op_iter
