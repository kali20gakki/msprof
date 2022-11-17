#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.


class CommonProfRule:
    """
    common prof rule class
    """
    # rule json key and value
    RULE_PROF = "prof_rules"
    RULE_ID = "Rule Id"
    RULE_CONDITION = "Rule Condition"
    RULE_TYPE = "Rule Type"
    RULE_SUBTYPE = "Rule Subtype"
    RULE_SUGGESTION = "Rule Suggestion"

    # condition json key and value
    CONDITIONS = "conditions"
    CONDITION_ID = "id"
    CONDITION_TYPE = "type"
    CONDITION_LEFT = "left"
    CONDITION_RIGHT = "right"
    CONDITION_CMP = "cmp"
    CONDITION_DEPENDENCY = "dependency"
    CONDITION_THRESHOLD = "threshold"
    CONDITION_ACCUMULATE = "accumulate"
    CONDITION_COMPARE = "compare"

    COND_TYPE_NORMAL = "normal"
    COND_TYPE_FORMULA = "formula"
    COND_TYPE_COUNT = "count"
    COND_TYPE_ACCUMULATE = "accumulate"

    # return json key and value
    RESULT_RULE_TYPE = "rule_type"
    RESULT_RULE_SUBTYPE = "rule_subtype"
    RESULT_RULE_SUGGESTION = "rule_suggestion"
    RESULT_OP_LIST = "op_list"
    RESULT_KEY = "result"

    RESULT_MODEL_COMPUTATION = "Model/Operator Computation"
    RESULT_MODEL_MEMORY = "Model/Operator Memory"
    RESULT_OPERATOR_SCHEDULE = "Operator Schedule"
    RESULT_OPERATOR_PROCESSING = "Operator Processing"
    RESULT_OPERATOR_METRICS = "Operator Metrics"

    # prof rule and condition file name
    RESULT_PROF_JSON = "prof_rule_{}.json"

    def get_common_prof_rule_class_name(self: any) -> any:
        """
        get common prof rule class name
        """
        return self.__class__.__name__

    def get_common_prof_rule_class_member(self: any) -> any:
        """
        get common prof rule class member num
        """
        return self.__dict__
