#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class NicDto(metaclass=InstanceCheckMeta):
    """
    Dto for ge model time data
    """
    bandwidth = None
    rx_dropped_rate = None
    rx_error_rate = None
    rx_packet = None
    rxbyte = None
    timestamp = None
    tx_dropped_rate = None
    tx_error_rate = None
    tx_packet = None
    txbyte = None
