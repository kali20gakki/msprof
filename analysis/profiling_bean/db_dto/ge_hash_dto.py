#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


class GeHashDto:
    """
    Dto for hash data
    """

    def __init__(self: any) -> None:
        self._hash_key = None
        self._hash_value = None
        self._level = None

    @property
    def hash_key(self: any) -> any:
        return self._hash_key

    @property
    def hash_value(self: any) -> any:
        return self._hash_value

    @property
    def level(self: any) -> any:
        return self._level

    @hash_key.setter
    def hash_key(self: any, value: any) -> None:
        self._hash_key = value

    @hash_value.setter
    def hash_value(self: any, value: any) -> None:
        self._hash_value = value

    @level.setter
    def level(self: any, value: any) -> None:
        self._level = value
