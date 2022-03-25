#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""


class RuntimeApiDto:
    """
    Dto for runtime api data
    """

    def __init__(self: any) -> None:
        self._thread = None
        self._entry_time = None
        self._exit_time = None
        self._api = None

    @property
    def thread(self: any) -> any:
        return self._thread

    @thread.setter
    def thread(self: any, value: any) -> None:
        self._thread = value

    @property
    def entry_time(self: any) -> any:
        return self._entry_time

    @entry_time.setter
    def entry_time(self: any, value: any) -> None:
        self._entry_time = value

    @property
    def exit_time(self: any) -> any:
        return self._exit_time

    @exit_time.setter
    def exit_time(self: any, value: any) -> None:
        self._exit_time = value

    @property
    def api(self: any) -> any:
        return self._api

    @api.setter
    def api(self: any, value: any) -> None:
        self._api = value
