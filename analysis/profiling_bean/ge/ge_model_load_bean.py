# coding:utf-8
"""
This script is used for hwts log struct
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""
import struct

from common_func.constant import Constant
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.struct_info.struct_decoder import StructDecoder


class GeModelLoadBean(StructDecoder):
    """
    ge fusion info bean
    """

    def __init__(self: any) -> None:
        self._fusion_data = ()
        self._model_id = Constant.DEFAULT_VALUE
        self._model_name = Constant.DEFAULT_VALUE
        self._start_time = Constant.DEFAULT_VALUE
        self._end_time = Constant.DEFAULT_VALUE
        self._hash_flag = Constant.NO_HASH_DICT_FLAG

    def fusion_decode(self: any, binary_data: bytes) -> any:
        """
        decode fusion info binary data
        :param binary_data:
        :return:
        """
        pre_fusion_data = struct.unpack_from(StructFmt.BYTE_ORDER_CHAR + StructFmt.GE_FUSION_PRE_FMT,
                                             binary_data[:StructFmt.GE_FUSION_PRE_SIZE])
        self._hash_flag = pre_fusion_data[3]
        fmt = "HHI8B120sQQ104B"
        if self.is_hash_type():
            fmt = "HHI8BQ112BQQ104B"
        self.construct_bean(struct.unpack_from(fmt, binary_data))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the acl data
        :param args: acl bin data
        :return: True or False
        """
        self._fusion_data = args[0]
        self._model_id = self._fusion_data[2]
        self._model_name = self._fusion_data[11]
        self._start_time = self._fusion_data[12]
        self._end_time = self._fusion_data[13]
        if self.is_hash_type():
            self._model_name = ''.join(str(self._fusion_data[11], encoding='utf-8')).replace('\x00', '')
            self._start_time = self._fusion_data[124]
            self._start_time = self._fusion_data[125]

    @property
    def hash_flag(self: any) -> int:
        """
        for hash flag
        """
        return self._hash_flag

    @property
    def model_id(self: any) -> int:
        """
        for model id
        """
        return self._model_id

    @property
    def model_name(self: any) -> str:
        """
        for model name
        """
        return self._model_name

    @property
    def start_time(self: any) -> int:
        """
        for start time
        """
        return self._start_time

    @property
    def end_time(self: any) -> int:
        """
        for end time
        """
        return self._end_time

    def is_hash_type(self: any) -> bool:
        """
        return is hash type
        """
        return self.hash_flag == Constant.HASH_DICT_FLAG
