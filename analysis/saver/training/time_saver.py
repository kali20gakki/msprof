# !/usr/bin/python
# coding=utf-8
"""
This script is used to save time data to db.
Copyright Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
"""
import logging
import os

from common_func.common import check_number_valid
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.utils import Utils


class TimeSaver:
    """
    saver of time data
    """

    @staticmethod
    def create_timedb_conn(sql_path: str) -> tuple:
        """
        create time DB connection
        """
        conn, curs = DBManager.create_connect_db(os.path.join(sql_path, DBNameConstant.DB_TIME))
        return conn, curs

    @staticmethod
    def _check_time_format(message: dict) -> bool:
        result = False
        if not message:
            logging.error("No time_sync message collected")
            return result
        for items in message["data"]:
            check_int_item = [items["dev_mon"],
                              items["dev_wall"],
                              items["dev_cntvct"],
                              items["host_mon"],
                              items["host_wall"]]
            if any(not check_number_valid(item) for item in check_int_item):
                logging.error("time_sync message check integer value failed")
                return result
        return result

    @staticmethod
    def _create_table(conn: any) -> None:
        sql = "CREATE TABLE IF NOT EXISTS {}(device_id int, " \
              "dev_mon int, dev_wall int, dev_cntvct int," \
              "host_mon int, host_wall int, " \
              "PRIMARY KEY (device_id))".format(DBNameConstant.TABLE_TIME)
        DBManager.execute_sql(conn, sql)

    @staticmethod
    def _insert_time_data(conn: any, message: dict) -> bool:
        if not message["data"]:
            return False
        data = Utils.generator_to_list((pb_time_mesg["device_id"],
                                        pb_time_mesg["dev_mon"],
                                        pb_time_mesg["dev_wall"],
                                        pb_time_mesg["dev_cntvct"],
                                        pb_time_mesg["host_mon"],
                                        pb_time_mesg["host_wall"])
                                       for pb_time_mesg in message["data"])
        insert_sql = 'INSERT OR REPLACE INTO {0} VALUES ({1})' \
            .format(DBNameConstant.TABLE_TIME, "?," * (len(data[0]) - 1) + "?")
        DBManager.executemany_sql(conn, insert_sql, data)
        return True

    @classmethod
    def save_data_to_db(cls: any, message: dict) -> bool:
        """
        Rewrite gRPC TimeSync method.
        Insert TimeMessage protobuf into time table and report status.
        """
        cls._check_time_format(message)
        conn, curs = TimeSaver.create_timedb_conn(message["sql_path"])
        TimeSaver._create_table(conn)
        result = TimeSaver._insert_time_data(conn, message)
        DBManager.destroy_db_connect(conn, curs)
        return result
