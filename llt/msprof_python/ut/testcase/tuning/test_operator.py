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
        with mock.patch('builtins.open', side_effect=FileNotFoundError), \
             mock.patch('tuning.network.logging.error'):
            condition_path = os.path.join(MsvpConstant.CONFIG_PATH, CommonProfRule.PROF_CONDITION_JSON)
            operator_rule_mgr = OperatorRuleManager()
            operator_condition_mgr = OperatorConditionManager(condition_path)
            operator_dicts = {}
            operator = Operator(operator_rule_mgr, operator_condition_mgr, operator_dicts)
            result = operator._load_rules_json()
        self.assertEqual(result, [])
