#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import struct

from common_func.constant import Constant
from common_func.ms_constant.level_type_constant import LevelDataType
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.ge.ge_tensor_base_bean import GeTensorBaseBean


class TensorAddInfoBean(GeTensorBaseBean):
    """
    ge tensor info bean
    """
    TENSOR_LEN = 11
    TENSOR_PER_LEN = 8

    def __init__(self: any) -> None:
        super().__init__()
        self._fusion_data = ()
        self._magic_num = Constant.DEFAULT_VALUE
        self._level = Constant.DEFAULT_VALUE
        self._add_info_type = Constant.DEFAULT_VALUE
        self._thread_id = Constant.DEFAULT_VALUE
        self._data_len = Constant.DEFAULT_VALUE
        self._timestamp = Constant.DEFAULT_VALUE
        self._node_id = Constant.DEFAULT_VALUE
        self._tensor_num = Constant.DEFAULT_VALUE

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
    def timestamp(self: any) -> str:
        """
        for timestamp
        """
        return str(self._timestamp)

    @property
    def tensor_num(self: any) -> int:
        """
        for tensor num
        """
        return self._tensor_num

    @property
    def thread_id(self: any) -> str:
        """
        for thread id
        """
        return str(self._thread_id)

    @property
    def node_id(self: any) -> str:
        """
        for node id
        """
        return str(self._node_id)

    def fusion_decode(self: any, binary_data: bytes) -> any:
        """
        decode ge tensor binary data
        :param binary_data:
        :return:
        """
        fmt = StructFmt.BYTE_ORDER_CHAR + self.get_fmt()
        self.construct_bean(struct.unpack_from(fmt, binary_data))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the ge tensor data
        :param args: ge tensor bean data
        :return:
        """
        self._fusion_data = args[0]
        self._magic_num = self._fusion_data[0]
        self._level = self._fusion_data[1]
        self._add_info_type = self._fusion_data[2]
        self._thread_id = self._fusion_data[3]
        self._data_len = self._fusion_data[4]
        self._timestamp = self._fusion_data[5]
        self._node_id = self._fusion_data[6]
        self._tensor_num = self._fusion_data[7]
        _tensor_datas = []
        for tensor_index in range(0, self._tensor_num):
            _tensor_datas.append(
                list(self._fusion_data[self.TENSOR_LEN * tensor_index + self.TENSOR_PER_LEN:
                                       self.TENSOR_LEN * tensor_index + (self.TENSOR_LEN + self.TENSOR_PER_LEN)]))
        for _tensor_data in _tensor_datas:
            # 0 input 1 output
            if _tensor_data[0] == 0:
                self._input_format.append(_tensor_data[1])
                self._input_data_type.append(_tensor_data[2])
                self._input_shape.append(_tensor_data[3:])
            if _tensor_data[0] == 1:
                self._output_format.append(_tensor_data[1])
                self._output_data_type.append(_tensor_data[2])
                self._output_shape.append(_tensor_data[3:])
