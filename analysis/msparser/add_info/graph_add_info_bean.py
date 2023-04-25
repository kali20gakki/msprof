#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

import struct

from common_func.constant import Constant
from common_func.ms_constant.level_type_constant import LevelDataType
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.struct_info.struct_decoder import StructDecoder


class GraphAddInfoBean(StructDecoder):
    """
    Graph Add Info Bean
    """

    def __init__(self: any) -> None:
        self._fusion_data = ()
        self._magic_num = Constant.DEFAULT_VALUE
        self._level = Constant.DEFAULT_VALUE
        self._add_info_type = Constant.DEFAULT_VALUE
        self._thread_id = Constant.DEFAULT_VALUE
        self._data_len = Constant.DEFAULT_VALUE
        self._timestamp = Constant.DEFAULT_VALUE
        self._graph_id = Constant.DEFAULT_VALUE
        self._model_name = Constant.DEFAULT_VALUE

    @property
    def add_info_type(self: any) -> str:
        """
        for additional info type
        """
        return str(self._add_info_type)

    @property
    def level(self: any) -> str:
        """
        for level
        """
        return LevelDataType(self._level).name.lower()

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
    def graph_id(self: any) -> str:
        """
        for graph id
        """
        return str(self._graph_id)

    @property
    def model_name(self: any) -> str:
        """
        for model name
        """
        return str(self._model_name)

    def fusion_decode(self: any, binary_data: bytes) -> any:
        """
        decode ge graph data
        :param binary_data:
        :return:
        """
        fmt = StructFmt.BYTE_ORDER_CHAR + self.get_fmt()
        self.construct_bean(struct.unpack_from(fmt, binary_data))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the ge graph info data
        :param args: ge graph data
        :return: None
        """
        self._fusion_data = args[0]
        self._level = self._fusion_data[1]
        self._add_info_type = self._fusion_data[2]
        self._data_len = self._fusion_data[3]
        self._thread_id = self._fusion_data[4]
        self._timestamp = self._fusion_data[5]
        self._graph_id = self._fusion_data[6]
        self._model_name = self._fusion_data[7]
