#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from config.meta_config import MetaConfig


class TuningRuleConfig(MetaConfig):
    DATA = [
        "rule_block_dim_1",
        "rule_vector_bound_1",
        "rule_vector_bound_2",
        "rule_scalar_bound_1",
        "rule_memory_bound_1",
        "rule_vec_bankgroup_cflt_ratio_1",
        "rule_vec_bank_cflt_ratio_1",
        "rule_vector_ratio_1",
        "rule_cube_ratio_1",
        "rule_transData_1",
        "rule_aicpu_1",
        "rule_aicpu_2",
        "rule_aicpu_3",
        "rule_wait_time_1",
        "rule_memory_workspace_1",
        "rule_ai_core_and_ai_cpu_parallelism"
    ]

    def __init__(self):
        self.support_parser = False
