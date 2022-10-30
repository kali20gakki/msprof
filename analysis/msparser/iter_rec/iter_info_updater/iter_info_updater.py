#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.

from msmodel.ge.ge_info_calculate_model import GeInfoModel
from msparser.iter_rec.iter_info_updater.iter_info import IterInfo
from msparser.iter_rec.iter_info_updater.iter_info_manager import IterInfoManager


class IterInfoUpdater:
    HWTS_TASK_END = 1

    def __init__(self: any, project_path) -> None:
        self.current_iter = -1
        self.iteration_manager = IterInfoManager(project_path)
        self.active_parallel_iter_id = set([])
        self.active_parallel_iter_info = set([])

    def update_parallel_iter_info_pool(self: any, iter_id: int) -> None:
        """
        update parallel iter info pool
        """
        if iter_id <= self.current_iter:
            return

        new_iter_info = self.iteration_manager.iter_to_iter_info.get(iter_id, IterInfo())

        new_add_parallel_id = []
        for it_id in new_iter_info.behind_parallel_iter - self.active_parallel_iter_id:
            new_add_parallel_id.append(self.iteration_manager.iter_to_iter_info.get(it_id))

        self.update_new_add_iter_info(new_add_parallel_id)

        self.current_iter = iter_id
        self.active_parallel_iter_id = new_iter_info.behind_parallel_iter
        self.active_parallel_iter_info = {
            self.iteration_manager.iter_to_iter_info.get(iter_id, IterInfo())
            for iter_id in self.active_parallel_iter_id}

    def update_new_add_iter_info(self: any, new_add_parallel_id: any) -> None:
        """
        update new add iter info
        """
        current_iter_info = self.iteration_manager.iter_to_iter_info.get(self.current_iter, IterInfo())

        for new_add_parallel_iter_info in new_add_parallel_id:
            new_add_parallel_iter_info.hwts_offset = current_iter_info.hwts_offset + current_iter_info.hwts_count
            new_add_parallel_iter_info.aic_offset = current_iter_info.aic_offset + current_iter_info.aic_count

    def update_count_and_offset(self: any, task: any, ai_core_task: set) -> None:
        """
        update count and offset
        """
        self.update_hwts(self.active_parallel_iter_info)

        if task.sys_tag == self.HWTS_TASK_END and \
                self.judge_ai_core(task, self.active_parallel_iter_info, ai_core_task):
            self.update_aicore(self.active_parallel_iter_info)

    @staticmethod
    def judge_ai_core(task: any, iter_info_list: list, ai_core_task: set) -> bool:
        """
        judge ai core
        """
        # if there are ge data, ai_core_task is empty
        if not ai_core_task:
            return any([iter_info_bean.is_aicore(task) for iter_info_bean in iter_info_list])
        return GeInfoModel.STREAM_TASK_KEY_FMT.format(task.stream_id, task.task_id) in ai_core_task

    @staticmethod
    def update_hwts(iter_info_list: list) -> None:
        """
        update hwts
        """
        for iter_info_bean in iter_info_list:
            iter_info_bean.hwts_count += 1

    @staticmethod
    def update_aicore(iter_info_list: list) -> None:
        """
        update aicore
        """
        for iter_info_bean in iter_info_list:
            iter_info_bean.aic_count += 1
