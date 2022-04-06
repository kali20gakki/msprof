import os
import unittest
from unittest import mock

from common_func.common_prof_rule import CommonProfRule
from common_func.msvp_constant import MsvpConstant
from tuning.meta_condition_manager import NetConditionManager
from tuning.meta_rule_manager import NetRuleManager
from tuning.ms_operator import OperatorManager
from tuning.network import Network


class TestNetwork(unittest.TestCase):

    def test_load_rules_json(self):
        with mock.patch('builtins.open', side_effect=FileNotFoundError),\
                mock.patch('tuning.network.logging.error'):
            network_condition_mgr = NetConditionManager(os.path.join(MsvpConstant.CONFIG_PATH, CommonProfRule.PROF_CONDITION_JSON))
            operator_mgr = OperatorManager()
            net_rule_mgr = NetRuleManager()
            net = Network(net_rule_mgr, network_condition_mgr, operator_mgr)
            result = net._load_rules_json()
        self.assertEqual(result, [])
