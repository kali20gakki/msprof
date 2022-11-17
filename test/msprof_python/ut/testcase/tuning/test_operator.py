import os
import unittest
from unittest import mock

from common_func.common_prof_rule import CommonProfRule
from common_func.msvp_constant import MsvpConstant
from tuning.meta_condition_manager import OperatorConditionManager
from tuning.meta_rule_manager import OperatorRuleManager
from tuning.ms_operator import Operator


class TestNetwork(unittest.TestCase):

    def test_load_rules_json(self):
        operator_rule_mgr = OperatorRuleManager()
        operator_condition_mgr = OperatorConditionManager()
        operator_dicts = {}
        operator = Operator(operator_rule_mgr, operator_condition_mgr, operator_dicts)
        result = operator._load_rules_json()
        self.assertEqual(len(result), 16)
