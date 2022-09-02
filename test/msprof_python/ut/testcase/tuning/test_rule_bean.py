import unittest
from unittest import mock
from tuning.rule_bean import RuleBean

args = {'rule_id': 'rule_memory_workspace_1', 'rule_description': 'check and count the number of memory workspace.',
        'rule_condition': 'condition_memory_workspace_1', 'rule_type': 'Operator Metrics', 'rule_subtype': 'Memory',
        'rule_suggestion': 'please check and reduce the memory workspace'}


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
