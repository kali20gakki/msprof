#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from common_func.constant import Constant


class NicDto:
    """
    Dto for ge model time data
    """

    def __init__(self: any) -> None:
        self._rx_packet = None
        self._rx_error_rate = None
        self._rx_dropped_rate = None
        self._tx_packet = None
        self._tx_error_rate = None
        self._tx_dropped_rate = None
        self._timestamp = None
        self._bandwidth = None
        self._rxbyte = None
        self._txbyte = None

    @property
    def rx_packet(self: any) -> any:
        return self._rx_packet

    @property
    def rx_error_rate(self: any) -> any:
        return self._rx_error_rate

    @property
    def rx_dropped_rate(self: any) -> any:
        return self._rx_dropped_rate

    @property
    def tx_packet(self: any) -> any:
        return self._tx_packet

    @property
    def tx_error_rate(self: any) -> any:
        return self._tx_error_rate

    @property
    def tx_dropped_rate(self: any) -> any:
        return self._tx_dropped_rate

    @property
    def timestamp(self: any) -> any:
        return self._timestamp

    @property
    def bandwidth(self: any) -> any:
        return self._bandwidth

    @property
    def rxbyte(self: any) -> any:
        return self._rxbyte

    @property
    def txbyte(self: any) -> any:
        return self._txbyte

    @rx_packet.setter
    def rx_packet(self: any, value: any) -> None:
        self._rx_packet = value

    @rx_error_rate.setter
    def rx_error_rate(self: any, value: any) -> None:
        self._rx_error_rate = value if value else Constant.DEFAULT_COUNT

    @rx_dropped_rate.setter
    def rx_dropped_rate(self: any, value: any) -> None:
        self._rx_dropped_rate = value if value else Constant.DEFAULT_COUNT

    @tx_packet.setter
    def tx_packet(self: any, value: any) -> None:
        self._tx_packet = value if value else Constant.DEFAULT_COUNT

    @tx_error_rate.setter
    def tx_error_rate(self: any, value: any) -> None:
        self._tx_error_rate = value if value else Constant.DEFAULT_COUNT

    @tx_dropped_rate.setter
    def tx_dropped_rate(self: any, value: any) -> None:
        self._tx_dropped_rate = value if value else Constant.DEFAULT_COUNT

    @timestamp.setter
    def timestamp(self: any, value: any) -> None:
        self._timestamp = value

    @bandwidth.setter
    def bandwidth(self: any, value: any) -> None:
        self._bandwidth = value

    @rxbyte.setter
    def rxbyte(self: any, value: any) -> None:
        self._rxbyte = value

    @txbyte.setter
    def txbyte(self: any, value: any) -> None:
        self._txbyte = value
