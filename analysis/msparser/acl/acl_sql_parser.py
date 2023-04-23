#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.

from msconfig.config_manager import ConfigManager
from common_func.common import CommonConstant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.path_manager import PathManager


class AclSqlParser:
    """
    db operator for acl parser
    """
    TABLES_PATH = ConfigManager.TABLES

    @classmethod
    def create_acl_data_table(cls: any, project_path: str) -> None:
        """
        create db and table of acl.
        :param project_path: project path
        :return: None
        """
        cls._create_table(project_path, DBNameConstant.DB_ACL_MODULE, DBNameConstant.TABLE_ACL_DATA,
                          CommonConstant.ACL_TABLE_MAP)

    @classmethod
    def insert_acl_data_to_db(cls: any, project_path: str, data: list) -> None:
        """
        insert data into the table of acl.
        :param data: acl data
        :param project_path:project path
        :return: None
        """
        cls._insert_db(project_path, data, DBNameConstant.DB_ACL_MODULE, DBNameConstant.TABLE_ACL_DATA)

    @classmethod
    def _create_table(cls: any, project_path: str, db_name: str, table_name: str, table_map: str) -> None:
        db_path = PathManager.get_db_path(project_path, db_name)
        conn, curs = DBManager.create_connect_db(db_path)
        sql = DBManager.sql_create_general_table(table_map, table_name, cls.TABLES_PATH)
        if conn:
            DBManager.execute_sql(conn, sql)
        DBManager.destroy_db_connect(conn, curs)

    @classmethod
    def _insert_db(cls: any, project_path: str, data: list, db_name: str, table_name: str) -> None:
        db_path = PathManager.get_db_path(project_path, db_name)
        conn, curs = DBManager.create_connect_db(db_path)
        if conn and data:
            sql = 'insert into {0} values ({1})'.format(table_name, "?," * (len(data[0]) - 1) + "?")
            DBManager.executemany_sql(conn, sql, data)
        DBManager.destroy_db_connect(conn, curs)