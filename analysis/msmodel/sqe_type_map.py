# coding=utf-8
"""
This script is used to establish sqetype map
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

from enum import Enum
from enum import unique


@unique
class SqeType(Enum):
    """
    establish sqe_type map
    """
    FFTS_SQE = 0
    CPU_SQE = 1
    AIV_SQE = 2
    PLACE_HOLDER_SQE = 3
    EVENT_RECORD_SQE = 4
    EVENT_WAIT_SQE = 5
    NOTIFY_RECORD_SQE = 6
    NOTIFY_WAIT_SQE = 7
    WRITE_VALUE_SQE = 8
    SDMA_SQE = 11
    VPC_SQE = 12
    JPEGE_SQE = 13
    DSA_SQE = 14
    RoCCE_SQE = 15
    PCIE_DMA_SQE = 16
    HOST_CPU_SQE = 17
    CDQM_SQE = 18
    C_CORE_SQE = 19
