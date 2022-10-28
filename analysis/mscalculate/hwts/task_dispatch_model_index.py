#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
from analyzer.scene_base.profiling_scene import ProfilingScene
from common_func.msprof_iteration import MsprofIteration
from common_func.constant import Constant


class TaskDispatchModelIndex:
    def __init__(self: any, model_id: int, index_id: int, result_dir: int) -> None:
        self.model_id = model_id
        self.index_id = index_id
        self.result_dir = result_dir
        self.iteration_info_list = self.init_iteration_info_list()

    def init_iteration_info_list(self: any) -> any:
        msprof_iteration = MsprofIteration(self.result_dir)
        iter_dict = msprof_iteration.get_iter_dict_with_index_and_model(
            self.index_id, self.model_id)

        iteration_info_list = []
        if not (ProfilingScene().is_mix_operator_and_graph() and self.model_id == Constant.GE_OP_MODEL_ID):
            return iteration_info_list

        for model_id, index_id in iter_dict.values():
            iteration_info = msprof_iteration.get_iteration_info_by_index_id(model_id, index_id)
            if iteration_info:
                iteration_info_list.append(iteration_info)
        iteration_info_list.sort(key=lambda iter_info: iter_info.step_end)

        return iteration_info_list

    def dispatch(self: any, end_time: int) -> tuple:
        for iter_info in self.iteration_info_list:
            if iter_info.step_start < end_time <= iter_info.step_end:
                return iter_info.model_id, iter_info.index_id
        return self.model_id, self.index_id
