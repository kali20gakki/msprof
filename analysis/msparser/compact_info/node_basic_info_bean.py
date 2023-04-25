#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import struct

from common_func.constant import Constant
from common_func.ms_constant.level_type_constant import LevelDataType
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.ge.ge_task_bean import GeTaskBean
from profiling_bean.struct_info.struct_decoder import StructDecoder


class NodeBasicInfoBean(StructDecoder):
    """
    Node Basic Info Bean
    """

    def __init__(self: any) -> None:
        self._fusion_data = ()
        self._magic_num = Constant.DEFAULT_VALUE
        self._level = Constant.DEFAULT_VALUE
        self._add_info_type = Constant.DEFAULT_VALUE
        self._thread_id = Constant.DEFAULT_VALUE
        self._data_len = Constant.DEFAULT_VALUE
        self._timestamp = Constant.DEFAULT_VALUE
        self._node_id = Constant.DEFAULT_VALUE
        self._task_type = Constant.DEFAULT_VALUE
        self._op_type = Constant.DEFAULT_VALUE
        self._block_dim = Constant.DEFAULT_VALUE
        self._mix_block_dim = Constant.DEFAULT_VALUE
        self._op_flag = Constant.DEFAULT_VALUE

    @property
    def level(self: any) -> str:
        """
        for level
        """
        return LevelDataType(self._level).name.lower()

    @property
    def add_info_type(self: any) -> str:
        """
        for additional info type
        """
        return str(self._add_info_type)

    @property
    def task_type(self: any) -> str:
        """
        for task type
        """
        return GeTaskBean.TASK_TYPE_DICT.get(self._task_type, 'unmatched')

    @property
    def op_type(self: any) -> str:
        """
        for op type
        """
        return str(self._op_type)

    @property
    def node_id(self: any) -> str:
        """
        for node id
        """
        return str(self._node_id)

    @property
    def op_flag(self: any) -> int:
        """
        for op flag
        """
        return self._op_flag

    @property
    def timestamp(self: any) -> str:
        """
        for time stamp
        """
        return str(self._timestamp)

    @property
    def block_dim(self: any) -> int:
        """
        for block dims
        get lower 16bit data of 32bit
        """
        return self._block_dim & 65535

    @property
    def mix_block_dim(self: any) -> int:
        """
        for mix block dims
        get the product of block dim and higher 16bit
        """
        return (self._block_dim & 65535) * (self._block_dim >> 16)

    @property
    def thread_id(self: any) -> str:
        """
        for thread id
        """
        return str(self._thread_id)

    def fusion_decode(self: any, binary_data: bytes) -> any:
        """
        decode fusion info binary data
        :param binary_data:
        :return:
        """
        fmt = StructFmt.BYTE_ORDER_CHAR + self.get_fmt()
        self.construct_bean(struct.unpack_from(fmt, binary_data))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh data
        :param args: bin data
        :return: None
        """
        self._fusion_data = args[0]
        self._magic_num = self._fusion_data[0]
        self._level = self._fusion_data[1]
        self._add_info_type = self._fusion_data[2]
        self._thread_id = self._fusion_data[3]
        self._data_len = self._fusion_data[4]
        self._timestamp = self._fusion_data[5]
        self._node_id = self._fusion_data[6]
        self._task_type = self._fusion_data[7]
        self._op_type = self._fusion_data[8]
        self._block_dim = self._fusion_data[9]
        self._op_flag = self._fusion_data[10]
        self._mix_block_dim = self.mix_block_dim