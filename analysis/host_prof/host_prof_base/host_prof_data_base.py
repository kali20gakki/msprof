#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.

import logging
import os
from abc import abstractmethod

from common_func.constant import Constant
from common_func.db_manager import DBManager
from common_func.msvp_constant import MsvpConstant
from common_func.path_manager import PathManager


class HostProfDataBase:
    """
    host prof data analysis base model
    """
    TABLES_PATH = os.path.join(MsvpConstant.CONFIG_PATH, 'Tables.ini')
    TABLE_CACHE_SIZE = 10000

    def __init__(self: any, result_dir: str, db_name: str, table_list: list) -> None:
        self.result_dir = result_dir
        self.conn = None
        self.cur = None
        self.table_list = table_list
        self.db_name = db_name
        self.cache_data = []

    def init(self: any) -> bool:
        """
        init db
        :return: init result
        """

        self.conn, self.cur = DBManager.create_connect_db(
            PathManager.get_db_path(self.result_dir, self.db_name))
        if not (self.conn and self.cur):
            return False
        self.create_table()
        return True

    def check_db(self: any) -> bool:
        """
        check db exist
        :return: result of checking db
        """
        self.conn, self.cur = DBManager.check_connect_db(self.result_dir, self.db_name)
        if not (self.conn and self.cur):
            return False
        return True

    def finalize(self: any) -> None:
        """
        release conn and cur
        :return: None
        """
        DBManager.destroy_db_connect(self.conn, self.cur)

    def create_table(self: any) -> None:
        """
        create table
        :return: None
        """
        fmt_table_map = "{0}Map"
        for table_name in self.table_list:
            if DBManager.judge_table_exist(self.cur, table_name):
                continue
            table_map = fmt_table_map.format(table_name)
            sql = DBManager.sql_create_general_table(table_map, table_name, self.TABLES_PATH)
            DBManager.execute_sql(self.conn, sql)

    def insert_single_data(self: any, data: list) -> None:
        """
        insert data
        :param data: data
        :return: None
        """
        self.cache_data.append(data)
        if len(self.cache_data) > self.TABLE_CACHE_SIZE:
            try:
                self.flush_data()
            except (OSError, SystemError, ValueError, TypeError, RuntimeError) as error:
                logging.exception(error, exc_info=Constant.TRACE_BACK_SWITCH)
            finally:
                self.cache_data.clear()

    @abstractmethod
    def flush_data(self: any) -> any:
        """
        flush all data
        """
