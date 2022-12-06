#!/usr/bin/python3
# -*- coding: utf-8 -*-
#  Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from config.meta_config import MetaConfig
from common_func.common_prof_rule import CommonProfRule


class ProfRuleConfig(MetaConfig):
    DATA = [
        {
            "Rule Id": "rule_block_dim_1",
            "Rule Description": "check whether using multi-core",
            "Rule Condition": "condition_block_dim_1 && condition_cube_ratio_2",
            "Rule Type": "Operator Processing",
            "Rule Subtype": "",
            "Rule Suggestion": "Use multi-core",
            "Tuning Type": CommonProfRule.TUNING_OPERATOR
        }
        , {
            "Rule Id": "rule_block_dim_2",
            "Rule Description": "check tiling strategy.",
            "Rule Condition": "condition_block_dim_2 && condition_block_dim_3",
            "Rule Type": "Operator Processing",
            "Rule Subtype": "",
            "Rule Suggestion": "Check the tiling policy",
            "Tuning Type": CommonProfRule.TUNING_OPERATOR
        }
        , {
            "Rule Id": "rule_transData_1",
            "Rule Description": "check and count the number of transData.",
            "Rule Condition": "condition_transData_2",
            "Rule Type": "Operator Processing",
            "Rule Subtype": "",
            "Rule Suggestion": "Check and reduce the transData",
            "Tuning Type": CommonProfRule.TUNING_OPERATOR
        }
        , {
            "Rule Id": "rule_memory_workspace_1",
            "Rule Description": "check and count the number of memory workspace.",
            "Rule Condition": "condition_memory_workspace_1",
            "Rule Type": "Operator Metrics",
            "Rule Subtype": "Memory",
            "Rule Suggestion": "Check and reduce the memory workspace",
            "Tuning Type": CommonProfRule.TUNING_OPERATOR
        }
        , {
            "Rule Id": "rule_wait_time_1",
            "Rule Description": "check the wait time for tasks.",
            "Rule Condition": "condition_wait_time_2",
            "Rule Type": "Operator Schedule",
            "Rule Subtype": "",
            "Rule Suggestion": "Task waiting time has reached the upper limit",
            "Tuning Type": CommonProfRule.TUNING_OPERATOR
        }
        , {
            "Rule Id": "rule_aicpu_1",
            "Rule Description": "check the aicpu operator for net",
            "Rule Condition": "condition_aicpu_1",
            "Rule Type": "Operator Processing",
            "Rule Subtype": "",
            "Rule Suggestion": "Check and reduce AI CPU operators",
            "Tuning Type": CommonProfRule.TUNING_OPERATOR
        }
        , {
            "Rule Id": "rule_memory_bound_1",
            "Rule Description": "check the memory bound",
            "Rule Condition": "condition_memory_bound_2",
            "Rule Type": "Memory",
            "Rule Subtype": "",
            "Rule Suggestion": "Low data memory handling efficiency",
            "Tuning Type": CommonProfRule.TUNING_OPERATOR
        }
        , {
            "Rule Id": "rule_vector_bound_1",
            "Rule Description": "check the vector bound",
            "Rule Condition": "condition_vector_bound_1",
            "Rule Type": "Memory",
            "Rule Subtype": "",
            "Rule Suggestion": "Check repeat counts and vector mask",
            "Tuning Type": CommonProfRule.TUNING_OPERATOR
        }
        , {
            "Rule Id": "rule_vector_bound_2",
            "Rule Description": "cube operators reach vector bound",
            "Rule Condition": "condition_vector_ratio_2 && condition_cube_ratio_2 && condition_task_duration_1",
            "Rule Type": "Computation",
            "Rule Subtype": "Cube",
            "Rule Suggestion": "Cube operators spend too much time on vector computing",
            "Tuning Type": CommonProfRule.TUNING_OPERATOR
        }
        , {
            "Rule Id": "rule_vector_ratio_1",
            "Rule Description": "check the vector compute utilization",
            "Rule Condition": "condition_vector_ratio_1",
            "Rule Type": "Computation",
            "Rule Subtype": "Vector",
            "Rule Suggestion": "Low vector computing usage",
            "Tuning Type": CommonProfRule.TUNING_OPERATOR
        }
        , {
            "Rule Id": "rule_cube_ratio_1",
            "Rule Description": "check the cube compute utilization",
            "Rule Condition": "condition_cube_ratio_1",
            "Rule Type": "Computation",
            "Rule Subtype": "Cube",
            "Rule Suggestion": "Low cube computing usage",
            "Tuning Type": CommonProfRule.TUNING_OPERATOR
        }
        , {
            "Rule Id": "rule_scalar_bound_1",
            "Rule Description": "check the scalar bound",
            "Rule Condition": "condition_scalar_ratio_1",
            "Rule Type": "Computation",
            "Rule Subtype": "Scalar",
            "Rule Suggestion": "High scalar computing usage",
            "Tuning Type": CommonProfRule.TUNING_OPERATOR
        }
        , {
            "Rule Id": "rule_vec_bankgroup_cflt_ratio_1",
            "Rule Description": "check the vec_bankgroup_cflt_ratio",
            "Rule Condition": "condition_vec_bankgroup_cflt_ratio_1",
            "Rule Type": "Computation",
            "Rule Subtype": "Vector",
            "Rule Suggestion": "The vector bank group conflict has reached the upper limit",
            "Tuning Type": CommonProfRule.TUNING_OPERATOR
        }
        , {
            "Rule Id": "rule_vec_bank_cflt_ratio_1",
            "Rule Description": "check the vec_bank_cflt_ratio",
            "Rule Condition": "condition_vec_bank_cflt_ratio_1",
            "Rule Type": "Computation",
            "Rule Subtype": "Vector",
            "Rule Suggestion": "The vector bank conflict has reached the upper limit",
            "Tuning Type": CommonProfRule.TUNING_OPERATOR
        }
        , {
            "Rule Id": "rule_aicpu_2",
            "Rule Description": "check if the aicpu operator running with int64",
            "Rule Condition": "condition_task_duration_1 && condition_aicpu_1 && condition_int64_1",
            "Rule Type": "Operator Processing",
            "Rule Subtype": "",
            "Rule Suggestion": "Try to cast the data type to INT32 for AI Core operators",
            "Tuning Type": CommonProfRule.TUNING_OPERATOR
        }
        , {
            "Rule Id": "rule_aicpu_3",
            "Rule Description": "check the StridedSliceGrad operator",
            "Rule Condition": "condition_strided_slice_grad_1 && condition_aicpu_1 && condition_task_duration_1",
            "Rule Type": "Operator Processing",
            "Rule Subtype": "",
            "Rule Suggestion": "Try StridedSliceGrad on AI Cores",
            "Tuning Type": CommonProfRule.TUNING_OPERATOR
        }
        , {
            "Rule Id": "rule_ai_core_and_ai_cpu_parallelism",
            "Rule Description": "Calculate the percentage of the AI CPU execution time to identify the "
                                "serial wait bottleneck between the AI CPU and AI Core.",
            "Rule Condition": "condition_ai_cpu_parallelism",
            "Rule Type": "Operator Parallelism",
            "Rule Subtype": "",
            "Rule Suggestion": "View the Timeline View to find the AI CPU operators serially executed "
                               "with the AI Core and place them in the AI Core to execute by using methods "
                               "such as operator replacement, data type modification, and fusion. ",
            "Tuning Type": CommonProfRule.TUNING_OP_PARALLEL
        }
    ]

    def __init__(self):
        self.support_parser = False
