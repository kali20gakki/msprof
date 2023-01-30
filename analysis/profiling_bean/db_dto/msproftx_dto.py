#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.

class MsprofTxDto:
    def __init__(self: any) -> None:
        self._pid = None
        self._tid = None
        self._start_time = None
        self._message = None

    @property
    def pid(self: any) -> any:
        return self._pid

    @pid.setter
    def pid(self: any, value: any) -> None:
        self._pid = value

    @property
    def tid(self: any) -> any:
        return self._tid

    @tid.setter
    def tid(self: any, value: any) -> None:
        self._tid = value

    @property
    def start_time(self: any) -> any:
        return self._start_time

    @start_time.setter
    def start_time(self: any, value: any) -> None:
        self._start_time = value

    @property
    def message(self: any) -> any:
        return self._message

    @message.setter
    def message(self: any, value: any) -> None:
        self._message = value
