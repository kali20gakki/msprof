"""

Copyright Huawei Technologies Co., Ltd. 2020. All rights reserved.
"""

from common_func.common_prof_rule import CommonProfRule


class RuleBean:
    """
    rule bean for rule json
    """

    def __init__(self: any, **args: dict) -> None:
        self.__init_param(**args)

    def get_rule_id(self: any) -> any:
        """
        get rule id
        :return: rule id
        """
        return self.rule_id

    def get_rule_condition(self: any) -> any:
        """
        get rule condition
        :return: rule condition
        """
        return self.rule_condition

    def get_rule_type(self: any) -> any:
        """
        get rule type
        :return: rule type
        """
        return self.rule_type

    def get_rule_subtype(self: any) -> any:
        """
        get rule subtype
        :return: rule subtype
        """
        return self.rule_subtype

    def get_rule_suggestion(self: any) -> any:
        """
        get rule suggestion
        :return: rule suggestion
        """
        return self.rule_suggestion

    def __init_param(self: any, **args: dict) -> None:
        self.rule_id = args.get(CommonProfRule.RULE_ID)
        self.rule_condition = args.get(CommonProfRule.RULE_CONDITION)
        self.rule_type = args.get(CommonProfRule.RULE_TYPE)
        self.rule_subtype = args.get(CommonProfRule.RULE_SUBTYPE)
        self.rule_suggestion = args.get(CommonProfRule.RULE_SUGGESTION)
