#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
import logging
import struct

from common_func.ms_constant.ge_enum_constant import GeDataType
from common_func.ms_constant.ge_enum_constant import GeDataFormat
from common_func.ms_constant.ge_enum_constant import GeTaskType
from profiling_bean.struct_info.struct_decoder import StructDecoder
from msparser.data_struct_size_constant import StructFmt

HEAD_FMT = "=II"
NAME_FMT = "s"
SHAPE_FMT = "Q"
NUM_FMT = "=I"
L1_OP_DESC_FMT = "=IIIIIBI"
L2_INPUT_DESC_FMT = "=IIIQQQI"
L2_OUTPUT_DESC_FMT = "=IIIIIIQQQI"

DATA_TYPE_DICT = GeDataType.member_map()
FORMAT_DICT = GeDataFormat.member_map()
TASK_TYPE_DICT = GeTaskType.member_map()


class MagicHeadBean(StructDecoder):
    """
    dbg magic head bean data for the dbg data.
    """

    def __init__(self: any) -> None:
        self._magic_num = -1
        self._fmt_size = struct.calcsize(HEAD_FMT)

    @property
    def magic_num(self: any) -> int:
        """
        head data type
        """
        return self._magic_num

    @property
    def fmt_size(self: any) -> int:
        """
        type head's fmt size
        """
        return self._fmt_size

    def decode(self: any, data: bytes, offset: int):
        """
        decode the head data
        """
        self.construct_bean(struct.unpack(HEAD_FMT, data[offset:offset + self._fmt_size]))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the head data
        """
        field = args[0]
        self._magic_num = field[1]


class TypeHeadBean(StructDecoder):
    """
    dbg type head bean data for the dbg data.
    """

    def __init__(self: any) -> None:
        self._type = -1
        self._len = 0
        self._fmt_size = struct.calcsize(HEAD_FMT)

    @property
    def type(self: any) -> int:
        """
        head data type
        """
        return self._type

    @property
    def len(self: any) -> int:
        """
        head data len
        """
        return self._len

    @property
    def fmt_size(self: any) -> int:
        """
        type head's fmt size
        """
        return self._fmt_size

    def decode(self: any, data: bytes, offset: int):
        """
        decode the head data
        """
        self.construct_bean(struct.unpack(HEAD_FMT, data[offset:offset + self._fmt_size]))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the head data
        """
        field = args[0]
        self._type = field[0]
        self._len = field[1]


class NameBean(StructDecoder):
    """
    dbg name bean data for the dbg data.
    """

    def __init__(self: any) -> None:
        self._name = ""

    @property
    def name(self: any) -> str:
        """
        name data in dbg
        """
        return self._name

    def decode(self: any, data: bytes, offset: int, type_head: TypeHeadBean):
        """
        decode the name data by TyptHeadBean's len
        """
        self.construct_bean(struct.unpack(
            StructFmt.BYTE_ORDER_CHAR + str(type_head.len) + NAME_FMT, data[offset:offset + type_head.len]))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the name data
        """
        field = args[0]
        self._name = field[0].decode()


class ShapeBean(StructDecoder):
    """
    dbg shape bean data for the dbg data.
    """

    def __init__(self: any) -> None:
        self._shape = ""
        self._count = 0

    @property
    def shape(self: any) -> str:
        """
        shape data in input/output
        """
        return self._shape

    def decode(self: any, data: bytes, offset: int, type_head: TypeHeadBean):
        """
        decode the shape bin data by TyptHeadBean's len
        """
        self._count = type_head.len // struct.calcsize(SHAPE_FMT)
        self.construct_bean(struct.unpack(
            StructFmt.BYTE_ORDER_CHAR + str(self._count) + SHAPE_FMT,
            data[offset:offset + self._count * struct.calcsize(SHAPE_FMT)]))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the shape data
        """
        field = args[0]
        for index in range(self._count - 1):
            self._shape += str(field[index]) + ","
        self._shape += str(field[self._count - 1])


class NumBean(StructDecoder):
    """
    dbg num bean data for the dbg data.
    """

    def __init__(self: any) -> None:
        self._num = -1
        self._fmt_size = struct.calcsize(NUM_FMT)

    @property
    def num(self: any) -> int:
        """
        num data
        """
        return self._num

    @property
    def fmt_size(self: any) -> int:
        """
        num fmt size
        """
        return self._fmt_size

    def decode(self: any, data: bytes, offset: int):
        """
        decode the num bin data
        """
        self.construct_bean(struct.unpack(NUM_FMT, data[offset:offset + self._fmt_size]))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the num data
        """
        field = args[0]
        self._num = field[0]


class L1OpDescBean(StructDecoder):
    """
    dbg op description bean data for the dbg data.
    """

    def __init__(self: any) -> None:
        self._task_id = -1
        self._stream_id = -1
        self._task_type = -1
        self._block_dim = -1
        self._len = 0
        self._fmt_size = struct.calcsize(L1_OP_DESC_FMT)

    @property
    def task_id(self: any) -> int:
        """
        op task_id data
        :return: task_id
        """
        return self._task_id

    @property
    def stream_id(self: any) -> int:
        """
        op stream_id data
        :return: stream_id
        """
        return self._stream_id

    @property
    def task_type(self: any) -> str:
        """
        op task_type data
        :return: task_type
        """
        if self._task_type not in TASK_TYPE_DICT:
            logging.error("Unsupported task_type %d", self._task_type)
            return str(self._task_type)
        return TASK_TYPE_DICT.get(self._task_type).name

    @property
    def block_dim(self: any) -> int:
        """
        op block_dim data
        :return: block_dim
        """
        return self._block_dim

    @property
    def len(self: any) -> int:
        """
        op description data's other info's len
        """
        return self._len

    @property
    def fmt_size(self: any) -> int:
        """
        op description's fmt size
        """
        return self._fmt_size

    def decode(self: any, data: bytes, offset: int):
        """
        decode the op description bin data
        """
        self.construct_bean(struct.unpack(L1_OP_DESC_FMT, data[offset:offset + self._fmt_size]))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh op description data
        """
        field = args[0]
        self._task_id = field[0]
        self._stream_id = field[1]
        self._task_type = field[3]
        self._block_dim = field[4]
        self._len = field[6]


class L2InputDescBean(StructDecoder):
    """
    dbg op input info bean data for the dbg data.
    """

    def __init__(self: any) -> None:
        self._data_type = -1
        self._format = -1
        self._len = 0
        self._fmt_size = struct.calcsize(L2_INPUT_DESC_FMT)

    @property
    def data_type(self: any) -> str:
        """
        intput data type
        """
        if self._data_type not in DATA_TYPE_DICT:
            logging.error("Unsupported data_type %d", self._data_type)
            return str(self._data_type)
        return DATA_TYPE_DICT.get(self._data_type).name

    @property
    def format(self: any) -> str:
        """
        intput data format
        """
        if self._format not in FORMAT_DICT:
            logging.error("Unsupported tensor format %d", self._format)
            return str(self._format)
        return FORMAT_DICT.get(self._format).name

    @property
    def len(self: any) -> int:
        """
        input other info's len
        """
        return self._len

    @property
    def fmt_size(self: any) -> int:
        """
        input's fmt size
        """
        return self._fmt_size

    def decode(self: any, data: bytes, offset: int):
        """
        decode the input bin data
        """
        self.construct_bean(struct.unpack(L2_INPUT_DESC_FMT, data[offset:offset + self._fmt_size]))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the input data
        """
        field = args[0]
        self._data_type = field[0]
        self._format = field[1]
        self._len = field[6]


class L2OutputDescBean(StructDecoder):
    """
    dbg op output info bean data for the dbg data.
    """

    def __init__(self: any) -> None:
        self._data_type = -1
        self._format = -1
        self._len = 0
        self._fmt_size = struct.calcsize(L2_OUTPUT_DESC_FMT)

    @property
    def data_type(self: any) -> str:
        """
        output data type
        """
        if self._data_type not in DATA_TYPE_DICT:
            logging.error("Unsupported data_type %d", self._data_type)
            return str(self._data_type)
        return DATA_TYPE_DICT.get(self._data_type).name

    @property
    def format(self: any) -> str:
        """
        output data format
        """
        if self._format not in FORMAT_DICT:
            logging.error("Unsupported tensor format %d", self._format)
            return str(self._format)
        return FORMAT_DICT.get(self._format).name

    @property
    def len(self: any) -> int:
        """
        output other info's len
        """
        return self._len

    @property
    def fmt_size(self: any) -> int:
        """
        output's fmt size
        """
        return self._fmt_size

    def decode(self: any, data: bytes, offset: int):
        """
        decode the output bin data
        """
        self.construct_bean(struct.unpack(L2_OUTPUT_DESC_FMT, data[offset:offset + self._fmt_size]))
        return self

    def construct_bean(self: any, *args: any) -> None:
        """
        refresh the output data
        """
        field = args[0]
        self._data_type = field[0]
        self._format = field[1]
        self._len = field[9]
