#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


class HwtsRecDto:
    """
    hwts rec dto
    """

    def __init__(self: any) -> None:
        self._ai_core_num = None
        self._task_count = None
        self._sys_cnt = None
        self._iter_id = None

    @property
    def ai_core_num(self: any) -> any:
        return self._ai_core_num

    @ai_core_num.setter
    def ai_core_num(self: any, value: any) -> None:
        self._ai_core_num = value

    @property
    def task_count(self: any) -> any:
        return self._task_count

    @task_count.setter
    def task_count(self: any, value: any) -> None:
        self._task_count = value

    @property
    def sys_cnt(self: any) -> any:
        return self._sys_cnt

    @sys_cnt.setter
    def sys_cnt(self: any, value: any) -> None:
        self._sys_cnt = value

    @property
    def iter_id(self: any) -> any:
        return self._iter_id

    @iter_id.setter
    def iter_id(self: any, value: any) -> None:
        self._iter_id = value
