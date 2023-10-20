#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
from typing import List
from typing import Union
from dataclasses import dataclass

from msparser.compact_info.task_track_bean import TaskTrackBean
from mscalculate.ascend_task.ascend_task import DeviceTask
from msparser.step_trace.ts_binary_data_reader.task_flip_bean import TaskFlip


@dataclass
class InfDataHelper:
    timestamp = float("inf")


class FlipCalculator:
    """
    calculate batch id by flip number
    """
    STREAM_DESTROY_FLIP = 65535

    @staticmethod
    def compute_batch_id(task_data: List[Union[TaskTrackBean, DeviceTask]],
                         flip_data: List[Union[TaskTrackBean, TaskFlip]]) -> None:
        if not task_data:
            return
        task_data_bin = FlipCalculator.sep_data_by_stream_id(task_data)
        flip_data = FlipCalculator.sep_data_by_stream_id(flip_data)
        for stream_id, data in task_data_bin.items():
            flip_data_stream = flip_data.get(stream_id, [])
            data.sort(key=lambda x: x.timestamp)
            flip_data_stream.sort(key=lambda x: x.timestamp)
            flip_data_stream.append(InfDataHelper())  # avoid overflow
            batch_id = 0  # flip index
            task_index = 0
            while task_index < len(data):
                task = data[task_index]
                flip = flip_data_stream[batch_id]
                if task.timestamp > flip.timestamp:
                    batch_id += 1  # next flip
                    FlipCalculator.calibrate_when_flip_task_id_not_zero(data, flip, task_index, batch_id)
                    continue
                task.batch_id = batch_id
                task_index += 1  # next task

    @staticmethod
    def calibrate_when_flip_task_id_not_zero(
            task_data: List[Union[TaskTrackBean, DeviceTask]],
            flip: Union[TaskTrackBean, TaskFlip],
            task_index: int,
            batch_id: int,
    ) -> None:
        if flip.flip_num == FlipCalculator.STREAM_DESTROY_FLIP:  # do not calibrate when stream destroy
            return
        # Because tasks in multi-threads will apply for task id 0 simultaneously,
        # the flip may not get the task_id 0, we should search backward to calibrate the task
        # which task id is less than flip's task_id, and set these tasks the next batch id.
        task_index_backward = task_index - 1
        while task_index_backward >= 0 and task_data[task_index_backward].task_id < flip.task_id:
            task_data[task_index_backward].batch_id = batch_id
            task_index_backward -= 1

    @staticmethod
    def sep_data_by_stream_id(raw_data: List[Union[TaskTrackBean, DeviceTask, TaskFlip]]) -> dict:
        sep_data = {}
        for data in raw_data:
            sep_data.setdefault(data.stream_id, []).append(data)
        return sep_data
