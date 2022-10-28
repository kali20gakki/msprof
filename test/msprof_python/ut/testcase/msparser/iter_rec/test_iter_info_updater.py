#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
import unittest
from unittest import mock

from msparser.iter_rec.iter_info_updater.iter_info_updater import IterInfoUpdater
from mock_tools import ClassMock


class TestIterInfoUpdater(unittest.TestCase):

    def test_update_parallel_iter_info_pool_1(self: any) -> None:
        instance = mock.Mock()
        instance.current_iter = 1
        IterInfoUpdater.update_parallel_iter_info_pool(instance, 1)

    def test_update_parallel_iter_info_pool_2(self: any) -> None:
        self_instance = mock.Mock()
        self_instance.current_iter = 10

        new_iter_info = mock.Mock()
        self_instance.iteration_manager.iter_to_iter_info.get.return_value = new_iter_info
        new_iter_info.behind_parallel_iter = {2, 3, 4}

        self_instance.active_parallel_iter_set = {1, 2, 3}
        IterInfoUpdater.update_parallel_iter_info_pool(self_instance, 11)

        self.assertEqual(self_instance.current_iter, 11)
        self.assertEqual(self_instance.active_parallel_iter_set, {2, 3, 4})

    def test_update_new_add_iter_info(self: any) -> None:
        self_instance = mock.Mock()

        current_iter_info = mock.Mock()
        current_iter_info.hwts_offset = 1
        current_iter_info.aic_offset = 2
        current_iter_info.hwts_count = 1
        current_iter_info.aic_count = 2
        self_instance.iteration_manager.iter_to_iter_info.get.return_value = current_iter_info

        IterInfoUpdater.update_new_add_iter_info(self_instance, [mock.Mock(), mock.Mock()])

    def test_update_count_and_offset(self: any) -> None:
        self_instance = mock.Mock()
        self_instance.iteration_manager.iter_to_iter_info.get.return_value = mock.Mock()
        self_instance.active_parallel_iter_set = {1, 2, 3}
        self_instance.judge_ai_core.return_value = True

        task = mock.Mock()
        task.sys_tag = IterInfoUpdater.HWTS_TASK_END

        ai_core_task = set([])

        IterInfoUpdater.update_count_and_offset(self_instance, task, ai_core_task)

    def test_judge_ai_core_1(self: any) -> None:
        task = mock.Mock()
        task.stream_id = 1
        task.task_id = 1

        iter_info_list = [mock.Mock(), mock.Mock()]
        ai_core_task = {"1-1", "2-10"}

        IterInfoUpdater.judge_ai_core(iter_info_list, ai_core_task)

    def test_judge_ai_core_2(self: any) -> None:
        task = mock.Mock()
        task.stream_id = 1
        task.task_id = 1

        iter_info_list = [mock.Mock(), mock.Mock()]
        ai_core_task = {}

        IterInfoUpdater.judge_ai_core(task, iter_info_list, ai_core_task)

    def test_update_hwts(self: any) -> None:
        iter_info_bean = mock.Mock()
        iter_info_bean.hwts_count = 1
        iter_info_list = [iter_info_bean]

        IterInfoUpdater.update_hwts(iter_info_list)

        self.assertEqual(iter_info_bean.hwts_count, 2)

    def test_update_aicore(self: any) -> None:
        iter_info_bean = mock.Mock()
        iter_info_bean.aic_count = 1
        iter_info_list = [iter_info_bean]

        IterInfoUpdater.update_aicore(iter_info_list)

        self.assertEqual(iter_info_bean.aic_count, 2)