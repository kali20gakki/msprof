# coding:utf-8
"""
This script is used for ge tensor struct
Copyright Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
"""
import struct

from common_func.constant import Constant
from common_func.ms_constant.ge_enum_constant import GeDataFormat
from common_func.ms_constant.ge_enum_constant import GeDataType
from common_func.utils import Utils
from profiling_bean.struct_info.struct_decoder import StructDecoder


class GeTensorBean(StructDecoder):
    """
    ge tensor info bean
    """
    TENSOR_LEN = 11
    TENSOR_PER_LEN = 8

    def __init__(self: any) -> None:
        self._fusion_data = ()
        self._data_tag = Constant.DEFAULT_VALUE
        self._model_id = Constant.DEFAULT_VALUE
        self._index_num = Constant.DEFAULT_VALUE
        self._stream_id = Constant.DEFAULT_VALUE
        self._task_id = Constant.DEFAULT_VALUE
        self._batch_id = Constant.DEFAULT_VALUE
        self._tensor_num = Constant.DEFAULT_VALUE
        self._tensor_type = Constant.DEFAULT_VALUE
        self._input_format = []
        self._input_data_type = []
        self._input_shape = []
        self._output_format = []
        self._output_data_type = []
        self._output_shape = []
        self._timestamp = Constant.DEFAULT_VALUE

    @property
    def model_id(self: any) -> int:
        """
        for model id
        """
        return self._model_id

    @property
    def index_num(self: any) -> int:
        """
        for task id
        """
        return self._index_num

    @property
    def batch_id(self: any) -> int:
        """
        for bacth id
        """
        return self._batch_id

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
    def tensor_num(self: any) -> int:
        """
        for tensor num
        """
        return self._tensor_num

    @property
    def tensor_type(self: any) -> int:
        """
        for tensor type
        """
        return self._tensor_type

    @property
    def input_format(self: any) -> str:
        """
        for input format
        """
        return ";".join(self._process_tensor_format(self._input_format))

    @property
    def input_data_type(self: any) -> str:
        """
        for input data type
        """

        return ";".join(self._process_tensor_data_type(self._input_data_type))

    @property
    def input_shape(self: any) -> str:
        """
        for input shape
        """
        input_shape = self._reshape_and_filter(self._input_shape, 0)
        return ";".join(input_shape)

    @property
    def output_format(self: any) -> str:
        """
        for output format
        """
        return ";".join(self._process_tensor_format(self._output_format))

    @property
    def output_data_type(self: any) -> str:
        """
        for output data type
        """
        return ";".join(self._process_tensor_data_type(self._output_data_type))

    @property
    def output_shape(self: any) -> str:
        """
        for output shape
        """
        output_shape = self._reshape_and_filter(self._output_shape, 0)
        return ";".join(output_shape)

    @property
    def timestamp(self: any) -> int:
        """
        for timestamp
        """
        return self._timestamp

    @staticmethod
    def _process_with_sub_format(tensor_format: int) -> tuple:
        """
        get the real tensor format and tensor sub format,
        real tensor_format need operate with 0xff when tensor sub format exist
        :param tensor_format:
        :return:
        """
        return tensor_format & 0xff, (tensor_format & 0xffff00) >> 8

    @staticmethod
    def _process_tensor_data_type(data_type: list) -> list:
        enum_dict = GeDataType.member_map()
        for index, _format in enumerate(data_type):
            enum_format = enum_dict.get(_format, GeDataType.UNDEFINED).name
            data_type[index] = enum_format
        return data_type

    @classmethod
    def _reshape_and_filter(cls: any, shape_data: list, filter_num: int) -> list:
        res_shape = []
        for single_shape in shape_data:
            _tmp_shape = []
            for _shape in single_shape:
                if _shape != filter_num:
                    _tmp_shape.append(str(_shape))
            res_shape.append(_tmp_shape)
        _res_shape_str_list = Utils.generator_to_list(",".join(i) for i in res_shape)
        return _res_shape_str_list

    def fusion_decode(self: any, binary_data: bytes) -> any:
        """
        decode ge tensor binary data
        :param binary_data:
        :return:
        """
        fmt = self.get_fmt()
        self.construct_bean(struct.unpack_from(fmt, binary_data))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the ge tensor data
        :param args: ge tensor bean data
        :return: True or False
        """
        self._fusion_data = args[0]
        self._data_tag = self._fusion_data[1]
        self._model_id = self._fusion_data[2]
        self._index_num = self._fusion_data[3]
        self._stream_id = self._fusion_data[4]
        self._task_id = self._fusion_data[5]
        self._batch_id = self._fusion_data[6]
        self._tensor_num = self._fusion_data[7]
        self._timestamp = self._fusion_data[63]

        # tensor data is 5, each tensor len is 11
        _tensor_datas = []
        for tensor_index in range(0, self._tensor_num):
            _tensor_datas.append(
                list(self._fusion_data[self.TENSOR_LEN * tensor_index + self.TENSOR_PER_LEN:
                                       self.TENSOR_LEN * tensor_index + (self.TENSOR_LEN + self.TENSOR_PER_LEN)]))
        for _tensor_data in _tensor_datas:
            if _tensor_data[0] == 0:
                self._input_format.append(_tensor_data[1])
                self._input_data_type.append(_tensor_data[2])
                self._input_shape.append(_tensor_data[3:])
            if _tensor_data[0] == 1:
                self._output_format.append(_tensor_data[1])
                self._output_data_type.append(_tensor_data[2])
                self._output_shape.append(_tensor_data[3:])

    def _process_tensor_format(self: any, _input_format) -> list:
        enum_dict = GeDataFormat.member_map()
        for index, _format in enumerate(_input_format):
            tensor_format, tensor_sub_format = self._process_with_sub_format(_format)
            if tensor_format not in enum_dict:
                continue
            enum_format = enum_dict.get(tensor_format).name
            if tensor_sub_format > 0:
                enum_format = enum_dict.get(tensor_format).name + ":" + str(tensor_sub_format)
            _input_format[index] = enum_format
        return _input_format
