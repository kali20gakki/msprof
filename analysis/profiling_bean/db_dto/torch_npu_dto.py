#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


class TorchNpuDto:
    """
    Dto for relationship between torch op and npu kernel
    """

    def __init__(self: any) -> None:
        self._torch_op_start_time = None
        self._torch_op_tid = None
        self._torch_op_pid = None
        self._op_name = None
        self._acl_start_time = None
        self._acl_end_time = None
        self._acl_tid = None
        self._acl_compile_time = None
        self._stream_id = None
        self._task_id = None
        self._batch_id = None
        self._context_id = None

    @property
    def torch_op_start_time(self: any) -> any:
        return self._torch_op_start_time

    @property
    def torch_op_tid(self: any) -> any:
        return self._torch_op_tid

    @property
    def torch_op_pid(self: any) -> any:
        return self._torch_op_pid

    @property
    def op_name(self: any) -> any:
        return self._op_name

    @property
    def acl_start_time(self: any) -> any:
        return self._acl_start_time

    @property
    def acl_end_time(self: any) -> any:
        return self._acl_end_time

    @property
    def acl_tid(self: any) -> any:
        return self._acl_tid

    @property
    def acl_compile_time(self: any) -> any:
        return self._acl_compile_time

    @property
    def stream_id(self: any) -> any:
        return self._stream_id

    @property
    def task_id(self: any) -> any:
        return self._task_id

    @property
    def batch_id(self: any) -> any:
        return self._batch_id

    @property
    def context_id(self: any) -> any:
        return self._context_id

    @torch_op_start_time.setter
    def torch_op_start_time(self: any, value: any) -> None:
        self._torch_op_start_time = value

    @torch_op_tid.setter
    def torch_op_tid(self: any, value: any) -> None:
        self._torch_op_tid = value

    @torch_op_pid.setter
    def torch_op_pid(self: any, value: any) -> None:
        self._torch_op_pid = value

    @op_name.setter
    def op_name(self: any, value: any) -> None:
        self._op_name = value

    @acl_start_time.setter
    def acl_start_time(self: any, value: any) -> None:
        self._acl_start_time = value

    @acl_end_time.setter
    def acl_end_time(self: any, value: any) -> None:
        self._acl_end_time = value

    @acl_tid.setter
    def acl_tid(self: any, value: any) -> None:
        self._acl_tid = value

    @acl_compile_time.setter
    def acl_compile_time(self: any, value: any) -> None:
        self._acl_compile_time = value

    @stream_id.setter
    def stream_id(self: any, value: any) -> None:
        self._stream_id = value

    @task_id.setter
    def task_id(self: any, value: any) -> None:
        self._task_id = value

    @batch_id.setter
    def batch_id(self: any, value: any) -> None:
        self._batch_id = value

    @context_id.setter
    def context_id(self: any, value: any) -> None:
        self._context_id = value