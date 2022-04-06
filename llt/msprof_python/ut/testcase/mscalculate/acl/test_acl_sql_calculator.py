#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import sqlite3
import unittest
from unittest import mock

from mscalculate.acl.acl_sql_calculator import AclSqlCalculator

from constant.ut_db_name_constant import TABLE_ACL_DATA, TABLE_HASH_ACL

NAMESPACE = 'mscalculate.acl.acl_sql_calculator'


class TestAclSqlCalculator(unittest.TestCase):
    def test_select_acl_data(self):
        with mock.patch(NAMESPACE + '.AclSqlCalculator._select_data'):
            AclSqlCalculator.select_acl_data("test", TABLE_ACL_DATA)

    def test_select_acl_hash_data(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(None, None)):
            AclSqlCalculator.select_acl_hash_data("test", TABLE_HASH_ACL)
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist'), \
             mock.patch(NAMESPACE + '.DBManager.fetch_all_data', return_value=[]), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            AclSqlCalculator.select_acl_hash_data("test", TABLE_HASH_ACL)

    def test_delete_acl_data(self):
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.judge_table_exist'), \
             mock.patch(NAMESPACE + '.DBManager.execute_sql'), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            AclSqlCalculator.delete_acl_data("test", TABLE_HASH_ACL)

    def test_insert_data_to_db(self):
        _acl_data = [('3074377499921637268', '3', 102026687320964, 102026687968135, 15489, 15489, 0)]
        with mock.patch(NAMESPACE + '.DBManager.check_connect_db_path', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.executemany_sql'), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            AclSqlCalculator.insert_data_to_db("test", TABLE_HASH_ACL, _acl_data)
