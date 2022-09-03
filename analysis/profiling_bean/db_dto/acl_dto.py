#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


class AclDto:
    """
    Dto for acl data
    """

    def __init__(self: any) -> None:
        self._api_name = None
        self._api_type = None
        self._start_time = None
        self._end_time = None
        self._thread_id = None

    @property
    def api_name(self: any) -> any:
        return self._api_name

    @api_name.setter
    def api_name(self: any, value: any) -> None:
        self._api_name = value

    @property
    def api_type(self: any) -> any:
        return self._api_type

    @api_type.setter
    def api_type(self: any, value: any) -> None:
        self._api_type = value

    @property
    def start_time(self: any) -> any:
        return self._start_time

    @start_time.setter
    def start_time(self: any, value: any) -> None:
        self._start_time = value

    @property
    def end_time(self: any) -> any:
        return self._end_time

    @end_time.setter
    def end_time(self: any, value: any) -> None:
        self._end_time = value

    @property
    def thread_id(self: any) -> any:
        return self._thread_id

    @thread_id.setter
    def thread_id(self: any, value: any) -> None:
        self._thread_id = value
