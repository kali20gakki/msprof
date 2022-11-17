import os
import unittest
from unittest import mock

from common_func.common_prof_rule import CommonProfRule
from common_func.msvp_constant import MsvpConstant
from tuning.meta_condition_manager import NetConditionManager
from tuning.meta_condition_manager import OperatorConditionManager
from tuning.meta_metric import NetWorkMetric
from tuning.meta_metric import OperatorMetric
from tuning.meta_rule_manager import NetRuleManager
from tuning.meta_rule_manager import OperatorRuleManager
from tuning.ms_operator import Operator
from tuning.ms_operator import OperatorManager
from tuning.network import Network

NAMESPACE = 'tuning.meta_metric'


class TestOperatorMetric(unittest.TestCase):

    def test_get_metric(self):
        with mock.patch(NAMESPACE + '.logging.warning'):
            operator_rule_mgr = OperatorRuleManager()
            operator_condition_mgr = OperatorConditionManager()
            operator_dicts = {}
            operator = Operator(operator_rule_mgr, operator_condition_mgr, operator_dicts)
            check = OperatorMetric(operator)
            result = check.get_metric(123)
        self.assertEqual(result, {})


class TestNetWorkMetric(unittest.TestCase):
    def test_get_metric(self):
        with mock.patch(NAMESPACE + '.logging.warning'):
            network_condition_mgr = NetConditionManager()
            operator_mgr = OperatorManager()
            net_rule_mgr = NetRuleManager()
            net = Network(net_rule_mgr, network_condition_mgr, operator_mgr)
            check = NetWorkMetric(net.operator_mgr, net.network_condition_mgr)
            result = check.get_metric(123)
        self.assertEqual(result, {})


