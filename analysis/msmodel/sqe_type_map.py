#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from enum import Enum
from enum import unique


@unique
class SqeType(Enum):
    """
    establish sqe_type map
    """
    AI_CORE = 0
    AI_CPU = 1
    AIV_SQE = 2
    PLACE_HOLDER_SQE = 3
    EVENT_RECORD_SQE = 4
    EVENT_WAIT_SQE = 5
    NOTIFY_RECORD_SQE = 6
    NOTIFY_WAIT_SQE = 7
    WRITE_VALUE_SQE = 8
    VQ6_SQE = 9
    TOF_SQE = 10
    SDMA_SQE = 11
    VPC_SQE = 12
    JPEGE_SQE = 13
    JPEGD_SQE = 14
    DSA_SQE = 15
    ROCCE_SQE = 16
    PCIE_DMA_SQE = 17
    HOST_CPU_SQE = 18
    CDQM_SQE = 19
    C_CORE_SQE = 20
