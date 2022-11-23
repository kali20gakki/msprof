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
    RESULT_RULE_TYPE = "Rule Type"
    RESULT_RULE_SUBTYPE = "Rule Subtype"
    RESULT_RULE_SUGGESTION = "Rule Suggestion"
    RESULT_OP_LIST = "Op List"
    RESULT_KEY = "result"

    RESULT_COMPUTATION = "Computation"
    RESULT_MEMORY = "Memory"
    RESULT_OPERATOR_SCHEDULE = "Operator Schedule"
    RESULT_OPERATOR_PROCESSING = "Operator Processing"
    RESULT_OPERATOR_METRICS = "Operator Metrics"

    RESULT_RULE_DESCRIPTION = "Rule Description"
    RESULT_COMPUTATION_DESCRIPTION = "Prompt users of some improperly high or low vector/cube/scalar" \
                                     " usages of operators on AI Cores."
    RESULT_MEMORY_DESCRIPTION = "Display improper memory usages of operators."
    RESULT_SCHEDULE_DESCRIPTION = "Display inefficient scheduling of operators."
    RESULT_PROCESSING_DESCRIPTION = "Provide various processing suggestions based on operator processing policy, " \
                                    "including multi-core processing, tiling policy, " \
                                    "and reduced use of AI CPU operators."
    RESULT_METRICS_DESCRIPTION = "Collect statistics on operator performance efficiency " \
                                 "and prompt users of high resource consumption."

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
