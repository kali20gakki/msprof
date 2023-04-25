#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


class HCCLInfoDto:
    def __init__(self: any) -> None:
        self._level = None
        self._type = None
        self._thread_id = None
        self._data_len = None
        self._timestamp = None
        self._item_id = None
        self._ccl_tag = None
        self._group_name = None
        self._local_rank = None
        self._remote_rank = None
        self._rank_size = None
        self._work_flow_mode = None
        self._plane_id = None
        self._notify_id = None
        self._stage = None
        self._role = None
        self._duration_estimated = None
        self._src_addr = None
        self._dst_addr = None
        self._size = None
        self._op_type = None
        self._data_type = None
        self._link_type = None
        self._transport_type = None
        self._rdma_type = None
        self._reserve = None

    @property
    def level(self: any) -> str:
        return self._level

    @level.setter
    def level(self: any, value: any) -> None:
        self._level = value

    @property
    def type(self: any) -> str:
        return str(self._type)

    @type.setter
    def type(self: any, value: any) -> None:
        self._type = value

    @property
    def thread_id(self: any) -> int:
        return self._thread_id

    @thread_id.setter
    def thread_id(self: any, value: any) -> None:
        self._thread_id = value

    @property
    def data_len(self):
        return self._data_len

    @data_len.setter
    def data_len(self, value):
        self._data_len = value

    @property
    def timestamp(self):
        return self._timestamp

    @timestamp.setter
    def timestamp(self, value):
        self._timestamp = value

    @property
    def item_id(self):
        return self._item_id

    @item_id.setter
    def item_id(self, value):
        self._item_id = value

    @property
    def ccl_tag(self):
        return self._ccl_tag

    @ccl_tag.setter
    def ccl_tag(self, value):
        self._ccl_tag = value

    @property
    def group_name(self):
        return self._group_name

    @group_name.setter
    def group_name(self, value):
        self._group_name = value

    @property
    def local_rank(self):
        return self._local_rank

    @local_rank.setter
    def local_rank(self, value):
        self._local_rank = value

    @property
    def remote_rank(self):
        return self._remote_rank

    @remote_rank.setter
    def remote_rank(self, value):
        self._remote_rank = value

    @property
    def rank_size(self):
        return self._rank_size

    @rank_size.setter
    def rank_size(self, value):
        self._rank_size = value

    @property
    def work_flow_mode(self):
        return self._work_flow_mode

    @work_flow_mode.setter
    def work_flow_mode(self, value):
        self._work_flow_mode = value

    @property
    def plane_id(self):
        return self._plane_id

    @plane_id.setter
    def plane_id(self, value):
        self._plane_id = value

    @property
    def notify_id(self):
        return self._notify_id

    @notify_id.setter
    def notify_id(self, value):
        self._notify_id = value

    @property
    def stage(self):
        return self._stage

    @stage.setter
    def stage(self, value):
        self._stage = value

    @property
    def role(self):
        return self._role

    @role.setter
    def role(self, value):
        self._role = value

    @property
    def duration_estimated(self):
        return self._duration_estimated

    @duration_estimated.setter
    def duration_estimated(self, value):
        self._duration_estimated = value

    @property
    def src_addr(self):
        return self._src_addr

    @src_addr.setter
    def src_addr(self, value):
        self._src_addr = value

    @property
    def dst_addr(self):
        return self._dst_addr

    @dst_addr.setter
    def dst_addr(self, value):
        self._dst_addr = value

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, value):
        self._size = value

    @property
    def op_type(self):
        return self._op_type

    @op_type.setter
    def op_type(self, value):
        self._op_type = value

    @property
    def data_type(self):
        return self._data_type

    @data_type.setter
    def data_type(self, value):
        self._data_type = value

    @property
    def link_type(self):
        return self._link_type

    @link_type.setter
    def link_type(self, value):
        self._link_type = value

    @property
    def transport_type(self):
        return self._transport_type

    @transport_type.setter
    def transport_type(self, value):
        self._transport_type = value

    @property
    def rdma_type(self):
        return self._rdma_type

    @rdma_type.setter
    def rdma_type(self, value):
        self._rdma_type = value

    @property
    def reserve(self):
        return self._reserve

    @reserve.setter
    def reserve(self, value):
        self._reserve = value
