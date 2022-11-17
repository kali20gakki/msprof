import unittest
from unittest import mock
from tuning.rule_bean import RuleBean

args = {'Rule Id': 'rule_memory_workspace_1', 'Rule Description': 'check and count the number of memory workspace.',
        'Rule Condition': 'condition_memory_workspace_1', 'Rule Type': 'Operator Metrics', 'Rule Subtype': 'Memory',
        'Rule Suggestion': 'please check and reduce the memory workspace'}


class TestRuleBean(unittest.TestCase):

    def test_get_rule_condition(self):
        check = RuleBean(**args)
        result = check.get_rule_condition()
        self.assertEqual(result, 'condition_memory_workspace_1')
        result_1 = check.get_rule_type()
        self.assertEqual(result_1, 'Operator Metrics')
        result_2 = check.get_rule_subtype()
        self.assertEqual(result_2, 'Memory')
        result_3 = check.get_rule_suggestion()
        self.assertEqual(result_3, 'please check and reduce the memory workspace')
