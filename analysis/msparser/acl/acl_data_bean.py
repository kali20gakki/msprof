#!/usr/bin/python3
# coding=utf-8
"""
function: acl bean data
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import logging
import struct

from profiling_bean.struct_info.struct_decoder import StructDecoder


class AclDataBean(StructDecoder):
    """
    ACL bean data for the data parsing by acl parser.
    """
    ACL_FMT = "HHIQQII64s4Q"
    ACL_MAGIC_NUM = 23130
    ACL_DATA_TAG = 0

    def __init__(self: any) -> None:
        self._api_type = None
        self._api = None
        self._api_start = None
        self._api_end = None
        self._process_id = None
        self._thread_id = None

    @property
    def api_type(self: any) -> str:
        """
        acl api type
        :return: api type
        """
        return self._api_type

    @property
    def api_name(self: any) -> str:
        """
        acl api name
        :return: api name
        """
        return self._api

    @property
    def api_start(self: any) -> int:
        """
        acl api start time
        :return: api start time
        """
        return self._api_start

    @property
    def api_end(self: any) -> int:
        """
        acl api end time
        :return: api end time
        """
        return self._api_end

    @property
    def process_id(self: any) -> int:
        """
        acl api process id
        :return: api process id
        """
        return self._process_id

    @property
    def thread_id(self: any) -> int:
        """
        acl api thread id
        :return: api thread id
        """
        return self._thread_id

    def acl_decode(self: any, bin_data: any) -> any:
        """
        decode the acl bin data
        :param bin_data: acl bin data
        :return: instance of acl
        """
        if self.construct_bean(struct.unpack(self.ACL_FMT, bin_data)):
            return self
        return {}

    def construct_bean(self: any, *args: dict) -> bool:
        """
        refresh the acl data
        :param args: acl bin data
        :return: True or False
        """
        _acl_data = args[0]
        _magic_num, _data_tag = _acl_data[:2]
        if _magic_num == self.ACL_MAGIC_NUM and _data_tag == self.ACL_DATA_TAG:
            self._api_type = _acl_data[2]
            self._api = bytes.decode(_acl_data[7]).replace('\x00', '')
            self._api_start = _acl_data[3]
            self._api_end = _acl_data[4]
            self._process_id = _acl_data[5]
            self._thread_id = _acl_data[6]
            return True
        logging.error("ACL data struct is incomplete: %s, please check the acl file.", hex(_magic_num))
        return False
