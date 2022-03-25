#!usr/bin/env python
# coding:utf-8
"""
report AI related data info
Copyright Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
"""

import sqlite3

from common_func.common import CommonConstant
from common_func.db_manager import DBManager
from common_func.db_name_constant import DBNameConstant
from common_func.ms_constant.str_constant import StrConstant
from common_func.msvp_constant import MsvpConstant
from common_func.ms_constant.number_constant import NumberConstant
from common_func.utils import Utils


def get_acl_data(db_path: str, table_name: str, device_id: str, configs: dict) -> tuple:
    """
    get acl data
    """
    conn, cursor = DBManager.check_connect_db_path(db_path)
    if not (conn and cursor):
        return MsvpConstant.MSVP_EMPTY_DATA
    acl_data = get_acl_data_for_device(cursor, table_name, device_id)
    DBManager.destroy_db_connect(conn, cursor)
    return configs.get(StrConstant.CONFIG_HEADERS), acl_data, len(acl_data)


def get_acl_data_for_device(cursor: any, table_name: str, device_id: str) -> list:
    """
    get table data for certain device
    """
    search_data_sql = "select api_name, api_type, start_time, (end_time-start_time)/{2} as duration, " \
                      "process_id, thread_id from {0} where device_id={1} order by start_time asc" \
        .format(table_name, device_id, NumberConstant.NS_TO_US)
    result = DBManager.fetch_all_data(cursor, search_data_sql)
    if result:
        return result
    return []


def get_acl_statistic_data(db_path: str, table_name: str, device_id: str, configs: dict) -> tuple:
    """
    get acl statistic data
    """
    conn, cursor = DBManager.check_connect_db_path(db_path)
    if not (conn and cursor) or not DBManager.judge_table_exist(cursor, DBNameConstant.TABLE_ACL_DATA):
        return MsvpConstant.MSVP_EMPTY_DATA
    acl_data = _get_get_acl_statistic_data(cursor, table_name, device_id)
    DBManager.destroy_db_connect(conn, cursor)
    return configs.get(StrConstant.CONFIG_HEADERS), acl_data, len(acl_data)


def _get_get_acl_statistic_data(cursor: any, table_name: str, device_id: str) -> list:
    search_data_sql = "select sum(end_time-start_time) from {0} " \
                      "where device_id={1}".format(table_name, device_id)
    total_time = cursor.execute(search_data_sql).fetchone()[0]
    if total_time is None:
        return []

    search_data_sql = "select api_name, api_type, {percent}*sum(end_time-start_time)/{total_time}, " \
                      "sum((end_time-start_time)/{2}), count(*), " \
                      "sum((end_time-start_time)/{2})/count(*), min((end_time-start_time)/{2}), " \
                      "max((end_time-start_time)/{2}), process_id, " \
                      "thread_id from {0} where device_id={1} group by api_name" \
        .format(table_name, device_id, NumberConstant.NS_TO_US, percent=CommonConstant.PERCENT, total_time=total_time)
    result = DBManager.fetch_all_data(cursor, search_data_sql)
    if result:
        acl_statistic_result = Utils.generator_to_list(list(acl_data[:2]) +
                                                       [round(acl_data[2], CommonConstant.ROUND_SIX)] + list(
            acl_data[3:]) for acl_data in result)
        return acl_statistic_result
    return []
