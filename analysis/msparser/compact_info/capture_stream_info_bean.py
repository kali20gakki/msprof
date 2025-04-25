#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2025-2025. All rights reserved.
from msparser.compact_info.compact_info_bean import CompactInfoBean
 
 
class CaptureStreamInfoBean(CompactInfoBean):
    """
    capture stream info bean
    """

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        data = args[0]
        self._act = data[6]
        self._model_stream_id = data[7]
        self._original_stream_id = data[8]
        self._model_id = data[9]
        self._device_id = data[10]

    @property
    def act(self: any) -> int:
        """
        capture status
        """
        return self._act

    @property
    def model_stream_id(self: any) -> int:
        """
        capture physic stream id
        """
        return self._model_stream_id

    @property
    def original_stream_id(self: any) -> int:
        """
        capture ori stream id
        """
        return self._original_stream_id

    @property
    def model_id(self: any) -> int:
        """
        capture model_id from rts
        """
        return self._model_id

    @property
    def device_id(self: any) -> int:
        """
        device_id
        """
        return self._device_id




