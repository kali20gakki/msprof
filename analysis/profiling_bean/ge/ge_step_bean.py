#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

import struct

from common_func.constant import Constant
from common_func.utils import Utils
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.struct_info.struct_decoder import StructDecoder


class GeStepBean(StructDecoder):
    """
    ge tensor info bean
    """

    def __init__(self: any) -> None:
        self._fusion_data = ()
        self._data_tag = Constant.DEFAULT_VALUE
        self._model_id = Constant.DEFAULT_VALUE
        self._stream_id = Constant.DEFAULT_VALUE
        self._task_id = Constant.DEFAULT_VALUE
        self._batch_id = Constant.DEFAULT_VALUE
        self._timestamp = Constant.DEFAULT_VALUE
        self._index_num = Constant.DEFAULT_VALUE
        self._thread_id = Constant.DEFAULT_VALUE
        self._tag = Constant.DEFAULT_VALUE

    @property
    def model_id(self: any) -> int:
        """
        for model  id
        """
        return self._model_id

    @property
    def stream_id(self: any) -> int:
        """
        for stream id
        """
        return self._stream_id

    @property
    def task_id(self: any) -> int:
        """
        for task id
        """
        return self._task_id

    @property
    def thread_id(self: any) -> int:
        """
        for thread id
        """
        return self._thread_id

    @property
    def batch_id(self: any) -> int:
        """
        for batch id
        """
        return self._batch_id

    @property
    def timestamp(self: any) -> int:
        """
        for timestamp
        """
        return self._timestamp

    @property
    def index_num(self: any) -> int:
        """
        for index num
        """
        return self._index_num

    @property
    def tag(self: any) -> int:
        """
        for tag
        """
        return self._tag

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
        self._model_id = self._fusion_data[2]
        self._stream_id = Utils.get_stream_id(self._fusion_data[3])
        self._task_id = self._fusion_data[4]
        self._batch_id = self._fusion_data[5]
        self._timestamp = self._fusion_data[6]
        self._index_num = self._fusion_data[7]
        self._thread_id = self._fusion_data[8]
        self._tag = self._fusion_data[9]
