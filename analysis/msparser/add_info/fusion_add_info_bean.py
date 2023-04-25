#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import struct

from common_func.constant import Constant
from common_func.ms_constant.level_type_constant import LevelDataType
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.struct_info.struct_decoder import StructDecoder


class FusionAddInfoBean(StructDecoder):
    """
    ge fusion info bean
    """
    FUSION_LEN = 13

    def __init__(self: any) -> None:
        self._fusion_data = ()
        self._magic_num = Constant.DEFAULT_VALUE
        self._level = Constant.DEFAULT_VALUE
        self._add_info_type = Constant.DEFAULT_VALUE
        self._thread_id = Constant.DEFAULT_VALUE
        self._data_len = Constant.DEFAULT_VALUE
        self._timestamp = Constant.DEFAULT_VALUE
        self._node_id = Constant.DEFAULT_VALUE
        self._fusion_op_num = Constant.DEFAULT_VALUE
        self._input_mem_size = Constant.DEFAULT_VALUE
        self._output_mem_size = Constant.DEFAULT_VALUE
        self._weight_mem_size = Constant.DEFAULT_VALUE
        self._workspace_mem_size = Constant.DEFAULT_VALUE
        self._total_mem_size = Constant.DEFAULT_VALUE
        self._fusion_op_id = []

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
    def thread_id(self: any) -> str:
        """
        for thread id
        """
        return str(self._thread_id)

    @property
    def timestamp(self: any) -> str:
        """
        for timestamp
        """
        return str(self._timestamp)

    @property
    def node_id(self: any) -> str:
        """
        for node id
        """
        return str(self._node_id)

    @property
    def fusion_op_num(self: any) -> int:
        """
        for fusion op num
        """
        return self._fusion_op_num

    @property
    def input_mem_size(self: any) -> str:
        """
        for input memory size
        """
        return str(self._input_mem_size)

    @property
    def output_mem_size(self: any) -> str:
        """
        for output memory size
        """
        return str(self._output_mem_size)

    @property
    def weight_mem_size(self: any) -> str:
        """
        for weight memory size
        """
        return str(self._weight_mem_size)

    @property
    def workspace_mem_size(self: any) -> str:
        """
        for workspace memory size
        """
        return str(self._workspace_mem_size)

    @property
    def total_mem_size(self: any) -> str:
        """
        for total memory size
        """
        return str(self._total_mem_size)

    @property
    def fusion_op_id(self: any) -> str:
        """
        for fusion op id
        """
        return ','.join(self._fusion_op_id)

    def fusion_decode(self: any, binary_data: bytes) -> any:
        """
        decode fusion info binary data
        :param binary_data:
        """
        fmt = StructFmt.BYTE_ORDER_CHAR + self.get_fmt()
        self.construct_bean(struct.unpack_from(fmt, binary_data))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh data
        :param args: bin data
        """
        self._fusion_data = args[0]
        self._level = self._fusion_data[1]
        self._add_info_type = self._fusion_data[2]
        self._thread_id = self._fusion_data[3]
        self._data_len = self._fusion_data[4]
        self._timestamp = self._fusion_data[5]
        self._node_id = self._fusion_data[6]
        self._fusion_op_num = self._fusion_data[7]
        self._input_mem_size = self._fusion_data[8]
        self._output_mem_size = self._fusion_data[9]
        self._weight_mem_size = self._fusion_data[10]
        self._workspace_mem_size = self._fusion_data[11]
        self._total_mem_size = self._fusion_data[12]
        for fusion_index in range(self.FUSION_LEN, self.FUSION_LEN + self._fusion_op_num):
            self._fusion_op_id.append(str(self._fusion_data[fusion_index]))
