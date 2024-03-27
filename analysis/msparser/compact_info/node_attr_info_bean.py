#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.

from msparser.compact_info.compact_info_bean import CompactInfoBean


class NodeAttrInfoBean(CompactInfoBean):
    """
    node attr info bean
    """

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        data = args[0]
        self._hashid = data[6]

    @property
    def hashid(self: any) -> str:
        """
        node attr hash id
        """
        return self._hashid


