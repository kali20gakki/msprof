#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.

from dataclasses import dataclass
from profiling_bean.db_dto.dto_meta_class import InstanceCheckMeta


@dataclass
class UBDto(metaclass=InstanceCheckMeta):
    """
    Dto for UB data
    """
    device_id: int = None
    port_id: int = None
    time_stamp: float = None
    udma_rx_bind: float = None
    udma_tx_bind: float = None
    rx_port_band_width: float = None
    rx_packet_rate: float = None
    rx_bytes: float = None
    rx_packets: float = None
    rx_errors: float = None
    rx_dropped: float = None
    tx_port_band_width: float = None
    tx_packet_rate: float = None
    tx_bytes: float = None
    tx_packets: float = None
    tx_errors: float = None
    tx_dropped: float = None
