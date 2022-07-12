#!/usr/bin/python3
# coding=utf-8
"""
function: acl db operate for calculating
Copyright Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
"""

import logging
import sqlite3

from common_func.constant import Constant
from common_func.db_manager import DBManager


class AclSqlCalculator:
    """
    db operator for acl calculator
    """

    @classmethod
    def select_acl_data(cls: any, db_path: str, table_name: str) -> list:
        """
        select acl data
        :param db_path: acl db path
        :param table_name: table name
        :return: list of acl data
        """
        sql = "select api_name, api_type, start_time, end_time, " \
              "process_id, thread_id, device_id from {0}".format(table_name)
        return cls._select_data(db_path, table_name, sql)

    @classmethod
    def select_acl_hash_data(cls: any, db_path: str, table_name: str) -> list:
        """
        select hash table for acl
        :param db_path: acl hash db path
        :param table_name: table name
        :return: hash mapping data
        """
        sql = "select acl_hash, api_name from {0}".format(table_name)
        return cls._select_data(db_path, table_name, sql)

    @classmethod
    def delete_acl_data(cls: any, db_path: str, table_name: str) -> None:
        """
        delete data from the db of acl
        :param db_path: db path
        :param table_name: table name
        :return: NA
        """
        conn, curs = DBManager.check_connect_db_path(db_path)
        if conn and curs and DBManager.judge_table_exist(curs, table_name):
            sql = "delete from {0}".format(table_name)
            DBManager.execute_sql(conn, sql)
        DBManager.destroy_db_connect(conn, curs)

    @classmethod
    def insert_data_to_db(cls: any, db_path: str, table_name: str, acl_data: list) -> None:
        """
        insert data into the table of acl.
        :param table_name: table name
        :param db_path: da path
        :param acl_data: acl data
        :return: None
        """
        conn, curs = DBManager.check_connect_db_path(db_path)
        if conn and acl_data:
            sql = 'insert into {0} values ({1})'.format(table_name, "?," * (len(acl_data[0]) - 1) + "?")
            DBManager.executemany_sql(conn, sql, acl_data)
        DBManager.destroy_db_connect(conn, curs)

    @classmethod
    def _select_data(cls: any, db_path: str, table_name: str, sql: str) -> list:
        """
        select data from the db of acl
        :param db_path: db path
        :param table_name: table name
        :return: acl data
        """
        conn, curs = DBManager.check_connect_db_path(db_path)
        acl_data = []
        if conn and curs and DBManager.judge_table_exist(curs, table_name):
            acl_data = DBManager.fetch_all_data(curs, sql)
        DBManager.destroy_db_connect(conn, curs)
        return acl_data
