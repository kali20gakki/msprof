#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.


from msparser.compact_info.compact_info_bean import CompactInfoBean
from profiling_bean.ge.ge_task_bean import GeTaskBean


class NodeBasicInfoBean(CompactInfoBean):
    """
    Node Basic Info Bean
    """

    def __init__(self: any, *args) -> None:
        super().__init__(*args)
        data = args[0]
        self._node_id = data[6]
        self._task_type = data[7]
        self._op_type = data[8]
        self._block_dim = data[9]
        self._op_flag = data[10]
        self._mix_block_dim = self.mix_block_dim

    @property
    def task_type(self: any) -> str:
        """
        for task type
        """
        return GeTaskBean.TASK_TYPE_DICT.get(self._task_type, self._task_type)

    @property
    def op_type(self: any) -> str:
        """
        for op type
        """
        return str(self._op_type)

    @property
    def node_id(self: any) -> str:
        """
        for node id
        """
        return str(self._node_id)

    @property
    def op_flag(self: any) -> int:
        """
        for op flag
        """
        return self._op_flag

    @property
    def block_dim(self: any) -> int:
        """
        for block dims
        get lower 16bit data of 32bit
        """
        return self._block_dim & 65535

    @property
    def mix_block_dim(self: any) -> int:
        """
        for mix block dims
        get the product of block dim and higher 16bit
        """
        return (self._block_dim & 65535) * (self._block_dim >> 16)
