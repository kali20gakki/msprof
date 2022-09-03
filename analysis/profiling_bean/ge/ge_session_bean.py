#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

import struct

from common_func.constant import Constant
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.struct_info.struct_decoder import StructDecoder


class GeSessionInfoBean(StructDecoder):
    """
    ge session info bean
    """

    def __init__(self: any) -> None:
        self._fusion_data = ()
        self._data_tag = Constant.DEFAULT_VALUE
        self._graph_id = Constant.DEFAULT_VALUE
        self._model_id = Constant.DEFAULT_VALUE
        self._session_id = Constant.DEFAULT_VALUE
        self._timestamp = Constant.DEFAULT_VALUE
        self._mod = Constant.DEFAULT_VALUE

    @property
    def model_id(self: any) -> int:
        """
        for model id
        """
        return self._model_id

    @property
    def graph_id(self: any) -> int:
        """
        for graph id
        """
        return self._graph_id

    @property
    def session_id(self: any) -> int:
        """
        for session id
        """
        return self._session_id

    @property
    def mod(self: any) -> int:
        """
        for mod
        """
        return self._mod

    @property
    def timestamp(self: any) -> int:
        """
        for timestamp
        """
        return self._timestamp

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
        refresh the acl data
        :param args: acl bin data
        :return: True or False
        """
        self._fusion_data = args[0]
        self._data_tag = self._fusion_data[1]
        self._graph_id = self._fusion_data[2]
        self._mod = self._fusion_data[6]
        self._model_id = self._fusion_data[3]
        self._session_id = self._fusion_data[4]
        self._timestamp = self._fusion_data[5]
