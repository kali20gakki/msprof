#!/usr/bin/env python
# coding=utf-8
"""
function:lowpower sample bean struct
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
from common_func.info_conf_reader import InfoConfReader
from common_func.utils import Utils
from profiling_bean.struct_info.struct_decoder import StructDecoder


class LowPowerBean(StructDecoder):
    """
    bean for lowpower sample data
    """

    def __init__(self: any, *args) -> None:
        self.filed = args[0]
        self._func_type = Utils.get_func_type(self.filed[0])
        self._cnt = Utils.get_cnt(self.filed[0])
        self._data_tag = self.filed[1]
        self._sys_cnt = self.filed[3]
        self._lp_info = self.filed[4:]

    @property
    def func_type(self: any) -> str:
        """
        for func type
        """
        return self._func_type

    @property
    def sys_cnt(self: any) -> int:
        """
        for sys cnt
        """
        return self._sys_cnt

    @property
    def sys_time(self: any) -> float:
        """
        for sys time
        """
        return InfoConfReader().time_from_syscnt(self._sys_cnt)

    @property
    def lp_info(self: any) -> float:
        """
        lowpower sample info
        """
        return self._lp_info
