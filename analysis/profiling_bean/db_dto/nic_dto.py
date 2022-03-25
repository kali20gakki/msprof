#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2022. All rights reserved.
"""
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

    @rx_packet.setter
    def rx_packet(self: any, value: any) -> None:
        self._rx_packet = value

    @property
    def rx_error_rate(self: any) -> any:
        return self._rx_error_rate

    @rx_error_rate.setter
    def rx_error_rate(self: any, value: any) -> None:
        self._rx_error_rate = value if value else Constant.DEFAULT_COUNT

    @property
    def rx_dropped_rate(self: any) -> any:
        return self._rx_dropped_rate

    @rx_dropped_rate.setter
    def rx_dropped_rate(self: any, value: any) -> None:
        self._rx_dropped_rate = value if value else Constant.DEFAULT_COUNT

    @property
    def tx_packet(self: any) -> any:
        return self._tx_packet

    @tx_packet.setter
    def tx_packet(self: any, value: any) -> None:
        self._tx_packet = value if value else Constant.DEFAULT_COUNT

    @property
    def tx_error_rate(self: any) -> any:
        return self._tx_error_rate

    @tx_error_rate.setter
    def tx_error_rate(self: any, value: any) -> None:
        self._tx_error_rate = value if value else Constant.DEFAULT_COUNT

    @property
    def tx_dropped_rate(self: any) -> any:
        return self._tx_dropped_rate

    @tx_dropped_rate.setter
    def tx_dropped_rate(self: any, value: any) -> None:
        self._tx_dropped_rate = value if value else Constant.DEFAULT_COUNT

    @property
    def timestamp(self: any) -> any:
        return self._timestamp

    @timestamp.setter
    def timestamp(self: any, value: any) -> None:
        self._timestamp = value

    @property
    def bandwidth(self: any) -> any:
        return self._bandwidth

    @bandwidth.setter
    def bandwidth(self: any, value: any) -> None:
        self._bandwidth = value

    @property
    def rxbyte(self: any) -> any:
        return self._rxbyte

    @rxbyte.setter
    def rxbyte(self: any, value: any) -> None:
        self._rxbyte = value

    @property
    def txbyte(self: any) -> any:
        return self._txbyte

    @txbyte.setter
    def txbyte(self: any, value: any) -> None:
        self._txbyte = value