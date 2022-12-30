#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from common_func.constant import Constant
from common_func.info_conf_reader import InfoConfReader
from common_func.ms_constant.number_constant import NumberConstant
from common_func.ms_constant.str_constant import StrConstant


class HCCLData:
    """
    HCCL data parser.
    """

    def __init__(self: any, hccl_data: dict) -> None:
        self._hccl_data = hccl_data
        self._hccl_name = Constant.NA
        self._plane_id = -1
        self._timestamp = -1
        self._duration = -1
        self._args = '{}'

    @property
    def hccl_name(self: any) -> str:
        """
        hccl name
        :return: hccl name
        """
        return self._hccl_data.get('name', self._hccl_name)

    @property
    def plane_id(self: any) -> str:
        """
        hccl plane id
        :return: hccl plane id
        """
        return self._hccl_data.get('tid', self._plane_id)

    @property
    def timestamp(self: any) -> float:
        """
        hccl timestamp
        :return: hccl timestamp
        """
        return InfoConfReader().time_from_syscnt(
            self._hccl_data.get('ts', self._timestamp) * InfoConfReader().get_freq(StrConstant.HWTS) / 1000000.0,
            NumberConstant.MICRO_SECOND)

    @property
    def duration(self: any) -> int:
        """
        hccl duration
        :return: hccl duration
        """
        return self._hccl_data.get('dur', self._duration)

    @property
    def args(self: any) -> str:
        """
        hccl args
        :return: hccl args
        """
        return str(self._hccl_data.get("args", self._args))
