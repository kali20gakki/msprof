#!/usr/bin/python3
# coding=utf-8
"""
function: runtime db operate
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

from msmodel.interface.base_model import BaseModel


class RuntimeModel(BaseModel):
    """
    db operator for runtime_api parser
    """

    @staticmethod
    def class_name() -> str:
        """
        class name
        """
        return 'RuntimeModel'
