#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024. All rights reserved.

from common_func.info_conf_reader import InfoConfReader
from profiling_bean.struct_info.struct_decoder import StructDecoder


class StarsQosBean(StructDecoder):
    """
    bean for stars qos data
    """
    def __init__(self: any, *args) -> None:
        field = args[0]
        self._die_id = field[0] >> 10
        self._sys_cnt = field[3]
        self._qos_bw_data = field[5:]

    @property
    def die_id(self: any) -> int:
        return self._die_id

    @property
    def timestamp(self: any) -> float:
        """
        for sys time
        """
        return InfoConfReader().time_from_syscnt(self._sys_cnt)

    @property
    def qos_bw_data(self: any) -> list:
        return list(self._qos_bw_data)
