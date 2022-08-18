#!/usr/bin/python3
# coding=utf-8
"""
function: this script used to operate AI_CPU
Copyright Huawei Technologies Co., Ltd. 2021. All rights reserved.
"""

from common_func.platform.chip_manager import ChipManager



class AiCpuFromTsFactory:

    @classmethod
    def generate_ai_cpu_model(cls: any) -> any:
        if ChipManager().is_chip_v1():

