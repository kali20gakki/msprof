#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.


class ClusterRankDto:
    """
    Dto for cluster rank data
    """

    def __init__(self: any) -> None:
        self._job_info = None
        self._device_id = None
        self._rank_id = None
        self._dir_name = None

    @property
    def job_info(self: any) -> any:
        return self._job_info

    @property
    def device_id(self: any) -> any:
        return self._device_id

    @property
    def rank_id(self: any) -> any:
        return self._rank_id

    @property
    def dir_name(self: any) -> any:
        return self._dir_name

    @job_info.setter
    def job_info(self: any, value: any) -> None:
        self._job_info = value

    @device_id.setter
    def device_id(self: any, value: any) -> None:
        self._device_id = value

    @rank_id.setter
    def rank_id(self: any, value: any) -> None:
        self._rank_id = value

    @dir_name.setter
    def dir_name(self: any, value: any) -> None:
        self._dir_name = value
