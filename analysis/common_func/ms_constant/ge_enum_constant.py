#!/usr/bin/python3
"""
This scripts is used to provide system constants
Copyright Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
"""
from enum import Enum
from enum import unique


@unique
class GeDataType(Enum):
    """
    DataType enum for ge graph operator data
    """
    FLOAT = 0
    FLOAT16 = 1
    INT8 = 2
    INT32 = 3
    UINT8 = 4
    INT16 = 6
    UINT16 = 7
    UINT32 = 8
    INT64 = 9
    UINT64 = 10
    DOUBLE = 11
    BOOL = 12
    STRING = 13
    DUAL_SUB_INT8 = 14
    DUAL_SUB_UINT8 = 15
    COMPLEX64 = 16
    COMPLEX128 = 17
    QINT8 = 18
    QINT16 = 19
    QINT32 = 20
    QUINT8 = 21
    QUINT16 = 22
    RESOURCE = 23
    STRING_REF = 24
    DUAL = 25
    DT_VARIANT = 26
    DT_BF16 = 27
    UNDEFINED = 28
    DT_INT4 = 29
    DT_UINT1 = 30
    DT_INT2 = 31
    DT_UINT2 = 32
    DT_MAX = 33

    @classmethod
    def member_map(cls: any) -> dict:
        """
        enum map for DataFormat value and data format member
        :return:
        """
        return cls._value2member_map_


@unique
class GeDataFormat(Enum):
    """
    DataFormat enum for ge graph operator data
    """
    NCHW = 0
    NHWC = 1
    FORMAT_ND = 2
    NC1HWC0 = 3
    FRACTAL_Z = 4
    NC1C0HWPAD = 5
    NHWC1C0 = 6
    FSR_NCHW = 7
    FRACTAL_DECONV = 8
    C1HWNC0 = 9
    FRACTAL_DECONV_TRANSPOSE = 10
    FRACTAL_DECONV_SP_STRIDE_TRANS = 11
    NC1HWC0_C04 = 12
    FRACTAL_Z_C04 = 13
    CHWN = 14
    FRACTAL_DECONV_SP_STRIDE8_TRANS = 15
    HWCN = 16
    NC1KHKWHWC0 = 17
    BN_WEIGHT = 18
    FILTER_HWCK = 19
    HASHTABLE_LOOKUP_LOOKUPS = 20
    HASHTABLE_LOOKUP_KEYS = 21
    HASHTABLE_LOOKUP_VALUE = 22
    HASHTABLE_LOOKUP_OUTPUT = 23
    HASHTABLE_LOOKUP_HITS = 24
    C1HWNCoC0 = 25
    FORMAT_MD = 26
    NDHWC = 27
    FRACTAL_ZZ = 28
    FRACTAL_NZ = 29
    NCDHW = 30
    DHWCN = 31
    NDC1HWC0 = 32
    FRACTAL_Z_3D = 33
    FORMAT_CN = 34
    FORMAT_NC = 35
    DHWNC = 36
    FRACTAL_Z_3D_TRANSPOSE = 37
    FRACTAL_ZN_LSTM = 38
    FRACTAL_Z_G = 39
    RESERVED = 40
    ALL = 41
    NULL = 42
    ND_RNN_BIAS = 43
    FRACTAL_ZN_RNN = 44
    END = 45
    MAX = 0xff

    @classmethod
    def member_map(cls: any) -> dict:
        """
        enum map for DataFormat value and data format member
        :return:
        """
        return cls._value2member_map_
