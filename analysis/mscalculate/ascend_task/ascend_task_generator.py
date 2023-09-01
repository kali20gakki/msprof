#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import logging
import time
from collections import deque
from typing import List
from typing import Tuple
from typing import Union

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from mscalculate.ascend_task.ascend_task import TopDownTask
from mscalculate.ascend_task.ascend_task import DeviceTask
from mscalculate.ascend_task.ascend_task import HostTask
from mscalculate.ascend_task.device_task_collector import DeviceTaskCollector
from mscalculate.ascend_task.host_task_collector import HostTaskCollector


class AscendTaskGenerator:
    EXCLUDE_HOST_TYPE = "PROFILER_TRACE_EX"

    def __init__(self, result_dir: str):
        self.device_task_collector = DeviceTaskCollector(result_dir)
        self.host_task_collector = HostTaskCollector(result_dir)
        self.iter_id = -1

    @classmethod
    def _sep_task_by_stream_task_ctx(cls, tasks: Union[List[DeviceTask], List[HostTask]]) -> dict:
        ret = {}
        for task in tasks:
            ret.setdefault((task.stream_id, task.task_id, task.context_id), []).append(task)
        return ret

    @classmethod
    def _is_task_in_static_model(cls, host_task: HostTask) -> bool:
        return host_task.index_id == NumberConstant.STATIC_GRAPH_INDEX

    def run(self, model_id: int, iter_start: int, iter_end: int) -> List[TopDownTask]:
        """
        get top-down ascend tasks
        :return: all ascend tasks in model(model_id) within iter(iter_start, iter_end)
        """
        ascend_tasks = []
        if model_id == NumberConstant.INVALID_MODEL_ID:
            _, ascend_tasks = self._get_all_ascend_tasks()
        else:
            for iter_ in range(iter_start, iter_end + 1):
                self.iter_id = iter_
                _, ascend_tasks_in_iter = self._get_ascend_tasks_within_iter(model_id, iter_)
                ascend_tasks.extend(ascend_tasks_in_iter)

        ascend_tasks = sorted(ascend_tasks, key=lambda x: x.start_time)
        return ascend_tasks

    def _gen_top_down_task(self, dt: DeviceTask, ht: HostTask) -> TopDownTask:
        # wait time will calculate after this
        return TopDownTask(ht.model_id, self.iter_id, ht.stream_id, ht.task_id, dt.context_id, ht.batch_id,
                           dt.start_time, dt.duration, ht.task_type, dt.task_type)

    def _gen_top_down_task_by_device_task(self, dt: DeviceTask) -> TopDownTask:
        return TopDownTask(Constant.GE_OP_MODEL_ID, self.iter_id, dt.stream_id, dt.task_id, dt.context_id,
                           NumberConstant.DEFAULT_BATCH_ID, dt.start_time, dt.duration,
                           Constant.TASK_TYPE_UNKNOWN, dt.task_type)

    def _gen_top_down_task_by_host_task(self, ht: HostTask) -> TopDownTask:
        return TopDownTask(ht.model_id, self.iter_id, ht.stream_id, ht.task_id, NumberConstant.DEFAULT_GE_CONTEXT_ID,
                           ht.batch_id, -1, -1, ht.task_type, Constant.TASK_TYPE_UNKNOWN)

    def _match_host_device_task_in_static_model(self, host_task: HostTask, device_tasks: List[DeviceTask]) \
            -> List[TopDownTask]:
        logging.debug("Found %d device tasks for stream_id %d and task_id %d in static model %d",
                      len(device_tasks), host_task.stream_id, host_task.task_id, host_task.model_id)
        return [self._gen_top_down_task(dt, host_task) for dt in device_tasks]

    def _exclude_profiling_trace_ex_host_task(self, host_tasks: List[HostTask]) -> List[HostTask]:
        return [host_task for host_task in host_tasks if host_task.task_type != self.EXCLUDE_HOST_TYPE]

    def _match_host_device_task(self, host_tasks: List[HostTask], device_tasks: List[DeviceTask]) -> \
            Tuple[List[TopDownTask], List[TopDownTask], List[TopDownTask]]:

        if len(host_tasks) == 1 and device_tasks and self._is_task_in_static_model(host_tasks[0]):
            # task in static model, there may be a one-to-many relationship between host tasks and device tasks.
            return self._match_host_device_task_in_static_model(host_tasks[0], device_tasks), [], []

        if host_tasks and device_tasks and len(host_tasks) != len(device_tasks):
            # notice: this will be removed since in normal host and device will both report profiling_trace_ex task
            host_tasks = self._exclude_profiling_trace_ex_host_task(host_tasks)

        host_queue = deque(host_tasks)
        device_queue = deque(device_tasks)
        # only when host and device task queue are all not empty then we match
        failed_match = host_queue and device_queue and len(host_queue) != len(device_queue)

        top_down_tasks = []
        pre_batch_id = -1
        if host_queue:
            pre_batch_id = host_queue[0].batch_id - 1
        while device_queue and host_queue:
            if host_queue[0].batch_id != pre_batch_id + 1:
                logging.error("lost host tasks for stream_id: %d, task_id: %d, batch_id: %d, ctx_id: %d."
                              " Tasks that use these IDs may have mismatches.",
                              host_queue[0].stream_id, host_queue[0].task_id,
                              pre_batch_id + 1, host_queue[0].context_id)
            device_task = device_queue.popleft()
            host_task = host_queue.popleft()
            # when unique id takes effect, judge match or not by it
            top_down_t = self._gen_top_down_task(device_task, host_task)
            top_down_tasks.append(top_down_t)
            pre_batch_id = host_task.batch_id

        if host_queue:
            if failed_match:
                logging.error("device tasks less than host tasks for stream_id: %d, task_id: %d, ctx_id: %d."
                              " Tasks that use these IDs may have mismatches.",
                              host_queue[0].stream_id, host_queue[0].task_id, host_queue[0].context_id)
            else:
                logging.debug("no device tasks found for stream_id: %d, task_id: %d, ctx_id: %d.",
                              host_queue[0].stream_id, host_queue[0].task_id, host_queue[0].context_id)
        if device_queue:
            if failed_match:
                logging.error("host tasks less than device tasks for stream_id: %d, task_id: %d, ctx_id: %d."
                              " Tasks that use these IDs may have mismatches.",
                              device_queue[0].stream_id, device_queue[0].task_id, device_queue[0].context_id)
            else:
                logging.debug("no host tasks found for stream_id: %d, task_id: %d, ctx_id: %d.",
                              device_queue[0].stream_id, device_queue[0].task_id, device_queue[0].context_id)
        mismatch_device_tasks = [self._gen_top_down_task_by_device_task(data) for data in list(device_queue)]
        mismatch_host_tasks = [self._gen_top_down_task_by_host_task(data) for data in list(host_queue)]
        return top_down_tasks, mismatch_host_tasks, mismatch_device_tasks

    def _generate_top_down_tasks(self: any, host_tasks: List[HostTask], device_tasks: List[DeviceTask]) \
            -> Tuple[List[TopDownTask], List[TopDownTask]]:
        """
        associate host and device task to generate up-down task which
        contains complete ascend software and hardware info.
        """
        stream_task_ctx_separated_host_tasks = self._sep_task_by_stream_task_ctx(host_tasks)
        stream_task_ctx_separated_device_tasks = self._sep_task_by_stream_task_ctx(device_tasks)

        top_down_tasks = []
        matched_top_down_tasks = []
        stream_task_set = stream_task_ctx_separated_host_tasks.keys() | stream_task_ctx_separated_device_tasks.keys()
        for key in stream_task_set:
            host_t = stream_task_ctx_separated_host_tasks.get(key, [])
            device_t = stream_task_ctx_separated_device_tasks.get(key, [])
            matched_top_down_t, mismatch_host_t, mismatch_device_t = self._match_host_device_task(host_t, device_t)
            # statistic mismatch task, for future interface
            top_down_tasks.extend([*matched_top_down_t, *mismatch_host_t, *mismatch_device_t])
            matched_top_down_tasks.extend(matched_top_down_t)

        logging.info("Found %d host device matched task, %d top down task",
                     len(matched_top_down_tasks), len(top_down_tasks))
        return matched_top_down_tasks, top_down_tasks

    def _get_all_ascend_tasks(self) -> Tuple[List[TopDownTask], List[TopDownTask]]:
        device_id = InfoConfReader().get_device_id()
        if device_id == Constant.NA:
            logging.error("No device id found.")
            return [], []
        host_tasks = self.host_task_collector.get_host_tasks(int(device_id))
        device_tasks = self.device_task_collector.get_all_device_tasks()
        matched_top_down_tasks, top_down_tasks = self._generate_top_down_tasks(host_tasks, device_tasks)
        return matched_top_down_tasks, top_down_tasks

    def _get_ascend_tasks_within_iter(self, model_id, iter_id) -> Tuple[List[TopDownTask], List[TopDownTask]]:
        device_id = InfoConfReader().get_device_id()
        if device_id == Constant.NA:
            logging.error("No device id found.")
            return [], []
        host_tasks = self.host_task_collector.get_host_tasks_by_model_and_iter(model_id, iter_id, int(device_id))
        device_tasks = self.device_task_collector.get_device_tasks_by_model_and_iter(model_id, iter_id)
        matched_top_down_tasks, top_down_tasks = self._generate_top_down_tasks(host_tasks, device_tasks)
        return matched_top_down_tasks, top_down_tasks
