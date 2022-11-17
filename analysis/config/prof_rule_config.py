#!/usr/bin/python3
# -*- coding: utf-8 -*-
#  Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.

from config.meta_config import MetaConfig


class ProfRuleConfig(MetaConfig):
    DATA = [
        {
          "Rule Id": "rule_block_dim_1",
          "Rule Description": "check whether using multi-core",
          "Rule Condition": "condition_block_dim_1 && condition_cube_ratio_2",
          "Rule Type": "Operator Processing",
          "Rule Subtype": "",
          "Rule Suggestion": "please using multi-core"
        }
        , {
          "Rule Id": "rule_block_dim_2",
          "Rule Description": "check tiling strategy.",
          "Rule Condition": "condition_block_dim_2 && condition_block_dim_3",
          "Rule Type": "Operator Processing",
          "Rule Subtype": "",
          "Rule Suggestion": "please check tiling strategy"
        }
        , {
          "Rule Id": "rule_transData_1",
          "Rule Description": "check and count the number of transData.",
          "Rule Condition": "condition_transData_2",
          "Rule Type": "Operator Processing",
          "Rule Subtype": "",
          "Rule Suggestion": "please check and reduce the transData"
        }
        , {
          "Rule Id": "rule_memory_workspace_1",
          "Rule Description": "check and count the number of memory workspace.",
          "Rule Condition": "condition_memory_workspace_1",
          "Rule Type": "Operator Metrics",
          "Rule Subtype": "Memory",
          "Rule Suggestion": "please check and reduce the memory workspace"
        }
        , {
          "Rule Id": "rule_wait_time_1",
          "Rule Description": "check the wait time for tasks.",
          "Rule Condition": "condition_wait_time_2",
          "Rule Type": "Operator Schedule",
          "Rule Subtype": "",
          "Rule Suggestion": "task wait time has reached the upper limit"
        }
        , {
          "Rule Id": "rule_aicpu_1",
          "Rule Description": "check the aicpu operator for net",
          "Rule Condition": "condition_aicpu_1",
          "Rule Type": "Operator Processing",
          "Rule Subtype": "",
          "Rule Suggestion": "please check and reduce aicpu operator"
        }
        , {
          "Rule Id": "rule_memory_bound_1",
          "Rule Description": "check the memory bound",
          "Rule Condition": "condition_memory_bound_2",
          "Rule Type": "Model/Operator Memory",
          "Rule Subtype": "",
          "Rule Suggestion": "low data memory handling efficiency"
        }
        , {
          "Rule Id": "rule_vector_bound_1",
          "Rule Description": "check the vector bound",
          "Rule Condition": "condition_vector_bound_1",
          "Rule Type": "Model/Operator Memory",
          "Rule Subtype": "",
          "Rule Suggestion": "please check repeat counts and vector mask"
        }
        , {
          "Rule Id": "rule_vector_bound_2",
          "Rule Description": "cube operators reach vector bound",
          "Rule Condition": "condition_vector_ratio_2 && condition_cube_ratio_2 && condition_task_duration_1",
          "Rule Type": "Model/Operator Computation",
          "Rule Subtype": "Cube",
          "Rule Suggestion": "cube operators cost too much time on vector computing"
        }
        , {
          "Rule Id": "rule_vector_ratio_1",
          "Rule Description": "check the vector compute utilization",
          "Rule Condition": "condition_vector_ratio_1",
          "Rule Type": "Model/Operator Computation",
          "Rule Subtype": "Vector",
          "Rule Suggestion": "low vector compute utilization"
        }
        , {
          "Rule Id": "rule_cube_ratio_1",
          "Rule Description": "check the cube compute utilization",
          "Rule Condition": "condition_cube_ratio_1",
          "Rule Type": "Model/Operator Computation",
          "Rule Subtype": "Cube",
          "Rule Suggestion": "low cube compute utilization"
        }
        , {
          "Rule Id": "rule_scalar_bound_1",
          "Rule Description": "check the scalar bound",
          "Rule Condition": "condition_scalar_ratio_1",
          "Rule Type": "Model/Operator Computation",
          "Rule Subtype": "Scalar",
          "Rule Suggestion": "high scalar compute utilization"
        }
        , {
          "Rule Id": "rule_vec_bankgroup_cflt_ratio_1",
          "Rule Description": "check the vec_bankgroup_cflt_ratio",
          "Rule Condition": "condition_vec_bankgroup_cflt_ratio_1",
          "Rule Type": "Model/Operator Computation",
          "Rule Subtype": "Vector",
          "Rule Suggestion": "vector bank group conflict has reached the upper limit"
        }
        , {
          "Rule Id": "rule_vec_bank_cflt_ratio_1",
          "Rule Description": "check the vec_bank_cflt_ratio",
          "Rule Condition": "condition_vec_bank_cflt_ratio_1",
          "Rule Type": "Model/Operator Computation",
          "Rule Subtype": "Vector",
          "Rule Suggestion": "vector bank conflict has reached the upper limit"
        }
        , {
          "Rule Id": "rule_aicpu_2",
          "Rule Description": "check if the aicpu operator running with int64",
          "Rule Condition": "condition_task_duration_1 && condition_aicpu_1 && condition_int64_1",
          "Rule Type": "Operator Processing",
          "Rule Subtype": "",
          "Rule Suggestion": "please try converting the type to int32 for aicore operators"
        }
        , {
          "Rule Id": "rule_aicpu_3",
          "Rule Description": "check the StridedSliceGrad operator",
          "Rule Condition": "condition_strided_slice_grad_1 && condition_aicpu_1 && condition_task_duration_1",
          "Rule Type": "Operator Processing",
          "Rule Subtype": "",
          "Rule Suggestion": "please try StridedSliceGrad on aicore"
        }
      ]

    def __init__(self):
        self.support_parser = False
