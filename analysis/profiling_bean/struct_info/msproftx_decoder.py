#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.

from profiling_bean.struct_info.struct_decoder import StructDecoder


class MsprofTxDecoder(StructDecoder):
    """
    class used to decode binary data
    """

    def __init__(self: any, *args: list) -> None:
        filed = args[0]
        self._id_data = filed[2:4]
        self._category = filed[4]
        self._event_type = filed[5]
        self._mix_message = filed[6:10]
        self._message_type = filed[10]
        self._message = bytes.decode(filed[11]).replace('\x00', '')

    @property
    def id_data(self: any) -> tuple:
        """
        get id_data
        mix_message[0]:p_id
        mix_message[1]:t_id
        :return: id_data
        """
        return self._id_data

    @property
    def category(self: any) -> str:
        """
        get category
        :return: category
        """
        return self._category

    @property
    def event_type(self: any) -> str:
        """
        get _event_type
        :return: event_type
        """
        return self._event_type

    @property
    def mix_message(self: any) -> tuple:
        """
        get mix_message
        mix_message[0]:payload_type
        mix_message[1]:payload_value
        mix_message[2]:Start_time
        mix_message[3]:End_time
        :return: mix_message
        """
        return self._mix_message

    @property
    def message_type(self: any) -> int:
        """
        get message_type
        :return: message_type
        """
        return self._message_type

    @property
    def message(self: any) -> str:
        """
        get message
        :return: message
        """
        return self._message
