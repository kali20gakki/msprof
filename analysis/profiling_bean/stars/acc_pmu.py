# coding=utf-8
"""
This script is used to parse accpmu data
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""
from common_func.utils import Utils
from profiling_bean.stars.stars_common import StarsCommon
from profiling_bean.struct_info.struct_decoder import StructDecoder


class AccPmuDecoder(StructDecoder):
    """
    class used to decode binary data
    """

    def __init__(self: any, *args: tuple) -> None:
        filed = args[0]
        self._func_type = Utils.get_func_type(filed[0])
        self._stars_common = StarsCommon(filed[3], filed[2], filed[4])
        self._block_id = filed[5]
        self._acc_id = filed[6]
        self._bandwidth = (filed[10], filed[11])
        self._ost = (filed[12], filed[13])

    @property
    def func_type(self: any) -> str:
        """
        get func type
        :return: func type
        """
        return self._func_type

    @property
    def block_id(self: any) -> int:
        """
        get task type
        :return: task type
        """
        return self._block_id

    @property
    def stars_common(self: any) -> object:
        """
        get task_id, stream id, sys_cnt
        :return: class object
        """
        return self._stars_common

    @property
    def bandwidth(self: any) -> tuple:
        """
        get write_bandwidth and read_bandwidth
        :return: (write_bandwidth， read_bandwidth)
        """
        return self._bandwidth

    @property
    def acc_id(self: any) -> int:
        """
        get acc id
        :return: acc_id
        """
        return self._acc_id

    @property
    def ost(self: any) -> tuple:
        """
        get write_ost and read_ost
        :return: (write_ost and read_ost)
        """
        return self._ost
