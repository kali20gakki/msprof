#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


class NanoPmuDto:
    """
    Dto for nano pmu data
    """

    def __init__(self: any) -> None:
        self._stream_id = None
        self._task_id = None
        self._total_cycle = None
        self._block_dim = None
        self._pmu0 = None
        self._pmu1 = None
        self._pmu2 = None
        self._pmu3 = None
        self._pmu4 = None
        self._pmu5 = None
        self._pmu6 = None
        self._pmu7 = None
        self._pmu8 = None
        self._pmu9 = None

    @property
    def stream_id(self: any) -> any:
        return self._stream_id

    @property
    def task_id(self: any) -> any:
        return self._task_id

    @property
    def total_cycle(self: any) -> any:
        return self._total_cycle

    @property
    def block_dim(self: any) -> any:
        return self._block_dim

    @property
    def pmu0(self: any) -> any:
        return self._pmu0

    @property
    def pmu1(self: any) -> any:
        return self._pmu1

    @property
    def pmu2(self: any) -> any:
        return self._pmu2

    @property
    def pmu3(self: any) -> any:
        return self._pmu3

    @property
    def pmu4(self: any) -> any:
        return self._pmu4

    @property
    def pmu5(self: any) -> any:
        return self._pmu5

    @property
    def pmu6(self: any) -> any:
        return self._pmu6

    @property
    def pmu7(self: any) -> any:
        return self._pmu7

    @property
    def pmu8(self: any) -> any:
        return self._pmu8

    @property
    def pmu9(self: any) -> any:
        return self._pmu9

    @property
    def pmu_list(self: any) -> any:
        return [self._pmu0, self._pmu1, self._pmu2, self._pmu3,
                self._pmu4, self._pmu5, self._pmu6, self._pmu7,
                self._pmu8, self._pmu9]

    @stream_id.setter
    def stream_id(self: any, value: any) -> None:
        self._stream_id = value

    @task_id.setter
    def task_id(self: any, value: any) -> None:
        self._task_id = value

    @total_cycle.setter
    def total_cycle(self: any, value: any) -> None:
        self._total_cycle = value

    @block_dim.setter
    def block_dim(self: any, value: any) -> None:
        self._block_dim = value

    @pmu0.setter
    def pmu0(self: any, value: any) -> None:
        self._pmu0 = value

    @pmu1.setter
    def pmu1(self: any, value: any) -> None:
        self._pmu1 = value

    @pmu2.setter
    def pmu2(self: any, value: any) -> None:
        self._pmu2 = value

    @pmu3.setter
    def pmu3(self: any, value: any) -> None:
        self._pmu3 = value

    @pmu4.setter
    def pmu4(self: any, value: any) -> None:
        self._pmu4 = value

    @pmu5.setter
    def pmu5(self: any, value: any) -> None:
        self._pmu5 = value

    @pmu6.setter
    def pmu6(self: any, value: any) -> None:
        self._pmu6 = value

    @pmu7.setter
    def pmu7(self: any, value: any) -> None:
        self._pmu7 = value

    @pmu8.setter
    def pmu8(self: any, value: any) -> None:
        self._pmu8 = value

    @pmu9.setter
    def pmu9(self: any, value: any) -> None:
        self._pmu9 = value
