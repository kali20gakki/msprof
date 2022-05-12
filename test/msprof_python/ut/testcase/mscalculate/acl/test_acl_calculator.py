#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import unittest
from unittest import mock

from mscalculate.acl.acl_calculator import AclCalculator

from constant.constant import CONFIG

NAMESPACE = 'mscalculate.acl.acl_calculator'


class TestAclCalculator(unittest.TestCase):
    def test_calculate(self):
        with mock.patch(NAMESPACE + '.AclCalculator._get_acl_data'), \
             mock.patch(NAMESPACE + '.AclCalculator._refresh_acl_api_name'), \
             mock.patch(NAMESPACE + '.AclCalculator._refresh_acl_api_type'), \
             mock.patch(NAMESPACE + '.AclSqlCalculator.delete_acl_data'), \
             mock.patch(NAMESPACE + '.AclSqlCalculator.insert_data_to_db'):
            check = AclCalculator({}, CONFIG)
            check.calculate()

    def test_get_acl_data(self):
        with mock.patch(NAMESPACE + '.AclCalculator._get_acl_db_path'), \
             mock.patch(NAMESPACE + '.AclSqlCalculator.select_acl_data', return_value=[]):
            check = AclCalculator({}, CONFIG)
            ret = check._get_acl_data()
            self.assertFalse(ret)

    def test_get_acl_hash_dict(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path'), \
             mock.patch(NAMESPACE + '.AclSqlCalculator.select_acl_hash_data',
                        return_value=[("3074377499921637268", "aclrtCreateContext")]):
            check = AclCalculator({}, CONFIG)
            ret = check._get_acl_hash_dict()
            self.assertEqual("aclrtCreateContext", ret.pop("3074377499921637268"))

    def test_refresh_acl_api_name(self):
        _data = [('3074377499921637268', '3', 102026687320964, 102026687968135, 15489, 15489, 0)]
        _after_api_name = [('aclrtCreateContext', '3', 102026687320964, 102026687968135, 15489, 15489, 0)]
        with mock.patch(NAMESPACE + '.AclCalculator._get_acl_hash_dict',
                        return_value={"3074377499921637268": "aclrtCreateContext"}):
            check = AclCalculator({}, CONFIG)
            check._acl_data = _data
            check._refresh_acl_api_name()
            self.assertEqual(check._acl_data, _after_api_name)

    def test_refresh_acl_api_type(self):
        _data = [('3074377499921637268', '3', 102026687320964, 102026687968135, 15489, 15489, 0)]
        _after_api_type = [('3074377499921637268', 'ACL_RTS', 102026687320964, 102026687968135, 15489, 15489, 0)]
        check = AclCalculator({}, CONFIG)
        check._acl_data = _data
        check._refresh_acl_api_type()
        self.assertEqual(check._acl_data, _after_api_type)

    def test_ms_run(self):
        with mock.patch(NAMESPACE + '.AclCalculator.calculate'):
            check = AclCalculator({}, CONFIG)
            check.ms_run()
