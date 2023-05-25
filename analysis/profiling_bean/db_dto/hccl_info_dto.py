#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
from common_func.constant import Constant


class HCCLInfoDto:
    def __init__(self: any) -> None:
        self._level = Constant.NA
        self._struct_type = Constant.NA
        self._thread_id = Constant.DEFAULT_INVALID_VALUE
        self._data_len = Constant.DEFAULT_INVALID_VALUE
        self._timestamp = Constant.DEFAULT_INVALID_VALUE
        self._op_name = Constant.NA
        self._ccl_tag = Constant.NA
        self._group_name = Constant.NA
        self._local_rank = Constant.DEFAULT_INVALID_VALUE
        self._remote_rank = Constant.DEFAULT_INVALID_VALUE
        self._rank_size = Constant.DEFAULT_INVALID_VALUE
        self._work_flow_mode = Constant.NA
        self._plane_id = Constant.DEFAULT_INVALID_VALUE
        self._notify_id = Constant.DEFAULT_INVALID_VALUE
        self._stage = Constant.DEFAULT_INVALID_VALUE
        self._role = Constant.NA
        self._duration_estimated = Constant.DEFAULT_INVALID_VALUE
        self._src_addr = Constant.DEFAULT_INVALID_VALUE
        self._dst_addr = Constant.DEFAULT_INVALID_VALUE
        self._size = Constant.DEFAULT_INVALID_VALUE
        self._op_type = Constant.NA
        self._data_type = Constant.NA
        self._link_type = Constant.NA
        self._transport_type = Constant.NA
        self._rdma_type = Constant.NA

    @property
    def level(self: any) -> str:
        return self._level

    @property
    def struct_type(self: any) -> str:
        return str(self._struct_type)

    @property
    def thread_id(self: any) -> int:
        return self._thread_id

    @property
    def data_len(self):
        return self._data_len

    @property
    def timestamp(self):
        return self._timestamp

    @property
    def op_name(self):
        return self._op_name

    @property
    def ccl_tag(self):
        return self._ccl_tag

    @property
    def group_name(self):
        return self._group_name

    @property
    def local_rank(self):
        return self._local_rank

    @property
    def remote_rank(self):
        return self._remote_rank

    @property
    def rank_size(self):
        return self._rank_size

    @property
    def work_flow_mode(self):
        return self._work_flow_mode

    @property
    def plane_id(self):
        return self._plane_id

    @property
    def notify_id(self):
        return self._notify_id

    @property
    def stage(self):
        return self._stage

    @property
    def role(self):
        return self._role

    @property
    def duration_estimated(self):
        return self._duration_estimated

    @property
    def src_addr(self):
        return self._src_addr

    @property
    def dst_addr(self):
        return self._dst_addr

    @property
    def size(self):
        return self._size

    @property
    def op_type(self):
        return self._op_type

    @property
    def data_type(self):
        return self._data_type

    @property
    def link_type(self):
        return self._link_type

    @property
    def transport_type(self):
        return self._transport_type

    @property
    def rdma_type(self):
        return self._rdma_type

    @level.setter
    def level(self: any, value: any) -> None:
        self._level = value

    @struct_type.setter
    def struct_type(self: any, value: any) -> None:
        self._struct_type = value

    @thread_id.setter
    def thread_id(self: any, value: any) -> None:
        self._thread_id = value

    @data_len.setter
    def data_len(self, value):
        self._data_len = value

    @timestamp.setter
    def timestamp(self, value):
        self._timestamp = value

    @op_name.setter
    def op_name(self, value):
        self._op_name = value

    @ccl_tag.setter
    def ccl_tag(self, value):
        self._ccl_tag = value

    @group_name.setter
    def group_name(self, value):
        self._group_name = value

    @local_rank.setter
    def local_rank(self, value):
        self._local_rank = value

    @remote_rank.setter
    def remote_rank(self, value):
        self._remote_rank = value

    @rank_size.setter
    def rank_size(self, value):
        self._rank_size = value

    @work_flow_mode.setter
    def work_flow_mode(self, value):
        self._work_flow_mode = value

    @plane_id.setter
    def plane_id(self, value):
        self._plane_id = value

    @notify_id.setter
    def notify_id(self, value):
        self._notify_id = value

    @stage.setter
    def stage(self, value):
        self._stage = value

    @role.setter
    def role(self, value):
        self._role = value

    @duration_estimated.setter
    def duration_estimated(self, value):
        self._duration_estimated = value

    @src_addr.setter
    def src_addr(self, value):
        self._src_addr = value

    @dst_addr.setter
    def dst_addr(self, value):
        self._dst_addr = value

    @size.setter
    def size(self, value):
        self._size = value

    @op_type.setter
    def op_type(self, value):
        self._op_type = value

    @data_type.setter
    def data_type(self, value):
        self._data_type = value

    @link_type.setter
    def link_type(self, value):
        self._link_type = value

    @transport_type.setter
    def transport_type(self, value):
        self._transport_type = value

    @rdma_type.setter
    def rdma_type(self, value):
        self._rdma_type = value

    def get_bandwidth(self):
        if self.duration_estimated == Constant.DEFAULT_INVALID_VALUE or self.size == Constant.DEFAULT_INVALID_VALUE or \
                self.duration_estimated == 0:
            return 0
        return self.size / self.duration_estimated / 1000  # 1000 scale(GB/s)

    def to_args_json(self, stream_id, task_id):
        bandwidth = self.get_bandwidth()
        json = {
            'notify_id': self.notify_id,
            'duration estimated(us)': self.duration_estimated,
            'bandwidth(GB/s)': bandwidth,
            'stream id': stream_id,
            'task id': task_id,
            'task type': self.op_name,
            'src rank': self.local_rank,
            'dst rank': self.remote_rank,
            'transport type': self.transport_type,
            'size(Byte)': self.size,
            'data type': self.data_type,
            'link type': self.link_type
        }
        return str(json)
