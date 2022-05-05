# coding:utf-8
"""
This script is used for hwts log struct
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""
import struct

from common_func.constant import Constant
from msparser.data_struct_size_constant import StructFmt
from profiling_bean.struct_info.struct_decoder import StructDecoder


class GeModelTimeBean(StructDecoder):
    """
    ge model time bean
    """

    def __init__(self: any) -> None:
        self._fusion_data = ()
        self._data_tag = Constant.DEFAULT_VALUE
        self._model_id = Constant.DEFAULT_VALUE
        self._model_name = Constant.DEFAULT_VALUE
        self._request_id = Constant.DEFAULT_VALUE
        self._thread_id = Constant.DEFAULT_VALUE
        self._input_data_start_time = Constant.DEFAULT_VALUE
        self._input_data_end_time = Constant.DEFAULT_VALUE
        self._infer_start_time = Constant.DEFAULT_VALUE
        self._infer_end_time = Constant.DEFAULT_VALUE
        self._output_data_start_time = Constant.DEFAULT_VALUE
        self._output_data_end_time = Constant.DEFAULT_VALUE
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
        fmt = "HHI8B120sII6Q64B"
        if self.is_hash_type():
            fmt = "HHI8BQ112BQII6Q64B"
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
        self._request_id = self._fusion_data[12]
        self._infer_end_time = self._fusion_data[17]
        self._thread_id = self._fusion_data[13]
        self._input_data_start_time = self._fusion_data[14]
        self._input_data_end_time = self._fusion_data[15]
        self._infer_start_time = self._fusion_data[16]
        self._output_data_start_time = self._fusion_data[18]
        self._output_data_end_time = self._fusion_data[19]
        if self.is_hash_type():
            self._model_name = ''.join(str(self._fusion_data[11], encoding='utf-8')).replace('\x00', '')
            self._request_id = self._fusion_data[124]
            self._thread_id = self._fusion_data[125]
            self._input_data_start_time = self._fusion_data[126]
            self._input_data_end_time = self._fusion_data[127]
            self._infer_start_time = self._fusion_data[128]
            self._infer_end_time = self._fusion_data[129]
            self._output_data_start_time = self._fusion_data[130]
            self._output_data_end_time = self._fusion_data[131]

    @property
    def model_name(self: any) -> str:
        """
        for model name
        """
        return self._model_name

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
    def request_id(self: any) -> int:
        """
        for request id
        """
        return self._request_id

    @property
    def thread_id(self: any) -> int:
        """
        for thread id
        """
        return self._thread_id

    @property
    def input_data_start_time(self: any) -> int:
        """
        for input data start time
        """
        return self._input_data_start_time

    @property
    def input_data_end_time(self: any) -> int:
        """
        for input data end time
        """
        return self._input_data_end_time

    @property
    def infer_start_time(self: any) -> int:
        """
        for infer start time
        """
        return self._infer_start_time

    @property
    def infer_end_time(self: any) -> int:
        """
        for infer end time
        """
        return self._infer_end_time

    @property
    def output_data_start_time(self: any) -> int:
        """
        for output data start time
        """
        return self._output_data_start_time

    @property
    def output_data_end_time(self: any) -> int:
        """
        for output data end time
        """
        return self._output_data_end_time

    def is_hash_type(self: any) -> bool:
        """
        check hwts log type ,and 0 is start log ,1 is end log.
        :return:
        """
        return self._hash_flag == Constant.HASH_DICT_FLAG
