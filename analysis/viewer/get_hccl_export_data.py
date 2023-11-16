#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.

import json
from collections import OrderedDict
from dataclasses import dataclass
from typing import List
from typing import Set

from common_func.db_name_constant import DBNameConstant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.trace_view_header_constant import TraceViewHeaderConstant
from common_func.trace_view_manager import TraceViewManager
from mscalculate.hccl.hccl_task import HcclTask
from msmodel.hccl.hccl_model import HcclViewModel


class HCCLExport:
    """
    hccl export
    """
    HCCL_SORTED_OFFSET = 70000
    INVALID_PLANE = -1
    DEFAULT_PLANE = 0
    INVALID_GROUP = 'N/A'

    @dataclass
    class HcclGroup:
        group_name: str
        planes: Set[int]
        id: int
        start_index: int

    def __init__(self: any, param: dict) -> None:
        self.project_path = param.get(StrConstant.PARAM_RESULT_DIR)
        self.result = []
        self.err_message = {}
        self.iter_range = param.get(StrConstant.PARAM_ITER_ID)
        self.pid_value = InfoConfReader().get_json_pid_data()
        self.hccl_groups = dict()

    @staticmethod
    def get_hccl_arg(hccl_task):
        return OrderedDict({
            'notify_id': hccl_task.notify_id,
            'duration estimated(us)': hccl_task.duration_estimated,
            'stream id': hccl_task.stream_id,
            'task id': hccl_task.task_id,
            'context id': hccl_task.context_id,
            'task type': hccl_task.hccl_name,
            'src rank': hccl_task.local_rank,
            'dst rank': hccl_task.remote_rank,
            'transport type': hccl_task.transport_type,
            'size(Byte)': hccl_task.size,
            'data type': hccl_task.data_type,
            'link type': hccl_task.link_type,
            "bandwidth(GB/s)": hccl_task.bandwidth
        })

    def get_hccl_timeline_data(self: any) -> str:
        """
        get data for hccl timeline
        """
        with HcclViewModel(self.project_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE,
                           [DBNameConstant.TABLE_HCCL_SINGLE_DEVICE]) as hccl_model:
            if not hccl_model.check_table():
                return json.dumps({
                    'status': NumberConstant.ERROR,
                    "info": "get hccl data failed, may be the hccl file not completed or hccl parser failed."
                            " please check data file."
                })

            hccl_data = hccl_model.get_all_data(DBNameConstant.TABLE_HCCL_SINGLE_DEVICE, dto_class=HcclTask)
            if not hccl_data:
                return json.dumps({
                    'status': NumberConstant.WARN,
                    "info": f"get hccl data failed, "
                            f"may be lack of hccl files containing iteration {self.iter_range.iteration_id}."
                })
        self._format_hccl_data(hccl_data)
        return json.dumps(self.result)

    def _add_hccl_bar(self):
        self.result = TraceViewManager.metadata_event(
            [["process_name", self.pid_value, InfoConfReader().get_json_tid_data(), "HCCL"]])

    def _add_group_threads(self, group: HcclGroup, start_sort_index: int, valid_group: bool) -> int:
        """
        add group thread meta data in json
        return: end_sort_index
        """
        index_now = start_sort_index
        # update start index
        group.start_index = start_sort_index
        thread_name = f"Group {group.id} Communication" if valid_group else "Communication"
        self.result.extend(TraceViewManager.metadata_event(
            [["thread_name", self.pid_value, index_now, thread_name]]))
        self.result.extend(TraceViewManager.metadata_event(
            [["thread_sort_index", self.pid_value, index_now, index_now]]))

        plane_infos = []
        plane_sort_indexes = []
        for plane in group.planes:
            index_now = start_sort_index + plane + 1
            plane_infos.append(["thread_name", self.pid_value, index_now, f"Plane {plane}"])
            plane_sort_indexes.append(["thread_sort_index", self.pid_value, index_now, index_now])
        self.result.extend(TraceViewManager.metadata_event(plane_infos))
        self.result.extend(TraceViewManager.metadata_event(plane_sort_indexes))
        index_now += 1
        return index_now

    def _init_hccl_group(self, hccl_data: List[HcclTask]) -> dict:
        name_planes_table: OrderedDict[str, Set[int]] = OrderedDict()
        hccl_groups = dict()
        for data in hccl_data:
            # L0 scene or something get error
            if data.plane_id == self.INVALID_PLANE:
                data = data.replace(plane_id=self.DEFAULT_PLANE)
            name_planes_table.setdefault(data.group_name, set()).add(data.plane_id)
        for _id, (group_name, planes) in enumerate(name_planes_table.items()):
            hccl_group = self.HcclGroup(group_name, planes, _id, -1)
            hccl_groups[group_name] = hccl_group
        return hccl_groups

    def _get_meta_data(self: any, hccl_data: List[HcclTask]) -> None:
        self.hccl_groups = self._init_hccl_group(hccl_data)

        self._add_hccl_bar()

        index_now = 0
        for group in self.hccl_groups.values():
            index_now = self._add_group_threads(group, index_now, group.group_name != self.INVALID_GROUP)

    def _format_hccl_data(self: any, hccl_data: list) -> None:
        self._get_meta_data(hccl_data)
        _hccl_format_data = self._format_hccl_communication_data(hccl_data)
        _hccl_format_op_data = self._format_hccl_op_data()
        self.result.extend(TraceViewManager.time_graph_trace(
            TraceViewHeaderConstant.GRPC_TIME_GRAPH_HEAD, _hccl_format_data + _hccl_format_op_data))

    def _format_hccl_op_data(self):
        with HcclViewModel(self.project_path, DBNameConstant.DB_HCCL_SINGLE_DEVICE,
                           DBNameConstant.TABLE_HCCL_SINGLE_DEVICE) as hccl_model:
            hccl_op_data = hccl_model.get_hccl_op_data_by_group()
            _hccl_format_op_data = [
                [
                    hccl_op.op_name, self.pid_value, self.hccl_groups.get(hccl_op.group_name).start_index,
                    InfoConfReader().trans_into_local_time(hccl_op.timestamp / NumberConstant.NS_TO_US),
                    hccl_op.duration / NumberConstant.NS_TO_US,
                    {
                        "connection_id": hccl_op.connection_id,
                        "model id": hccl_op.model_id,
                    },
                ]
                for hccl_op in hccl_op_data
            ]
        return _hccl_format_op_data

    def _format_hccl_communication_data(self, hccl_data: List[HcclTask]):
        # for L0 collect, plane id will be filled -1
        if not hccl_data or hccl_data[0].group_name == self.INVALID_GROUP:
            return []
        _hccl_format_data = [0] * len(hccl_data)
        for index, _hccl_data in enumerate(hccl_data):
            hccl_args = HCCLExport.get_hccl_arg(_hccl_data)
            hccl_args["model id"] = _hccl_data.model_id
            thread_id = self.hccl_groups.get(_hccl_data.group_name).start_index + _hccl_data.plane_id + 1
            _hccl_data_pice = [
                _hccl_data.hccl_name, self.pid_value, thread_id,
                InfoConfReader().trans_into_local_time(_hccl_data.timestamp / NumberConstant.NS_TO_US),
                _hccl_data.duration / NumberConstant.NS_TO_US, hccl_args
            ]
            _hccl_format_data[index] = _hccl_data_pice
        return _hccl_format_data
