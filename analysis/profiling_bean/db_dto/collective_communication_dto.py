#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


class CollectiveCommunicationDto:

    def __init__(self):
        self._rank_id = None
        self._compute_time = None
        self._communication_time = None
        self._stage_time = None

    @property
    def rank_id(self: any) -> any:
        return self._rank_id

    @property
    def compute_time(self: any) -> any:
        return self._compute_time

    @property
    def communication_time(self: any) -> any:
        return self._communication_time

    @property
    def stage_time(self: any) -> any:
        return self._stage_time

    @rank_id.setter
    def rank_id(self: any, value: any) -> None:
        self._rank_id = value

    @compute_time.setter
    def compute_time(self: any, value: any) -> None:
        self._compute_time = value

    @communication_time.setter
    def communication_time(self: any, value: any) -> None:
        self._communication_time = value

    @stage_time.setter
    def stage_time(self: any, value: any) -> None:
        self._stage_time = value
