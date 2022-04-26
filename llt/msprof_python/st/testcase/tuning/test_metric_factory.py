import configparser
import unittest
from unittest import mock

from tuning.metric_factory import MetricFactory


class TestMetricFactory(unittest.TestCase):

    def test_get_support_rules(self):
        with mock.patch('configparser.ConfigParser.read', side_effect=configparser.Error):
            result = MetricFactory.get_support_rules('test')
        self.assertEqual(result, [])
