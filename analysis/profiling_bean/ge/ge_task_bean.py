#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

import logging
import struct

from common_func.constant import Constant
from common_func.empty_class import EmptyClass
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.struct_info.struct_decoder import StructDecoder


class GeTaskBean(StructDecoder):
    """
    ge fusion info bean
    """

    ALL_HASH_ENUM = 0
    OP_NAME_HASH_ENUM = 1
    OP_TYPE_HASH_ENUM = 2
    NO_HASH_ENUM = 3

    TASK_TYPE_DICT = {
        0: Constant.TASK_TYPE_AI_CORE,
        1: Constant.TASK_TYPE_AI_CPU,
        2: Constant.TASK_TYPE_AIV,
        3: Constant.TASK_TYPE_WRITE_BACK,
        4: Constant.TASK_TYPE_MIX_AIC,
        5: Constant.TASK_TYPE_MIX_AIV,
        6: Constant.TASK_TYPE_FFTS_PLUS,
        7: Constant.TASK_TYPE_DSA,
        8: Constant.TASK_TYPE_DVPP,
        9: Constant.TASK_TYPE_HCCL,
        10: Constant.TASK_TYPE_INVALID
    }

    FMT_MAP = {
        NO_HASH_ENUM: {"fmt": "HHI8B120s8B56sQQ4IHHII12B", "function": "_update_task_data_by_no_hash"},
        OP_TYPE_HASH_ENUM: {"fmt": "HHI8B120s8BQ48BQQ4IHHII12B", "function": "_update_task_data_by_hash_type"},
        OP_NAME_HASH_ENUM: {"fmt": "HHI8BQ112B8B56sQQ4IHHII12B", "function": "_update_task_data_by_hash_name"},
        ALL_HASH_ENUM: {"fmt": "HHI8BQ112B8BQ48BQQ4IHHII12B", "function": "_update_task_data_by_all_hash"}
    }

    def __init__(self: any) -> None:
        self._fusion_data = ()
        self._task_type = Constant.DEFAULT_VALUE
        self._op_name = Constant.DEFAULT_VALUE
        self._op_type = Constant.DEFAULT_VALUE
        self._index_num = Constant.DEFAULT_VALUE
        self._timestamp = Constant.DEFAULT_VALUE
        self._shape_type = Constant.DEFAULT_VALUE
        self._block_dim = Constant.DEFAULT_VALUE
        self._mix_block_dim = Constant.DEFAULT_VALUE
        self._model_id = Constant.DEFAULT_VALUE
        self._stream_id = Constant.DEFAULT_VALUE
        self._task_id = Constant.DEFAULT_VALUE
        self._batch_id = Constant.DEFAULT_VALUE
        self._thread_id = Constant.DEFAULT_VALUE
        self._op_name_hash_flag = Constant.NO_HASH_DICT_FLAG
        self._op_type_hash_flag = Constant.NO_HASH_DICT_FLAG
        self._context_id = Constant.DEFAULT_VALUE

    @property
    def task_type(self: any) -> int:
        """
        for task type
        """
        return self.TASK_TYPE_DICT.get(self._task_type, self._task_type)

    @property
    def op_name(self: any) -> str:
        """
        for op name
        """
        return self._op_name

    @property
    def op_type(self: any) -> int:
        """
        for op type
        """
        return self._op_type

    @property
    def index_num(self: any) -> int:
        """
        for index num
        """
        return self._index_num

    @property
    def timestamp(self: any) -> str:
        """
        for time stamp
        """
        return str(self._timestamp)

    @property
    def shape_type(self: any) -> int:
        """
        for shape type
        """
        return self._shape_type

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
    def model_id(self: any) -> int:
        """
        for model id
        """
        return self._model_id

    @property
    def thread_id(self: any) -> int:
        """
        for thread id
        """
        return self._thread_id

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
    def batch_id(self: any) -> int:
        """
        for batch id
        """
        return self._batch_id

    @property
    def context_id(self: any) -> int:
        """
        for context id
        """
        return self._context_id

    def is_op_name_hash_type(self: any) -> bool:
        """
        check hwts log type ,and 0 is start log ,1 is end log.
        :return:
        """
        return self._op_name_hash_flag == Constant.HASH_DICT_FLAG

    def is_op_type_hash_type(self: any) -> bool:
        """
        check hwts log type ,and 0 is start log ,1 is end log.
        :return:
        """
        return self._op_type_hash_flag == Constant.HASH_DICT_FLAG

    def fusion_decode(self: any, binary_data: bytes) -> any:
        """
        decode fusion info binary data
        :param binary_data:
        :return:
        """
        op_name_hash_index = 9
        op_type_hash_index = 137
        self._op_name_hash_flag = struct.unpack_from(StructFmt.BYTE_ORDER_CHAR + "B",
                                                     binary_data[(op_name_hash_index - 1): op_name_hash_index])[0]
        self._op_type_hash_flag = struct.unpack_from(StructFmt.BYTE_ORDER_CHAR + "B",
                                                     binary_data[(op_type_hash_index - 1):op_type_hash_index])[0]
        fmt_key = "{}{}".format(self._op_name_hash_flag, self._op_type_hash_flag)
        decode_info = self.FMT_MAP.get(int(fmt_key, 2), None)
        if not decode_info:
            logging.error("Ge task data struct is invalid, please check the ge task file.")
            return EmptyClass()
        self.construct_bean(struct.unpack_from(decode_info.get("fmt"), binary_data), decode_info.get("function", None))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the acl data
        :param args: acl bin data
        :return: True or False
        """
        self._fusion_data = args[0]
        if hasattr(self, args[1]):
            getattr(self, args[1])()

    def _update_task_data_by_no_hash(self: any) -> None:
        self._task_type = self._fusion_data[2]
        self._op_name = self._fusion_data[11]
        self._op_type = self._fusion_data[20]
        self._index_num = self._fusion_data[21]
        self._timestamp = self._fusion_data[22]
        self._shape_type = self._fusion_data[23]
        self._block_dim = self._fusion_data[24]
        self._model_id = self._fusion_data[25]
        self._stream_id = self._fusion_data[26]
        self._task_id = self._fusion_data[27]
        self._batch_id = self._fusion_data[28]
        self._thread_id = self._fusion_data[29]
        self._context_id = self._fusion_data[30]

    def _update_task_data_by_hash_type(self: any) -> None:
        self._task_type = self._fusion_data[2]
        self._op_name = self._fusion_data[11]
        self._op_type = self._fusion_data[20]
        self._index_num = self._fusion_data[69]
        self._timestamp = self._fusion_data[70]
        self._shape_type = self._fusion_data[71]
        self._block_dim = self._fusion_data[72]
        self._model_id = self._fusion_data[72]
        self._stream_id = self._fusion_data[74]
        self._task_id = self._fusion_data[75]
        self._batch_id = self._fusion_data[76]
        self._thread_id = self._fusion_data[77]
        self._context_id = self._fusion_data[78]

    def _update_task_data_by_hash_name(self: any) -> None:
        self._task_type = self._fusion_data[2]
        self._op_name = self._fusion_data[11]
        self._op_type = self._fusion_data[132]
        self._index_num = self._fusion_data[133]
        self._timestamp = self._fusion_data[134]
        self._shape_type = self._fusion_data[135]
        self._block_dim = self._fusion_data[136]
        self._model_id = self._fusion_data[137]
        self._stream_id = self._fusion_data[138]
        self._task_id = self._fusion_data[139]
        self._batch_id = self._fusion_data[140]
        self._thread_id = self._fusion_data[141]
        self._context_id = self._fusion_data[142]

    def _update_task_data_by_all_hash(self: any) -> None:
        self._task_type = self._fusion_data[2]
        self._op_name = self._fusion_data[11]
        self._op_type = self._fusion_data[132]
        self._index_num = self._fusion_data[181]
        self._timestamp = self._fusion_data[182]
        self._shape_type = self._fusion_data[183]
        self._block_dim = self._fusion_data[184]
        self._model_id = self._fusion_data[185]
        self._stream_id = self._fusion_data[186]
        self._task_id = self._fusion_data[187]
        self._batch_id = self._fusion_data[188]
        self._thread_id = self._fusion_data[189]
        self._context_id = self._fusion_data[190]
