#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.msprof_iteration import MsprofIteration
from common_func.constant import Constant


class TaskDispatchModelIndex:
    """
    Dispatch model id and index id for each task
    """
    def __init__(self: any, model_id: int, index_id: int, result_dir: int) -> None:
        self.model_id = model_id
        self.index_id = index_id
        self.result_dir = result_dir
        self.iteration_info_list = self.init_iteration_info_list()

    def init_iteration_info_list(self: any) -> any:
        """
        init iteration info list
        """
        iteration_info_list = []
        if not (ProfilingScene().is_mix_operator_and_graph() and self.model_id == Constant.GE_OP_MODEL_ID):
            return iteration_info_list

        msprof_iteration = MsprofIteration(self.result_dir)
        iter_list = msprof_iteration.get_iter_list_with_index_and_model(
            self.index_id, self.model_id)

        for model_id, index_id in iter_list:
            iteration_info = msprof_iteration.get_iteration_info_by_index_id(model_id, index_id)
            if iteration_info:
                iteration_info_list.append(iteration_info)

        # after sorting, the last element of iteration info list is op model id and its index id
        iteration_info_list.sort(key=lambda iter_info: iter_info.step_end)

        return iteration_info_list

    def dispatch(self: any, end_time: int) -> tuple:
        """
        dispatch model id and index id for each task
        """
        # only mix and model id is op model id sence, model id and index id for each task might be different
        # the last element of iteration info list is op model id and its index id
        for iter_info in self.iteration_info_list:
            if iter_info.step_start < end_time <= iter_info.step_end:
                return iter_info.model_id, iter_info.index_id
        # For parallel scene, can not find accurately model id and index id
        return self.model_id, self.index_id
