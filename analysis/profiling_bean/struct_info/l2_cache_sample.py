#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

import logging
import struct

from msparser.data_struct_size_constant import StructFmt
from msparser.interface.idata_bean import IDataBean


class L2CacheSampleDataBean(IDataBean):
    """
    l2 cache bean data for the data parsing by l2 cache parser.
    """
    EVENT_NUM_INDEX = 3
    L2_CACHE_EVENT_START_INDEX = 7
    L2_CACHE_DATA_NUM = 15

    def __init__(self: any) -> None:
        self._events_list = []

    @property
    def events_list(self: any) -> list:
        """
        l2 cache events list
        """
        return self._events_list

    def decode(self: any, bin_data: bytes) -> None:
        """
        decode the l2 cache bin data
        :param bin_data: l2 cache bin data
        :return: instance of l2 cache
        """
        if not self.construct_bean(struct.unpack(StructFmt.L2_CACHE_SAMPLE_STRUCT_FMT, bin_data)):
            logging.error("l2 cache data struct is incomplete, please check the l2 cache file.")

    def construct_bean(self: any, *args: any) -> bool:
        """
        refresh the l2 cache data
        :param args: l2 cache data
        :return: True or False
        """
        l2_cache_data = args[0]
        if len(l2_cache_data) != self.L2_CACHE_DATA_NUM:
            return False
        event_num = l2_cache_data[self.EVENT_NUM_INDEX]
        self._events_list = l2_cache_data[
                            self.L2_CACHE_EVENT_START_INDEX:self.L2_CACHE_EVENT_START_INDEX + event_num]
        return True
