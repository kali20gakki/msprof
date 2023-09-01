#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from msmodel.msproftx.msproftx_model import MsprofTxModel
from viewer.get_trace_timeline import TraceViewer


class TaskQueueViewer:
    def __init__(self: any, params: dict):
        self._result_dir = params.get(StrConstant.PARAM_RESULT_DIR)
        self._pid = None

    def get_task_queue_data(self):
        with MsprofTxModel(self._result_dir, DBNameConstant.DB_MSPROFTX, [DBNameConstant.TABLE_MSPROFTX]) as model:
            task_queue_origin_data = model.get_task_queue_origin_data()
            tid_meta_data = model.get_task_queue_tid_meta_data()
        task_queue_data = self._get_timeline_data(task_queue_origin_data)
        meta_data = self._get_meta_data(tid_meta_data)
        task_queue_data.extend(meta_data)
        return TraceViewer("TaskQueueViewer").format_trace_events(task_queue_data)

    def _get_timeline_data(self: any, origin_data: list):
        result = []
        current_category = None
        current_task_queue = None
        for data in origin_data:
            if current_category is None or data.category != current_category:
                self._pid = data.pid
                current_category = data.category
                current_task_queue = data
                continue
            if not current_task_queue or current_task_queue.message != data.message:
                current_task_queue = data
            else:
                result.append({
                    "name": current_task_queue.message, "pid": current_task_queue.pid, "tid": current_task_queue.tid,
                    "ts": InfoConfReader().time_from_host_syscnt(current_task_queue.start_time,
                                                                 NumberConstant.MICRO_SECOND),
                    "ph": "X", "args": {},
                    "dur": InfoConfReader().get_host_duration((data.start_time - current_task_queue.start_time),
                                                              NumberConstant.MICRO_SECOND)
                })
                current_task_queue = None
        return result

    def _get_meta_data(self: any, tid_meta_data: list) -> list:
        meta_data = [
            ["process_name", self._pid, InfoConfReader().get_json_tid_data(), TraceViewHeaderConstant.PROCESS_PTA]
        ]
        for tid_data in tid_meta_data:
            if tid_data.is_enqueue_category:
                meta_data.append(["thread_name", self._pid, tid_data.tid, f"Thread {tid_data.tid} (Enqueue)"])
            else:
                meta_data.append(["thread_name", self._pid, tid_data.tid, f"Thread {tid_data.tid} (Dequeue)"])
            meta_data.append(["thread_sort_index", self._pid, tid_data.tid, tid_data.category])
        return TraceViewManager.metadata_event(meta_data)
