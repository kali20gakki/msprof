#!/usr/bin/env python
# coding=utf-8
"""
function:
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""
import sqlite3
import unittest
from unittest import mock

from msparser.acl.acl_sql_parser import AclSqlParser

NAMESPACE = 'msparser.acl.acl_sql_parser'


class TestAclSqlParser(unittest.TestCase):
    def test_create_acl_data_table(self):
        with mock.patch(NAMESPACE + '.AclSqlParser._create_table'):
            AclSqlParser.create_acl_data_table("test")

    def test_insert_acl_data_to_db(self):
        with mock.patch(NAMESPACE + '.AclSqlParser._insert_db'):
            AclSqlParser.insert_acl_data_to_db("tets", "")

    def test_create_acl_hash_table(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path'), \
             mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.execute_sql'), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            AclSqlParser.create_acl_hash_table("test")

    def test_insert_acl_hash_to_db(self):
        with mock.patch(NAMESPACE + '.PathManager.get_db_path'), \
             mock.patch(NAMESPACE + '.DBManager.create_connect_db', return_value=(True, True)), \
             mock.patch(NAMESPACE + '.DBManager.executemany_sql'), \
             mock.patch(NAMESPACE + '.DBManager.destroy_db_connect'):
            AclSqlParser.insert_acl_hash_to_db("test", "test_data")
